#include "flash.h"

#if FLASH_WREN

#if FLASH_SIZE < 256
#define FLASH_SECTOR_SIZE 1024U
#else
#define FLASH_SECTOR_SIZE 2048U
#endif

static u16 FLASH_BUF[FLASH_SECTOR_SIZE / 2U];

static void FLASH_Write_NoCheck(u32 WriteAddr, u16 *pBuffer, u16 NumToWrite)
{
    u16 i;

    for (i = 0; i < NumToWrite; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, WriteAddr, pBuffer[i]);
        WriteAddr += 2U;
    }
}

void FLASH_Write(u32 WriteAddr, u16 *pBuffer, u16 NumToWrite)
{
    u32 secpos;
    u16 secoff;
    u16 secremain;
    u16 i;
    u32 offaddr;

    if (WriteAddr < FLASH_BASE_ADDRESS || WriteAddr >= (FLASH_BASE_ADDRESS + 1024U * FLASH_SIZE))
    {
        return;
    }

    HAL_FLASH_Unlock();
    offaddr = WriteAddr - FLASH_BASE_ADDRESS;
    secpos = offaddr / FLASH_SECTOR_SIZE;
    secoff = (offaddr % FLASH_SECTOR_SIZE) / 2U;
    secremain = FLASH_SECTOR_SIZE / 2U - secoff;
    if (NumToWrite <= secremain)
    {
        secremain = NumToWrite;
    }

    while (1)
    {
        FLASH_Read(secpos * FLASH_SECTOR_SIZE + FLASH_BASE_ADDRESS, FLASH_BUF, FLASH_SECTOR_SIZE / 2U);
        for (i = 0; i < secremain; i++)
        {
            if (FLASH_BUF[secoff + i] != 0xFFFFU)
            {
                break;
            }
        }

        if (i < secremain)
        {
            FLASH_EraseInitTypeDef erase = {0};
            u32 page_error = 0;

            erase.TypeErase = FLASH_TYPEERASE_PAGES;
            erase.PageAddress = secpos * FLASH_SECTOR_SIZE + FLASH_BASE_ADDRESS;
            erase.NbPages = 1;
            HAL_FLASHEx_Erase(&erase, &page_error);

            for (i = 0; i < secremain; i++)
            {
                FLASH_BUF[i + secoff] = pBuffer[i];
            }
            FLASH_Write_NoCheck(secpos * FLASH_SECTOR_SIZE + FLASH_BASE_ADDRESS, FLASH_BUF, FLASH_SECTOR_SIZE / 2U);
        }
        else
        {
            FLASH_Write_NoCheck(WriteAddr, pBuffer, secremain);
        }

        if (NumToWrite == secremain)
        {
            break;
        }

        secpos++;
        secoff = 0;
        pBuffer += secremain;
        WriteAddr += (u32)secremain * 2U;
        NumToWrite -= secremain;
        if (NumToWrite > (FLASH_SECTOR_SIZE / 2U))
        {
            secremain = FLASH_SECTOR_SIZE / 2U;
        }
        else
        {
            secremain = NumToWrite;
        }
    }

    HAL_FLASH_Lock();
}

#endif

void FLASH_Read(u32 ReadAddr, u16 *pBuffer, u16 NumToRead)
{
    u16 i;

    for (i = 0; i < NumToRead; i++)
    {
        pBuffer[i] = *(vu16 *)ReadAddr;
        ReadAddr += 2U;
    }
}
