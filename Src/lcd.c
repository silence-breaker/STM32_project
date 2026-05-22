//////////////////////////////////////////////////////////////////////////////////
// Originally: QDtech-TFT LCD driver for STM32 (QDtech, lcdwiki.com).
// Adapted for STM32F103C8T6 + ST7796S 4-inch panel, HAL library.
// Original hardware: STM32F103RCT6 (72 MHz, 12 MHz HSE).
// Current build:    STM32F103C8T6 (72 MHz, 8 MHz HSE), GPIO bit-bang for ctrl
//                   pins, SPI1 (PA5/PA7) for data.
/****************************************************************************************************
// ===== Power =====
//   LCD       STM32
//   VCC <---> 3.3V or 5V (see module spec)
//   GND <---> GND
//
// ===== Data lines (SPI1) =====
//   SDI(MOSI) <---> PA7   (LCD SDI / data input)
//   SDO(MISO) <---> PA6   (only needed if reading from LCD)
//   SCK       <---> PA5   (SPI clock)
//
// ===== Control lines =====
//   LED   <---> PB6   (backlight, HIGH = on)
//   DC/RS <---> PB7   (data/command select)
//   RST   <---> PB8   (hardware reset, active low)
//   CS    <---> PB5   (chip select, active low)
//
// ===== Touch panel =====
//   T_IRQ <---> PA0
//   T_DO  <---> PA1
//   T_DIN <---> PB3
//   T_CS  <---> PB4
//   T_CLK <---> PA8
**************************************************************************************************/
 /* @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**************************************************************************************************/
#include "lcd.h"
#include "stdlib.h"

extern SPI_HandleTypeDef hspi1;


// LCD device state (defaults to portrait orientation).
_lcd_dev lcddev;

// Foreground / background colors used by drawing routines.
uint16_t POINT_COLOR = 0x0000,BACK_COLOR = 0xFFFF;
uint16_t DeviceCode;

/*****************************************************************************
 * @name       :void LCD_WR_REG(uint8_t data)
 * @date       :2018-08-09
 * @function   :Write an 8-bit command to the LCD screen
 * @parameters :data:Command value to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WR_REG(uint8_t data)
{
   LCD_CS_CLR;
   LCD_RS_CLR;
   HAL_SPI_Transmit(&hspi1, &data, 1, 10);
   LCD_CS_SET;
}

/*****************************************************************************
 * @name       :void LCD_WR_DATA(uint8_t data)
 * @date       :2018-08-09
 * @function   :Write an 8-bit data to the LCD screen
 * @parameters :data:data value to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WR_DATA(uint8_t data)
{
   LCD_CS_CLR;
   LCD_RS_SET;
   HAL_SPI_Transmit(&hspi1, &data, 1, 10);
   LCD_CS_SET;
}

/*****************************************************************************
 * @name       :void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
 * @date       :2018-08-09
 * @function   :Write data into registers
 * @parameters :LCD_Reg:Register address
                LCD_RegValue:Data to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{
	LCD_WR_REG(LCD_Reg);
	LCD_WR_DATA(LCD_RegValue);
}

/*****************************************************************************
 * @name       :void LCD_WriteRAM_Prepare(void)
 * @date       :2018-08-09
 * @function   :Write GRAM
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
}

/*****************************************************************************
 * @name       :void Lcd_WriteData_16Bit(uint16_t Data)
 * @date       :2018-08-09
 * @function   :Write an 16-bit command to the LCD screen
 * @parameters :Data:Data to be written
 * @retvalue   :None
******************************************************************************/
void Lcd_WriteData_16Bit(uint16_t Data)
{
   LCD_CS_CLR;
   LCD_RS_SET;
   uint8_t t[2];
   t[0] = Data >> 8;
   t[1] = Data;
   HAL_SPI_Transmit(&hspi1, t, 2, 10);
   LCD_CS_SET;
}

/*****************************************************************************
 * @name       :void LCD_DrawPoint(uint16_t x,uint16_t y)
 * @date       :2018-08-09
 * @function   :Write a pixel data at a specified location
 * @parameters :x:the x coordinate of the pixel
                y:the y coordinate of the pixel
 * @retvalue   :None
******************************************************************************/
void LCD_DrawPoint(uint16_t x,uint16_t y)
{
	LCD_SetCursor(x,y);// position the cursor
	Lcd_WriteData_16Bit(POINT_COLOR);
}

/*****************************************************************************
 * @name       :void LCD_Clear(uint16_t Color)
 * @date       :2018-08-09
 * @function   :Full screen filled LCD screen
 * @parameters :color:Filled color
 * @retvalue   :None
******************************************************************************/
void LCD_Clear(uint16_t Color)
{
  unsigned int i,m;
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);
	LCD_CS_CLR;
	LCD_RS_SET;
	for(i=0;i<lcddev.height;i++)
	{
    for(m=0;m<lcddev.width;m++)
    {
			Lcd_WriteData_16Bit(Color);
		}
	}
	 LCD_CS_SET;
}

/*****************************************************************************
 * @name       :void LCD_GPIOInit(void)
 * @date       :2018-08-09
 * @function   :Initialization LCD screen GPIO (now handled by MX_GPIO_Init)
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void LCD_GPIOInit(void) {
  // Handled by MX_GPIO_Init
}

/*****************************************************************************
 * @name       :void LCD_RESET(void)
 * @date       :2018-08-09
 * @function   :Reset LCD screen
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void LCD_RESET(void)
{
	LCD_RST_CLR;
	HAL_Delay(100);
	LCD_RST_SET;
	HAL_Delay(50);
}

/*****************************************************************************
 * @name       :void LCD_Init(void)
 * @date       :2018-08-09
 * @function   :Initialization LCD screen
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void LCD_Init(void)
{
	// Handled by MX_SPI1_Init();
	LCD_GPIOInit();// LCD GPIO init
 	LCD_RESET();   // LCD hardware reset
//************* ST7796S init sequence **********//
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0xC3);
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0x96);
	LCD_WR_REG(0x36);
	LCD_WR_DATA(0x68);
	LCD_WR_REG(0x3A);
	LCD_WR_DATA(0x05);
	LCD_WR_REG(0xB0);
	LCD_WR_DATA(0x80);
	LCD_WR_REG(0xB6);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x02);
	LCD_WR_REG(0xB5);
	LCD_WR_DATA(0x02);
	LCD_WR_DATA(0x03);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x04);
	LCD_WR_REG(0xB1);
	LCD_WR_DATA(0x80);
	LCD_WR_DATA(0x10);
	LCD_WR_REG(0xB4);
	LCD_WR_DATA(0x00);
	LCD_WR_REG(0xB7);
	LCD_WR_DATA(0xC6);
	LCD_WR_REG(0xC5);
	LCD_WR_DATA(0x24);
	LCD_WR_REG(0xE4);
	LCD_WR_DATA(0x31);
	LCD_WR_REG(0xE8);
	LCD_WR_DATA(0x40);
	LCD_WR_DATA(0x8A);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x29);
	LCD_WR_DATA(0x19);
	LCD_WR_DATA(0xA5);
	LCD_WR_DATA(0x33);
	LCD_WR_REG(0xC2);
	LCD_WR_REG(0xA7);

	LCD_WR_REG(0xE0);
	LCD_WR_DATA(0xF0);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x13);
	LCD_WR_DATA(0x12);
	LCD_WR_DATA(0x12);
	LCD_WR_DATA(0x2B);
	LCD_WR_DATA(0x3C);
	LCD_WR_DATA(0x44);
	LCD_WR_DATA(0x4B);
	LCD_WR_DATA(0x1B);
	LCD_WR_DATA(0x18);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x1D);
	LCD_WR_DATA(0x21);

	LCD_WR_REG(0XE1);
	LCD_WR_DATA(0xF0);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x13);
	LCD_WR_DATA(0x0C);
	LCD_WR_DATA(0x0D);
	LCD_WR_DATA(0x27);
	LCD_WR_DATA(0x3B);
	LCD_WR_DATA(0x44);
	LCD_WR_DATA(0x4D);
	LCD_WR_DATA(0x0B);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x1D);
	LCD_WR_DATA(0x21);

  LCD_WR_REG(0X36);
	LCD_WR_DATA(0xEC);
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0xC3);
	LCD_WR_REG(0xF0);
	LCD_WR_DATA(0x69);
	LCD_WR_REG(0X13);
	LCD_WR_REG(0X11);
	HAL_Delay(120);
	LCD_WR_REG(0X29);

  LCD_direction(USE_HORIZONTAL);// set LCD display orientation
	LCD_LED_ON();                 // turn on backlight
	LCD_Clear(WHITE);             // clear screen to white
}

/*****************************************************************************
 * @name       :void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
 * @date       :2018-08-09
 * @function   :Setting LCD display window
 * @parameters :xStar:the bebinning x coordinate of the LCD display window
								yStar:the bebinning y coordinate of the LCD display window
								xEnd:the endning x coordinate of the LCD display window
								yEnd:the endning y coordinate of the LCD display window
 * @retvalue   :None
******************************************************************************/
void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
{
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(xStar>>8);
	LCD_WR_DATA(0x00FF&xStar);
	LCD_WR_DATA(xEnd>>8);
	LCD_WR_DATA(0x00FF&xEnd);

	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(yStar>>8);
	LCD_WR_DATA(0x00FF&yStar);
	LCD_WR_DATA(yEnd>>8);
	LCD_WR_DATA(0x00FF&yEnd);

	LCD_WriteRAM_Prepare();	// begin writing to GRAM
}

/*****************************************************************************
 * @name       :void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
 * @date       :2018-08-09
 * @function   :Set coordinate value
 * @parameters :Xpos:the  x coordinate of the pixel
								Ypos:the  y coordinate of the pixel
 * @retvalue   :None
******************************************************************************/
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
	LCD_SetWindows(Xpos,Ypos,Xpos,Ypos);
}

/*****************************************************************************
 * @name       :void LCD_direction(uint8_t direction)
 * @date       :2018-08-09
 * @function   :Setting the display direction of LCD screen
 * @parameters :direction:0-0 degree
                          1-90 degree
													2-180 degree
													3-270 degree
 * @retvalue   :None
******************************************************************************/
void LCD_direction(uint8_t direction)
{
	lcddev.setxcmd=0x2A;
	lcddev.setycmd=0x2B;
	lcddev.wramcmd=0x2C;
	switch(direction){
		case 0:
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;
			LCD_WriteReg(0x36,(1<<3)|(1<<6));
		break;
		case 1:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(1<<5));
		break;
		case 2:
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;
			LCD_WriteReg(0x36,(1<<3)|(1<<7));
		break;
		case 3:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(1<<7)|(1<<6)|(1<<5));
		break;
		default:break;
	}
}
