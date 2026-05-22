# 当前 HAL 工程引脚定义说明

生成日期：2026-05-21  
适用工程：`F:/stm32cubemx/project/4inch_cubeproject`  
目标芯片：`STM32F103C8Tx`，LQFP48

## 说明

本文档以当前 HAL 工程为准，主要核对来源如下：

- `4inch_cubeproject.ioc`
- `Inc/main.h`
- `Inc/lcd.h`
- `Inc/touch.h`
- `Inc/aht20.h`
- `Inc/bmp280.h`
- `Src/gpio.c`
- `Src/stm32f1xx_hal_msp.c`
- `STM32F103XX_FLASH.ld`

旧版 `引脚定义说明_V2.md` 不再作为依据。按键与 LED 定义已与原标准库工程 `F:/stm32cubemx/project/system/user/main.h` 对齐。

## 关键注意事项

- 调试/下载器使用 DAPlink，通过 SWD 方式连接。
- 当前工程保留 SWD，关闭 JTAG：`__HAL_AFIO_REMAP_SWJ_NOJTAG()`。
- `PA13`、`PA14` 用于 SWD 调试，不要改作普通 GPIO。
- `PB3`、`PB4` 已释放给触摸屏使用，因此 CubeMX 中必须关闭 JTAG，仅保留 Serial Wire。
- 按键为低电平有效，内部上拉输入。
- LED 为低电平点亮，默认输出高电平熄灭。
- LCD 片选 `LCD_CS` 低有效，触摸片选 `TP_CS` 低有效。
- LCD 背光 `LCD_LED` 当前按高电平打开处理。

## 系统与调试引脚

| 引脚 | CubeMX 信号/标签 | 用途 | 配置/说明 |
| --- | --- | --- | --- |
| `PD0-OSC_IN` | `RCC_OSC_IN` | HSE 外部晶振输入 | `HSE-External-Oscillator` |
| `PD1-OSC_OUT` | `RCC_OSC_OUT` | HSE 外部晶振输出 | `HSE-External-Oscillator` |
| `PA13` | `SYS_JTMS-SWDIO` | SWD 数据线 | DAPlink 调试/下载使用 |
| `PA14` | `SYS_JTCK-SWCLK` | SWD 时钟线 | DAPlink 调试/下载使用 |
| `SysTick` | `VP_SYS_VS_Systick` | HAL 时基 | CubeMX 使能 |

当前系统时钟配置为 PLL 输出 `SYSCLK = 72 MHz`。

## TFT LCD 引脚

### SPI1 总线

| 引脚 | CubeMX 信号 | 用途 | 配置/说明 |
| --- | --- | --- | --- |
| `PA5` | `SPI1_SCK` | LCD SPI 时钟 | SPI1 Master |
| `PA6` | `SPI1_MISO` | LCD SPI MISO | SPI1 Full Duplex |
| `PA7` | `SPI1_MOSI` | LCD SPI MOSI | SPI1 Full Duplex |

SPI1 当前配置：

- `Mode = SPI_MODE_MASTER`
- `Direction = SPI_DIRECTION_2LINES`
- `CLKPolarity = SPI_POLARITY_LOW`
- `CLKPhase = SPI_PHASE_1EDGE`
- `BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8`
- CubeMX 计算速率约 `9.0 MBits/s`

### LCD 控制脚

| 引脚 | CubeMX 标签 | HAL 宏 | 用途 | 初始电平/说明 |
| --- | --- | --- | --- | --- |
| `PB5` | `LCD_CS` | `LCD_CS_Pin` / `LCD_CS_GPIO_Port` | LCD 片选 | 初始 `GPIO_PIN_SET`，低有效 |
| `PB6` | `LCD_LED` | `LCD_LED_Pin` / `LCD_LED_GPIO_Port` | LCD 背光 | 初始 `GPIO_PIN_SET`，高电平开背光 |
| `PB7` | `LCD_DC` | `LCD_DC_Pin` / `LCD_DC_GPIO_Port` | LCD 数据/命令选择 | 初始 `GPIO_PIN_SET` |
| `PB8` | `LCD_RST` | `LCD_RST_Pin` / `LCD_RST_GPIO_Port` | LCD 复位 | 初始 `GPIO_PIN_SET` |

LCD 驱动参数：

- `Inc/lcd.h` 中 `USE_HORIZONTAL = 3`
- `LCD_W = 320`
- `LCD_H = 480`

## 触摸屏引脚

| 引脚 | CubeMX 标签 | HAL 宏 | 触摸宏 | 用途 | 配置/说明 |
| --- | --- | --- | --- | --- | --- |
| `PA0` | `TP_IRQ` | `TP_IRQ_Pin` / `TP_IRQ_GPIO_Port` | `PEN PAin(0)` | 触摸中断/按下检测 | 输入，上拉 |
| `PA1` | `TP_DO` | `TP_DO_Pin` / `TP_DO_GPIO_Port` | `DOUT PAin(1)` | 触摸数据输出 | 输入，上拉 |
| `PB3` | `TP_DIN` | `TP_DIN_Pin` / `TP_DIN_GPIO_Port` | `TDIN PBout(3)` | 触摸数据输入 | 输出，初始 `GPIO_PIN_SET` |
| `PB4` | `TP_CS` | `TP_CS_Pin` / `TP_CS_GPIO_Port` | `TCS PBout(4)` | 触摸片选 | 输出，初始 `GPIO_PIN_SET`，低有效 |
| `PA8` | `TP_CLK` | `TP_CLK_Pin` / `TP_CLK_GPIO_Port` | `TCLK PAout(8)` | 触摸时钟 | 输出，初始 `GPIO_PIN_RESET` |

触摸屏使用软件时序访问。因为 `PB3/PB4` 原本属于 JTAG 相关引脚，工程中必须保持 `NOJTAG: JTAG-DP Disabled and SW-DP Enabled`。

触摸校准数据：

- `TOUCH_ADDRESS = 0x0800FFEC`
- 链接脚本 `STM32F103XX_FLASH.ld` 中 `FLASH LENGTH = 63K`
- 芯片标称 64KB Flash，当前预留最后 1KB 给触摸校准等 Flash 数据，避免程序区覆盖。

## AHT20 / BMP280 软件 I2C 引脚

| 引脚 | CubeMX 标签 | HAL 宏 | 用途 | 配置/说明 |
| --- | --- | --- | --- | --- |
| `PB10` | `AHT2_SCL` | `AHT2_SCL_Pin` / `AHT2_SCL_GPIO_Port` | 软件 I2C SCL | 输出，上拉，高速，初始 `GPIO_PIN_SET` |
| `PB11` | `AHT20_SDA` | `AHT20_SDA_Pin` / `AHT20_SDA_GPIO_Port` | 软件 I2C SDA | 输出/输入动态切换，上拉，高速，初始 `GPIO_PIN_SET` |

代码宏对应关系：

| 宏 | 实际引脚 | 说明 |
| --- | --- | --- |
| `AHT20_SCL_PIN(val)` | `PB10` | 写软件 I2C SCL |
| `AHT20_SDA_PIN(val)` | `PB11` | 写软件 I2C SDA |
| `AHT20_SDA_READ` | `PB11` | 读软件 I2C SDA |

BMP280 复用 AHT20 的软件 I2C 底层，因此 BMP280 也使用 `PB10/PB11`。

## 物理按键引脚

按键全部为低电平有效，GPIO 配置为输入上拉。

| 引脚 | CubeMX 标签 | HAL 宏 | 兼容业务宏 | 用途 |
| --- | --- | --- | --- | --- |
| `PA2` | `KEY_LEFT` | `KEY_LEFT_Pin` / `KEY_LEFT_GPIO_Port` | `KEY_LEFT_PIN` / `KEY_LEFT_PORT` | 左翻页 |
| `PA3` | `KEY_RIGHT` | `KEY_RIGHT_Pin` / `KEY_RIGHT_GPIO_Port` | `KEY_RIGHT_PIN` / `KEY_RIGHT_PORT` | 右翻页 |
| `PA4` | `KEY_UP` | `KEY_UP_Pin` / `KEY_UP_GPIO_Port` | `KEY_UP_PIN` / `KEY_UP_PORT` | 阈值加 |
| `PA11` | `KEY_DOWN` | `KEY_DOWN_Pin` / `KEY_DOWN_GPIO_Port` | `KEY_DOWN_PIN` / `KEY_DOWN_PORT` | 阈值减 |
| `PA12` | `KEY_OK` | `KEY_OK_Pin` / `KEY_OK_GPIO_Port` | `KEY_OK_PIN` / `KEY_OK_PORT` | 确认 |

业务参数：

- `KEY_DEBOUNCE_MS = 20`
- `KEY_REPEAT_INTERVAL = 200`

## LED 引脚

LED 为低电平点亮，默认初始化为高电平熄灭。

| 引脚 | CubeMX 标签 | HAL 宏 | 兼容业务宏 | 用途 | 初始电平 |
| --- | --- | --- | --- | --- | --- |
| `PB0` | `LED_GREEN` | `LED_GREEN_Pin` / `LED_GREEN_GPIO_Port` | `LED_GREEN_PIN` / `LED_GPIO_PORT` | 绿灯 | `GPIO_PIN_SET` |
| `PB1` | `LED_RED` | `LED_RED_Pin` / `LED_RED_GPIO_Port` | `LED_RED_PIN` / `LED_GPIO_PORT` | 红灯 | `GPIO_PIN_SET` |

业务代码中 `LED_Control()` 采用 `GPIO_PIN_RESET` 点亮、`GPIO_PIN_SET` 熄灭。

## USART1 引脚

| 引脚 | CubeMX 信号 | 用途 | 配置/说明 |
| --- | --- | --- | --- |
| `PA9` | `USART1_TX` | USART1 发送 | 异步模式 |
| `PA10` | `USART1_RX` | USART1 接收 | 异步模式 |

当前 CubeMX 已初始化 USART1。若业务暂未使用串口，也建议保留该引脚说明，避免后续误占用。

## GPIO 初始化状态汇总

| 引脚组 | 模式 | 上拉/速度 | 初始状态 |
| --- | --- | --- | --- |
| `TP_IRQ`、`TP_DO`、`KEY_LEFT`、`KEY_RIGHT`、`KEY_UP`、`KEY_DOWN`、`KEY_OK` | 输入 | 上拉 | 无输出状态 |
| `LED_GREEN`、`LED_RED`、`TP_DIN`、`TP_CS`、`LCD_CS` | 推挽输出 | 默认速度 | `GPIO_PIN_SET` |
| `AHT2_SCL`、`AHT20_SDA` | 推挽输出 | 上拉，高速 | `GPIO_PIN_SET` |
| `TP_CLK` | 推挽输出 | 高速 | `GPIO_PIN_RESET` |
| `LCD_LED`、`LCD_DC` | 推挽输出 | 中速 | `GPIO_PIN_SET` |
| `LCD_RST` | 推挽输出 | 高速 | `GPIO_PIN_SET` |

## CubeMX 配置检查清单

- `System Core -> SYS -> Debug` 设置为 `Serial Wire`。
- 不要启用完整 JTAG，否则 `PB3/PB4` 会与触摸屏冲突。
- `RCC -> HSE` 使用外部晶振，系统时钟保持 `72 MHz`。
- `SPI1` 使用 Master、Full Duplex，`PA5/PA6/PA7` 不要改动。
- `PB5/PB6/PB7/PB8` 保持为 LCD 控制脚。
- `PA0/PA1/PB3/PB4/PA8` 保持为触摸屏软件时序引脚。
- `PB10/PB11` 保持为 AHT20/BMP280 共用软件 I2C。
- `PA2/PA3/PA4/PA11/PA12` 保持输入上拉，用于低有效按键。
- `PB0/PB1` 保持输出高电平默认熄灭，用于低有效 LED。
