
/*================================================================
  Copyright (c) 2021, Quectel Wireless Solutions Co., Ltd. All rights reserved.
  Quectel Wireless Solutions Proprietary and Confidential.
=================================================================*/
    
/*=================================================================

						EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN			  WHO		  WHAT, WHERE, WHY
------------	 -------	 -------------------------------------------------------------------------------

=================================================================*/


/*===========================================================================
 * include files
 ===========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ql_api_osi.h"
#include "ql_log.h"
#include "ql_uart.h"
#include "ql_gpio.h"
#include "ql_pin_cfg.h"
#include "ql_usb.h"
#include "i2c_RTC_LCD.h"
#include "ql_api_sim.h"
#include "json_parser_sp.h"
#include "dlms_meter.h"
#include "Configuration.h"
#include "define.h"
#include "rs232_UART.h"
/*===========================================================================
 *Definition
 ===========================================================================*/
#define QL_UART_DEMO_LOG_LEVEL			QL_LOG_LEVEL_INFO
#define QL_UART_DEMO_LOG(msg, ...)		QL_LOG(QL_UART_DEMO_LOG_LEVEL, "ql_uart_rs232", msg, ##__VA_ARGS__)


#define QL_UART_TASK_STACK_SIZE     		4096
#define QL_UART_TASK_PRIO          	 	    APP_PRIORITY_NORMAL
#define QL_UART_TASK_EVENT_CNT      		5


#define QL_UART_RX_BUFF_SIZE                2048
#define QL_UART_TX_BUFF_SIZE                2048

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define QL_USB_PRINTER_ENABLE	0

/*===========================================================================
 * Variate
 ===========================================================================*/

uint8_t cim_recv_buff[2048];
uint16_t Rec_char;
uint8_t SerialTXBuffer[2048];
uint8_t DLMS_init_flag = 1;
uint8_t ProductionMode = 0;

extern DataTime_t rtc_time;
extern ql_LvlMode  gpio_lvl;
extern uint8_t DI_Final_value[];
extern uint8_t DO_Final_value[];
extern uint8_t FlagswitchInt11;
extern uint8_t FlagswitchInt22;
extern uint8_t FlagswitchInt33;
extern uint8_t FlagswitchInt44;
extern uint8_t fNetworkRegistered;
extern ql_sim_status_e card_status;
extern int mqtt_connected;
extern char csq_sim;
extern uint32_t fMQTTpublishCount;
extern uint8_t ram_buff[];
extern CMD_TYPE current_cmd;
/*===========================================================================
 * Functions
 ===========================================================================*/

void ql_uart_notify_cb(unsigned int ind_type, ql_uart_port_number_e port, unsigned int size)
{
    unsigned char *recv_buff = calloc(1, QL_UART_RX_BUFF_SIZE+1);
    unsigned int real_size = 0;
    int read_len = 0;//write_len = 0;
    // JSON_ERROR_RESPONSE JSON_ret;
    // char ACK_string[100] = {0};
    char dummy[100];
    char *str = dummy;
    QL_UART_DEMO_LOG("UART port %d receive ind type:0x%x, receive data size:%d", port, ind_type, size);
    switch(ind_type)
    {
        case QUEC_UART_RX_OVERFLOW_IND:  //rx buffer overflow
        case QUEC_UART_RX_RECV_DATA_IND:
        {
            while(size > 0)
            {
                memset(recv_buff, 0, QL_UART_RX_BUFF_SIZE+1);
                real_size= MIN(size, QL_UART_RX_BUFF_SIZE);
                
                read_len = ql_uart_read(port, recv_buff, real_size);

                // QL_UART_DEMO_LOG("read_len=%d, recv_data=%x", read_len, recv_buff);

                for(uint16_t i=0;i<read_len;i++)
                {
                    cim_recv_buff[Rec_char++] = recv_buff[i];
                    if(Rec_char >= 2048)
                    {
                        Rec_char = 0;
                    }
                    sprintf(str+2*i,"%02x", recv_buff[i]);
                    #if 0
                    if(buf_position >= 2048)
                    {
                        buf_position = 0;
                    }
                    if(recv_buff[i] == 0)
                    {
                        QL_UART_DEMO_LOG("buf_position=%d, cim_recv_buff=%s", buf_position, cim_recv_buff);
                        JSON_ret = parse_JSON_frame(MQTT,(char *)cim_recv_buff,(char *)ACK_string);
                        if(JSON_ret!=JSON_SUCCESS)
                        {
                            QL_UART_DEMO_LOG("parse_JSON_frame error");
                        }
                        else
                        {
                            QL_UART_DEMO_LOG("parse_JSON_frame OK");
                        }
                        buf_position = 0;
                        memset(cim_recv_buff, 0, 2048);
                        break;
                    }
                    #endif
                }
                QL_UART_DEMO_LOG("recv_data=%s", str);
                if((cim_recv_buff[0] == 0x7F)&&(cim_recv_buff[1] == 0x7E)&&(cim_recv_buff[2] == 0x7F)&&(cim_recv_buff[3] == 0x7E))
                {
                	ProductionMode = 1;
                }
                if((read_len > 0) && (size >= read_len))
                {
                    size -= read_len;
                }
                else
                {
                    break;
                }
            }
            break;
        }
        case QUEC_UART_TX_FIFO_COMPLETE_IND: 
        {
            QL_UART_DEMO_LOG("tx fifo complete");
            break;
        }
    }
    free(recv_buff);
    recv_buff = NULL;
}

static void ql_uart_rs232_thread(void *param)
{
    int ret = 0;
	QlOSStatus err = 0;
    ql_uart_config_s uart_cfg = {0};
    // int write_len = 0;
    //ql_uart_tx_status_e tx_status;
    // unsigned char data[256] = "hello uart demo\r\n";
    // char tempPrintReg[16] = {0};
    ql_rtos_task_sleep_s(5);

    /***********************************************************
	Note start:
        1.If the BAUD rate is QL UART BAUD_AUTO,a string of 'at'should be sent at least once to identify the baud rate.
        2.Once the baud rate is identified, it cannot be changed unless restarted.
    ************************************************************/
    uart_cfg.baudrate = QL_UART_BAUD_9600;
    uart_cfg.flow_ctrl = QL_FC_NONE;
    uart_cfg.data_bit = QL_UART_DATABIT_8;
    uart_cfg.stop_bit = QL_UART_STOP_1;
    uart_cfg.parity_bit = QL_UART_PARITY_NONE;

    ret = ql_uart_set_dcbconfig(QL_UART_PORT_1, &uart_cfg);
    QL_UART_DEMO_LOG("ret: 0x%x", ret);
	if(QL_UART_SUCCESS != ret)
	{
		goto exit;
	}
	
	/***********************************************************
	Note start:
		1. If QL_UART_PORT_1 is selected for use, there is no need to set TX and RX pin and function
		2. According to the QuecOpen GPIO table, user should select the correct PIN to set function
		3. CTS and RTS pins (UART2 and UART3) also need to be initialized if hardware flow control function is required
	************************************************************/
	ret = ql_pin_set_func(QL_UART2_TX_PIN, QL_UART2_TX_FUNC);
	if(QL_GPIO_SUCCESS != ret)
	{
		goto exit;
	}
	ret = ql_pin_set_func(QL_UART2_RX_PIN, QL_UART2_RX_FUNC);
	if(QL_GPIO_SUCCESS != ret)
	{
		goto exit;
	}
	/*Note end*/
	
    ret = ql_uart_open(QL_UART_PORT_1);
    QL_UART_DEMO_LOG("ret: 0x%x", ret);
	
#if QL_USB_PRINTER_ENABLE
	ret = ql_uart_open(QL_USB_PORT_PRINTER);
	QL_UART_DEMO_LOG("ret: 0x%x", ret);
#endif

	if(QL_UART_SUCCESS == ret)
	{
        ret = ql_uart_register_cb(QL_UART_PORT_1, ql_uart_notify_cb);
	    QL_UART_DEMO_LOG("ret: 0x%x", ret);

        // EPROM_General.MeterType = THREEPHASE; 
		// EPROM_General.MeterType = SINGLEPHASE;
//        strcpy((char *)data, "iFPC3000_Quectel_EC200UCN_AA_APPIMAGE_TEST_V1.0.4");
//        write_len = ql_uart_write(QL_UART_PORT_1, data, strlen((char *)data));
//	    QL_UART_DEMO_LOG("write_len:%d", write_len);

        uint16_t counterForInitMeter = 10;

        while(1)
        {

            if((counterForInitMeter++ >= 6)&&(DLMS_init_flag == 1)&&(ProductionMode == 0))
            {
            	counterForInitMeter = 0;
                DLMS_init_flag=0;
                init_DLMS();
                ReceiveTimer = 2;
                DI_Final_value[9]=0;
                if(EPROM_General.MeterType == THREEPHASE)
                {
                    DLMS_Status = SEND_CURRENTL1;
                }
                else if (EPROM_General.MeterType == SINGLEPHASE)
                {
                    DLMS_Status = SEND_CURRENTL_1PHASE;
                }
            }
            else if((DLMS_init_flag == 0)&&(ProductionMode == 0))
            {
                DLMS_Get_data();
                DI_Final_value[9]=1;
                CalculateBulbFailure();
            }
            else
            {
            	parshingDataForProductionMode();
            }
            ql_rtos_task_sleep_ms(500);
        }
	}

exit:
    err = ql_rtos_task_delete(NULL);
	if(err != QL_OSI_SUCCESS)
	{
		QL_UART_DEMO_LOG("task deleted failed");
	}
}

void ql_uart_rs232_init(void)
{
	QlOSStatus err = 0;
	ql_task_t uart_task = NULL;

#if QL_USB_PRINTER_ENABLE
/*
	1. 重启生效,使能USB打印功能后,USB NMEA口将会被枚举为usb打印设备,用户可在重启后使用ql_uart_open, ql_uart_register_cb,
	ql_uart_write等函数,以QL_USB_PORT_PRINTER为参数来从usb打印设备中读取和写入数据;使能后usb NMEA口将不会被枚举
	2. 如果开启了UAC功能,则不能使用usb打印设备
*/
	ql_usb_set_enum_mode(QL_USB_ENUM_USBNET_COM_PRINTER);
#endif

	err = ql_rtos_task_create(&uart_task, QL_UART_TASK_STACK_SIZE, QL_UART_TASK_PRIO, "QUARTRS232", ql_uart_rs232_thread, NULL, QL_UART_TASK_EVENT_CNT);
	if (err != QL_OSI_SUCCESS)
	{
		QL_UART_DEMO_LOG("rs232 task created failed");
        return;
	}
}

void parshingDataForProductionMode(void)
{
	uint8_t ACK_string[5] = {0x00,0x00,0x00,0x00,0x00};

    if((cim_recv_buff[0] == 0x7F)&&(cim_recv_buff[1] == 0x7E)&&(cim_recv_buff[2] == 0x7F)&&(cim_recv_buff[3] == 0x7E))
    {
    	uint16_t jsonDataLen =  (cim_recv_buff[4] << 8) + cim_recv_buff[5];

    	//ql_uart_write(QL_UART_PORT_1, &cim_recv_buff[6], jsonDataLen);

    	ql_rtos_task_sleep_ms(20);

    	if(Rec_char >= jsonDataLen+6)
		{
            QL_UART_DEMO_LOG("jsonDataLen=%d, jsonDataLen=%s", jsonDataLen, &cim_recv_buff[6]);

            char JSON_ret = parse_JSON_frame(SERIAL,(char *)&cim_recv_buff[6],(char *)ACK_string);
            if(JSON_ret!=JSON_SUCCESS)
            {
                QL_UART_DEMO_LOG("parse_JSON_frame error");
            }
            else
            {
                QL_UART_DEMO_LOG("parse_JSON_frame OK");
            }

			switch(current_cmd)
			{
				case CMD_PRODUCTION :
				{
					if(flagSERIAL_ID_First == 1)
					{
						flagSERIAL_ID_First = 0;
						buildProIdFrameJson(3,0);
					}
					else if(flagSERIAL_ID_afterPowerCycle == 1)
					{
						flagSERIAL_ID_afterPowerCycle = 0;
						buildProIdFrameJson(3,1);
					}
					else if(flagSERIAL_TestMethod_1_ACK == 1)
					{
						flagSERIAL_TestMethod_1_ACK = 0;
						buildTestMethodAckJson(3,1);
					}
					else if(flagSERIAL_TestMethod_1_Result == 1)
					{
						flagSERIAL_TestMethod_1_Result = 0;
						buildTestMethodResultJson(3,1);
					}
					else if(flagSERIAL_TestMethod_2_ACK == 1)
					{
						flagSERIAL_TestMethod_2_ACK = 0;
						buildTestMethodAckJson(3,2);
					}
					else if(flagSERIAL_TestMethod_2_Result == 1)
					{
						flagSERIAL_TestMethod_2_Result = 0;
						buildTestMethodResultJson(3,2);
					}
					ql_uart_write(QL_UART_PORT_1, SerialTXBuffer, strlen((char *)SerialTXBuffer));
				}
				break;
				default:
				{

				}
				break;
			}
			Rec_char = 0;
			memset(cim_recv_buff, 0, 2048);
		}
    }
}

void CalculateBulbFailure(void)
{
    float calucate_lamp,fractpart, intpart;
    unsigned int param;
    
    // calculate no of LAmp
    /** Feeder ON && (Dim is not running || Dimming is running with zero dim %) */
    if(0 == DI_Final_value[0])
    {
        QL_UART_DEMO_LOG("Bulb failure Calculate Enter");
        
        /**
         * EPROM.sp.hi_value[7] : Total expected load in output.
         * EPROM.sp.lo_value[7] : Number of bulb connected in output line.
         */
        if(THREEPHASE == EPROM_General.MeterType)
        {
            /*
             * In case total load is more than set load. This thing happens in case of electricity thief.
             */
            param = 0;
            if(EPROM_General.sp.hi_value[7] > gFinalAnaValF[450])
            {
                calucate_lamp =  ((EPROM_General.sp.hi_value[7]-gFinalAnaValF[450])/EPROM_General.sp.hi_value[7])*EPROM_General.sp.lo_value[7];

                param = (unsigned int)calucate_lamp;
                intpart = (float)param;
                fractpart = calucate_lamp - intpart;     
                if(fractpart >= 0.5)     
                {
                    param++;
                }
            }
            gFinalAnaValF[345] = (float)param;

            param = 0;
            if(EPROM_General.sp.hi_value[8] > gFinalAnaValF[452])
            {
                calucate_lamp = ((EPROM_General.sp.hi_value[8]-gFinalAnaValF[452])/EPROM_General.sp.hi_value[8])*EPROM_General.sp.lo_value[8];
                param = (unsigned int) calucate_lamp;
                intpart = (float)param;
                fractpart = calucate_lamp - intpart;
                if(fractpart >= 0.5)
                {
                    param++;
                }
            }
            gFinalAnaValF[346] = (float)param;

            param = 0;
            if(EPROM_General.sp.hi_value[9] > gFinalAnaValF[454])
            {
                calucate_lamp = ((EPROM_General.sp.hi_value[9]-gFinalAnaValF[454])/EPROM_General.sp.hi_value[9])*EPROM_General.sp.lo_value[9];
                param = (unsigned int) calucate_lamp;
                intpart = (float)param;
                fractpart = calucate_lamp - intpart;
                if(fractpart >= 0.5)
                {
                    param++;
                }
            }
            gFinalAnaValF[347] = (float)param;
        }
        else if(SINGLEPHASE == EPROM_General.MeterType)
        {
            param = 0;
            
            if(EPROM_General.sp.hi_value[7] > gFinalAnaValF[450])
            {
                calucate_lamp = ((EPROM_General.sp.hi_value[7]-gFinalAnaValF[450])/EPROM_General.sp.hi_value[7])*EPROM_General.sp.lo_value[7];
                param = (unsigned int) calucate_lamp;
                intpart = (float)param;
                fractpart = calucate_lamp - intpart;
                if(fractpart >= 0.5)
                {
                    param++;
                }
            }
            gFinalAnaValF[345]= (float)param;
        }
    }
    //else if((InOut.dig_bit_array[0]==0)&& (Previousschedulematched==0)) //-verify it
    //else if(0 == EPROM_General.AI_DI_DO_Detail.dig_bit_array[0])
    else if(1 == DI_Final_value[0])
    {
        QL_UART_DEMO_LOG("Bulb fail Calculate Exit");
        gFinalAnaValF[345] = 0;
        gFinalAnaValF[346] = 0;
        gFinalAnaValF[347] = 0;
    }
}