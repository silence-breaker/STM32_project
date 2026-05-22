#ifndef __SYS_H
#define __SYS_H

#include "main.h"
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef volatile uint16_t vu16;

#define SYSTEM_SUPPORT_UCOS 0

#define BITBAND(addr, bitnum) ((addr & 0xF0000000U) + 0x02000000U + ((addr & 0xFFFFFU) << 5) + ((bitnum) << 2))
#define MEM_ADDR(addr) *((volatile unsigned long *)(addr))
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum))

#define GPIOA_ODR_Addr (GPIOA_BASE + 12U)
#define GPIOB_ODR_Addr (GPIOB_BASE + 12U)
#define GPIOC_ODR_Addr (GPIOC_BASE + 12U)
#define GPIOD_ODR_Addr (GPIOD_BASE + 12U)

#define GPIOA_IDR_Addr (GPIOA_BASE + 8U)
#define GPIOB_IDR_Addr (GPIOB_BASE + 8U)
#define GPIOC_IDR_Addr (GPIOC_BASE + 8U)
#define GPIOD_IDR_Addr (GPIOD_BASE + 8U)

#define PAout(n) BIT_ADDR(GPIOA_ODR_Addr, n)
#define PAin(n) BIT_ADDR(GPIOA_IDR_Addr, n)
#define PBout(n) BIT_ADDR(GPIOB_ODR_Addr, n)
#define PBin(n) BIT_ADDR(GPIOB_IDR_Addr, n)
#define PCout(n) BIT_ADDR(GPIOC_ODR_Addr, n)
#define PCin(n) BIT_ADDR(GPIOC_IDR_Addr, n)
#define PDout(n) BIT_ADDR(GPIOD_ODR_Addr, n)
#define PDin(n) BIT_ADDR(GPIOD_IDR_Addr, n)

void NVIC_Configuration(void);

#endif
