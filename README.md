# tickboard

一个适用于多场景的机械按键矩阵签到板。

## 当前固件功能

当前 `software` 目录中的固件是一个最小启动版本，只实现以下行为：

1. **电源自锁**：按下电源键后，MCU 在启动最早期把 `PG1` 拉高，维持 Q1 导通；
   松开按键后继续供电。
2. **长按关机**：运行中长按电源键 2 秒，关闭 WS2812、清屏，然后拉低 `PG1`。
   注意：长按期间按键本身仍会把 Q1 栅极拉低，所以**真正断电发生在松开按键之后**。
   开机时按着的那一下必须先松开，之后再次按下才会开始 2 秒计时，
   避免“按住电源键开机”被误判成长按关机。
3. **灯效**：开机后只点亮第一颗 WS2812（白色），其余全部熄灭；
   电源键按下时第一颗变红，松开恢复白色；长按关机后全部熄灭。
4. **屏幕**：当前固件不初始化 LCD（屏幕硬件待修），`lcd.c` 等驱动仅保留未调用。

按键矩阵、编码器、语音芯片的驱动源码仍然保留，但当前固件未调用。

### 引脚分配（依据 2026-10-20 原理图）

| 模块 | 信号 | MCU 引脚 |
| --- | --- | --- |
| 电源 | PG1（保持供电，高有效） | PC8 |
| 电源 | PG2（电源按键，低有效） | PC9 |
| LCD ST7789 284×76 | RST / DC / CS | PA2 / PA3 / PA4 |
| LCD | SCK / MOSI | PA5 / PA7 |
| WS2812 ×30 | RGB_DATA | PA1（经 74HCT1G125 缓冲） |
| 按键矩阵 3 行 × 10 列 | ROW1–3 | PB11, PC6, PC7 |
| 按键矩阵 | COL1–10 | PC0–PC5, PB0, PB1, PB2, PB10 |
| EC11 编码器 | A / B / SW | PB6 / PB7 / PA0 |
| WT588F04A-8S-A2 | BUSY / DATA / CLK | PB14 / PB13 / PB12 |
| 晶振 | HSE 8 MHz | PD0 / PD1 |

按键编号：ROW1–ROW3 的 10 列依次为 K1–K30。
行线经 1N4148WS 二极管接列线（ROW → COL），所以行输出高、列下拉输入扫描。

> 备注：新原理图中 PA5/PA7 同时标有 `MAX_SCL`/`MAX_SDA`（MAX17055 电量计），
> LCD 的 SCK/MOSI 在 FPC 连接器上未画出网络名。当前固件沿用 PA5/PA7 软件 SPI，
> 如果实际 LCD 接线不同，需要同步修改 `User/Inc/spi.h`。

### 编译

```bash
cd software
cmake --preset Debug
cmake --build build/Debug
```

产物：

- `build/Debug/tickboard.elf`
- `build/Debug/tickboard.hex`
- `build/Debug/tickboard.bin`

### 烧录

使用 ST-Link + STM32CubeProgrammer 烧录 `tickboard.hex`，或使用 OpenOCD：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Debug/tickboard.elf verify reset exit"
```

### 电源控制说明

电源按键电路（原理图第 1 页）：

- 按下 SW1：Q1 栅极被拉低，Q1 导通，VSYS 上电，MCU 启动。
- MCU 拉高 `PG1`(PC8)：Q3 导通，Q1 栅极保持低电平，松开按键后继续供电。
- MCU 拉低 `PG1`：Q3 截止，Q1 栅极被 R4 拉回源极，Q1 截止，系统断电。
- `PG2`(PC9) 经 R3 上拉到 3V3，按下时为低，用于检测按键。

### 源码结构

| 文件 | 说明 |
| --- | --- |
| `User/Src/app.c` | 开机流程（电源锁存、30 灯全亮、helloworld）与长按关机 |
| `User/Src/power.c` | PG1 电源保持、PG2 按键消抖与 2 秒长按检测 |
| `User/Src/board.c` | DWT 微秒延时、GPIO 工具 |
| `User/Src/spi.c` | LCD 软件 SPI（4 线） |
| `User/Src/lcd.c` / `lcd_init.c` | 中景园 ST7789 驱动、字库（厂商例程） |
| `User/Src/ws2812.c` | WS2812 位时序驱动（30 颗） |
| `User/Src/keypad.c` | 矩阵扫描与消抖（当前固件未使用） |
| `User/Src/encoder.c` | EC11 正交解码与按键消抖（当前固件未使用） |
| `User/Src/audio.c` | WT588F04A 两线串口协议（当前固件未使用） |

### 注意事项

- `Core/Inc/stm32f1xx_hal_conf.h` 中的 `HSE_VALUE` 为 **8 MHz**，
  与 2026-10-20 原理图上的晶振一致。若板上仍是 16 MHz 晶振，需要改回并调整 PLL。
- `Core/Src/main.c` 的 `SystemClock_Config()` 使用 HSE 8 MHz × 8 = 64 MHz，
  失败时回退到 HSI/2 × 16 = 64 MHz。该函数位于 USER CODE 之外，
  **用 CubeMX 重新生成工程会被覆盖**；`tickboard.ioc` 目前只配置了
  最基本的 RCC/SYS，请以当前源码为准。
- `main()` 开头会在 `HAL_Init()` 之前直接操作寄存器拉高 `PG1`，
  确保按键松开前完成电源锁存。
- WS2812 的位时序对编译器优化敏感，`CMakeLists.txt` 中单独为
  `ws2812.c` 指定了 `-O2`。
- `Core/Src/main_temp.c` 是厂商 LCD 例程，未参与编译。
