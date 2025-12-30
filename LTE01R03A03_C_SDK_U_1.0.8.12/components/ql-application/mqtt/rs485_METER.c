
/**  
  @file
  rs485_demo.c

  @brief
  quectel rs485_demo.

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
07/06/2021        Neo         Init version
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
#include "I2C_RTC_LCD.h"
#include "gpio_DIDO.h"
#include "ql_api_sim.h"
#include "configuration.h"
#include "json_parser_sp.h"
#include "dlms_meter.h"
#include "gpio_int_KEY.h"
#include "configuration.h"

/*===========================================================================
 *Definition
 ===========================================================================*/
#define QL_RS485_DEMO_LOG_LEVEL			QL_LOG_LEVEL_INFO
#define QL_RS485_DEMO_LOG(msg, ...)		QL_LOG(QL_RS485_DEMO_LOG_LEVEL, "ql_rs485_demo", msg, ##__VA_ARGS__)


#define QL_RS485_TASK_STACK_SIZE     		4096
#define QL_RS485_TASK_PRIO          	 	APP_PRIORITY_HIGH           //ensure the priority of the task
#define QL_RS485_TASK_EVENT_CNT      		10
#define QL_RS485_WRITE_WAIT_TIMEOUT         500                         //send timeout


#define QL_UART_RX_BUFF_SIZE                2048
#define QL_UART_TX_BUFF_SIZE                2048

/*===========================================================================
 * Variate
 ===========================================================================*/
ql_task_t rs485_task = NULL;
extern DataTime_t rtc_time;
extern ql_LvlMode  gpio_lvl;
extern uint8_t DI_Final_value[];
extern uint8_t DO_Final_value[];
extern uint8_t FlagswitchInt1;
extern uint8_t FlagswitchInt2;
extern uint8_t FlagswitchInt3;
extern uint8_t FlagswitchInt4;
extern uint8_t fNetworkRegistered;
extern ql_sim_status_e card_status;
extern int mqtt_connected;
extern char csq_sim;
extern uint32_t fMQTTpublishCount;
extern uint8_t ram_buff[];
// extern char DLMS_Status;
/*===========================================================================
 * Functions
 ===========================================================================*/
void ql_rs485_notify_cb(unsigned int ind_type, ql_uart_port_number_e port, unsigned int size)
{    
    //QL_RS485_DEMO_LOG("UART port %d receive ind type:0x%x, receive data size:%d", port, ind_type, size);
    ql_event_t event;
    
    switch(ind_type)
    {
        case QUEC_UART_RX_OVERFLOW_IND:  //rx buffer overflow
        {
            QL_RS485_DEMO_LOG("rx overflow");
        }

        case QUEC_UART_RX_RECV_DATA_IND:
        {
            QL_RS485_DEMO_LOG("rx data coming");
            if(size > 0)
            {
                event.id = QUEC_UART_RX_RECV_DATA_APP_IND;
                event.param1 = size;
                ql_rtos_event_send(rs485_task, &event);
            }
            break;
        }

        case QUEC_UART_TX_FIFO_COMPLETE_IND:
        {
            QL_RS485_DEMO_LOG("tx fifo complete");
            event.id = QUEC_UART_TX_COMPLETE_APP_IND;
            ql_rtos_event_send(rs485_task, &event);
            break;
        }
    }
}

int ql_rs485_init(ql_uart_port_number_e port, ql_uart_config_s *dcb, ql_uart_callback uart_cb)
{
    int ret = 0;
    ql_uart_config_s dcb_uart;

    ret = ql_uart_set_dcbconfig(port, dcb);
	if(QL_UART_SUCCESS != ret)
	{
	    QL_RS485_DEMO_LOG("ret: 0x%x", ret);
		return 1;
	}

    ret = ql_uart_open(port);
	if(QL_UART_SUCCESS != ret)
	{
	    QL_RS485_DEMO_LOG("ret: 0x%x", ret);
	    return 1;
    }
    ret = ql_uart_register_cb(port, uart_cb);
    if(QL_UART_SUCCESS != ret)
	{
	    QL_RS485_DEMO_LOG("ret: 0x%x", ret);
	    return 1;
    }

    memset(&dcb_uart, 0, sizeof(ql_uart_config_s));
    ret = ql_uart_get_dcbconfig(port, &dcb_uart);
    if(QL_UART_SUCCESS != ret)
	{
	    QL_RS485_DEMO_LOG("ret: 0x%x", ret);
	    return 1;
    }
    
    QL_RS485_DEMO_LOG("ret: 0x%x, baudrate=%d, flow_ctrl=%d, data_bit=%d, stop_bit=%d, parity_bit=%d", 
                       ret, dcb_uart.baudrate, dcb_uart.flow_ctrl, dcb_uart.data_bit, dcb_uart.stop_bit, dcb_uart.parity_bit);

    return 0;
}

int ql_rs485_deinit(ql_uart_port_number_e port)
{
    int ret = 0;
    ret = ql_uart_close(port);
    if(ret)
    {
        QL_RS485_DEMO_LOG("ret: 0x%x", ret);
        return 1;
    }
    
    return 0;
}

//when the module is in the write state, the data sent by host will be lost
int ql_rs485_write(ql_uart_port_number_e port, unsigned char *data, unsigned int data_len)
{
    int write_len = 0;
    ql_uart_tx_status_e tx_status;
    ql_uart_errcode_e ret;
    
    ql_gpio_set_level(QL_RS485_GPIO_NUM, LVL_HIGH);
    write_len = ql_uart_write(port, data, data_len);
    while(1)
    {
        //wait for FIFO data transmission to complete
        ret = ql_uart_get_tx_fifo_status(port, &tx_status);
        if(ret)
        {
            break;
        }
        if(tx_status == QL_UART_TX_COMPLETE)
        {
            break;
        }
    }

    //UART FIFO is counted in bytes. When FIFO is empty, there may still be several bits that are not sent out in the hardware circuit, 
    //including stop bits, so it takes some time to delay at low baud rate
    //The baud rate is 115200 without delay. The baud rate is 9600. It is recommended to delay 180us. 
    //The specific delay time needs to be combined with the actual test
    //ql_delay_us(180);
    
    //after writing, pull down directly to make the module in the state of receiving data
    ql_gpio_set_level(QL_RS485_GPIO_NUM, LVL_LOW);
    
    return write_len;
}

//the serial port cache is only 4K bytes. after receiving the data, it should be taken away as soon as possible. If the cache is full, it will lose the data
int ql_rs485_read(ql_uart_port_number_e port, unsigned char *data, unsigned int data_len)
{
    ql_gpio_set_level(QL_RS485_GPIO_NUM, LVL_LOW);
    return ql_uart_read(port, data, data_len);
}

static void ql_rs485_meter_thread(void *param)
{
    int ret = 0;
	QlOSStatus err = 0;
    ql_uart_config_s uart_cfg = {0};
    int write_len = 0;
    char tempPrintReg[16] = {0};
    
    char data[256] = "hello rs485 demo\r\n";
    unsigned char *recv_buff = calloc(1, QL_UART_RX_BUFF_SIZE+1);

    ql_rtos_task_sleep_s(5);

    //step2:gpio pin init, default pull down, 485 needs a gpio pin to control the converter send or receive
    ret = ql_pin_set_func(QL_RS485_PIN_GPIO, QL_RS485_PIN_GPIO_FUNC_GPIO);
    if(QL_GPIO_SUCCESS != ret)
    {
        goto exit;
    }
    ql_gpio_deinit(QL_RS485_GPIO_NUM);
    ql_gpio_init(QL_RS485_GPIO_NUM, GPIO_OUTPUT, QUEC_PIN_NONE, LVL_LOW);

    //step4:rs485 init, baud rate and other parameters need to be configured
    uart_cfg.baudrate = QL_UART_BAUD_115200;
    uart_cfg.flow_ctrl = QL_FC_NONE;
    uart_cfg.data_bit = QL_UART_DATABIT_8;
    uart_cfg.stop_bit = QL_UART_STOP_1;
    uart_cfg.parity_bit = QL_UART_PARITY_NONE;

    ret = ql_rs485_init(QL_UART_PORT_1, &uart_cfg, ql_rs485_notify_cb);
	if(ret)
	{
		goto exit;
	}
    
    sprintf((char *)data, "\r\n***********************************\r\niFPC3000_EC200U_V%s\r\n***********************************\r\n", DEFAULT_FV_VERSION);
    write_len = ql_rs485_write(QL_UART_PORT_1, (unsigned char *)data, strlen((char *)data));
    QL_RS485_DEMO_LOG("write_len:%d", write_len);
    while(1)
    {
		if(card_status == 0)
		{
			strcpy(tempPrintReg,"OK");
		}
		else
		{
			strcpy(tempPrintReg,"NO");
            fNetworkRegistered = 0;
		}
//RTC:02-01-2000,05:30:55, DO:1,1,1, DI:1,1,1,1,1,1, SWITCH:0,0,0,0, SIM:NO/DTD,REG:NOK/OK,STR:00, MQTT:0/1, SEND COUNT:00, RBM:35, BT: 0/1, AC: 0/1
        sprintf((char *)data,"\r\n\r\nRTC:%02d-%02d-%02d,%02d:%02d:%02d  DO:%d,%d,%d  DI:%d,%d,%d,%d,%d,%d  SWITCH:%d,%d,%d,%d  SIM:%s  REG:%d  STR:%d MQTT:%d  PUB_S:%lu  RBM:%d  AC:%d  BT:%d\r\n",
                    rtc_time.mDate, rtc_time.month, rtc_time.myear, rtc_time.mHour, rtc_time.minute, rtc_time.mSecond,
	        	    EPROM_General.AI_DI_DO_Detail.DOSignal[0],EPROM_General.AI_DI_DO_Detail.DOSignal[1],EPROM_General.AI_DI_DO_Detail.DOSignal[2],
				    DI_Final_value[0],DI_Final_value[1],DI_Final_value[2],DI_Final_value[3],DI_Final_value[4],DI_Final_value[5],
				    FlagswitchInt1,FlagswitchInt2,FlagswitchInt3,FlagswitchInt4,
				    tempPrintReg,fNetworkRegistered,csq_sim,
				    mqtt_connected, fMQTTpublishCount, /*ram_buff[1]*/EPROM_General.rebootCount,
                    DI_Final_value[15],DI_Final_value[14]);

        FlagswitchInt1=0;
        FlagswitchInt2=0;
        FlagswitchInt3=0;
        FlagswitchInt4=0;
        write_len = ql_rs485_write(QL_UART_PORT_1, (unsigned char *)data, strlen((char *)data));
        QL_RS485_DEMO_LOG("write_len:%d", write_len);
        if(EPROM_General.MeterType == THREEPHASE)
        {
            sprintf((char *)data,"Query No:%d, CurrentL1:%0.2f, VoltageL1:%0.2f, CurrentL2:%0.2f, VoltageL2:%0.2f, CurrentL3:%0.2f, VoltageL3:%0.2f, Avg Power Factor:%0.2f, Avtive Power:%0.3fKW, Frequency:%0.2fHz, Cum Power:%0.6fKWh\r\n",
            DLMS_Status, gFinalAnaValF[450], gFinalAnaValF[451], gFinalAnaValF[452], gFinalAnaValF[453], gFinalAnaValF[454], gFinalAnaValF[455], gFinalAnaValF[456], gFinalAnaValF[457], gFinalAnaValF[458], gFinalAnaValF[459]);                      
        }
        else
        {
            sprintf((char *)data,"Query No:%d, Current:%0.2f, Voltage:%0.2f, Avg Power Factor:%0.2f, Avtive Power:%0.3fKW, Frequency:%0.2fHz, Cum Power:%0.6fKWh\r\n",
            DLMS_Status, gFinalAnaValF[450], gFinalAnaValF[451], gFinalAnaValF[456], gFinalAnaValF[457], gFinalAnaValF[458], gFinalAnaValF[459]);
        }
        write_len = ql_rs485_write(QL_UART_PORT_1, (unsigned char *)data, strlen((char *)data));
        QL_RS485_DEMO_LOG("write_len:%d", write_len);

        // for(uint8_t i_index=0;i_index<=EPROM_General.MaxofSMS;i_index++)
	    // {
        //     sprintf((char *)data,"[%d]:%f ",i_index+1,(gFinalAnaValF[(EPROM_General.ModSMSList[i_index]-1)/2]));
        //     write_len = ql_rs485_write(QL_UART_PORT_1, (unsigned char *)data, strlen((char *)data));
        //     QL_RS485_DEMO_LOG("write_len:%d", write_len);
	    // }

        ql_rtos_task_sleep_s(1);
    }

exit:
    QL_RS485_DEMO_LOG("ret: 0x%x", ret);
    free(recv_buff);
    err = ql_rtos_task_delete(NULL);
	if(err != QL_OSI_SUCCESS)
	{
		QL_RS485_DEMO_LOG("task deleted failed");
	}
}

void ql_rs485_app_meter_init(void)
{
	QlOSStatus err = 0;
	
	err = ql_rtos_task_create(&rs485_task, QL_RS485_TASK_STACK_SIZE, QL_RS485_TASK_PRIO, "QRS485METER", ql_rs485_meter_thread, NULL, QL_RS485_TASK_EVENT_CNT);
	if (err != QL_OSI_SUCCESS)
	{
		QL_RS485_DEMO_LOG("rs485 task created failed");
        return;
	}
}
