# STM32F103C8T6 BootLoader + OTA

基于 **STM32F103C8T6** 的 BootLoader + APP 双工程方案，支持 **Keil MDK-ARM** 与 **CMake + arm-none-eabi-gcc** 两套构建方式。

BootLoader 位于 Flash 起始地址，复位后完成最小时钟 / 外设初始化并跳转到 APP；
APP 通过 **SI24R1（2.4G，nRF24L01 兼容）** 无线收发数据，并把结果经由 **USART1** 转发出去。

仓库只提交源码、CubeMX / Keil 工程配置与最终固件 `.hex`；编译中间产物（`.o` / `.crf` / `.d` / `.axf` / `.map` / `build/` 等）由 `.gitignore` 过滤，不入库。

---

## 1. 硬件环境

| 项目 | 说明 |
| --- | --- |
| MCU | STM32F103C8T6，LQFP48，64 KB Flash / 20 KB SRAM |
| 时钟 | 外部 8 MHz 晶振（HSE）+ PLL x9，系统主频 **72 MHz** |
| 低速时钟 | LSI 打开，供 IWDG 独立看门狗使用 |
| 无线模块 | SI24R1（nRF24L01 兼容），走 SPI1 |
| 调试串口 | USART1，115200 8-N-1 |

### 外设与引脚

| 功能 | 外设 | 引脚 |
| --- | --- | --- |
| 2.4G 片选 CSN | GPIO 输出 | PA4 |
| 2.4G 使能 CE | GPIO 输出 | PB0 |
| 2.4G 中断 IRQ | GPIO 输入（上拉） | PB1 |
| LED 指示灯 | GPIO 输出，低电平点亮；由 TIM2 中断驱动闪烁 | PB12 |
| 2.4G SPI | SPI1 主机，8 bit，CPOL=0 / CPHA=0，分频 8（约 9 Mbit/s） | PA5=SCK，PA6=MISO，PA7=MOSI |
| 串口 | USART1 异步 | PA9=TX，PA10=RX |
| 看门狗 | IWDG（LSI），4 分频，重装载 4095，超时约 410 ms | — |
| LED 闪烁定时器 | TIM2 内部时钟，Prescaler 7200-1 / Period 1000-1 → 100 ms 中断，每 5 次翻转一次（约 1 Hz） | — |

---

## 2. Flash 分区

| 区域 | 起始地址 | 大小 |
| --- | --- | --- |
| BootLoader | `0x08000000` | `0x00002500`（9.25 KB） |
| APP | `0x08002500` | `0x00002500`（Keil IROM1） |

BootLoader 启动后执行 `Goto_APP()`（见 `bootloader/bootloader/Core/Src/main.c`）：

1. 关中断 `__disable_irq()`；
2. 取 APP 向量表第二个字作为复位入口 `*(uint32_t *)(0x08002500 + 4)`；
3. `__set_MSP(*(uint32_t *)0x08002500)` 设置 APP 栈顶；
4. `SCB->VTOR = 0x08002500` 重定向中断向量表；
5. 以函数指针方式跳转到 APP 运行。

> 当前 BootLoader 只承担跳转职责，OTA 搬运 / 校验逻辑尚未实现，可作为后续扩展点。
>
> ⚠️ 注意：CubeMX 生成的 GCC 链接脚本 `STM32F103xx_FLASH.ld` 中 APP 的 `FLASH` 起始地址仍为 `0x8000000`，
> 与 Keil 工程的 `0x8002500` 不一致。若用 CMake 构建 APP，需先把该链接脚本的 FLASH ORIGIN 改为 `0x8002500`
> 并相应缩短 LENGTH，否则会覆盖 BootLoader。

---

## 3. 目录结构

```
.
|-- README.md
|-- .gitignore
|-- ref/                                    # 参考资料（SI24R1 数据手册）
|-- APP/
|   `-- APP/                                # 应用程序工程
|       |-- APP.ioc                         # STM32CubeMX 工程文件
|       |-- Core/                           # main.c / usart.c / spi.c / gpio.c / iwdg.c / tim.c / 中断
|       |-- Drivers/                        # CMSIS + STM32F1xx HAL 驱动（仅保留用到的文件）
|       |-- hardware/                       # 用户驱动：Delay、FREQ2_4G(SI24R1)、TIM_LED、Mydef
|       |-- MDK-ARM/                        # Keil 工程文件与固件 APP.hex（中间产物不入库）
|       |-- CMakeLists.txt / CMakePresets.json / cmake/   # CMake（arm-none-eabi-gcc）构建
|       |-- STM32F103xx_FLASH.ld            # GCC 链接脚本
|       |-- startup_stm32f103xb.s           # GCC 启动文件
|       `-- tmp_manuals/                    # 芯片资料（SI24R1、G01S 手册）
`-- bootloader/
    `-- bootloader/                         # BootLoader 工程（结构与 APP 相同）
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
3. 先烧录 BootLoader（`0x08000000`），再烧录 APP（`0x08002500`）；
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
| 2026-10-12 | TIM2 改由 CubeMX / HAL 管理（Core/Src/tim.c，100 ms 周期），LED 闪烁回调拆分到 hardware/TIM_LED.c |
