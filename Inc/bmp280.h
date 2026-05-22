#ifndef __BMP280_H
#define __BMP280_H

#include "sys.h"
#include "delay.h"
#include "aht20.h"

#define BMP280_ADDR         0x77

// 复用 AHT20 的 I2C 引脚 (PB10/PB11)
#define BMP280_SCL_PIN   AHT20_SCL_PIN
#define BMP280_SDA_PIN   AHT20_SDA_PIN
#define BMP280_SDA_READ  AHT20_SDA_READ

void BMP280_Init(void);
u8 BMP280_GetPressure(float *pressure);

#endif
