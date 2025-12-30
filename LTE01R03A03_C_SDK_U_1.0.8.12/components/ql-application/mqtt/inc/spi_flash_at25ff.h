/**  @file
  spi_demo.h

  @brief
  This file is used to define bt demo for different Quectel Project.

*/

/*================================================================
  Copyright (c) 2020 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
=================================================================*/
/*=================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN              WHO         WHAT, WHERE, WHY
------------     -------     -------------------------------------------------------------------------------

=================================================================*/


#ifndef SPI_DEMO_H
#define SPI_DEMO_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ql_api_osi.h"
/*========================================================================
 *  Variable Definition
 *========================================================================*/
#define SPI_DEMO_TASK_PRIO           12
#define SPI_DEMO_TASK_STACK_SIZE     8*1024
#define SPI_DEMO_TASK_EVENT_CNT      4

/** Device ready/busy status bit */
#define AT25_STATUS_RDYBSY          (1 << 0)
/** Device is ready */
#define AT25_STATUS_RDYBSY_READY    (0 << 0)
/** Device is busy with internal operations */
#define AT25_STATUS_RDYBSY_BUSY     (1 << 0)
/** Write enable latch status bit */
#define AT25_STATUS_WEL             (1 << 1)
/** Device is not write enabled */
#define AT25_STATUS_WEL_DISABLED    (0 << 1)
/** Device is write enabled */
#define AT25_STATUS_WEL_ENABLED     (1 << 1)
/** Software protection status bitfield */
#define AT25_STATUS_SWP             (3 << 2)
/** All sectors are software protected */
#define AT25_STATUS_SWP_PROTALL     (3 << 2)
/** Some sectors are software protected */
#define AT25_STATUS_SWP_PROTSOME    (1 << 2)
/** No sector is software protected */
#define AT25_STATUS_SWP_PROTNONE    (0 << 2)
/** Write protect pin status bit */
#define AT25_STATUS_WPP             (1 << 4)
/** Write protect signal is not asserted */
#define AT25_STATUS_WPP_NOTASSERTED (0 << 4)
/** Write protect signal is asserted */
#define AT25_STATUS_WPP_ASSERTED    (1 << 4)
/** Erase/program error bit */
#define AT25_STATUS_EPE             (1 << 5)
/** Erase or program operation was successful */
#define AT25_STATUS_EPE_SUCCESS     (0 << 5)
/** Erase or program error is detected */
#define AT25_STATUS_EPE_ERROR       (1 << 5)
/** Sector protection registers locked bit */
#define AT25_STATUS_SPRL            (1 << 7)
/** Sector protection registers are unlocked */
#define AT25_STATUS_SPRL_UNLOCKED   (0 << 7)
/** Sector protection registers are locked */
#define AT25_STATUS_SPRL_LOCKED     (1 << 7)

/** Read array command code */
#define AT25_READ_ARRAY             0x0B
/** Read array (low frequency) command code */
#define AT25_READ_ARRAY_LF          0x03
/** Block erase command code (4K block) */
#define AT25_BLOCK_ERASE_4K         0x20
/** Block erase command code (32K block) */
#define AT25_BLOCK_ERASE_32K        0x52
/** Block erase command code (64K block) */
#define AT25_BLOCK_ERASE_64K        0xD8
/** Chip erase command code 1 */
#define AT25_CHIP_ERASE_1           0x60
/** Chip erase command code 2 */
#define AT25_CHIP_ERASE_2           0xC7
/** Byte/page program command code */
#define AT25_BYTE_PAGE_PROGRAM      0x02
/** Sequential program mode command code 1 */
#define AT25_SEQUENTIAL_PROGRAM_1   0xAD
/** Sequential program mode command code 2 */
#define AT25_SEQUENTIAL_PROGRAM_2   0xAF
/** Write enable command code */
#define AT25_WRITE_ENABLE           0x06
/** Write disable command code */
#define AT25_WRITE_DISABLE          0x04
/** Protect sector command code */
#define AT25_PROTECT_SECTOR         0x36
/** Unprotect sector command code */
#define AT25_UNPROTECT_SECTOR       0x39
/** Read sector protection registers command code */
#define AT25_READ_SECTOR_PROT       0x3C
/** Read status register command code */
#define AT25_READ_STATUS            0x05
/** Write status register command code */
#define AT25_WRITE_STATUS           0x01
/** Read manufacturer and device ID command code */
#define AT25_READ_JEDEC_ID          0x9F
/** Deep power-down command code */
#define AT25_DEEP_PDOWN             0xB9
/** Resume from deep power-down command code */
#define AT25_RES_DEEP_PDOWN         0xAB

/** Global protection data */
#define AT25_GLOBAL_PROTECT_VALUE         0x3C

/** Sector Protection Register value is 1 (sector is protected) */
#define AT25_SECTOR_PROTECTED_VALUE    0xff

/** Sector Protection Register value is 0 (sector is unprotected) */
#define AT25_SECTOR_UNPROTECTED_VALUE    0x0

/** Protect type code */
#define AT25_TYPE_PROTECT  0x1

/** Unprotect type code */
#define AT25_TYPE_UNPROTECT  0x0

/** Supporting AT25 device type */
#define AT25DFX_041A 0 /* AT25DF041A */
#define AT25DFX_161  1 /* AT25DF161  */
#define AT25DFX_081A 2 /* AT26DF081A */
#define AT25DFX_0161 3 /* AT26DF0161 */
#define AT25DFX_161A 4 /* AT26DF161A */
#define AT25DFX_321  5 /* AT25DF321  */
#define AT25DFX_321A 6 /* AT25DF321A */
#define AT25DFX_512B 7 /* AT25DF512B */
#define AT25DFX_021  8 /* AT25DF021  */
#define AT25DFX_641A 9 /* AT25DF641A */

/** AT25 device ID */
#define AT25DFX_DEV_ID                   0x0008471F
/** AT25 total size */
#define AT25DFX_SIZE                     (4096 * 1024)
/** AT25 block size */
#define AT25DFX_BLOCK_SIZE               (64*1024)
/** AT25 block size */
#define AT25DFX_4K_BLOCK_SIZE               (4*1024)
/** AT25 block erase command */
#define AT25DFX_64K_BLOCK_ERASE_CMD    AT25_BLOCK_ERASE_64K
/** AT25 block erase command */
#define AT25DFX_32K_BLOCK_ERASE_CMD    AT25_BLOCK_ERASE_32K
/** AT25 block erase command */
#define AT25DFX_4K_BLOCK_ERASE_CMD    AT25_BLOCK_ERASE_4K
/** Test size */
#define AT25DFX_TEST_DATA_SIZE   (32)//(1024)

#define AT25DFX_PAGE_SIZE              256

/** Test block start address */
#define AT25DFX_TEST_BLOCK_ADDR  (0)

typedef struct
{
	unsigned short int s_ExtDataFlash_CheckByte;  							//	2
	unsigned char Runptr;
	unsigned int TotalMinuteckt1;      // 
	unsigned int TotalMinuteckt2;      // 
}flashRunTimeParaSturct;

typedef struct
{
	unsigned short int s_ExtDataFlash_CheckByte;  							//	2
	unsigned short int s_ExtDataFlash_IsDataLogOverwritten;  		//	2
	unsigned int s_ExtDataFlash_PageCounter; 									//	4
	unsigned char s_temp_Counter;
	unsigned int s_ExtDataFlash_SendPointer;     // NEW: Read/send pointer
	unsigned char unused[499];				// reduce 
}flashHistoryParaSturct;

/** AT25 operation status, each operation returns one of the following status */
typedef enum at25_status {
	AT25_SUCCESS = 0,  /** Current operation successful */
	AT25_SECTOR_PROTECTED, /** Sector protected */
	AT25_SECTOR_UNPROTECTED,  /** Sector unprotected */
	AT25_ERROR_INIT,  /** Initialization error: p_at25->pdesc is not initialized */
	AT25_ERROR_NOT_FOUND,  /** The specific SerialFlash Not found  */
	AT25_ERROR_WRITE, /** Write error returned by the SerialFlash */
	AT25_ERROR_BUSY,  /** Current operation failed, SerialFlash is busy */
	AT25_ERROR_PROTECTED,  /** Current operation failed, SerialFlash is protected */
	AT25_ERROR_SPI,  /** SPI transfer failed */
	AT25_ERROR  /** Current operation failed */
} at25_status_t;

extern uint8_t flag_flashUpdateEPROM_General;
extern uint8_t flag_flashUpdateEPROM_Schedule;
extern uint8_t flag_flashUpdateEPROM_Schedule_WaitCounter;
extern uint8_t flag_flashUpdateEPROM_General_WaitCounter;
extern flashRunTimeParaSturct g_flashRunTimeParaSturct;
extern flashHistoryParaSturct g_flashhistoryParaSturct;

/*========================================================================
 *  function Definition
 *========================================================================*/
QlOSStatus ql_spi_flash_init(void);

at25_status_t at25dfx_initialize(void);
void at25dfx_set_mem_active(uint8_t cs);
at25_status_t at25dfx_mem_check(void);
at25_status_t at25dfx_read_status(uint8_t *status);
at25_status_t at25dfx_write_status(uint8_t status);
at25_status_t at25dfx_read_sector_protect_status(uint32_t address);
at25_status_t at25dfx_protect_sector(uint32_t address, uint8_t protect_type);
at25_status_t at25dfx_protect_chip(uint8_t protect_type);
at25_status_t at25dfx_erase_chip(void);
at25_status_t at25dfx_erase_block_64K(uint32_t address);
at25_status_t at25dfx_erase_block_32K(uint32_t address);
at25_status_t at25dfx_erase_block_4K(uint32_t address);
at25_status_t at25dfx_read(uint8_t *data, uint16_t size, uint32_t address);
at25_status_t at25dfx_write(uint8_t *data, uint16_t size, uint32_t address);


void ExtFlash_Read_EPROM_General(unsigned char makeDefault);
void ExtFlash_update_EPROM_General(void);
void ExtFlash_Read_EPROM_Schedule(unsigned char makeDefault);
void ExtFlash_update_EPROM_Schedule(void);
void ExtFlash_write_RuntimePara(void);
void ExtFlash_Read_HistoryPara(unsigned char makeDefault);
void ExtFlash_Write_HistoryPara(void);

#ifdef __cplusplus
} /*"C" */
#endif

#endif /* SPI_DEMO_H */


