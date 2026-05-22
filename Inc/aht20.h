#ifndef __AHT20_H
#define __AHT20_H

#include "sys.h"
#include "delay.h"


#define AHT20_ADDR          0x38
#define AHT20_INIT_CMD      0xE1
#define AHT20_TRIG_MEASURE  0xAC
#define AHT20_SOFT_RESET    0xBA
#define AHT20_GET_STATUS    0x71

// PB10(SCL), PB11(SDA)
#define AHT20_SCL_PIN(val) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, (val)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define AHT20_SDA_PIN(val) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, (val)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define AHT20_SDA_READ HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)

void AHT20_Init(void);
uint8_t AHT20_GetData(float *temperature, float *humidity);
void AHT20_Start(void);
void AHT20_Stop(void);
uint8_t AHT20_Wait_Ack(void);
void AHT20_Ack(void);
void AHT20_NAck(void);
void AHT20_Send_Byte(uint8_t txd);
uint8_t AHT20_Read_Byte(uint8_t ack);

#endif



