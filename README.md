# STM32F103C8T6 BootLoader + OTA

基于 **STM32F103C8T6** 的 BootLoader + APP 双工程方案，支持 **Keil MDK-ARM** 与 **CMake + arm-none-eabi-gcc** 两套构建方式。

BootLoader 位于 Flash 起始地址，复位后完成最小时钟 / 外设初始化并跳转到 APP；
APP 通过 **SI24R1（2.4G，nRF24L01 兼容）** 无线收发数据；UART2 数据可经 2.4G 发出；2.4G 接收完一帧后，通道 0 数据从 **USART1 TX** 输出，通道 1 数据从 **USART2 TX** 输出。

仓库只提交源码、CubeMX / Keil 工程配置与最终固件 `.hex`；编译中间产物（`.o` / `.crf` / `.d` / `.axf` / `.map` / `build/` 等）由 `.gitignore` 过滤，不入库。

---

## 1. 硬件环境

| 项目 | 说明 |
| --- | --- |
| MCU | STM32F103C8T6，LQFP48，64 KB Flash / 20 KB SRAM |
| 时钟 | 外部 8 MHz 晶振（HSE）+ PLL x9，系统主频 **72 MHz** |
| 低速时钟 | LSI 打开，供 IWDG 独立看门狗使用 |
| 无线模块 | SI24R1（nRF24L01 兼容），SPI1；2400 MHz、2 Mbps、7 dBm、5 字节地址、动态负载（单包 1~32 字节）；P0=`A5A5A5A5A5`、P1=`C2C2C2C2C2` |
| 调试串口 | USART1，115200 8-N-1 |
| 2.4G 数据串口 | USART2，115200 8-N-1，空闲中断接收，1024 字节环形缓冲 | PA2=TX，PA3=RX |

### 外设与引脚

| 功能 | 外设 | 引脚 |
| --- | --- | --- |
| 2.4G 片选 CSN | GPIO 输出 | PA4 |
| 2.4G 使能 CE | GPIO 输出 | PB0 |
| 2.4G 中断 IRQ | GPIO 输入（上拉） | PB1 |
| LED 指示灯 | GPIO 输出，低电平点亮；APP 由 TIM2、BootLoader 由 TIM1 中断驱动闪烁 | PB12 |
| 2.4G SPI | SPI1 主机，8 bit，CPOL=0 / CPHA=0，分频 8（约 9 Mbit/s） | PA5=SCK，PA6=MISO，PA7=MOSI |
| 调试串口 | USART1 异步 | PA9=TX，PA10=RX |
| 看门狗 | IWDG（LSI），16 分频，重装载 2499，超时约 1.0 s | — |
| LED 闪烁定时器 | TIM2 内部时钟，Prescaler 7200-1 / Period 1000-1 → 100 ms 中断，每 5 次翻转一次（约 1 Hz） | — |

---

## 2. Flash 分区

| 区域 | 起始地址 | 程序数据 | 控制页 |
| --- | --- | --- | --- |
| BootLoader | `0x08000000` | `0x00004000`（16 KB） | — |
| 一号运行区（APP） | `0x08004000` | `0x08004000~0x08007BFF`（15 KB，Keil IROM1=`0x3C00`） | `0x08007C00~0x08007FFF` |
| 二号区（升级缓存/备份） | `0x08008000` | `0x08008000~0x0800BBFF`（15 KB） | `0x0800BC00~0x0800BFFF` |
| 三号区 | `0x0800C000` | `0x0800C000~0x0800FBFF`（15 KB） | `0x0800FC00~0x0800FFFF` |

BootLoader 启动后执行 `Goto_APP()`（见 `bootloader/bootloader/Core/Src/main.c`）：

1. 关中断 `__disable_irq()`；
2. 取 APP 向量表第二个字作为复位入口 `*(uint32_t *)(0x08004000 + 4)`；
3. `__set_MSP(*(uint32_t *)0x08004000)` 设置 APP 栈顶；
4. `SCB->VTOR = 0x08004000` 重定向中断向量表；
5. 以函数指针方式跳转到 APP 运行。

> BootLoader 在跳转前还会判断复位源并清除复位标志：`RCC_FLAG_IWDGRST`（看门狗复位）成立时不跳转 APP，而是进入安全模式——由 TIM1（100 ms）驱动 PB12 LED 闪烁并在主循环中喂狗；其他复位（上电 / 按键 / 软件）才跳转 APP。
>
> OTA 基础组件已落地：CRC（CRC 外设 + HAL 驱动）、Flash 读写/擦除（`hardware/MyFLASH.c`）、控制页与备份元数据（`hardware/FlashManage.c`，含开机初始化 `FlashManage_FirstOnly()`）；已实现 `GetCRC` / `GetMID` / `GetSta` 查询事务、`AckCRC` 停等确认、`SetDat` 控制页/程序页接收与逐页擦写、非零 CRC 页复制、自动选槽、`SetEnd` 全量校验与成功/失败回报。实机联调与收帧健壮性（拆包/粘包）仍待完善。
>
> ⚠️ 注意：CubeMX 生成的 GCC 链接脚本 `STM32F103xx_FLASH.ld` 中 APP 的 `FLASH` 起始地址仍为 `0x8000000`，
> 与 Keil 工程的 `0x8004000` 不一致。若用 CMake 构建 APP，需先把该链接脚本的 FLASH ORIGIN 改为 `0x8004000`、
> LENGTH 改为 `16K`，否则会覆盖 BootLoader。

---

## 3. 目录结构

```
.
|-- README.md
|-- .gitignore
|-- ref/                                    # 参考资料（SI24R1 数据手册、板卡原理图）
|-- doc/                                    # OTA 通讯协议文档（上位机 / 设备各 3 份）
|-- APP/
|   `-- APP/                                # 应用程序工程
|       |-- APP.ioc                         # STM32CubeMX 工程文件
|       |-- Core/                           # main.c / usart.c / spi.c / gpio.c / iwdg.c / tim.c / crc.c / 中断
|       |-- Drivers/                        # CMSIS + STM32F1xx HAL 驱动（仅保留用到的文件）
|       |-- hardware/                       # 用户驱动：Delay、FREQ2_4G(SI24R1)、TIM_LED、Uart、MyFLASH、FlashManage、Mydef
|       |-- MDK-ARM/                        # Keil 工程文件与固件 APP.hex（中间产物不入库）
|       |-- CMakeLists.txt / CMakePresets.json / cmake/   # CMake（arm-none-eabi-gcc）构建
|       |-- STM32F103xx_FLASH.ld            # GCC 链接脚本
|       |-- startup_stm32f103xb.s           # GCC 启动文件
|       `-- tmp_manuals/                    # 芯片资料（SI24R1、G01S 手册）
`-- bootloader/
    `-- bootloader/                         # BootLoader 工程（结构与 APP 相同）
        |-- Core/                           # main.c / gpio.c / iwdg.c / tim.c(TIM1) / crc.c / 中断
        |-- hardware/                       # 用户驱动：Delay、TIM_LED、MyFLASH、Mydef
        |-- CMakeLists.txt / CMakePresets.json / cmake/
        |-- STM32F103xx_FLASH.ld
        |-- MDK-ARM/                        # Keil 工程文件与固件 bootloader.hex
        `-- ...
```

---

## 4. 编译与烧录

### 方式一：Keil MDK-ARM

1. 打开 `bootloader/bootloader/MDK-ARM/bootloader.uvprojx`，编译得到 `bootloader.hex`；
2. 打开 `APP/APP/MDK-ARM/APP.uvprojx`，编译得到 `APP.hex`；
3. 先烧录 BootLoader（`0x08000000`），再烧录 APP（`0x08004000`）；
4. 也可以直接使用仓库中已提交的 `.hex` 固件，无需重新编译。

### 方式二：CMake + arm-none-eabi-gcc

需要 CMake 3.22+、Ninja 与 arm-none-eabi-gcc 工具链：

```bash
cmake --preset Debug        # 或 Release
cmake --build --preset Debug
```

生成物位于 `build/Debug/`（该目录已由 `.gitignore` 排除）。

> 注：当前 CubeMX 工程的工具链设置为 **MDK-ARM V5.32**，仓库中的 CMake 构建文件保留可继续使用。

> `*.ioc` 为 CubeMX 工程文件，修改外设配置后可重新生成 `Core/` 代码。

---

## 5. 版本管理约定

**纳入版本管理**

- 源码：`Core/`、`hardware/`、`Drivers/`；
- 工程配置：`*.ioc`、`.mxproject`、`*.uvprojx`、`*.uvoptx`、`CMakeLists.txt`、`CMakePresets.json`、`cmake/`、`*.ld`、启动文件；
- 最终固件：`APP.hex`、`bootloader.hex`；
- 资料文档：`ref/`、`tmp_manuals/`。

**由 `.gitignore` 过滤**

- Keil 编译中间产物：`*.o`、`*.crf`、`*.d`、`*.lst`、`*.lnp`、`*.dep`、`*.sct`、`*.map`、`*.axf`、`*.htm`（构建日志）等；
- CMake 构建目录：`build/`、`cmake-build-*/`；
- Keil 运行时目录：`MDK-ARM/RTE/`、`MDK-ARM/DebugConfig/`（重新编译时自动重建）；
- 用户 / 调试相关：`*.uvguix.*`、`JLinkLog.txt`、`JLinkSettings.ini`；
- 编辑器与操作系统临时文件：`.vscode/`、`.idea/`、`*.tmp`、`*.bak`、`Thumbs.db` 等。
- 对话日志与过程性临时文件：/tmp/（存放每次对话日志 `tmp/logs/*.md` 与全部临时产物，详见 AGENTS.md）。

---

## 6. 版本记录

| 日期 | 说明 |
| --- | --- |
| 2026-10-11 | 初始提交：BootLoader + APP 工程 |
| 2026-10-11 | 新增 IWDG 独立看门狗：使能 LSI 时钟，BootLoader / APP 均初始化，APP 主循环定期喂狗 |
| 2026-10-11 | 新增 `README.md` 与 `.gitignore`；将 Keil 编译中间产物等临时文件移出版本控制 |
| 2026-10-11 | CubeMX 改用 CMake 工具链重新生成两个工程，并裁剪未使用的 HAL / CMSIS 库文件 |
| 2026-10-11 | CubeMX 工具链由 CMake 切回 MDK-ARM V5.32，重新生成两个工程的 Keil 工程文件（仅 XML 格式规范化，无实质配置变化） |
| 2026-10-12 | TIM2 改由 CubeMX / HAL 管理（`Core/Src/tim.c`，100 ms 周期），LED 闪烁回调拆分到 `hardware/TIM_LED.c` |
| 2026-10-16 | BootLoader 增加看门狗复位检测与安全模式（TIM1 驱动 LED 闪烁 + 喂狗），非看门狗复位才跳转 APP；新增 `bootloader/hardware` 驱动目录与原理图 `ref/SCH_Schematic1_2026-10-16.pdf` |
| 2026-10-20 | Flash 分区调整为 BootLoader / APP / 二号程序区 / 数据备份区各 16 KB（APP 起始改为 `0x08004000`）；启用 CRC 外设，新增 `MyFLASH`、`FlashManage` 驱动与备份数据结构；IWDG 超时改为约 1.0 s |
| 2026-10-21 | FlashManage 备份写函数重命名为 `FlashManage_BackupWritePage`；新增 0xA5 帧协议处理骨架 `FlashManage_HCC`；`Mydef.h` 补充 `string.h`、`crc.h` 引用 |
| 2026-10-21 | 2.4G 启用 P0/P1 双通道接收（`EN_RXADDR`、`DYNPD` 均为 `0x03`），P1 地址改为 `C2C2C2C2C2`；发送端地址与速率对齐为 2 Mbps / 7 dBm；`RXData_P1` 复用接收缓冲 |
| 2026-10-26 | 2.4G 接收完一帧后按通道选择串口：通道 0 从 USART1 TX 输出，通道 1 从 USART2 TX 输出；接收缓冲扩至 32 字节，支持最大动态负载 |
| 2026-10-26 | UART2 接收缓冲、环形队列、空闲/错误回调与发送封装迁移至 `hardware/Uart.c/.h`；取消 2.4G 上电稳定等待，初始化后直接进入接收模式 |
| 2026-10-26 | UART2 环形缓冲扩至 1024 字节；收到 UART2 数据后只发送一次，随后立即恢复接收模式，发送失败不重试 |
| 2026-11-03 | 新增 `doc/` 上位机与设备 OTA 通讯协议文档各 3 份（完整规范、简明说明、对接测试清单） |
| 2026-11-07 | OTA 查询事务落地（`GetCRC`/`GetMID`/`GetSta` + `AckCRC`），新增 `FlashManage_FirstOnly()` 开机写控制页与 `APP_MID=0x0002`；APP 链接上限调整为 15 KB（`0x3C00`），Keil 构建自动生成 `APP.bin` |
| 2026-11-04 | `FlashManage` 备份页宏同步到新分区：`BackupPageADD1=0x08007C00`、`ADD2=0x0800BC00`、新增 `ADD3=0x0800FC00` |
| 2026-11-08 | 实现 SetDat 主体流程（控制页接收、自动选槽、非零 CRC 页复制、程序页逐块写入与 CRC 回读）与 SetEnd 全量校验/成功失败回报；MyFLASH 新增连续读写 Buffer 接口；`APP_MID` 改为 `0x0003` |
| 2026-11-12 | 2.4G 接收前先读 `FIFO_STATUS`，RX FIFO 为空时直接返回，避免无效读；Keil 的 `fromelf` 生成 `APP.bin` 步骤由 BeforeMake 调整为 AfterMake（确保 bin 与本次 axf 一致） |
