#include "aht20.h"

// AHT20 I2C pins: PB10(SCL), PB11(SDA)
// Following the same style as myiic.c

#define AHT20_SDA_IN() { GPIO_InitTypeDef GPIO_InitStruct = {0}; GPIO_InitStruct.Pin = GPIO_PIN_11; GPIO_InitStruct.Mode = GPIO_MODE_INPUT; GPIO_InitStruct.Pull = GPIO_PULLUP; HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); }
#define AHT20_SDA_OUT() { GPIO_InitTypeDef GPIO_InitStruct = {0}; GPIO_InitStruct.Pin = GPIO_PIN_11; GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); }

// I2C Start
void AHT20_Start(void)
{
    AHT20_SDA_OUT();
    AHT20_SDA_PIN(1);
    AHT20_SCL_PIN(1);
    for(int delay_i=0; delay_i< (4 * 10); delay_i++) {__NOP();};
    AHT20_SDA_PIN(0);
    for(int delay_i=0; delay_i< (4 * 10); delay_i++) {__NOP();};
    AHT20_SCL_PIN(0);
}

// I2C Stop
void AHT20_Stop(void)
{
    AHT20_SDA_OUT();
    AHT20_SCL_PIN(0);
    AHT20_SDA_PIN(0);
    for(int delay_i=0; delay_i< (4 * 10); delay_i++) {__NOP();};
    AHT20_SCL_PIN(1);
    AHT20_SDA_PIN(1);
    for(int delay_i=0; delay_i< (4 * 10); delay_i++) {__NOP();};
}

// Wait ACK
uint8_t AHT20_Wait_Ack(void)
{
    uint16_t ucErrTime = 0;
    AHT20_SDA_IN();
    AHT20_SDA_PIN(1);
    for(int delay_i=0; delay_i< (1 * 10); delay_i++) {__NOP();};
    AHT20_SCL_PIN(1);
    for(int delay_i=0; delay_i< (1 * 10); delay_i++) {__NOP();};
    while (AHT20_SDA_READ)
    {
        ucErrTime++;
        if (ucErrTime > 5000)
        {
            AHT20_Stop();
            return 1;
        }
    }
    AHT20_SCL_PIN(0);
    return 0;
}

// Send ACK
void AHT20_Ack(void)
{
    AHT20_SCL_PIN(0);
    AHT20_SDA_OUT();
    AHT20_SDA_PIN(0);
    for(int delay_i=0; delay_i< (2 * 10); delay_i++) {__NOP();};
    AHT20_SCL_PIN(1);
    for(int delay_i=0; delay_i< (2 * 10); delay_i++) {__NOP();};
    AHT20_SCL_PIN(0);
}

// Send NACK
void AHT20_NAck(void)
{
    AHT20_SCL_PIN(0);
    AHT20_SDA_OUT();
    AHT20_SDA_PIN(1);
    for(int delay_i=0; delay_i< (2 * 10); delay_i++) {__NOP();};
    AHT20_SCL_PIN(1);
    for(int delay_i=0; delay_i< (2 * 10); delay_i++) {__NOP();};
    AHT20_SCL_PIN(0);
}

// Send one byte
void AHT20_Send_Byte(uint8_t txd)
{
    uint8_t t;
    AHT20_SDA_OUT();
    AHT20_SCL_PIN(0);
    for (t = 0; t < 8; t++)
    {
        AHT20_SDA_PIN((txd & 0x80) >> 7);
        txd <<= 1;
        for(int delay_i=0; delay_i< (2 * 10); delay_i++) {__NOP();};
        AHT20_SCL_PIN(1);
        for(int delay_i=0; delay_i< (2 * 10); delay_i++) {__NOP();};
        AHT20_SCL_PIN(0);
    }
}

// Read one byte
uint8_t AHT20_Read_Byte(uint8_t ack)
{
    uint8_t i, receive = 0;
    AHT20_SDA_IN();
    for (i = 0; i < 8; i++)
    {
        AHT20_SCL_PIN(0);
        for(int delay_i=0; delay_i< (2 * 10); delay_i++) {__NOP();};
        AHT20_SCL_PIN(1);
        receive <<= 1;
        if (AHT20_SDA_READ) receive++;
        for(int delay_i=0; delay_i< (1 * 10); delay_i++) {__NOP();};
    }
    AHT20_SCL_PIN(0);
    if (!ack)
        AHT20_NAck();
    else
        AHT20_Ack();
    return receive;
}

// AHT20 GPIO Init
void AHT20_Init(void)
{
    uint8_t status;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    // GPIO initialized by CubeMX in main.c

    AHT20_SCL_PIN(1);
    AHT20_SDA_PIN(1);

    HAL_Delay(100);

    // Soft reset
    AHT20_Start();
    AHT20_Send_Byte(AHT20_ADDR << 1);
    AHT20_Wait_Ack();
    AHT20_Send_Byte(AHT20_SOFT_RESET);
    AHT20_Wait_Ack();
    AHT20_Stop();

    HAL_Delay(20);

    AHT20_Start();
    AHT20_Send_Byte(AHT20_ADDR << 1);
    AHT20_Wait_Ack();
    AHT20_Send_Byte(AHT20_GET_STATUS);
    AHT20_Wait_Ack();
    AHT20_Stop();

    HAL_Delay(1);

    AHT20_Start();
    AHT20_Send_Byte((AHT20_ADDR << 1) | 0x01);
    AHT20_Wait_Ack();
    status = AHT20_Read_Byte(0);
    AHT20_Stop();

    if ((status & 0x08) == 0)
    {
        HAL_Delay(10);
        AHT20_Start();
        AHT20_Send_Byte(AHT20_ADDR << 1);
        AHT20_Wait_Ack();
        AHT20_Send_Byte(AHT20_INIT_CMD);
        AHT20_Wait_Ack();
        AHT20_Send_Byte(0x08);
        AHT20_Wait_Ack();
        AHT20_Send_Byte(0x00);
        AHT20_Wait_Ack();
        AHT20_Stop();
        HAL_Delay(10);
    }
}

// Full measurement
uint8_t AHT20_GetData(float *temperature, float *humidity)
{
    uint8_t rxBuf[6];
    uint32_t S_RH, S_T;
    uint8_t i;

    // Step 1: Trigger measurement
    AHT20_Start();
    AHT20_Send_Byte(AHT20_ADDR << 1);
    if (AHT20_Wait_Ack()) return 1;
    AHT20_Send_Byte(AHT20_TRIG_MEASURE);
    if (AHT20_Wait_Ack()) return 1;
    AHT20_Send_Byte(0x33);
    if (AHT20_Wait_Ack()) return 1;
    AHT20_Send_Byte(0x00);
    if (AHT20_Wait_Ack()) return 1;
    AHT20_Stop();

    // Step 2: Wait 80ms for measurement
    HAL_Delay(80);

    // Step 3: Poll status and read data
    for (i = 0; i < 100; i++)
    {
        AHT20_Start();
        AHT20_Send_Byte((AHT20_ADDR << 1) | 0x01);
        if (AHT20_Wait_Ack()) { AHT20_Stop(); HAL_Delay(1); continue; }
        rxBuf[0] = AHT20_Read_Byte(1);  // Status byte

        if ((rxBuf[0] & 0x80) == 0)     // Bit[7] = 0, data ready
        {
            rxBuf[1] = AHT20_Read_Byte(1);
            rxBuf[2] = AHT20_Read_Byte(1);
            rxBuf[3] = AHT20_Read_Byte(1);
            rxBuf[4] = AHT20_Read_Byte(1);
            rxBuf[5] = AHT20_Read_Byte(0);
            AHT20_Stop();
            break;
        }
        else
        {
            AHT20_Read_Byte(1);
            AHT20_Read_Byte(1);
            AHT20_Read_Byte(1);
            AHT20_Read_Byte(1);
            AHT20_Read_Byte(0);
            AHT20_Stop();
            HAL_Delay(1);
        }
    }

    if (i >= 100) return 1;  // Timeout

    // Step 4: Convert data
    S_RH = ((uint32_t)rxBuf[1] << 12) | ((uint32_t)rxBuf[2] << 4) | (rxBuf[3] >> 4);
    S_T = (((uint32_t)rxBuf[3] & 0x0F) << 16) | ((uint32_t)rxBuf[4] << 8) | rxBuf[5];

    *humidity = (float)S_RH / 1048576.0f * 100.0f;
    *temperature = (float)S_T / 1048576.0f * 200.0f - 50.0f;

    return 0;
}




