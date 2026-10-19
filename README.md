# tickboard

一个适合于多场景的 5×6 机械按键矩阵签到板，可以“计数”、“签到”、“点名”。

## 硬件自检固件

刚焊接完板子时，可烧录 `software` 目录中的自检程序，逐项验证各个模块。
所有外设均为 GPIO 软件驱动，不依赖 CubeMX 生成的 SPI/UART/TIM 配置。

### 引脚分配（依据 2026-09-14 原理图）

| 模块 | 信号 | MCU 引脚 |
| --- | --- | --- |
| LCD ST7789 284×76 | SCK / MOSI / CS / DC / RST | PA5 / PA7 / PA4 / PA3 / PA2 |
| WS2812 ×32 | RGB_DATA | PA1（经 74HCT1G125 缓冲） |
| 按键矩阵 7 行 × 6 列 | ROW1–7 | PB0, PB1, PB2, PB10, PB11, PC6, PC7 |
| 按键矩阵 | COL1–6 | PC0–PC5 |
| EC11 编码器 | A / B / SW | PB6 / PB7 / PA0 |
| WT588F04A 语音芯片 | BUSY / DATA / CLK | PB14 / PB13 / PB12 |
| 晶振 | HSE 16 MHz | PD0 / PD1 |

按键编号：ROW1–ROW5 的 6 列依次为 K1–K30，ROW6/COL1 为 K31，ROW7/COL1 为 K32。
行线经 1N4148WS 二极管接列线（ROW → COL），所以行输出高、列下拉输入扫描。

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

### 自检内容与预期现象

上电后按顺序执行：

1. **LCD**：白屏显示 `TICKBOARD / SELF TEST`，随后红、绿、蓝、白全屏闪烁，
   最后进入自检面板。
2. **RGB 灯链**：32 颗灯依次显示红、绿、蓝、白，然后进入彩虹渐变。
3. **按键矩阵**：按下任意键，面板左侧对应的格子变绿，`KEY:xx Kxx`
   显示当前按下数量和最近按下的键号。
4. **编码器**：旋转时 `ENC:±xxxx` 计数变化，并切换 RGB 模式
   （RAINBOW / RED / GREEN / BLUE / WHITE / OFF）；按下编码器重新执行语音测试。
   `AB:x x` 显示 A/B 两相电平，`SW:DOWN` 表示按键按下。
5. **语音芯片**：上电约 0.5 s 后自动发送“最大音量 + 地址 0x00/0x01/0x02”探测命令。
   - 若 BUSY 电平发生变化并听到声音：`AUD:PASS`
   - 若 3 个地址都无响应：`AUD:NO RSP`（芯片可能未烧录音源，或通讯异常）
6. **时钟**：右上角 `HSE 64M` 表示 16 MHz 晶振正常；若显示 `HSI 64M`，
   表示晶振未起振，程序已自动回退到内部 HSI/2×16 = 64 MHz。
7. **电源**：板上没有把电源电压接入 ADC，5 V / 3.3 V 需要用万用表测量。

### 源码结构

| 文件 | 说明 |
| --- | --- |
| `User/Src/app.c` | 自检流程、LCD 面板、音频测试状态机 |
| `User/Src/board.c` | 时钟相关辅助、DWT 微秒延时、GPIO 工具 |
| `User/Src/spi.c` | LCD 软件 SPI（4 线） |
| `User/Src/lcd.c` / `lcd_init.c` | 中景园 ST7789 驱动、字库（厂商例程） |
| `User/Src/ws2812.c` | WS2812 位时序驱动 |
| `User/Src/keypad.c` | 7×6 矩阵扫描与消抖 |
| `User/Src/encoder.c` | EC11 正交解码与按键消抖 |
| `User/Src/audio.c` | WT588F04A 两线串口协议 |

### 注意事项

- `Core/Inc/stm32f1xx_hal_conf.h` 中的 `HSE_VALUE` 已改为 **16 MHz**，
  与板上晶振一致。若改回 8 MHz，系统时钟和所有延时都会差一倍。
- `Core/Src/main.c` 的 `SystemClock_Config()` 已改为 HSE 16 MHz × 4 = 64 MHz，
  失败时回退到 HSI/2 × 16 = 64 MHz。该函数位于 USER CODE 之外，
  **用 CubeMX 重新生成工程会被覆盖**；`tickboard.ioc` 目前只配置了
  最基本的 RCC/SYS，请以当前源码为准。
- WS2812 的位时序对编译器优化敏感，`CMakeLists.txt` 中单独为
  `ws2812.c` 指定了 `-O2`。
- `Core/Src/main_temp.c` 是厂商 LCD 例程，未参与编译。
- 语音芯片协议参考唯创知音 WT588FxxA-8S-A2 数据手册：CLK 低 5 ms 起始、
  LSB first、上升沿采样、半周期 300 µs。
