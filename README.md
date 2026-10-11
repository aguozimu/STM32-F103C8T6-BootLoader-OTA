# STM32F103C8T6 BootLoader + OTA

基于 **STM32F103C8T6** 的 BootLoader + APP 双工程方案（Keil MDK-ARM + STM32CubeMX）。

BootLoader 位于 Flash 起始地址，复位后完成最小时钟/外设初始化并跳转到 APP；
APP 通过 **SI24R1（2.4G，nRF24L01 兼容）** 无线收发数据，并把结果经由 **USART1** 转发出去。
本仓库同时归档两个工程的 Keil 编译产物（`.hex` / `.axf` / `.map`），便于随时取用固件。

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
| LED 指示灯 | GPIO 输出（预留） | PB12 |
| 2.4G SPI | SPI1 主机，8 bit，CPOL=0 / CPHA=0，分频 8（约 9 Mbit/s） | PA5=SCK，PA6=MISO，PA7=MOSI |
| 串口 | USART1 异步 | PA9=TX，PA10=RX |
| 看门狗 | IWDG（LSI），4 分频，重装载 4095，超时约 410 ms | — |

---

## 2. Flash 分区

| 区域 | 起始地址 | 大小（Keil IROM1） |
| --- | --- | --- |
| BootLoader | `0x08000000` | `0x00002500`（9.25 KB） |
| APP | `0x08002500` | `0x00002500` |

BootLoader 启动后执行 `Goto_APP()`（见 `bootloader/bootloader/Core/Src/main.c`）：

1. 关中断 `__disable_irq()`；
2. 取 APP 向量表第二个字作为复位入口 `*(uint32_t *)(0x08002500 + 4)`；
3. `__set_MSP(*(uint32_t *)0x08002500)` 设置 APP 栈顶；
4. `SCB->VTOR = 0x08002500` 重定向中断向量表；
5. 以函数指针方式跳转到 APP 运行。

> 当前 BootLoader 只承担跳转职责，OTA 搬运/校验逻辑尚未实现，可作为后续扩展点。

---

## 3. 目录结构

```
.
|-- README.md
|-- .gitignore
|-- APP/
|   `-- APP/                        # 应用程序工程
|       |-- APP.ioc                 # STM32CubeMX 工程文件
|       |-- Core/                   # main.c / usart.c / spi.c / gpio.c / iwdg.c / 中断
|       |-- Drivers/                # CMSIS + STM32F1xx HAL 驱动
|       |-- hardware/               # 用户驱动：Delay、FREQ2_4G(SI24R1)、Mydef
|       |-- MDK-ARM/                # Keil 工程与编译产物（APP.hex / APP.axf / APP.map）
|       `-- tmp_manuals/            # 芯片资料（SI24R1、G01S 手册）
`-- bootloader/
    `-- bootloader/                 # BootLoader 工程（结构与 APP 相同）
        |-- bootloader.ioc
        |-- Core/
        `-- MDK-ARM/                # bootloader.hex / bootloader.axf / bootloader.map
```

---

## 4. 编译与烧录

1. 用 **Keil uVision5（MDK-ARM）** 打开 `bootloader/bootloader/MDK-ARM/bootloader.uvprojx`，编译得到 `bootloader.hex`；
2. 打开 `APP/APP/MDK-ARM/APP.uvprojx`，编译得到 `APP.hex`；
3. 先烧录 BootLoader（`0x08000000`），再烧录 APP（`0x08002500`）；
4. 也可以直接使用仓库中已归档的 `.hex` 文件，无需重新编译。

> `*.ioc` 为 CubeMX 工程文件，修改外设配置后可重新生成 `Core/` 代码。

---

## 5. 版本记录

| 日期 | 说明 |
| --- | --- |
| 2026-10-11 | 初始提交：BootLoader + APP 工程 |
| 2026-10-11 | 新增 IWDG 独立看门狗：使能 LSI 时钟，BootLoader / APP 均初始化，APP 主循环定期喂狗 |
