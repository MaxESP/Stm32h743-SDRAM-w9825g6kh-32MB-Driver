
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w9825g6kh.c
  * @brief   This file provides driver functions for Winbond W9825G6KH SDRAM
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Your Company.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "w9825g6kh.h"
#include <string.h>
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define W9825G6KH_COMMAND_TIMEOUT    0xFFFFU
#define W9825G6KH_BUSY_TIMEOUT_MS    1000      /* 1 second timeout for busy state */

/* Private variables ---------------------------------------------------------*/
static SDRAM_HandleTypeDef *hsdram_ptr = NULL;
static W9825G6KH_InitTypeDef DeviceConfig = W9825G6KH_DEFAULT_CONFIG;
static uint32_t sdram_size_bytes = W9825G6KH_SIZE_BYTES;

/* Private function prototypes -----------------------------------------------*/
static W9825G6KH_StatusTypeDef W9825G6KH_WaitReady(void);

static W9825G6KH_StatusTypeDef W9825G6KH_CheckAddressRange(uint32_t addr, uint32_t size);
//W9825G6KH_StatusTypeDef W9825G6KH_CheckAddressRange(uint32_t addr, uint32_t size)
static void W9825G6KH_PrintModeRegisterDetails(uint32_t mode_register);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Wait for SDRAM to be ready
  * @retval W9825G6KH status
  */
static W9825G6KH_StatusTypeDef W9825G6KH_WaitReady(void)
{
    uint32_t timeout = 0;

    if (hsdram_ptr == NULL) {
        return W9825G6KH_ERROR;
    }

    /* Wait if SDRAM is busy */
    while (HAL_SDRAM_GetState(hsdram_ptr) == HAL_SDRAM_STATE_BUSY &&
           timeout < W9825G6KH_BUSY_TIMEOUT_MS) {
        HAL_Delay(1);
        timeout++;
    }

    if (timeout >= W9825G6KH_BUSY_TIMEOUT_MS) {
        return W9825G6KH_TIMEOUT;
    }

    return W9825G6KH_OK;
}

/**
  * @brief  Check if address range is valid
  * @param  addr: Starting address (offset from base)
  * @param  size: Size in bytes
  * @retval W9825G6KH status
  */
static W9825G6KH_StatusTypeDef W9825G6KH_CheckAddressRange(uint32_t addr, uint32_t size)
{
    /* NO printf statements here! */
    if (addr >= sdram_size_bytes) {
        return W9825G6KH_INVALID_PARAM;
    }

    if (size == 0) {
        return W9825G6KH_INVALID_PARAM;
    }

    /* Check for overflow */
    if ((addr + size) > sdram_size_bytes) {
        return W9825G6KH_INVALID_PARAM;
    }

    return W9825G6KH_OK;
}

/**
  * @brief  Print detailed mode register information
  * @param  mode_register: Mode register value
  */

//************************************************************************************************
/*static void W9825G6KH_PrintModeRegisterDetails(uint32_t mode_register)
{
    printf("  Mode Register: 0x%08lX\n", mode_register);

     Burst Length
    uint32_t burst_len = mode_register & 0x7;
    printf("    Burst Length: ");
    switch(burst_len) {
        case 0: printf("1\n"); break;
        case 1: printf("2\n"); break;
        case 2: printf("4\n"); break;
        case 3: printf("8\n"); break;
        case 7: printf("Full Page\n"); break;
        default: printf("Reserved\n"); break;
    }

     Burst Type
    printf("    Burst Type: %s\n",
           (mode_register & W9825G6KH_MR_BURST_TYPE_INTERLEAVED) ?
           "Interleaved" : "Sequential");

     CAS Latency
    printf("    CAS Latency: ");
    switch((mode_register >> 4) & 0x3) {
        case 1: printf("2\n"); break;
        case 2: printf("3\n"); break;
        default: printf("Reserved\n"); break;
    }

     Operating Mode
    printf("    Operating Mode: %s\n",
           ((mode_register >> 7) & 0x1) ? "Test Mode" : "Standard");

     Write Burst Mode
    printf("    Write Burst Mode: %s\n",
           (mode_register & W9825G6KH_MR_WRITE_BURST_MODE_SINGLE) ?
           "Single Location" : "Programmed Burst");
}*/

void W9825G6KH_PrintModeRegisterDetails(uint32_t mode_register) {
    printf("  Mode Register: 0x%08lX\n", mode_register);

    // Burst Length
    uint32_t burst_len = mode_register & 0x7;
    const char* burst_str;
    switch(burst_len) {
        case 0: burst_str = "1"; break;
        case 1: burst_str = "2"; break;
        case 2: burst_str = "4"; break;
        case 3: burst_str = "8"; break;
        case 7: burst_str = "Full Page"; break;
        default: burst_str = "Reserved"; break;
    }
    printf("    Burst Length: %s\n", burst_str);

    // Burst Type
    printf("    Burst Type: %s\n", (mode_register & 0x8) ? "Interleaved" : "Sequential");

    // CAS Latency
    uint32_t cas_bits = (mode_register >> 4) & 0x7;
    const char* cas_str;
    switch(cas_bits) {
        case 0x1: cas_str = "1"; break;
        case 0x2: cas_str = "2"; break;
        case 0x3: cas_str = "3"; break;  // This is what 0x32 gives us
        case 0x4: cas_str = "4"; break;
        default: cas_str = "Reserved"; break;
    }
    printf("    CAS Latency: %s\n", cas_str);

    // Operating Mode
    uint32_t op_mode = (mode_register >> 7) & 0x3;
    printf("    Operating Mode: %s\n", op_mode == 0 ? "Standard" : "Reserved");

    // Write Burst Mode
    printf("    Write Burst Mode: %s\n",
           (mode_register & 0x200) ? "Single Location" : "Programmed");
}



/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initializes the W9825G6KH SDRAM device
  * @param  hsdram_param: SDRAM handle pointer
  * @param  Config: Configuration structure
  * @retval W9825G6KH status
  */

W9825G6KH_StatusTypeDef W9825G6KH_Init(SDRAM_HandleTypeDef *hsdram_param, W9825G6KH_InitTypeDef *Config)
{
    FMC_SDRAM_CommandTypeDef Command = {0};
    uint32_t mode_register;

    if (hsdram_param == NULL || Config == NULL) {
        return W9825G6KH_ERROR;
    }

    hsdram_ptr = hsdram_param;
    memcpy(&DeviceConfig, Config, sizeof(W9825G6KH_InitTypeDef));

    printf("=== SDRAM Initialization Started ===\n");

    /* Step 1: Clock Enable Command */
    printf("1. Sending Clock Enable command...\n");
    Command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget = Config->TargetBank;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = 0;

    if (HAL_SDRAM_SendCommand(hsdram_param, &Command, W9825G6KH_COMMAND_TIMEOUT) != HAL_OK) {
        printf("  ERROR: Clock Enable failed\n");
        return W9825G6KH_ERROR;
    }
    printf("  OK: Clock enabled\n");

    /* Step 2: Wait at least 100µs (minimum 2 SD clock cycles) */
    HAL_Delay(1);  /* 1ms is more than enough */

    /* Step 3: Precharge All Command */
    printf("2. Sending Precharge All command...\n");
    Command.CommandMode = FMC_SDRAM_CMD_PALL;
    Command.AutoRefreshNumber = 1;
    if (HAL_SDRAM_SendCommand(hsdram_param, &Command, W9825G6KH_COMMAND_TIMEOUT) != HAL_OK) {
        printf("  ERROR: Precharge All failed\n");
        return W9825G6KH_ERROR;
    }
    printf("  OK: All banks precharged\n");

    /* Step 4: Auto Refresh Command (minimum 2 cycles, 8 is typical) */
    printf("3. Sending Auto Refresh commands (8 cycles)...\n");
    Command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.AutoRefreshNumber = 8;
    if (HAL_SDRAM_SendCommand(hsdram_param, &Command, W9825G6KH_COMMAND_TIMEOUT) != HAL_OK) {
        printf("  ERROR: Auto Refresh failed\n");
        return W9825G6KH_ERROR;
    }
    printf("  OK: Auto refresh completed\n");

    /* Step 5: Load Mode Register Command */
    printf("4. Loading Mode Register...\n");

    // USE CUBEMX'S CAS SETTING, not Config->CASLatency
    uint32_t cube_mx_cas;
    if (hsdram_param->Init.CASLatency == FMC_SDRAM_CAS_LATENCY_2) {
        cube_mx_cas = W9825G6KH_MR_CAS_LATENCY_2;
    } else {
        cube_mx_cas = W9825G6KH_MR_CAS_LATENCY_3;
    }

    mode_register = Config->BurstLength |
                    Config->BurstType |
                    cube_mx_cas |
                    Config->OperatingMode |
                    Config->WriteBurstMode;

    W9825G6KH_PrintModeRegisterDetails(mode_register);

    Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = mode_register;

    if (HAL_SDRAM_SendCommand(hsdram_param, &Command, W9825G6KH_COMMAND_TIMEOUT) != HAL_OK) {
        printf("  ERROR: Load Mode Register failed\n");
        return W9825G6KH_ERROR;
    }
    printf("  OK: Mode register loaded\n");

    /* Step 6: Fix FMC Hardware CAS Configuration */
    printf("5. Verifying FMC hardware CAS configuration...\n");

    // FIXED: Use FMC_Bank5_6_R instead of FMC_Bank5_6
    uint32_t sdcr = FMC_Bank5_6_R->SDCR[0];
    printf("  Current SDCR[0] = 0x%08lX\n", sdcr);

    // Check CAS bits (bits 4:3)
    uint32_t current_cas = (sdcr >> 3) & 0x3;
    printf("  Current FMC CAS bits (4:3) = %lu\n", current_cas);

    // Determine what CAS we should have based on mode register
    uint32_t sdram_cas = (mode_register >> 4) & 0x7;
    uint32_t expected_fmc_cas;

    // Map SDRAM CAS (bits 6-4) to FMC CAS (bits 4:3)
    if (sdram_cas == 0x1) expected_fmc_cas = 0x1;  // CAS1
    else if (sdram_cas == 0x2) expected_fmc_cas = 0x2;  // CAS2
    else if (sdram_cas == 0x3) expected_fmc_cas = 0x3;  // CAS3
    else expected_fmc_cas = 0x3;  // Default to CAS3

    printf("  SDRAM Mode Register CAS = %lu\n", sdram_cas);
    printf("  Expected FMC CAS = %lu\n", expected_fmc_cas);

    // We need FMC hardware to match SDRAM mode register
    if (current_cas != expected_fmc_cas) {
        printf("  Patching FMC CAS from %lu to %lu...\n", current_cas, expected_fmc_cas);
        sdcr &= ~(0x3 << 3);      // Clear bits 4:3
        sdcr |= (expected_fmc_cas << 3);  // Set to correct value

        // FIXED: Use FMC_Bank5_6_R for writing too
        FMC_Bank5_6_R->SDCR[0] = sdcr;
        printf("  New SDCR[0] = 0x%08lX\n", sdcr);
    } else {
        printf("  OK: FMC hardware matches SDRAM CAS configuration\n");
    }

    /* Step 7: Set Refresh Rate */
    printf("6. Setting Refresh Rate to %lu...\n", Config->RefreshRate);
    if (HAL_SDRAM_ProgramRefreshRate(hsdram_param, Config->RefreshRate) != HAL_OK) {
        printf("  ERROR: Set Refresh Rate failed\n");
        return W9825G6KH_ERROR;
    }
    printf("  OK: Refresh rate configured\n");

    /* Final delay and status check */
    HAL_Delay(10);
    printf("=== SDRAM Initialization Complete ===\n\n");

    // Print final configuration
    printf("  CAS Latency: %s\n", sdram_cas == 0x3 ? "3" :
                                   sdram_cas == 0x2 ? "2" :
                                   sdram_cas == 0x1 ? "1" : "Unknown");
    printf("  Refresh Rate: %lu\n", Config->RefreshRate);

    return W9825G6KH_OK;
}



/**
  * @brief  Deinitializes SDRAM
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_DeInit(void)
{
    if (hsdram_ptr != NULL) {
        /* Send power down command if needed */
        FMC_SDRAM_CommandTypeDef Command = {0};
        Command.CommandMode = FMC_SDRAM_CMD_POWERDOWN_MODE;
        Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
        HAL_SDRAM_SendCommand(hsdram_ptr, &Command, W9825G6KH_COMMAND_TIMEOUT);
    }

    hsdram_ptr = NULL;
    memset(&DeviceConfig, 0, sizeof(W9825G6KH_InitTypeDef));
    printf("SDRAM deinitialized\n");

    return W9825G6KH_OK;
}

/**
  * @brief  Send custom SDRAM command (for advanced use)
  * @param  Command: Pointer to FMC command structure
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_SendCommand(FMC_SDRAM_CommandTypeDef *Command)
{
    if (hsdram_ptr == NULL || Command == NULL) {
        return W9825G6KH_ERROR;
    }

    if (HAL_SDRAM_SendCommand(hsdram_ptr, Command, W9825G6KH_COMMAND_TIMEOUT) != HAL_OK) {
        return W9825G6KH_ERROR;
    }

    return W9825G6KH_OK;
}

/**
  * @brief  Set mode register directly
  * @param  mode_value: Mode register value
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_SetModeRegister(uint32_t mode_value)
{
    FMC_SDRAM_CommandTypeDef Command = {0};

    if (hsdram_ptr == NULL) {
        return W9825G6KH_ERROR;
    }

    Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    Command.CommandTarget = DeviceConfig.TargetBank;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = mode_value;

    printf("Setting mode register to: 0x%08lX\n", mode_value);
    W9825G6KH_PrintModeRegisterDetails(mode_value);

    if (HAL_SDRAM_SendCommand(hsdram_ptr, &Command, W9825G6KH_COMMAND_TIMEOUT) != HAL_OK) {
        return W9825G6KH_ERROR;
    }

    DeviceConfig.BurstLength = mode_value & 0x7;
    DeviceConfig.BurstType = mode_value & W9825G6KH_MR_BURST_TYPE_INTERLEAVED;
    DeviceConfig.CASLatency = mode_value & 0x30;
    DeviceConfig.WriteBurstMode = mode_value & W9825G6KH_MR_WRITE_BURST_MODE_SINGLE;

    return W9825G6KH_OK;
}

/* Memory Access Functions ---------------------------------------------------*/

/**
  * @brief  Writes a buffer to SDRAM (8-bit)
  * @param  pBuffer: Pointer to data buffer
  * @param  WriteAddr: Write address (offset from SDRAM base)
  * @param  BufferSize: Size of buffer in bytes
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_WriteBuffer(uint8_t *pBuffer, uint32_t WriteAddr, uint32_t BufferSize)
{
    uint8_t *pSdram;
    W9825G6KH_StatusTypeDef status;

    if (pBuffer == NULL) {
        return W9825G6KH_INVALID_PARAM;
    }

    status = W9825G6KH_CheckAddressRange(WriteAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint8_t *)(W9825G6KH_BANK_ADDR + WriteAddr);
    memcpy(pSdram, pBuffer, BufferSize);

    /* Ensure memory barrier for cache coherency */
    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Reads a buffer from SDRAM (8-bit)
  * @param  pBuffer: Pointer to data buffer
  * @param  ReadAddr: Read address (offset from SDRAM base)
  * @param  BufferSize: Size of buffer in bytes
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_ReadBuffer(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t BufferSize)
{
    uint8_t *pSdram;
    W9825G6KH_StatusTypeDef status;

    if (pBuffer == NULL) {
        return W9825G6KH_INVALID_PARAM;
    }

    status = W9825G6KH_CheckAddressRange(ReadAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint8_t *)(W9825G6KH_BANK_ADDR + ReadAddr);
    memcpy(pBuffer, pSdram, BufferSize);

    /* Ensure memory barrier for cache coherency */
    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Writes a buffer to SDRAM (16-bit)
  * @param  pBuffer: Pointer to data buffer
  * @param  WriteAddr: Write address (offset from SDRAM base)
  * @param  NumHalfWords: Number of half-words (16-bit) to write
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_WriteBuffer16(uint16_t *pBuffer, uint32_t WriteAddr, uint32_t NumHalfWords)
{
    uint16_t *pSdram;
    W9825G6KH_StatusTypeDef status;
    uint32_t BufferSize = NumHalfWords * 2;

    if (pBuffer == NULL) {
        return W9825G6KH_INVALID_PARAM;
    }

    status = W9825G6KH_CheckAddressRange(WriteAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint16_t *)(W9825G6KH_BANK_ADDR + WriteAddr);

    /* Use optimized copy for aligned access */
    if (((uint32_t)pSdram & 0x1) == 0 && ((uint32_t)pBuffer & 0x1) == 0) {
        /* Aligned access - use memcpy for better performance */
        memcpy(pSdram, pBuffer, BufferSize);
    } else {
        /* Unaligned access - copy word by word */
        for (uint32_t i = 0; i < NumHalfWords; i++) {
            pSdram[i] = pBuffer[i];
        }
    }

    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Reads a buffer from SDRAM (16-bit)
  * @param  pBuffer: Pointer to data buffer
  * @param  ReadAddr: Read address (offset from SDRAM base)
  * @param  NumHalfWords: Number of half-words (16-bit) to read
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_ReadBuffer16(uint16_t *pBuffer, uint32_t ReadAddr, uint32_t NumHalfWords)
{
    uint16_t *pSdram;
    W9825G6KH_StatusTypeDef status;
    uint32_t BufferSize = NumHalfWords * 2;

    if (pBuffer == NULL) {
        return W9825G6KH_INVALID_PARAM;
    }

    status = W9825G6KH_CheckAddressRange(ReadAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint16_t *)(W9825G6KH_BANK_ADDR + ReadAddr);

    /* Use optimized copy for aligned access */
    if (((uint32_t)pSdram & 0x1) == 0 && ((uint32_t)pBuffer & 0x1) == 0) {
        /* Aligned access - use memcpy for better performance */
        memcpy(pBuffer, pSdram, BufferSize);
    } else {
        /* Unaligned access - copy word by word */
        for (uint32_t i = 0; i < NumHalfWords; i++) {
            pBuffer[i] = pSdram[i];
        }
    }

    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Writes a buffer to SDRAM (32-bit)
  * @param  pBuffer: Pointer to data buffer
  * @param  WriteAddr: Write address (offset from SDRAM base)
  * @param  NumWords: Number of words (32-bit) to write
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_WriteBuffer32(uint32_t *pBuffer, uint32_t WriteAddr, uint32_t NumWords)
{
    uint32_t *pSdram;
    W9825G6KH_StatusTypeDef status;
    uint32_t BufferSize = NumWords * 4;

    if (pBuffer == NULL) {
        return W9825G6KH_INVALID_PARAM;
    }

    status = W9825G6KH_CheckAddressRange(WriteAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint32_t *)(W9825G6KH_BANK_ADDR + WriteAddr);

    /* Ensure address is 32-bit aligned for optimal performance */
    if (((uint32_t)pSdram & 0x3) == 0 && ((uint32_t)pBuffer & 0x3) == 0) {
        /* Aligned access - use memcpy for better performance */
        memcpy(pSdram, pBuffer, BufferSize);
    } else {
        /* Unaligned access - slower */
        uint8_t *pSdram8 = (uint8_t *)pSdram;
        uint8_t *pBuffer8 = (uint8_t *)pBuffer;
        for (uint32_t i = 0; i < BufferSize; i++) {
            pSdram8[i] = pBuffer8[i];
        }
    }

    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Reads a buffer from SDRAM (32-bit)
  * @param  pBuffer: Pointer to data buffer
  * @param  ReadAddr: Read address (offset from SDRAM base)
  * @param  NumWords: Number of words (32-bit) to read
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_ReadBuffer32(uint32_t *pBuffer, uint32_t ReadAddr, uint32_t NumWords)
{
    uint32_t *pSdram;
    W9825G6KH_StatusTypeDef status;
    uint32_t BufferSize = NumWords * 4;

    if (pBuffer == NULL) {
        return W9825G6KH_INVALID_PARAM;
    }

    status = W9825G6KH_CheckAddressRange(ReadAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint32_t *)(W9825G6KH_BANK_ADDR + ReadAddr);

    /* Ensure address is 32-bit aligned for optimal performance */
    if (((uint32_t)pSdram & 0x3) == 0 && ((uint32_t)pBuffer & 0x3) == 0) {
        /* Aligned access - use memcpy for better performance */
        memcpy(pBuffer, pSdram, BufferSize);
    } else {
        /* Unaligned access - slower */
        uint8_t *pSdram8 = (uint8_t *)pSdram;
        uint8_t *pBuffer8 = (uint8_t *)pBuffer;
        for (uint32_t i = 0; i < BufferSize; i++) {
            pBuffer8[i] = pSdram8[i];
        }
    }

    __DSB();

    return W9825G6KH_OK;
}

/* Memory Operation Functions ------------------------------------------------*/

/**
  * @brief  Fills SDRAM memory with a specific 8-bit value
  * @param  StartAddr: Starting address (offset from SDRAM base)
  * @param  BufferSize: Size in bytes
  * @param  Value: 8-bit value to fill
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_FillBuffer(uint32_t StartAddr, uint32_t BufferSize, uint8_t Value)
{
    uint8_t *pSdram;
    W9825G6KH_StatusTypeDef status;

    status = W9825G6KH_CheckAddressRange(StartAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint8_t *)(W9825G6KH_BANK_ADDR + StartAddr);
    memset(pSdram, Value, BufferSize);

    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Fills SDRAM memory with a 16-bit value
  * @param  StartAddr: Starting address (offset from SDRAM base)
  * @param  NumHalfWords: Number of half-words (16-bit)
  * @param  Value: 16-bit value to fill
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_FillBuffer16(uint32_t StartAddr, uint32_t NumHalfWords, uint16_t Value)
{
    uint16_t *pSdram;
    W9825G6KH_StatusTypeDef status;
    uint32_t BufferSize = NumHalfWords * 2;

    status = W9825G6KH_CheckAddressRange(StartAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint16_t *)(W9825G6KH_BANK_ADDR + StartAddr);

    for (uint32_t i = 0; i < NumHalfWords; i++) {
        pSdram[i] = Value;
    }

    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Fills SDRAM memory with a 32-bit value
  * @param  StartAddr: Starting address (offset from SDRAM base)
  * @param  NumWords: Number of words (32-bit)
  * @param  Value: 32-bit value to fill
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_FillBuffer32(uint32_t StartAddr, uint32_t NumWords, uint32_t Value)
{
    uint32_t *pSdram;
    W9825G6KH_StatusTypeDef status;
    uint32_t BufferSize = NumWords * 4;

    status = W9825G6KH_CheckAddressRange(StartAddr, BufferSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint32_t *)(W9825G6KH_BANK_ADDR + StartAddr);

    for (uint32_t i = 0; i < NumWords; i++) {
        pSdram[i] = Value;
    }

    __DSB();

    return W9825G6KH_OK;
}

/**
  * @brief  Performs a comprehensive memory test
  * @param  StartAddr: Starting address (offset from SDRAM base)
  * @param  TestSize: Size in bytes to test
  * @retval W9825G6KH status
  */
W9825G6KH_StatusTypeDef W9825G6KH_MemoryTest(uint32_t StartAddr, uint32_t TestSize)
{
    uint32_t *pSdram;
    uint32_t num_words;
    W9825G6KH_StatusTypeDef status;

    static const uint32_t test_patterns[] = {
        0x00000000,  /* All zeros */
        0xFFFFFFFF,  /* All ones */
        0x55555555,  /* Alternating 01 */
        0xAAAAAAAA,  /* Alternating 10 */
        0x33333333,  /* Alternating 0011 */
        0xCCCCCCCC,  /* Alternating 1100 */
        0x0F0F0F0F,  /* Alternating 00001111 */
        0xF0F0F0F0,  /* Alternating 11110000 */
    };

    const uint32_t num_patterns = sizeof(test_patterns) / sizeof(test_patterns[0]);

    status = W9825G6KH_CheckAddressRange(StartAddr, TestSize);
    if (status != W9825G6KH_OK) {
        return status;
    }

    /* Wait if SDRAM is busy */
    status = W9825G6KH_WaitReady();
    if (status != W9825G6KH_OK) {
        return status;
    }

    pSdram = (uint32_t *)(W9825G6KH_BANK_ADDR + StartAddr);
    num_words = TestSize / 4;

    printf("Running memory test (%lu bytes, %lu words)...\n", TestSize, num_words);

    for (uint32_t p = 0; p < num_patterns; p++) {
        uint32_t pattern = test_patterns[p];
        printf("  Pattern 0x%08lX: ", pattern);

        /* Write pattern */
        for (uint32_t i = 0; i < num_words; i++) {
            pSdram[i] = pattern;
        }

        /* Read back and verify */
        uint32_t errors = 0;
        for (uint32_t i = 0; i < num_words; i++) {
            if (pSdram[i] != pattern) {
                errors++;
                if (errors == 1) {  /* Print first error only */
                    printf("FAIL at word %lu (got 0x%08lX)\n",
                           i, pSdram[i]);
                }
            }
        }

        if (errors == 0) {
            printf("PASS\n");
        } else {
            printf("  Total errors: %lu\n", errors);
            return W9825G6KH_ERROR;
        }
    }

    /* Test incremental pattern */
    printf("  Incremental pattern: ");
    for (uint32_t i = 0; i < num_words; i++) {
        pSdram[i] = i;
    }

    uint32_t errors = 0;
    for (uint32_t i = 0; i < num_words; i++) {
        if (pSdram[i] != i) {
            errors++;
            if (errors == 1) {
                printf("FAIL at word %lu (got 0x%08lX)\n",
                       i, pSdram[i]);
            }
        }
    }

    if (errors == 0) {
        printf("PASS\n");
    } else {
        printf("  Total errors: %lu\n", errors);
        return W9825G6KH_ERROR;
    }

    printf("Memory test completed successfully\n");
    return W9825G6KH_OK;
}

/* Refresh Control Functions -------------------------------------------------*/

/**
  * @brief  Calculates refresh rate based on clock frequency
  * @param  SDRAMClockFreqMHz: SDRAM clock frequency in MHz
  * @param  RefreshTimeMs: Refresh time in milliseconds (typically 64ms)
  * @retval Calculated refresh rate value
  */



uint32_t W9825G6KH_CalculateRefreshRate(uint32_t clock_freq_mhz, uint32_t refresh_time_ms)
{
    // For 100MHz, 64ms refresh, 4096 rows
    // Correct calculation: (64ms / 4096) / (10ns) = 1562.5 ≈ 1563

    uint32_t refresh_interval_ns = (refresh_time_ms * 1000000UL) / 4096;  // 15625ns
    uint32_t clock_period_ns = 1000 / clock_freq_mhz;  // 10ns for 100MHz

    uint32_t refresh_count = (refresh_interval_ns + clock_period_ns/2) / clock_period_ns;

    // Round up to even number (FMC requirement)
    if (refresh_count & 1) refresh_count++;

    return refresh_count;  // Should be 1563 for 100MHz
}
