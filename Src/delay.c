#include "delay.h"

void Sys_Delay_Init(void)
{
}

void delay_ms(u32 nms)
{
    HAL_Delay(nms);
}

void delay_us(u32 nus)
{
    uint32_t ticks = (HAL_RCC_GetHCLKFreq() / 1000000U) * nus / 5U;
    while (ticks--)
    {
        __NOP();
    }
}

void NVIC_Configuration(void)
{
}
