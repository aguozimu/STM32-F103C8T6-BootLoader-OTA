# 设备 OTA 槽位状态实施型补丁

## 1. 实施目标

在 `STM32-F103C8T6-BootLoader-OTA` 中实现以下闭环：

1. 新增 `0x08/0x09/0x0A` 三个槽位状态；
2. 保持 `0x06 RUNNING` 只出现在一号槽；
3. BootLoader 在用户手动复位后把 `0x05` 缓存镜像复制到一号槽；
4. 一号槽与对应缓存槽同步进入 `0x08`；
5. APP 自检成功写 `0x06/0x09`，失败写 `0x0A`；
6. 任意 IWDG 复位把当前版本标坏并立即回滚到另一 `0x09`；
7. 无回滚版本时进入 BootLoader 安全模式；
8. `0x0A` 保留到对应槽下一次 OTA 覆盖。

实施基线：`origin/main` 提交 `d377741`。当前设备工作树中的未提交改动不作为本补丁依赖。

## 2. 文件变更清单

| 文件 | 类型 | 作用 |
| --- | --- | --- |
| `APP/APP/hardware/FlashManage.h` | 修改 | 新状态常量、初始化/确认/拒绝接口 |
| `APP/APP/hardware/FlashManage.c` | 修改 | 初始化不再覆盖状态，增加双槽状态同步 |
| `APP/APP/Core/Src/main.c` | 修改 | 调用安全初始化，预留自检成功/失败调用点 |
| `bootloader/bootloader/hardware/OtaBootManager.h` | 新增 | BootLoader 槽管理接口 |
| `bootloader/bootloader/hardware/OtaBootManager.c` | 新增 | 元数据校验、复制、回滚、看门狗判坏 |
| `bootloader/bootloader/hardware/MyFLASH.h` | 修改 | 增加缓冲区读写接口 |
| `bootloader/bootloader/hardware/MyFLASH.c` | 修改 | 实现缓冲区读写 |
| `bootloader/bootloader/hardware/Mydef.h` | 修改 | 引入新模块 |
| `bootloader/bootloader/Core/Src/main.c` | 修改 | 启动策略、IWDG 检测、切换和回滚 |
| `bootloader/bootloader/MDK-ARM/bootloader.uvprojx` | 修改 | 加入 `OtaBootManager.c/.h` |
| `bootloader/bootloader/CMakeLists.txt` | 修改 | 加入新源文件和 include 路径 |
| `APP/APP/CMakeLists.txt` | 按实际工程情况修改 | 若源文件未自动收集，加入 APP 用户源文件 |
| `doc/设备/01-通讯协议完整规范.md` | 修改 | 协议版本、状态表、状态生命周期 |
| `doc/设备/02-通讯协议简明说明.md` | 修改 | 简明状态表和手动复位流程 |
| `doc/设备/03-对接与测试清单.md` | 修改 | 增加切换、自检、回滚联调用例 |
| `README.md` | 修改 | 更新 OTA 槽位生命周期 |

## 3. 状态常量

### 3.1 `APP/APP/hardware/FlashManage.h`

追加：

```c
#define OTA_RUN_STATUS_RUN_PENDING_CONFIRM 0x08U
#define OTA_RUN_STATUS_RUN_CONFIRMED_OK    0x09U
#define OTA_RUN_STATUS_RUN_CONFIRMED_BAD   0x0AU
```

追加接口：

```c
void FlashManage_InitIfNeeded(void);
void FlashManage_ConfirmRunning(void);
void FlashManage_RejectRunning(void);
```

要求：

- `OTA_RUN_STATUS_RUNNING` 只能写一号槽。
- `FlashManage_ConfirmRunning()` 和 `FlashManage_RejectRunning()` 必须幂等。
- 不能把 `0x09` 写到一号槽作为正常终态；一号槽正常终态固定为 `0x06`。

## 4. APP 初始化与状态同步

### 4.1 替换 `FlashManage_FirstOnly()`

现有 `FlashManage_FirstOnly()` 每次启动都会擦除并重写一号槽控制页，导致 BootLoader 写入的 ID 和 `0x08` 被覆盖。必须替换为：

```c
void FlashManage_InitIfNeeded(void)
{
    Data_Backup backup;

    if (!FlashManage_ReadAndValidateControlPage(BackupPageADD1, &backup)) {
        FlashManage_RebuildRunningMetadata(APP_MID);
    }

    FlashManage_InitEmptyBackup(BackupPageADD2);
    FlashManage_InitEmptyBackup(BackupPageADD3);
}
```

实施要点：

- 一号槽控制页有效时不得重写。
- 仅当控制页自身 CRC 无效或状态为 `0xFF` 时，才根据一号槽真实内容重建元数据。
- 二、三号槽只在原始字节为 `0xFF` 时初始化为 `0x00 EMPTY`，不得覆盖已有合法状态。
- 如果 `Data_Backup` 结构存在编译器填充，增加静态断言确保前 132 字节布局不变。

### 4.2 读取控制页

新增内部函数：

```c
static uint8_t FlashManage_ReadAndValidateControlPage(
    uint32_t control_page,
    Data_Backup* backup);
```

验证内容：

1. `ID != 0x0000`；
2. `Run_status` 是已定义状态；
3. 16 个 `Page_ADD` 为合法的槽内相对地址或对应槽绝对地址；
4. 将 `Page[15].Page_CRC` 清零后计算 1 KB 硬件 CRC，结果与存储值一致；
5. 15 个程序页的实际 CRC 与元数据一致。

### 4.3 查找匹配源槽

新增内部函数：

```c
static uint8_t FlashManage_FindMatchingSourceSlot(
    const Data_Backup* running,
    uint8_t* slot_index);
```

匹配条件：

- 缓存槽 ID 等于一号槽 ID；
- 对应状态为 `0x08` 或 `0x09`；
- 15 个程序页 CRC 与一号槽完全一致；
- 控制页有效。

只有唯一匹配时才返回成功。匹配不唯一或不存在时只更新一号槽，不猜测源槽。

### 4.4 自检成功

`FlashManage_ConfirmRunning()` 的逻辑：

1. 读取一号槽元数据。
2. 查找匹配源槽。
3. 若一号槽已是 `0x06` 且源槽已是 `0x09`，直接返回，避免重复擦写。
4. 一号槽写 `0x06 RUNNING`，`Data_area=0xFF`。
5. 匹配源槽写 `0x09 RUN_CONFIRMED_OK`，`Data_area=0xFF`。
6. 扫描另一个缓存槽：
   - 若状态为 `0x0A`，保留；
   - 若状态为 `0x05`、`0x02`、`0x03`、`0x04`，保留，避免破坏进行中升级；
   - 其它旧状态写 `0x00 EMPTY`，`Data_area=0xFF`。

### 4.5 自检失败

`FlashManage_RejectRunning()` 的逻辑：

1. 读取一号槽元数据。
2. 一号槽写 `0x0A RUN_CONFIRMED_BAD`，`Data_area=0xFF`。
3. 若找到匹配源槽，也写 `0x0A`。
4. 执行 `NVIC_SystemReset()`，让 BootLoader 进入回滚逻辑。
5. 若复位前 Flash 写入失败，允许 IWDG 超时复位，BootLoader 仍需按 IWDG 路径判坏。

## 5. APP `main.c` 集成

### 5.1 替换初始化调用

现有约第 204 行：

```c
/* 开机元数据初始化：刷新 APP_MID、运行状态、页地址与页 CRC */
FlashManage_FirstOnly();
```

替换为：

```c
/* 仅在控制页无效时初始化，不得覆盖 BootLoader 写入的 ID 和运行状态 */
FlashManage_InitIfNeeded();
```

### 5.2 自检调用位置

在 `TIM_LED_Start()`、`Uart2_Init()`、`FREQ2_4G_ReadInit()` 完成后、进入主循环前预留：

```c
/* USER CODE BEGIN APP_SELF_TEST */
/*
 * 后续在此加入业务检测。
 * 全部通过：FlashManage_ConfirmRunning();
 * 明确失败：FlashManage_RejectRunning();
 * 尚未实现检测时保持 0x08，不得无条件写成功。
 */
/* USER CODE END APP_SELF_TEST */
```

当前补丁不自动调用成功接口。后续检测函数实现后再启用。

## 6. BootLoader `MyFLASH` 扩展

在 `bootloader/bootloader/hardware/MyFLASH.h` 增加：

```c
void MyFLASH_ReadBuffer(uint32_t Address, uint32_t* Data, uint16_t WordCount);
void MyFLASH_ProgramBuffer(uint32_t Address, const uint32_t* Data, uint16_t WordCount);
```

`MyFLASH_ReadBuffer()` 应使用连续 32 位读取；`MyFLASH_ProgramBuffer()` 应逐字编程并检查 `HAL_FLASH_Program()` 返回值。为兼容现有工程习惯，可在函数内部解锁一次、循环编程、最后加锁；必须避免每字重复解锁造成额外风险。

## 7. BootLoader 槽管理模块

新增：

- `bootloader/bootloader/hardware/OtaBootManager.h`
- `bootloader/bootloader/hardware/OtaBootManager.c`

### 7.1 推荐接口

```c
typedef enum {
    OTA_BOOT_RESULT_JUMP_RUNNING = 0,
    OTA_BOOT_RESULT_ACTIVATED,
    OTA_BOOT_RESULT_ROLLED_BACK,
    OTA_BOOT_RESULT_SAFE_MODE,
    OTA_BOOT_RESULT_ERROR
} OtaBoot_Result;

OtaBoot_Result OtaBoot_HandleWatchdogReset(void);
OtaBoot_Result OtaBoot_ActivatePendingImage(void);
OtaBoot_Result OtaBoot_RollbackFromConfirmedBad(void);
OtaBoot_Result OtaBoot_EvaluateBootState(void);
```

模块内部负责：

- 控制页读取与 CRC 校验；
- 15 个程序页 CRC 校验；
- 先进先出/新建候选选择；
- 程序页复制；
- 一号槽控制页地址归一化；
- 双槽状态同步；
- 安全模式判定。

### 7.2 元数据规则

二、三号槽控制页保留绝对地址；复制到一号槽时改为：

```c
slot1.Page[page].Page_ADD = APP_RUN_BASE + page * OTA_PAGE_SIZE;
slot1.Page[15].Page_ADD = BackupPageADD1;
```

复制后的 `slot1.ID` 使用源槽 ID，`Run_status=0x08`，`Data_area=0xFF`。

### 7.3 激活顺序

```c
/* 1. 校验源槽 */
if (!OtaBoot_ValidateSlot(source, &metadata)) {
    return OTA_BOOT_RESULT_ERROR;
}

/* 2. 逐页复制并回读 CRC，过程中刷新 IWDG */
if (!OtaBoot_CopyProgramPages(source, metadata)) {
    return OTA_BOOT_RESULT_ERROR;
}

/* 3. 写一号槽控制页，状态 0x08，作为切换提交点 */
if (!OtaBoot_WriteSlot1Metadata(&metadata, OTA_RUN_STATUS_RUN_PENDING_CONFIRM)) {
    return OTA_BOOT_RESULT_ERROR;
}

/* 4. 更新源槽状态为 0x08 */
if (!OtaBoot_WriteSlotStatus(source, OTA_RUN_STATUS_RUN_PENDING_CONFIRM)) {
    return OTA_BOOT_RESULT_ERROR;
}

return OTA_BOOT_RESULT_ACTIVATED;
```

说明：

- 一号槽控制页写完但源槽未更新时，下次复位会看到源槽仍为 `0x05`，可安全重复复制。
- 两侧均为 `0x08` 后，下一次复位不得重复复制，除非 IWDG 分支判坏。
- 控制页写入必须最后回填 Page16 CRC。

### 7.4 IWDG 判坏

```c
OtaBoot_Result OtaBoot_HandleWatchdogReset(void)
{
    Data_Backup running;
    uint8_t source_slot;

    if (OtaBoot_ReadRunning(&running)) {
        OtaBoot_WriteSlotStatus(OTA_SLOT_1, OTA_RUN_STATUS_RUN_CONFIRMED_BAD);
        if (OtaBoot_FindMatchingSource(&running, &source_slot)) {
            OtaBoot_WriteSlotStatus(source_slot, OTA_RUN_STATUS_RUN_CONFIRMED_BAD);
        }
    }

    return OtaBoot_RollbackFromConfirmedBad();
}
```

要求：

- 任意 IWDG 复位都走该分支。
- `OtaBoot_RollbackFromConfirmedBad()` 只选另一槽的 `0x09`。
- 回滚源不能是刚刚标记为 `0x0A` 的源槽。
- 找到回滚源时，复制后一号槽与回滚源槽写 `0x08`。
- 没有回滚源时返回 `OTA_BOOT_RESULT_SAFE_MODE`。

## 8. BootLoader `main.c` 集成

现有逻辑约第 132~157 行只判断 IWDG 后闪烁 LED，否则直接跳 APP。替换为：

```c
uint32_t is_iwdg = (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0U);
__HAL_RCC_CLEAR_RESET_FLAGS();

OtaBoot_Result boot_result = OtaBoot_EvaluateBootState();

if (is_iwdg) {
    boot_result = OtaBoot_HandleWatchdogReset();
}

if (boot_result == OTA_BOOT_RESULT_JUMP_RUNNING ||
    boot_result == OTA_BOOT_RESULT_ACTIVATED ||
    boot_result == OTA_BOOT_RESULT_ROLLED_BACK) {
    Goto_APP();
}

/* SAFE_MODE：保持现有 TIM1 LED 闪烁和喂狗循环 */
```

实施顺序建议：

1. 先读取并处理 IWDG 分支；
2. 再处理 `0x05` 激活；
3. 再检查一号槽 `0x06/0x08`；
4. 若一号槽 `0x0A`，直接回滚；
5. 其它异常进入安全模式。

注意：`OtaBoot_EvaluateBootState()` 内不得在 IWDG 分支之前把 `0x0A` 自动清为空。

## 9. 工程配置

### 9.1 Keil

在 `bootloader/bootloader/MDK-ARM/bootloader.uvprojx` 中加入：

- `..\hardware\OtaBootManager.c`
- `..\hardware\OtaBootManager.h`

确保 `hardware` 目录仍在 include path 中。

### 9.2 CMake

在 `bootloader/bootloader/CMakeLists.txt` 的 `target_sources` 中加入：

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/OtaBootManager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware/MyFLASH.c
```

如当前 CubeMX 子目录未自动加入 `hardware`，同时加入：

```cmake
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware
)
```

APP 工程保持现有 `FlashManage.c`、`MyFLASH.c` 和 `crc.c` 参与编译。

## 10. 协议文档同步

### 10.1 设备协议

两份设备规范追加状态表：

```text
0x08 RUN_PENDING_CONFIRM 运行待确认
0x09 RUN_CONFIRMED_OK 确认运行正常
0x0A RUN_CONFIRMED_BAD 确认无法运行
```

补充：

- `0x06` 只允许一号槽；
- `0x05` 需要用户手动复位才切换；
- 自检失败和 IWDG 都进入 `0x0A`；
- 坏槽保留到下一次 OTA 覆盖；
- 无回滚版本时停留安全模式。

### 10.2 对接与测试清单

增加以下条目：

- [ ] OTA 成功后只出现 `0x05`，不自动切换。
- [ ] 手动复位后一号槽和源槽进入 `0x08`。
- [ ] APP 自检成功后一号槽为 `0x06`、源槽为 `0x09`。
- [ ] APP 自检失败后双槽为 `0x0A`，并触发回滚。
- [ ] IWDG 复位后旧版本被拷贝到一号槽并进入 `0x08`。
- [ ] 无回滚版本时停留安全模式。
- [ ] `0x0A` 在下一次成功 OTA 覆盖前保持。
- [ ] 二、三号槽从不出现 `0x06`。

## 11. 验证步骤

### 11.1 编译

分别构建 BootLoader 和 APP：

```powershell
Set-Location C:\Users\aguoz\Desktop\手柄\UART_2_4G\bootloader\bootloader\MDK-ARM
& 'E:\kile 5\UV4\UV4.exe' -r 'bootloader.uvprojx' -t 'bootloader' -o 'codex_boot_build.log'
```

```powershell
Set-Location C:\Users\aguoz\Desktop\手柄\UART_2_4G\APP\APP\MDK-ARM
& 'E:\kile 5\UV4\UV4.exe' -r 'APP.uvprojx' -t 'APP' -o 'codex_app_build.log'
```

要求两个日志均为 `0 Errors`。

### 11.2 硬件测试流程

1. 烧录旧版本，确认 `GetSta` 返回 `0x06`。
2. 执行 OTA，最终返回 `0x05`。
3. 断电重启，观察一号槽和对应缓存槽均为 `0x08`。
4. 接入自检成功调用，确认一号槽 `0x06`、源槽 `0x09`。
5. 构造自检失败，确认双槽 `0x0A`。
6. 复位，确认 BootLoader 回滚到上一 `0x09` 并进入 `0x08`。
7. 让 APP 触发 IWDG，确认当前版本和匹配源槽均变为 `0x0A`。
8. 删除回滚候选或将其标为 `0x0A`，确认进入安全模式而不启动坏程序。
9. 在复制过程中断电，重新上电确认可重复复制且不会启动半成品。
10. 重新 OTA 覆盖坏槽，确认 `0x0A` 被新流程覆盖。

## 12. 验收标准

- 状态常量、协议文档和设备行为完全一致。
- `0x06` 不出现在二、三号槽。
- 双槽状态更新不是“只写一份”，但任何一份写入失败都能在下次启动安全恢复。
- APP 不再每次启动覆盖一号槽 ID/状态。
- IWDG 复位不会无限循环启动同一坏程序。
- `0x0A` 在未覆盖前保留。
- 无回滚版本时安全模式可正常喂狗和 LED 指示。