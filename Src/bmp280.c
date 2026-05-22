#include "bmp280.h"

// BMP280 复用 AHT20 的软件 I2C (PB10/PB11)
// 使用 AHT20 的 I2C 底层函数

// Write register
static void BMP280_WriteReg(u8 reg, u8 val)
{
    AHT20_Start();
    AHT20_Send_Byte(BMP280_ADDR << 1);
    AHT20_Wait_Ack();
    AHT20_Send_Byte(reg);
    AHT20_Wait_Ack();
    AHT20_Send_Byte(val);
    AHT20_Wait_Ack();
    AHT20_Stop();
}

// Read register
static u8 BMP280_ReadReg(u8 reg)
{
    u8 val;
    AHT20_Start();
    AHT20_Send_Byte(BMP280_ADDR << 1);
    AHT20_Wait_Ack();
    AHT20_Send_Byte(reg);
    AHT20_Wait_Ack();
    AHT20_Start();
    AHT20_Send_Byte((BMP280_ADDR << 1) | 0x01);
    AHT20_Wait_Ack();
    val = AHT20_Read_Byte(0);
    AHT20_Stop();
    return val;
}

// Read calibration data
static u16 dig_T1;
static u16 dig_P1;
static s16 dig_T2;
static s16 dig_P2;
static s16 dig_T3;
static s16 dig_P3;
static s16 dig_P4;
static s16 dig_P5;
static s16 dig_P6;
static s16 dig_P7;
static s16 dig_P8;
static s16 dig_P9;
static s32 t_fine;

static void BMP280_ReadCal(void)
{
    u8 cal[24];
    u8 i;
    AHT20_Start();
    AHT20_Send_Byte(BMP280_ADDR << 1);
    AHT20_Wait_Ack();
    AHT20_Send_Byte(0x88);
    AHT20_Wait_Ack();
    AHT20_Start();
    AHT20_Send_Byte((BMP280_ADDR << 1) | 0x01);
    AHT20_Wait_Ack();
    for(i = 0; i < 23; i++)
        cal[i] = AHT20_Read_Byte(1);
    cal[23] = AHT20_Read_Byte(0);
    AHT20_Stop();

    dig_T1 = (cal[1] << 8) | cal[0];
    dig_T2 = (s16)((cal[3] << 8) | cal[2]);
    dig_T3 = (s16)((cal[5] << 8) | cal[4]);
    dig_P1 = (cal[7] << 8) | cal[6];
    dig_P2 = (s16)((cal[9] << 8) | cal[8]);
    dig_P3 = (s16)((cal[11] << 8) | cal[10]);
    dig_P4 = (s16)((cal[13] << 8) | cal[12]);
    dig_P5 = (s16)((cal[15] << 8) | cal[14]);
    dig_P6 = (s16)((cal[17] << 8) | cal[16]);
    dig_P7 = (s16)((cal[19] << 8) | cal[18]);
    dig_P8 = (s16)((cal[21] << 8) | cal[20]);
    dig_P9 = (s16)((cal[23] << 8) | cal[22]);
}

// BMP280 Init
void BMP280_Init(void)
{
    delay_ms(100);

    // Check chip ID
    if(BMP280_ReadReg(0xD0) != 0x58) return;

    // Read calibration
    BMP280_ReadCal();

    // Config: temp x1, pressure x1, normal mode
    BMP280_WriteReg(0xF4, 0x27);
    // Config: IIR filter off
    BMP280_WriteReg(0xF5, 0x00);
}

// Compensate temperature
static s32 BMP280_Compensate_T(s32 adc_T)
{
    s32 var1, var2, T;
    var1 = ((((adc_T >> 3) - ((s32)dig_T1 << 1))) * ((s32)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((s32)dig_T1)) * ((adc_T >> 4) - ((s32)dig_T1))) >> 12) * ((s32)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// Compensate pressure
static u32 BMP280_Compensate_P(s32 adc_P)
{
    long long var1, var2, p;
    var1 = ((long long)t_fine) - 128000;
    var2 = var1 * var1 * (long long)dig_P6;
    var2 = var2 + ((var1 * (long long)dig_P5) << 17);
    var2 = var2 + (((long long)dig_P4) << 35);
    var1 = ((var1 * var1 * (long long)dig_P3) >> 8) + ((var1 * (long long)dig_P2) << 12);
    var1 = (((((long long)1) << 47) + var1)) * ((long long)dig_P1) >> 33;
    if(var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((long long)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((long long)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((long long)dig_P7) << 4);
    return (u32)(p / 256);
}

// Get pressure (hPa)
u8 BMP280_GetPressure(float *pressure)
{
    u8 data[6];
    s32 adc_T, adc_P;
    u32 press;

    AHT20_Start();
    AHT20_Send_Byte(BMP280_ADDR << 1);
    AHT20_Wait_Ack();
    AHT20_Send_Byte(0xF7);
    AHT20_Wait_Ack();
    AHT20_Start();
    AHT20_Send_Byte((BMP280_ADDR << 1) | 0x01);
    AHT20_Wait_Ack();
    data[0] = AHT20_Read_Byte(1);
    data[1] = AHT20_Read_Byte(1);
    data[2] = AHT20_Read_Byte(1);
    data[3] = AHT20_Read_Byte(1);
    data[4] = AHT20_Read_Byte(1);
    data[5] = AHT20_Read_Byte(0);
    AHT20_Stop();

    adc_P = ((s32)data[0] << 12) | ((s32)data[1] << 4) | (data[2] >> 4);
    adc_T = ((s32)data[3] << 12) | ((s32)data[4] << 4) | (data[5] >> 4);

    BMP280_Compensate_T(adc_T);
    press = BMP280_Compensate_P(adc_P);

    *pressure = (float)press / 100.0f;

    return 0;
}
