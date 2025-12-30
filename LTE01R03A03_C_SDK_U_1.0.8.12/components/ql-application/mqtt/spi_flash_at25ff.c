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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ql_api_osi.h"

#include "ql_api_spi.h"

#include "ql_log.h"
#include "spi_flash_at25ff.h"
#include "ql_gpio.h"
#include "ql_power.h"
#include "configuration.h"


#define QL_SPI_DEMO_LOG_LEVEL       	        QL_LOG_LEVEL_INFO
#define QL_SPI_DEMO_LOG(msg, ...)			    QL_LOG(QL_SPI_DEMO_LOG_LEVEL, "ql_SPI_FLASH", msg, ##__VA_ARGS__)
#define QL_SPI_DEMO_LOG_PUSH(msg, ...)	        QL_LOG_PUSH("ql_SPI_FLASH", msg, ##__VA_ARGS__)


#define Min(a,b) ((a) < (b) ? (a) : (b)) //(((a)<(b))?(a):(b))
/**
 *  使用SPI DMA注意事项：
 *  1. SPI DMA POLLING和SPI DMA IRQ只支持8bit和16bit的数据传输，不支持32bit数据传输
 *  2. 在使用16bit传输数据时，DMA实际使用的是32bit位宽，需要对输出数据插入一些无效数据，对输入数据去除无效数据，
 *     因此新增16bit dma api用于16bit情况下的读写，api包含ql_spi_write_16bit_dma、ql_spi_read_16bit_dma、
 *     ql_spi_write_read_16bit_dma，这部分代码在demo中开源，客户可自行优化，或直接使用
 *  3. QL_SPI_16BIT_DMA置为1表示使用16bit DMA demo
 */
#define QL_SPI_16BIT_DMA                   0        //16bit DMA demo
#define QL_SPI_DEMO_LOW_POWER_USE          0        //0-not run in lower power mode；1-run in lower power mode

ql_sem_t  spi_demo_write;
ql_sem_t  spi_demo_read;
int spi_power_lock = 0;


#define QL_SPI_DEMO_WAIT_NONE              0
#define QL_SPI_DEMO_WAIT_WRITE             1
#define QL_SPI_DEMO_WAIT_READ              2

unsigned char spi_demo_wait_write_read = QL_SPI_DEMO_WAIT_NONE;

#define QL_CUR_SPI_PORT             QL_SPI_PORT1
#define QL_CUR_SPI_CS_PIN           QL_CUR_SPI1_CS_PIN
#define QL_CUR_SPI_CS_FUNC          QL_CUR_SPI1_CS_FUNC
#define QL_CUR_SPI_CLK_PIN          QL_CUR_SPI1_CLK_PIN
#define QL_CUR_SPI_CLK_FUNC         QL_CUR_SPI1_CLK_FUNC
#define QL_CUR_SPI_DO_PIN           QL_CUR_SPI1_DO_PIN
#define QL_CUR_SPI_DO_FUNC          QL_CUR_SPI1_DO_FUNC
#define QL_CUR_SPI_DI_PIN           QL_CUR_SPI1_DI_PIN
#define QL_CUR_SPI_DI_FUNC          QL_CUR_SPI1_DI_FUNC

#define QL_TYPE_SHIFT_8             8

/** RAM buffer used in this example */
uint8_t ram_buff[AT25DFX_TEST_DATA_SIZE];
flashRunTimeParaSturct g_flashRunTimeParaSturct;
flashHistoryParaSturct g_flashhistoryParaSturct;

// uint8_t flag_flashUpdateEPROM_General;
// uint8_t flag_flashUpdateEPROM_Schedule;
// uint8_t flag_flashUpdateEPROM_Schedule_WaitCounter=10;
// uint32_t flag_flashUpdateEPROM_General_WaitCounter=10;

typedef struct at25_cmd {
	/** Data buffer to be sent or received */
	uint8_t *data;
	/** SerialFlash internal address */
	uint32_t address;
	/** Number of bytes to send/receive */
	uint16_t data_size;
	/** Command byte opcode */
	uint8_t cmd;
	/** Size of command (command byte + address bytes + dummy bytes) in bytes */
	uint8_t cmd_size;
} at25_cmd_t;

uint32_t g_inbuf[QL_SPI_DMA_IRQ_SIZE/4] OSI_CACHE_LINE_ALIGNED;
uint32_t g_outbuf[QL_SPI_DMA_IRQ_SIZE/4] OSI_CACHE_LINE_ALIGNED;

static void ql_spi_flash_task_pthread(void *ctx)
{
    QlOSStatus err = 0;
    ql_errcode_gpio ret;
    ql_spi_clk_e spiclk;
    ql_spi_transfer_mode_e transmode;
    ql_errcode_spi_e err_code = QL_SPI_SUCCESS;
    uint8_t cmd_buffer[6];
    uint8_t cmd_buffer1[6];
    uint32_t address = 0;

//	ql_rtos_task_sleep_s(5);

    if (QL_CUR_SPI_CS_PIN == QUEC_PIN_NONE || QL_CUR_SPI_CS_PIN == QUEC_PIN_NONE || \
        QL_CUR_SPI_DO_PIN == QUEC_PIN_NONE || QL_CUR_SPI_DI_PIN == QUEC_PIN_NONE)
    {
        QL_SPI_DEMO_LOG("pin err");
        goto QL_SPI_EXIT;
    }

    ret = ql_pin_set_func(QL_CUR_SPI_CS_PIN, QL_CUR_SPI_CS_FUNC);
    if (ret != QL_GPIO_SUCCESS)
    {
        QL_SPI_DEMO_LOG("set pin err");
        goto QL_SPI_EXIT;
    }
    
    ret = ql_pin_set_func(QL_CUR_SPI_CLK_PIN, QL_CUR_SPI_CLK_FUNC);
    if (ret != QL_GPIO_SUCCESS)
    {
        QL_SPI_DEMO_LOG("set pin err");
        goto QL_SPI_EXIT;
    }
    ret = ql_pin_set_func(QL_CUR_SPI_DO_PIN, QL_CUR_SPI_DO_FUNC);
    if (ret != QL_GPIO_SUCCESS)
    {
        QL_SPI_DEMO_LOG("set pin err");
        goto QL_SPI_EXIT;
    }
    ret = ql_pin_set_func(QL_CUR_SPI_DI_PIN, QL_CUR_SPI_DI_FUNC);
    if (ret != QL_GPIO_SUCCESS)
    {
        QL_SPI_DEMO_LOG("set pin err");
        goto QL_SPI_EXIT;
    }
    
    //If you use the default parameters, you can initialize it with ql_spi_init
    transmode = QL_SPI_DIRECT_POLLING;
    spiclk = QL_SPI_CLK_100KHZ;

    err_code = ql_spi_init(QL_CUR_SPI_PORT, transmode, spiclk);

    if (err_code != QL_SPI_SUCCESS)
    {
        QL_SPI_DEMO_LOG("spi init err");
        goto QL_SPI_EXIT;
    }

    cmd_buffer[0] = 0x9f; //Device ID
    cmd_buffer[3] = address & 0xff;
    cmd_buffer[2] = (address >> 8) & 0xff;
    cmd_buffer[1] = (address >> 16) & 0xff;

    memset(cmd_buffer1, 0, sizeof(cmd_buffer1));
    ql_spi_cs_low(QL_CUR_SPI_PORT);

    err_code =ql_spi_write(QL_CUR_SPI_PORT, cmd_buffer, 1);
    if (err_code != QL_SPI_SUCCESS)
    {
        QL_SPI_DEMO_LOG("write err");
		// QL_SPI_DEMO_LOG("write err for meter1");        // to print 
    }
    else
    {
        QL_SPI_DEMO_LOG("write ok:%x", cmd_buffer[0]);
        err_code = ql_spi_read(QL_CUR_SPI_PORT, cmd_buffer1, 3);
        if (err_code != QL_SPI_SUCCESS)
        {
            QL_SPI_DEMO_LOG("read err");
        }
        else
        {
            QL_SPI_DEMO_LOG("Read ok:%x", cmd_buffer1[0]);
            QL_SPI_DEMO_LOG("Read ok:%x", cmd_buffer1[1]);
            QL_SPI_DEMO_LOG("read ok:%x", cmd_buffer1[2]);
        }
    }
    ql_spi_cs_high(QL_CUR_SPI_PORT);
    ql_rtos_task_sleep_ms(3);

    /* Unprotect the chip */
	if (at25dfx_protect_chip(AT25_TYPE_UNPROTECT) == AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("AT25_SUCCESS");
	} else {
        QL_SPI_DEMO_LOG("AT25_FAILED");
	}

	/* Check if the SerialFlash is valid */
	if (at25dfx_mem_check() == AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("AT25_SUCCESS");
	} else {
        QL_SPI_DEMO_LOG("AT25_FAILED");
	}
#if 0
    at25dfx_read(ram_buff, 3, AT25DFX_TEST_BLOCK_ADDR);
    QL_SPI_DEMO_LOG("at25dfx_read:%X, %d, %X", ram_buff[0], ram_buff[1], ram_buff[2]);
    if(ram_buff[0] != 0xAB)
    {
        ram_buff[0] = 0xAB;
        ram_buff[1] = 1;
        ram_buff[2] = 0X55;
        // at25dfx_erase_block_64K(AT25DFX_TEST_BLOCK_ADDR);
        at25dfx_write(ram_buff, 3, AT25DFX_TEST_BLOCK_ADDR);
        QL_SPI_DEMO_LOG("AT25_FAILED at25dfx_erase_block at25dfx_write");
    }
    else
    {
        ram_buff[1]=ram_buff[1]+1;
        // at25dfx_erase_block_64K(AT25DFX_TEST_BLOCK_ADDR);
        at25dfx_write(ram_buff, 3, AT25DFX_TEST_BLOCK_ADDR);
        QL_SPI_DEMO_LOG("AT25_SUCCESS:%X, %d, %X", ram_buff[0], ram_buff[1], ram_buff[2]);
    }
#endif
	ExtFlash_Read_RuntimePara(0);
  	ExtFlash_Read_EPROM_General(0);
	ExtFlash_Read_EPROM_Schedule(0);
	ExtFlash_Read_EPROM_PermanentData(0);
	syncExtFlashVariableWithPCBPLCVariable();
	ExtFlash_Read_HistoryPara(0);

	while (1)
    {
        ql_rtos_task_sleep_ms(1000);
        // QL_SPI_DEMO_LOG("Flash Task Running...1st");
		if(flag_flashUpdateEPROM_General == 1)
		{
			flag_flashUpdateEPROM_General_WaitCounter--;
			if(flag_flashUpdateEPROM_General_WaitCounter == 0)
			{
				flag_flashUpdateEPROM_General = 0;
				// if(xSemaphoreTake(sendExternalFlashSemaphore, 1000) == pdTRUE )
				//if(ql_rtos_semaphore_wait(sendExternalFlashSemaphore, 1000) == QL_OSI_SUCCESS )
				{
					ExtFlash_update_EPROM_General();
					// xSemaphoreGive(sendExternalFlashSemaphore);
					//ql_rtos_semaphore_release(sendExternalFlashSemaphore);
				}
				// else
				// {
				// 	flag_flashUpdateEPROM_General = 1;
				// 	flag_flashUpdateEPROM_General_WaitCounter = 1;
				// }
			}
		}

		if(flag_flashUpdateEPROM_Schedule == 1)
		{
			flag_flashUpdateEPROM_Schedule_WaitCounter--;
			if(flag_flashUpdateEPROM_Schedule_WaitCounter == 0)
			{
				flag_flashUpdateEPROM_Schedule = 0;
				// if(xSemaphoreTake(sendExternalFlashSemaphore, 1000) == pdTRUE )
				//if(ql_rtos_semaphore_wait(sendExternalFlashSemaphore, 1000) == QL_OSI_SUCCESS )
				{
					ExtFlash_update_EPROM_Schedule();
					// xSemaphoreGive(sendExternalFlashSemaphore);
					// ql_rtos_semaphore_release(sendExternalFlashSemaphore);
				}
				// else
				// {
				// 	flag_flashUpdateEPROM_Schedule = 1;
				// 	flag_flashUpdateEPROM_Schedule_WaitCounter = 1;
				// }
			}
		}
		
		if(flag_flashUpdateEPROM_PermanentData == 1)
		{
			flag_flashUpdateEPROM_PermanentData_WaitCounter--;
			if(flag_flashUpdateEPROM_PermanentData_WaitCounter == 0)
			{
				flag_flashUpdateEPROM_PermanentData = 0;
				// if(xSemaphoreTake(sendExternalFlashSemaphore, 1000) == pdTRUE )
				//if(ql_rtos_semaphore_wait(sendExternalFlashSemaphore, 1000) == QL_OSI_SUCCESS )
				{
					ExtFlash_update_EPROM_PermanentData();
					// xSemaphoreGive(sendExternalFlashSemaphore);
					// ql_rtos_semaphore_release(sendExternalFlashSemaphore);
				}
				// else
				// {
				// 	flag_flashUpdateEPROM_Schedule = 1;
				// 	flag_flashUpdateEPROM_Schedule_WaitCounter = 1;
				// }
			}
		}
    }

QL_SPI_EXIT:
    ql_spi_release(QL_CUR_SPI_PORT);

    QL_SPI_DEMO_LOG("ql_rtos_task_delete");
	err = ql_rtos_task_delete(NULL);
	if(err != QL_OSI_SUCCESS)
	{
		QL_SPI_DEMO_LOG("task deleted failed");
	}

}

QlOSStatus ql_spi_flash_init(void)
{	
    ql_task_t spi_demo_task = NULL;
	QlOSStatus err = QL_OSI_SUCCESS;
#if QL_SPI_DEMO_LOW_POWER_USE
    spi_power_lock = ql_lpm_wakelock_create("spi_irq", strlen("spi_irq"));
#endif
	err = ql_rtos_task_create(&spi_demo_task, SPI_DEMO_TASK_STACK_SIZE, SPI_DEMO_TASK_PRIO, "ql_spi_flash", ql_spi_flash_task_pthread, NULL, SPI_DEMO_TASK_EVENT_CNT);
	if(err != QL_OSI_SUCCESS)
	{
		QL_SPI_DEMO_LOG("flash_task created failed");
        return err;
	}
    
    return err;
}

/**
 * \brief Start an AT25DFx command transfer. This is a non blocking function. It will
 *  return as soon as the transfer is started.
 *
 * \param pat25_cmd_t  Pointer to the command transfer request.
 *
 * \return AT25_SUCCESS if the transfer has been started successfully; otherwise return
 * AT25_ERROR_SPI if the driver is in use.
 */
static at25_status_t at25dfx_send_command(at25_cmd_t *at25cmd)
{
	uint8_t cmd_buffer[4];
	ql_errcode_spi_e spi_stat;

	/* Enable Chip select corresponding to the SerialFlash */
	//at25dfx_spi_select_device(active_sf_cs);
    ql_spi_cs_low(QL_SPI_PORT1);
	/* Store command and address in command buffer */
	// cmd_buffer[0] = (at25cmd->cmd & 0x000000FF)
	// 		| ((at25cmd->address & 0x0000FF) << 24)
	// 		| ((at25cmd->address & 0x00FF00) << 8)
	// 		| ((at25cmd->address & 0xFF0000) >> 8);
    cmd_buffer[0] = at25cmd->cmd & 0xFF;
    cmd_buffer[3] = (at25cmd->address) & 0xFF;
    cmd_buffer[2] = ((at25cmd->address >> 8) & 0xFF);
    cmd_buffer[1] = ((at25cmd->address >> 16) & 0xFF);
	/* Send the Status Register Read command followed by a dummy data */
	spi_stat = ql_spi_write(QL_SPI_PORT1, (uint8_t *) cmd_buffer, at25cmd->cmd_size);//at25dfx_spi_write_packet((uint16_t *) cmd_buffer, at25cmd->cmd_size);

	if (spi_stat != QL_SPI_SUCCESS) {
		
        QL_SPI_DEMO_LOG("at25dfx_send_command: spi_stat=%d", spi_stat);
        return AT25_ERROR_SPI;
	}

	/* Receive the manufacturer and device ID */
    if(at25cmd->data_size > 0)
    {
        if ((at25cmd->cmd == AT25_BYTE_PAGE_PROGRAM)
                || (at25cmd->cmd == AT25_WRITE_STATUS)) {
            spi_stat = ql_spi_write(QL_SPI_PORT1, at25cmd->data, at25cmd->data_size);//at25dfx_spi_write_packet(at25cmd->data, at25cmd->data_size);
        } else {
            spi_stat = ql_spi_read(QL_SPI_PORT1, at25cmd->data, at25cmd->data_size);//at25dfx_spi_read_packet(at25cmd->data,  at25cmd->data_size);
        }

        if (spi_stat != QL_SPI_SUCCESS) {
            
            QL_SPI_DEMO_LOG("at25dfx_send_command: spi_stat=%d", spi_stat);
            return AT25_ERROR_SPI;
        }
    }

	/* Disable chip select */
	//at25dfx_spi_deselect_device(active_sf_cs);
    ql_spi_cs_high(QL_SPI_PORT1);

	return (AT25_SUCCESS);
}

/**
 * \brief  Wait for the SerialFlash device to be ready to accept new commands.
 *
 * \return AT25_SUCCESS if successful; otherwise failed.
 */
static at25_status_t at25dfx_wait_ready(void)
{
	at25_status_t op_stat;
	uint8_t at25_stat;
	uint8_t ready = 0;

	/* Read status register and check busy bit */
	while (!ready) {
		op_stat = at25dfx_read_status(&at25_stat);
		if (op_stat != AT25_SUCCESS) {
            return AT25_ERROR_SPI;
			return op_stat;
		}

		/* Exit when the device is ready */
		if ((at25_stat & AT25_STATUS_RDYBSY) == AT25_STATUS_RDYBSY_READY) {
			ready = 1;
		}
	}
	return AT25_SUCCESS;
}

/**
 * \brief Read and return the SerialFlash device ID.
 *
 * \param p_dev_id  Pointer to the data of the device ID.
 *
 * \return AT25_SUCCESS if the device ID has been read out; otherwise failed.
 */
static at25_status_t at25dfx_read_dev_id(uint32_t *dev_id)
{
	at25_status_t op_stat;
	at25_cmd_t at25cmd;

	/* Issue a read ID command */
	at25cmd.cmd = AT25_READ_JEDEC_ID;
	at25cmd.cmd_size = 1;
	at25cmd.data = (uint8_t *) dev_id;
	at25cmd.data_size = 3;
	at25cmd.address = 0;
	op_stat = at25dfx_send_command(&at25cmd);
	*dev_id &= 0x00FFFFFF;

	return op_stat;
}

/**
 * \brief Enable critical write operation on a SerialFlash device, such as sector
 * protection, status register, etc.
 *
 * \return AT25_SUCCESS if the device has been unprotected; otherwise return
 * AT25_ERROR_PROTECTED.
 */
static at25_status_t at25dfx_enable_write(void)
{
	at25_status_t op_stat;
	at25_cmd_t at25cmd;

	/* Issue a write enable command */
	at25cmd.cmd = AT25_WRITE_ENABLE;
	at25cmd.cmd_size = 1;
	at25cmd.data = NULL;
	at25cmd.data_size = 0;
	at25cmd.address = 0;
	op_stat = at25dfx_send_command(&at25cmd);

	return op_stat;
}

// /**
//  * \brief Initialize the SerialFlash.
//  *
//  * \return AT25_SUCCESS for success, AT25_ERROR_INIT for error.
//  */
// at25_status_t at25dfx_initialize(void)
// {
// 	at25dfx_spi_init();

// 	return AT25_SUCCESS;
// }

// /**
//  * \brief Select the SerialFlash by the corresponding chip select.
//  *
//  * \param cs  SerialFlash chip select.
//  */
// void at25dfx_set_mem_active(uint8_t cs)
// {
// 	active_sf_cs = cs;
// }

/**
 * \brief Check if the SerialFlash is valid. It will read the device id from the device and compare the
 * value set in the configuration file.
 *
 * \return AT25_SUCCESS for success, AT25_ERROR_NOT_FOUND for error.
 */
at25_status_t at25dfx_mem_check(void)
{
	uint32_t dev_id = 0x0;

	/* Read SerialFlash device id */
	at25dfx_read_dev_id(&dev_id);

	if (dev_id == AT25DFX_DEV_ID) {
        QL_SPI_DEMO_LOG("at25dfx_mem_check: dev_id=%08x", dev_id);
		return AT25_SUCCESS;
	} else {
        QL_SPI_DEMO_LOG("at25dfx_mem_check fail");
		return AT25_ERROR_NOT_FOUND;
	}
}

/**
 * \brief Read and return the status register of the SerialFlash.
 *
 * \param status  Pointer to an AT25 device status.
 *
 * \return AT25_SUCCESS for success, otherwise for error.
 */
at25_status_t at25dfx_read_status(uint8_t *status)
{
	at25_status_t op_stat;
	at25_cmd_t at25cmd;

	/* Issue a read status command */
	at25cmd.cmd = AT25_READ_STATUS;
	at25cmd.cmd_size = 1;
	at25cmd.data = status;
	at25cmd.data_size = 1;
	at25cmd.address = 0;
	op_stat = at25dfx_send_command(&at25cmd);

	return op_stat;
}

/**
 * \brief Write the given value in the status register of the SerialFlash device.
 *
 * \param status  Status to write.
 *
 * \return AT25_SUCCESS if successful; otherwise failed. 
 */
at25_status_t at25dfx_write_status(uint8_t status)
{
	at25_status_t op_stat;
	at25_cmd_t at25cmd;

	/* Issue a write status command */
	at25cmd.cmd = AT25_WRITE_STATUS;
	at25cmd.cmd_size = 1;
	at25cmd.data = (uint8_t *)&status;
	at25cmd.data_size = 1;
	at25cmd.address = 0;
	op_stat = at25dfx_send_command(&at25cmd);

	return op_stat;
}

/**
 * \brief Read sector protection status.
 *
 * \param ul_address  Sector address to be read.
 *
 * \return Sector protect status, AT25_ERROR when failed.
 */
at25_status_t at25dfx_read_sector_protect_status(uint32_t address)
{
	at25_cmd_t at25cmd;
	uint8_t at25_stat;

	/* Issue a read sector protection status command */
	at25cmd.cmd = AT25_READ_SECTOR_PROT;
	at25cmd.cmd_size = 4;
	at25cmd.data = (uint8_t *)&at25_stat;
	at25cmd.data_size = 1;
	at25cmd.address = address;
	at25dfx_send_command(&at25cmd);

	switch (at25_stat) {
	case AT25_SECTOR_PROTECTED_VALUE:
		return AT25_SECTOR_PROTECTED;

	case AT25_SECTOR_UNPROTECTED_VALUE:
		return AT25_SECTOR_UNPROTECTED;

	default:
		return AT25_ERROR;
	}
}

/**
 * \brief Protect/unprotect the specific sector.
 *
 * \param address  Address to be protected.
 * \param protect_type  AT25_TYPE_PROTECT to protect the sector, AT25_TYPE_UNPROTECT to unprotect. 
 *
 * \return Sector protect operation status.
 */
at25_status_t at25dfx_protect_sector(uint32_t address, uint8_t protect_type)
{
	at25_status_t op_stat;
	at25_cmd_t at25cmd;

	/* Enable write operation first */
	at25dfx_enable_write();

	/* Issue a read ID command */
	if (protect_type == AT25_TYPE_PROTECT) {
		at25cmd.cmd = AT25_PROTECT_SECTOR;
	} else {
		at25cmd.cmd = AT25_UNPROTECT_SECTOR;
	}

	at25cmd.cmd_size = 4;
	at25cmd.data = NULL;
	at25cmd.data_size = 0;
	at25cmd.address = address;
	op_stat = at25dfx_send_command(&at25cmd);

	return op_stat;
}

/**
 * \brief Protect the SerialFlash device.
 *
 * \param protect_type  AT25_TYPE_PROTECT to protect the sector, AT25_TYPE_UNPROTECT to unprotect. 
 *
 * \return AT25_SUCCESS if the device has been protected; otherwise return the AT25 error code.
 */
at25_status_t at25dfx_protect_chip(uint8_t protect_type)
{
	at25_status_t op_stat;
	uint8_t at25_stat;

	/* Perform a global unprotect command */
	op_stat = at25dfx_enable_write();
	if (op_stat != AT25_SUCCESS)
    QL_SPI_DEMO_LOG("at25dfx_protect_chip: op_stat=%d", op_stat);
		return op_stat;

	if (protect_type == AT25_TYPE_PROTECT) {
		/* Check the new status */
		op_stat = at25dfx_read_status(&at25_stat);
		if (op_stat != AT25_SUCCESS) {
            QL_SPI_DEMO_LOG("at25dfx_protect_chip: op_stat=%d", op_stat);
			return op_stat;
		}

		op_stat = at25dfx_write_status(at25_stat | 
				AT25_GLOBAL_PROTECT_VALUE);
		if (op_stat != AT25_SUCCESS) {
            QL_SPI_DEMO_LOG("at25dfx_protect_chip: op_stat=%d", op_stat);
			return op_stat;
		}
	} else {
		op_stat = at25dfx_write_status(0);
		if (op_stat != AT25_SUCCESS) {
            QL_SPI_DEMO_LOG("at25dfx_protect_chip: op_stat=%d", op_stat);
			return op_stat;
		}
	}

	/* Check the new status */
	op_stat = at25dfx_read_status(&at25_stat);
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_protect_chip: op_stat=%d", op_stat);
		return op_stat;
	}

	if (protect_type == AT25_TYPE_PROTECT) {
		if ((at25_stat & AT25_STATUS_SWP) != AT25_STATUS_SWP) {
            QL_SPI_DEMO_LOG("at25dfx_protect_chip: op_stat=%d", op_stat);
			return AT25_ERROR;
		}
	} else {
		if ((at25_stat & (AT25_STATUS_SPRL | AT25_STATUS_SWP)) != 0) {
            QL_SPI_DEMO_LOG("at25dfx_protect_chip: op_stat=%d", op_stat);
			return AT25_ERROR;
		}
	}
	return AT25_SUCCESS;
}

/**
 * \brief Erase all the content of the memory chip.
 *
 * \return AT25_SUCCESS if the device has been unprotected; otherwise return
 * AT25_ERROR_PROTECTED.
 */
at25_status_t at25dfx_erase_chip(void)
{
	at25_status_t op_stat;
	uint8_t at25_stat;
	at25_cmd_t at25cmd;

	/* Check if the flash is unprotected */
	op_stat = at25dfx_read_status(&at25_stat);
	if (op_stat != AT25_SUCCESS) {
		return op_stat;
	}

	if ((at25_stat & AT25_STATUS_SWP) != AT25_STATUS_SWP_PROTNONE) {
		return AT25_ERROR_PROTECTED;
	}

	/* Enable critical write operation */
	op_stat = at25dfx_enable_write();
	if (op_stat != AT25_SUCCESS) {
		return op_stat;
	}

	/* Erase the chip */
	at25cmd.cmd = AT25_CHIP_ERASE_2;
	at25cmd.cmd_size = 1;
	at25cmd.data = NULL;
	at25cmd.data_size = 0;
	at25cmd.address = 0;
	op_stat = at25dfx_send_command(&at25cmd);

	if (op_stat != AT25_SUCCESS) {
		return op_stat;
	}

	/* Wait for transfer to finish */
	op_stat = at25dfx_wait_ready();
	return op_stat;
}

/**
 *\brief  Erase the specified block of the SerialFlash.
 *
 * \param address  Address of the block to erase.
 *
 * \return AT25_SUCCESS if successful; otherwise return AT25_ERROR_PROTECTED if the
 * device is protected or AT25_ERROR_BUSY if busy executing a command.
 */
at25_status_t at25dfx_erase_block_64K(uint32_t address)
{
	at25_status_t op_stat;
	uint8_t at25_stat;
	at25_cmd_t at25cmd;

	/* Check if beyond the memory size */
	if (address > AT25DFX_SIZE) {
		return AT25_ERROR;
	}

	/* Check if the flash is ready and unprotected */
	op_stat = at25dfx_read_status(&at25_stat);
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	if ((at25_stat & AT25_STATUS_RDYBSY) != AT25_STATUS_RDYBSY_READY) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return AT25_ERROR_BUSY;
	} else if ((at25_stat & AT25_STATUS_SWP) !=
			AT25_STATUS_SWP_PROTNONE) {
                QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return AT25_ERROR_PROTECTED;
	}

	/* Enable critical write operation */
	op_stat = at25dfx_enable_write();
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	/* Start the block erase command */
	at25cmd.cmd = AT25DFX_64K_BLOCK_ERASE_CMD;
	at25cmd.cmd_size = 4;
	at25cmd.data = NULL;
	at25cmd.data_size = 0;
	at25cmd.address = address;
	op_stat = at25dfx_send_command(&at25cmd);

	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	/* Wait for transfer to finish */
	op_stat = at25dfx_wait_ready();
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	return AT25_SUCCESS;
}
/**
 *\brief  Erase the specified block of the SerialFlash.
 *
 * \param address  Address of the block to erase.
 *
 * \return AT25_SUCCESS if successful; otherwise return AT25_ERROR_PROTECTED if the
 * device is protected or AT25_ERROR_BUSY if busy executing a command.
 */
at25_status_t at25dfx_erase_block_32K(uint32_t address)
{
	at25_status_t op_stat;
	uint8_t at25_stat;
	at25_cmd_t at25cmd;

	/* Check if beyond the memory size */
	if (address > AT25DFX_SIZE) {
		return AT25_ERROR;
	}

	/* Check if the flash is ready and unprotected */
	op_stat = at25dfx_read_status(&at25_stat);
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	if ((at25_stat & AT25_STATUS_RDYBSY) != AT25_STATUS_RDYBSY_READY) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return AT25_ERROR_BUSY;
	} else if ((at25_stat & AT25_STATUS_SWP) !=
			AT25_STATUS_SWP_PROTNONE) {
                QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return AT25_ERROR_PROTECTED;
	}

	/* Enable critical write operation */
	op_stat = at25dfx_enable_write();
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	/* Start the block erase command */
	at25cmd.cmd = AT25DFX_32K_BLOCK_ERASE_CMD;
	at25cmd.cmd_size = 4;
	at25cmd.data = NULL;
	at25cmd.data_size = 0;
	at25cmd.address = address;
	op_stat = at25dfx_send_command(&at25cmd);

	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	/* Wait for transfer to finish */
	op_stat = at25dfx_wait_ready();
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	return AT25_SUCCESS;
}

/**
 *\brief  Erase the specified block of the SerialFlash.
 *
 * \param address  Address of the block to erase.
 *
 * \return AT25_SUCCESS if successful; otherwise return AT25_ERROR_PROTECTED if the
 * device is protected or AT25_ERROR_BUSY if busy executing a command.
 */
at25_status_t at25dfx_erase_block_4K(uint32_t address)
{
	at25_status_t op_stat;
	uint8_t at25_stat;
	at25_cmd_t at25cmd;

	/* Check if beyond the memory size */
	if (address > AT25DFX_SIZE) {
		return AT25_ERROR;
	}

	/* Check if the flash is ready and unprotected */
	op_stat = at25dfx_read_status(&at25_stat);
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	if ((at25_stat & AT25_STATUS_RDYBSY) != AT25_STATUS_RDYBSY_READY) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return AT25_ERROR_BUSY;
	} else if ((at25_stat & AT25_STATUS_SWP) !=
			AT25_STATUS_SWP_PROTNONE) {
                QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return AT25_ERROR_PROTECTED;
	}

	/* Enable critical write operation */
	op_stat = at25dfx_enable_write();
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	/* Start the block erase command */
	at25cmd.cmd = AT25DFX_4K_BLOCK_ERASE_CMD;
	at25cmd.cmd_size = 4;
	at25cmd.data = NULL;
	at25cmd.data_size = 0;
	at25cmd.address = address;
	op_stat = at25dfx_send_command(&at25cmd);

	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	/* Wait for transfer to finish */
	op_stat = at25dfx_wait_ready();
	if (op_stat != AT25_SUCCESS) {
        QL_SPI_DEMO_LOG("at25dfx_erase_block: op_stat=%d", op_stat);
		return op_stat;
	}

	return AT25_SUCCESS;
}

/**
 * \brief Write data at the specified address on the serial firmware SerialFlash. The
 * page(s) to program must have been erased prior to writing. This function
 * handles page boundary crossing automatically.
 *
 * \param data  Data buffer.
 * \param size  Number of bytes in buffer.
 * \param address  Write address.
 *
 * \return AT25_SUCCESS if successful; otherwise, return AT25_WRITE_ERROR if there has
 * been an error during the data programming.
 */
at25_status_t at25dfx_write(uint8_t *data, uint16_t size, uint32_t address)
{
	uint32_t write_size;
	at25_status_t op_stat;
	uint8_t at25_stat;
	at25_cmd_t at25cmd;
	uint32_t  current_addr=0,last_addr = 0;

	/* Check if beyond the memory size */
	if ((size + address) > AT25DFX_SIZE) {
		
        QL_SPI_DEMO_LOG("at25dfx_write: address=%d, size=%d", address, size);
        return AT25_ERROR;
	}
	// if((size > 0) && (size <= (4*1024)))
	// {
	// 	at25dfx_erase_block_4K(AT25DFX_TEST_BLOCK_ADDR);
	// }
	current_addr = address;
	last_addr = current_addr+size;

	while((current_addr < last_addr) && ((current_addr % 4096) == 0))
//	while(current_addr < last_addr)
	{
		op_stat=at25dfx_erase_block_4K(current_addr); //SECTOR_COUNT
		if(op_stat!= AT25_SUCCESS){
			return op_stat;
		}
		current_addr+=AT25DFX_4K_BLOCK_SIZE;
	}

	/* Program one page after another */
	while (size > 0) {
		/* Compute the number of bytes to program in page */
		write_size = Min(size,AT25DFX_PAGE_SIZE - (address % AT25DFX_PAGE_SIZE));

		/* Enable critical write operation */
		op_stat = at25dfx_enable_write();
        if (op_stat != AT25_SUCCESS) {
			
            QL_SPI_DEMO_LOG("at25dfx_write: op_stat=%d", op_stat);
            return op_stat;
		}

		at25cmd.cmd = AT25_BYTE_PAGE_PROGRAM;
		at25cmd.cmd_size = 4;
		at25cmd.data = data;
		at25cmd.data_size = write_size;
		at25cmd.address = address;

		/* Program page */
		op_stat = at25dfx_send_command(&at25cmd);
		if (op_stat != AT25_SUCCESS) {
			
            QL_SPI_DEMO_LOG("at25dfx_write: op_stat=%d", op_stat);
            return op_stat;
		}

		/* Poll the SerialFlash status register until the operation is achieved */
		op_stat = at25dfx_wait_ready();
		if (op_stat != AT25_SUCCESS) {
			
            QL_SPI_DEMO_LOG("at25dfx_write: op_stat=%d", op_stat);
            return op_stat;
		}

		/* Make sure that the write has no error */
		op_stat = at25dfx_read_status(&at25_stat);
		if (op_stat != AT25_SUCCESS) {
			
            QL_SPI_DEMO_LOG("at25dfx_write: op_stat=%d", op_stat);
            return op_stat;
		}

		if ((at25_stat & AT25_STATUS_EPE) == AT25_STATUS_EPE_ERROR) {
			
            QL_SPI_DEMO_LOG("at25dfx_write: op_stat=%d", op_stat);
            return AT25_ERROR_WRITE;
		}

		data += write_size;
		size -= write_size;
		address += write_size;
	}

	return AT25_SUCCESS;
}

/**
 * \brief Read data from the specified address on the SerialFlash.
 *
 * \param data  Data buffer.
 * \param size  Number of bytes to read.
 * \param address  Read address.
 *
 * \return AT25_SUCCESS if successful; otherwise, failed.
 */
at25_status_t at25dfx_read(uint8_t *data, uint16_t size, uint32_t address)
{
	at25_status_t op_stat;
	at25_cmd_t at25cmd;

	/* Check if beyond the memory size */
	if ((size + address) > AT25DFX_SIZE) {
		
        
        return AT25_ERROR;
	}
    QL_SPI_DEMO_LOG("at25dfx_read: size=%d address=%d", size, address); 
	/* Initialize a Read command to be sent through SPI */
	at25cmd.cmd = AT25_READ_ARRAY_LF;
	at25cmd.cmd_size = 4;
	at25cmd.data = data;
	at25cmd.data_size = size;
	at25cmd.address = address;

	/* Start a read operation */
	op_stat = at25dfx_send_command(&at25cmd);

	return op_stat;
}