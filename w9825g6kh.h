

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w9825g6kh.h
  * @brief   This file contains all the function prototypes for the
  *          w9825g6kh.c driver for Winbond W9825G6KH SDRAM
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Your Company.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __W9825G6KH_H
#define __W9825G6KH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fmc.h"
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* W9825G6KH Memory Organization */
#define W9825G6KH_SIZE_MB                32      /* 32MB = 256Mbit */
#define W9825G6KH_SIZE_BYTES            (W9825G6KH_SIZE_MB * 1024UL * 1024UL)
#define W9825G6KH_BANK_COUNT             4
#define W9825G6KH_ROW_COUNT              4096    /* 12-bit address: 2^12 = 4096 */
#define W9825G6KH_COLUMN_COUNT           256     /* 8-bit address: 2^8 = 256 */
#define W9825G6KH_PAGE_SIZE_BYTES        512     /* 256 columns × 16-bit = 512 bytes */

/* Memory Address */
#define W9825G6KH_BANK_ADDR              ((uint32_t)0xC0000000)
#define W9825G6KH_END_ADDR               (W9825G6KH_BANK_ADDR + W9825G6KH_SIZE_BYTES - 1)

/* Mode Register Definitions - BIT POSITIONS */
#define W9825G6KH_MR_BURST_LENGTH_POS    0
#define W9825G6KH_MR_BURST_TYPE_POS      3
#define W9825G6KH_MR_CAS_LATENCY_POS     4
#define W9825G6KH_MR_OPERATING_MODE_POS  7
#define W9825G6KH_MR_WRITE_BURST_MODE_POS 9

/* Mode Register Values */
#define W9825G6KH_MR_BURST_LENGTH_1      (0x0000U)
#define W9825G6KH_MR_BURST_LENGTH_2      (0x0001U)
#define W9825G6KH_MR_BURST_LENGTH_4      (0x0002U)
#define W9825G6KH_MR_BURST_LENGTH_8      (0x0003U)
#define W9825G6KH_MR_BURST_TYPE_SEQUENTIAL   (0x0000U)
#define W9825G6KH_MR_BURST_TYPE_INTERLEAVED  (0x0008U)  /* Bit 3 = 1 */
#define W9825G6KH_MR_CAS_LATENCY_2       (0x0020U)  /* Bits 6-4: 010 */
#define W9825G6KH_MR_CAS_LATENCY_3       (0x0030U)  /* Bits 6-4: 011 */ // ← FIXED!
#define W9825G6KH_MR_OPERATING_MODE_STANDARD (0x0000U)
#define W9825G6KH_MR_WRITE_BURST_MODE_PROGRAMMED (0x0000U)  /* Full page bursts */
#define W9825G6KH_MR_WRITE_BURST_MODE_SINGLE (0x0200U)      /* Bit 9 = 1 */

/* Timeouts */
#define W9825G6KH_CMD_TIMEOUT            1000    /* Command timeout in ms */
#define W9825G6KH_INIT_DELAY_MS          1       /* Minimum 100µs delay */
#define W9825G6KH_BUSY_TIMEOUT_MS        1000    /* Timeout for busy state */

/* Exported types ------------------------------------------------------------*/
typedef enum {
    W9825G6KH_OK        = 0x00,
    W9825G6KH_ERROR     = 0x01,
    W9825G6KH_BUSY      = 0x02,
    W9825G6KH_TIMEOUT   = 0x03,
    W9825G6KH_INVALID_PARAM = 0x04
} W9825G6KH_StatusTypeDef;

typedef struct {
    uint32_t TargetBank;         /* FMC_SDRAM_CMD_TARGET_BANK1 or BANK2 */
    uint32_t BurstLength;        /* W9825G6KH_MR_BURST_LENGTH_x */
    uint32_t BurstType;          /* W9825G6KH_MR_BURST_TYPE_x */
    uint32_t CASLatency;         /* W9825G6KH_MR_CAS_LATENCY_x */
    uint32_t OperatingMode;      /* W9825G6KH_MR_OPERATING_MODE_x */
    uint32_t WriteBurstMode;     /* W9825G6KH_MR_WRITE_BURST_MODE_x */
    uint32_t RefreshRate;        /* Auto-refresh timer value */
} W9825G6KH_InitTypeDef;

/* Default Configuration for 100MHz SDRAM clock */
#define W9825G6KH_DEFAULT_CONFIG {                       \
    .TargetBank = FMC_SDRAM_CMD_TARGET_BANK1,            \
    .BurstLength = W9825G6KH_MR_BURST_LENGTH_4,          \
    .BurstType = W9825G6KH_MR_BURST_TYPE_SEQUENTIAL,     \
    .CASLatency = W9825G6KH_MR_CAS_LATENCY_3,            \
    .OperatingMode = W9825G6KH_MR_OPERATING_MODE_STANDARD, \
    .WriteBurstMode = W9825G6KH_MR_WRITE_BURST_MODE_SINGLE, \
    .RefreshRate = 1563                                  /* For 100MHz: (64ms*100MHz*1000)/4096 - 20 */ \
}

/* Exported functions prototypes ---------------------------------------------*/

/* Initialization and Configuration */
W9825G6KH_StatusTypeDef W9825G6KH_Init(SDRAM_HandleTypeDef *hsdram, W9825G6KH_InitTypeDef *Config);
W9825G6KH_StatusTypeDef W9825G6KH_DeInit(void);

/* Memory Access Functions */
W9825G6KH_StatusTypeDef W9825G6KH_WriteBuffer(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t BufferSize);
W9825G6KH_StatusTypeDef W9825G6KH_ReadBuffer(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t BufferSize);
W9825G6KH_StatusTypeDef W9825G6KH_WriteBuffer16(uint16_t *pBuffer, uint32_t WriteAddr, uint32_t NumHalfWords);
W9825G6KH_StatusTypeDef W9825G6KH_ReadBuffer16(uint16_t *pBuffer, uint32_t ReadAddr, uint32_t NumHalfWords);
W9825G6KH_StatusTypeDef W9825G6KH_WriteBuffer32(uint32_t *pBuffer, uint32_t WriteAddr, uint32_t NumWords);
W9825G6KH_StatusTypeDef W9825G6KH_ReadBuffer32(uint32_t *pBuffer, uint32_t ReadAddr, uint32_t NumWords);

/* Memory Operation Functions */
W9825G6KH_StatusTypeDef W9825G6KH_FillBuffer(uint32_t StartAddr, uint32_t BufferSize, uint8_t Value);
W9825G6KH_StatusTypeDef W9825G6KH_FillBuffer16(uint32_t StartAddr, uint32_t NumHalfWords, uint16_t Value);
W9825G6KH_StatusTypeDef W9825G6KH_FillBuffer32(uint32_t StartAddr, uint32_t NumWords, uint32_t Value);
W9825G6KH_StatusTypeDef W9825G6KH_MemoryTest(uint32_t StartAddr, uint32_t TestSize);

/* Refresh Control */
uint32_t W9825G6KH_CalculateRefreshRate(uint32_t SDRAMClockFreqMHz, uint32_t RefreshTimeMs);
W9825G6KH_StatusTypeDef W9825G6KH_SetRefreshRate(uint32_t RefreshRate);

/* Power Management */
W9825G6KH_StatusTypeDef W9825G6KH_EnterSelfRefresh(void);
W9825G6KH_StatusTypeDef W9825G6KH_ExitSelfRefresh(void);
W9825G6KH_StatusTypeDef W9825G6KH_EnterPowerDown(void);
W9825G6KH_StatusTypeDef W9825G6KH_ExitPowerDown(void);

/* Status and Information Functions */
uint32_t W9825G6KH_GetSize(void);
W9825G6KH_StatusTypeDef W9825G6KH_CheckAddress(uint32_t Address);
W9825G6KH_StatusTypeDef W9825G6KH_GetStatus(void);

/* Debug and Diagnostic Functions */
const char* W9825G6KH_StatusToString(W9825G6KH_StatusTypeDef status);
W9825G6KH_StatusTypeDef W9825G6KH_DumpConfig(void);
W9825G6KH_StatusTypeDef W9825G6KH_DebugReadModeRegister(void);
W9825G6KH_StatusTypeDef W9825G6KH_RunDiagnostics(uint32_t test_size);

/* Low-level Functions (for advanced use) */
W9825G6KH_StatusTypeDef W9825G6KH_SendCommand(FMC_SDRAM_CommandTypeDef *Command);
W9825G6KH_StatusTypeDef W9825G6KH_SetModeRegister(uint32_t mode_value);

#ifdef __cplusplus
}
#endif

#endif /* __W9825G6KH_H */
