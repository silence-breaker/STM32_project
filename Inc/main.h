/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;

typedef struct {
    int high;
    int low;
    u8 edit_state;
} Threshold_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TP_IRQ_Pin GPIO_PIN_0
#define TP_IRQ_GPIO_Port GPIOA
#define TP_DO_Pin GPIO_PIN_1
#define TP_DO_GPIO_Port GPIOA
#define KEY_LEFT_Pin GPIO_PIN_2
#define KEY_LEFT_GPIO_Port GPIOA
#define KEY_RIGHT_Pin GPIO_PIN_3
#define KEY_RIGHT_GPIO_Port GPIOA
#define KEY_UP_Pin GPIO_PIN_4
#define KEY_UP_GPIO_Port GPIOA
#define LED_STATUS_Pin GPIO_PIN_0
#define LED_STATUS_GPIO_Port GPIOB
#define AHT2_SCL_Pin GPIO_PIN_10
#define AHT2_SCL_GPIO_Port GPIOB
#define AHT20_SDA_Pin GPIO_PIN_11
#define AHT20_SDA_GPIO_Port GPIOB
#define TP_CLK_Pin GPIO_PIN_8
#define TP_CLK_GPIO_Port GPIOA
#define KEY_DOWN_Pin GPIO_PIN_11
#define KEY_DOWN_GPIO_Port GPIOA
#define KEY_OK_Pin GPIO_PIN_12
#define KEY_OK_GPIO_Port GPIOA
#define TP_DIN_Pin GPIO_PIN_3
#define TP_DIN_GPIO_Port GPIOB
#define TP_CS_Pin GPIO_PIN_4
#define TP_CS_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_5
#define LCD_CS_GPIO_Port GPIOB
#define LCD_LED_Pin GPIO_PIN_6
#define LCD_LED_GPIO_Port GPIOB
#define LCD_DC_Pin GPIO_PIN_7
#define LCD_DC_GPIO_Port GPIOB
#define LCD_RST_Pin GPIO_PIN_8
#define LCD_RST_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define PAGE_HOME       0
#define PAGE_TEMP       1
#define PAGE_HUMIDITY   2
#define PAGE_PRESSURE   3
#define PAGE_WAVE       4
#define PAGE_COUNT      5

#define TEMP_HIGH_DEFAULT   350
#define TEMP_LOW_DEFAULT    100
#define HUM_HIGH_DEFAULT    800
#define HUM_LOW_DEFAULT     300
#define PRESS_HIGH_DEFAULT  10500
#define PRESS_LOW_DEFAULT   9800

#define THRESH_EDIT_NONE    0
#define THRESH_EDIT_HIGH    1
#define THRESH_EDIT_LOW     2

#define NAV_BTN_Y_START     280
#define NAV_BTN_Y_END       320
#define NAV_BTN_WIDTH       (480 / PAGE_COUNT)

#define KEY_LEFT_PIN        KEY_LEFT_Pin
#define KEY_RIGHT_PIN       KEY_RIGHT_Pin
#define KEY_UP_PIN          KEY_UP_Pin
#define KEY_DOWN_PIN        KEY_DOWN_Pin
#define KEY_OK_PIN          KEY_OK_Pin

#define KEY_LEFT_PORT       KEY_LEFT_GPIO_Port
#define KEY_RIGHT_PORT      KEY_RIGHT_GPIO_Port
#define KEY_UP_PORT         KEY_UP_GPIO_Port
#define KEY_DOWN_PORT       KEY_DOWN_GPIO_Port
#define KEY_OK_PORT         KEY_OK_GPIO_Port

#define LED_STATUS_PIN      LED_STATUS_Pin
#define LED_GPIO_PORT       LED_STATUS_GPIO_Port

#define KEY_DEBOUNCE_MS     20
#define KEY_REPEAT_INTERVAL 200

extern u8 current_page;
extern int g_temp;
extern int g_humidity;
extern int g_pressure;
extern Threshold_t g_temp_thresh;
extern Threshold_t g_hum_thresh;
extern Threshold_t g_press_thresh;
extern int g_temp_alert;
extern int g_hum_alert;
extern int g_press_alert;

extern const unsigned char gImage_normal[];
extern const unsigned char gImage_high_temp[];
extern const unsigned char gImage_high_humidity[];
extern const unsigned char gImage_highQIYA[];
extern const unsigned char gImage_peixiao[];

void KEY_Init(void);
u8 KEY_Scan(void);
void HandleKey(u8 key);
void SwitchPage(u8 page);
void RefreshPage(void);
void DrawPage_Home(void);
void DrawPage_Temp(void);
void DrawPage_Humidity(void);
void DrawPage_Pressure(void);
void DrawPage_Wave(void);
void UpdateSensorData(void);
void main_test(void);

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
