/**  @file
  I2C_demo.c

  @brief
  This file is demo of I2C.

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

/*===========================================================================
 * include files
 ===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ql_api_osi.h"
#include "ql_log.h"
#include "json_parser_sp.h"
#include "ql_i2c.h"
#include "I2C_RTC_LCD.h"
#include "ql_uart.h"
#include "configuration.h"
#include "gpio_int_KEY.h"
#include "dlms_meter.h"
#include "gpio_DIDO.h"
#include "spi_flash_at25ff.h"

/*===========================================================================
 * Macro Definition
 ===========================================================================*/

#define QL_APP_I2C_LOG_LEVEL             QL_LOG_LEVEL_INFO
#define QL_APP_I2C_LOG(msg, ...)         QL_LOG(QL_APP_I2C_LOG_LEVEL, "QL_APP_I2C", msg, ##__VA_ARGS__)
#define QL_APP_I2C_LOG_PUSH(msg, ...)    QL_LOG_PUSH("QL_APP_I2C", msg, ##__VA_ARGS__)
    
#define QL_I2C_TASK_STACK_SIZE     		4*1024
#define QL_I2C_TASK_PRIO          	 	APP_PRIORITY_NORMAL
#define QL_I2C_TASK_EVENT_CNT      		5

#define SalveAddr_w_8bit        (0x51) //(0xA2 >> 1)
#define SalveAddr_r_8bit        (0x51) //(0xA3 >> 1)
#define LCD_I2C_ADDRESS         (0x3C)//(0x1E)//(0x3C) // change this according to ur setup

#define BIN2BCD(__data__)	(((((__data__) / 10) & 0x0F) << 4)  | (((__data__) % 10) & 0x0F))
#define BCD2DEC(__data__)	(((((__data__ )& 0xF0) >> 4) * 10)  + ((__data__) & 0x0F) )


/*===========================================================================
 * Struct
 ===========================================================================*/


/*===========================================================================
 * Enum
 ===========================================================================*/

/*===========================================================================
 * Variate
 ===========================================================================*/
 DataTime_t rtc_time,update_time;
 uint8_t d[4];
 uint8_t rtc_intialized = 0;
 uint8_t lcd_intialized = 0;
 uint8_t keypad_initialized = 0;

 extern uint8_t ProductionMode;
 struct SaveHistory EPROM4HISTORY;
 extern char csq_sim;

unsigned char 	shour,sminute,ssecond;
unsigned char sdate,smonth,syear;
char OldSecomdRec,OldclearSecomdRec;
unsigned char dispStr[BUFFERSIZE];
char disp_buffer1[200],disp_buffer2[200];
unsigned char CommaStr[22][25],CommPosition=0,ColonPositionFound=0;
char Setup_ID[5], Setup_rd[5], Setup_cd[5], Setup_log[5], MAXSMS_ID[5];
char Alarm_MO[NUM_MOBILE_NUMBERS][DIGITS_PER_NUMBER];
char GPRSPort[5];
int GPRSLiveIPbufVar[5];
unsigned char GPRSLiveIPbuf[20];
uint8_t Meter_reset, Pcbplc_Display;
unsigned long prog_page_count;

struct Decimal Deci;
unsigned char dispDecimal[16];
uint8_t modbusminute;
unsigned char DOname[3][5]={"DO1","DO2","DO3"};
/*===========================================================================
 * Functions
 ===========================================================================*/

// Function to write a command to the Raystar LCD
ql_errcode_i2c_e lcd_write_command(uint8_t command)
{
    unsigned char tI2c_tx_data_write[16] = {0,};
    ql_errcode_i2c_e ret;
    unsigned int tIndex = 0;
    tI2c_tx_data_write[tIndex++]=0x00;
    tI2c_tx_data_write[tIndex++]=command;

    ret = ql_I2cWrite(i2c_2, LCD_I2C_ADDRESS, tI2c_tx_data_write[0], &tI2c_tx_data_write[1], tIndex-1);
    if(ret != QL_I2C_SUCCESS)
    {
		lcd_intialized = 0;
      	QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
    }
    ql_rtos_task_sleep_ms(1);

	return ret;
}

// Function to write data to the Raystar LCD
ql_errcode_i2c_e lcd_write_data(uint8_t data)
{

	uint8_t d[2];
    ql_errcode_i2c_e ret;
    unsigned int tIndex = 0;
	d[tIndex++]=0x40;
	d[tIndex++]=data;

    ret = ql_I2cWrite(i2c_2, LCD_I2C_ADDRESS, d[0], &d[1], tIndex-1);
    if(ret != QL_I2C_SUCCESS)
    {
		lcd_intialized = 0;
      	QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
    }
    ql_rtos_task_sleep_ms(1);

	return ret;
}

ql_errcode_i2c_e lcd_clear(void)
{
	ql_errcode_i2c_e ret;
    ret = lcd_write_command(0x01); // Wait for the clear display command to complete
    ql_rtos_task_sleep_ms(2);
    return ret;
}

ql_errcode_i2c_e lcd_initialize(void)
{
    // Initialize Raystar LCD

	ql_errcode_i2c_e ret = QL_I2C_SUCCESS;

    // Step 1: Configure display for 8-bit data, 2-line display, 5x8 font
    ret = lcd_write_command(0x38); // Wait for the command to be processed
    ql_rtos_task_sleep_ms(2);
    if(ret != QL_I2C_SUCCESS)
	{
    	lcd_intialized = 0;
    	return ret;
	}
    // Step 2: Turn on display, turn off cursor, disable blinking
    ret = lcd_write_command(0x0C); // Wait for the command to be processed
    ql_rtos_task_sleep_ms(2);
	if(ret != QL_I2C_SUCCESS)
	{
		lcd_intialized = 0;
		return ret;
	}

    // Step 3: Clear the display
    ret = lcd_write_command(0x01); // Wait for the clear display command to complete
    ql_rtos_task_sleep_ms(2);
	if(ret != QL_I2C_SUCCESS)
	{
		lcd_intialized = 0;
		return ret;
	}

    // Step 4: Set entry mode - Increment cursor, no display shift
    ret = lcd_write_command(0x06); // Wait for the clear display command to complete
    ql_rtos_task_sleep_ms(2);
	if(ret != QL_I2C_SUCCESS)
	{
		lcd_intialized = 0;
		return ret;
	}

    ql_rtos_task_sleep_ms(10);
    ret = lcd_set_cursor(1,0);
    ret = lcd_display_string("CIMCON SOFTWARE");
    ret = lcd_set_cursor(2,0);
    ret = lcd_display_string("    iFPC3000    ");
    ql_rtos_task_sleep_ms(3000);

    return ret;
}

ql_errcode_i2c_e lcd_set_cursor(uint8_t row, uint8_t  col)
{
	//ql_errcode_i2c_e ret;
	uint8_t pos;
	switch(row)
	{
		case 1:
			pos = 0x80 + col;
			break;
		case 2:
			pos = 0xC0 + col;
			break;
		default:
			pos = 0x80 + col; // Default to the first row if an invalid row is provided
			break;
	}
	return lcd_write_command(pos);
}

ql_errcode_i2c_e lcd_float_print(float value, int precision)
{
	unsigned char LCD_data[16] = {0};
	//lcd_display_string("        ");
	sprintf((char *)LCD_data,"%07.3f ", value);
	return lcd_display_string((char *)LCD_data);
}

// Function to display a string on the Raystar LCD
ql_errcode_i2c_e lcd_display_string(const char *string)
{
	ql_errcode_i2c_e ret = QL_I2C_SUCCESS;
    while (*string) {
        ret = lcd_write_data(*string++);
    }
    return ret;
}

void ql_i2c_LCD_thread(void *param)
{
    ql_errcode_i2c_e ret;
	uint16_t mainPageCount = 0;
   
    ret = ql_I2cInit(i2c_2, STANDARD_MODE);
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("ql_I2cInit fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("ql_I2cInit OK ret=%d",ret);
    }

    ql_rtos_task_sleep_s(3);

    //lcd_initialize();

    while(1)
    {
    	if(lcd_intialized == 0)
		{
			ql_rtos_task_sleep_s(3);
			lcd_intialized = 1;
			if(lcd_initialize() != QL_I2C_SUCCESS)
			{
				keypad_initialized = 0;
			}
			else
			{
				keypad_initialized = 1;
			}
		}

    	if((lcd_intialized == 1) && (keypad_initialized == 1) && (pro_DO_DI_TestFinish == 1) && (ProductionMode == 1))
    	{
    		if(FlagswitchInt1 == 1)
    		{
    			FlagswitchInt1 = 0;
    		    ql_rtos_task_sleep_ms(5);
    		    lcd_set_cursor(1,0);
    		    lcd_display_string("PRODUCTION TEST");
    		    lcd_set_cursor(2,0);
    		    lcd_display_string("  UP KEY PRESS  ");
    		    ql_rtos_task_sleep_ms(5);
    		    DO_Opration=1;
    		}
    		else if(FlagswitchInt2 == 1)
    		{
    			FlagswitchInt2 = 0;
    		    ql_rtos_task_sleep_ms(5);
    		    lcd_set_cursor(1,0);
    		    lcd_display_string("PRODUCTION TEST");
    		    lcd_set_cursor(2,0);
    		    lcd_display_string(" DOWN KEY PRESS ");
    		    ql_rtos_task_sleep_ms(5);
    		    DO_Opration=1;
    		}
    		else if(FlagswitchInt3 == 1)
    		{
    			FlagswitchInt3 = 0;
    		    ql_rtos_task_sleep_ms(5);
    		    lcd_set_cursor(1,0);
    		    lcd_display_string("PRODUCTION TEST");
    		    lcd_set_cursor(2,0);
    		    lcd_display_string("  PROG KEY PRESS ");
    		    ql_rtos_task_sleep_ms(5);
    		    DO_Opration=1;
    		}
    		else if(FlagswitchInt4 == 1)
    		{
    			FlagswitchInt4 = 0;
    		    ql_rtos_task_sleep_ms(5);
    		    lcd_set_cursor(1,0);
    		    lcd_display_string("PRODUCTION TEST");
    		    lcd_set_cursor(2,0);
    		    lcd_display_string(" SET KEY PRESS  ");
    		    ql_rtos_task_sleep_ms(5);
    		    DO_Opration=1;
    		}
    		if(DO_Opration == 3)
			{
				DO_Opration=0;
				lcd_set_cursor(2,0);
    		    lcd_display_string("                ");
    		    ql_rtos_task_sleep_ms(5);
			}
    	}
		
		if((lcd_intialized == 1) && (keypad_initialized == 1) && (ProductionMode == 0))
		{
			mainPageCount++;
			//if(mainPageCount%2 == 0)
			{
				b.blink = ~b.blink;
			}
			if(mainPageCount > 2)
			{
				mainPageCount = 0;
				// KBPage.main_page++; // TODO: check for overflow
			}
			check_key();

			if((b.progset))//&&(!b.calib))	//check for prog page ]
			{
				prog_page_count ++;
				if(prog_page_count >= PROG_TIMEOUT)	 //shift to run mode after prog timeout
				{
					prog_page_count = 0;
					b.progset ^= 1;
					KBPage.enter_count = 1;
					keybyte = PROGSET;
					// b.calib=0;
					KBPage.main_page = 1;
				}
			}

			QL_APP_I2C_LOG("b.progset%d main_page:%d prog_page:%d enter_count:%d keybyte:%4X", b.progset, KBPage.main_page, KBPage.prog_page, KBPage.enter_count, keybyte);
			QL_APP_I2C_LOG("setup:%d setpoint:%d schedule%d alarm:%d timer:%d energy:%d", KBPage.setup_page,KBPage.setpoint_page, KBPage.schedual_page, KBPage.alarm_page, KBPage.timer_page, KBPage.energy_page);
			QL_APP_I2C_LOG("version:%d do:%d mod:%d maxsms:%d gprs:%d StreetLight_page:%d", KBPage.version_page, KBPage.do_page, KBPage.mode_page, KBPage.MaxSMS_page, KBPage.gprs_page, KBPage.StreetLight_page);
			if(b.progset)
			{
				Disp_prog_menu();
			}
			else
			{
				Disp_main_menu();
			}
		}
    	ql_rtos_task_sleep_ms(100);
    }
}

ql_errcode_i2c_e Stop_enable_RTC(uint8_t *tx_data)
{
    ql_errcode_i2c_e ret;
    ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, 0x2E, tx_data, 1);
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("ql_I2cWrite OK ret=%d",ret);
    }
    return ret;
}

int RTC_enable_real_time_clock_mode()
{
	ql_errcode_i2c_e ret;
	unsigned char tI2c_tx_data_write[16] = {0,};
	//unsigned char tI2c_rx_data_read[16] = {0,};
	
	tI2c_tx_data_write[0] = 0x28; /* RTC function register, seting RTCM bit to zero and others also */
	tI2c_tx_data_write[1] = 0x00; /* 100TH: 100th second disabled ,RTCM: Real Time Clock Mode */
	
	// Add debug log to log RTC values
QL_APP_I2C_LOG("RTC Debug Log: RTC Time - %02d:%02d:%02d, Date - %02d/%02d/%04d",
	rtc_time.mHour, rtc_time.minute, rtc_time.mSecond,
	rtc_time.mDate, rtc_time.month, rtc_time.myear + 2000);

    ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, tI2c_tx_data_write[0], &tI2c_tx_data_write[1], 1);
	if(ret != QL_I2C_SUCCESS)
	{
		QL_APP_I2C_LOG("RTC_enable_real_time_clock_mode fail ret=%d",ret);
		return ret;
	}
    ql_rtos_task_sleep_s(1);
	
	return ret;
}

void ql_i2c_RTC_thread(void *param)
{
    unsigned char tI2c_tx_data_write[16] = {0,};
    unsigned char tI2c_rx_data_read[16] = {0,};
    ql_errcode_i2c_e ret;

    unsigned int tIndex = 0;
	//float tempfloat=0.0;    //to remove
	 ql_rtos_task_sleep_s(3);
        
    ret = ql_I2cInit(i2c_1, STANDARD_MODE);
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("ql_I2cInit fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("ql_I2cInit OK ret=%d",ret);
    }
    ql_rtos_task_sleep_s(1);
    ret = RTC_enable_real_time_clock_mode();
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("RTC_enable_real_time_clock_mode fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("RTC_enable_real_time_clock_mode OK ret=%d",ret);
    }

    tI2c_tx_data_write[0] = 0x2E; 			/* Stop_enable register address */
    tI2c_tx_data_write[1] = 0x01;			/* Stop RTC clock, to update time */
    ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, tI2c_tx_data_write[0], &tI2c_tx_data_write[1], 1);
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("ql_I2cWrite OK ret=%d",ret);
    }
    
    tI2c_tx_data_write[0] = 0x25;
    tI2c_tx_data_write[1] = 0x01;
    ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, tI2c_tx_data_write[0], &tI2c_tx_data_write[1], 1);
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("ql_I2cWrite OK ret=%d",ret);
    }
    
    tI2c_tx_data_write[0] = 0x2E; 			/* Stop_enable register address */
    tI2c_tx_data_write[1] = 0x00;           /* Stop RTC clock, to update time */
    ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, tI2c_tx_data_write[0], &tI2c_tx_data_write[1], 1);
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("ql_I2cWrite OK ret=%d",ret);
    }

	  // Log RTC time before entering the loop
	  QL_APP_I2C_LOG("RTC Initial Time - %02d:%02d:%02d, Date - %02d/%02d/%04d",
		rtc_time.mHour, rtc_time.minute, rtc_time.mSecond,
		rtc_time.mDate, rtc_time.month, rtc_time.myear + 2000);

    
  /*  ret = ql_I2cRead(i2c_1, SalveAddr_r_8bit, tI2c_tx_data_write[0], tI2c_rx_data_read, 3);		// review: not required
    if(ret != QL_I2C_SUCCESS)
    {
        QL_APP_I2C_LOG("ql_I2cRead fail ret=%d",ret);
    }
    else
    {
        QL_APP_I2C_LOG("ql_I2cRead OK tI2c_rx_data_read:%x",tI2c_rx_data_read[0]);
        QL_APP_I2C_LOG("ql_I2cRead OK tI2c_rx_data_read:%x",tI2c_rx_data_read[1]);
        QL_APP_I2C_LOG("ql_I2cRead OK tI2c_rx_data_read:%x",tI2c_rx_data_read[2]);
    }*/

    while(1)
    {
        if(rtc_intialized == 1)
        {
        	rtc_intialized = 0;

        /*    ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, tI2c_tx_data_write[0], tI2c_tx_data_write, 1);	// review: not required 
            if(ret != QL_I2C_SUCCESS)
            {
                QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
            }
            else
            {
                QL_APP_I2C_LOG("ql_I2cWrite OK ret=%d",ret);
            }
			*/
			memset(tI2c_tx_data_write,0,sizeof(tI2c_tx_data_write));
			tIndex = 0;
            tI2c_tx_data_write[tIndex++] = 0x2E; 			/* Stop_enable register address */
            tI2c_tx_data_write[tIndex++] = 0x01;			/* Stop RTC clock, to update time */
            tI2c_tx_data_write[tIndex++] = 0xA4;			/* clear prescaler */
            tI2c_tx_data_write[tIndex++] = 0x00;			/* 100th seconds Register to, 00h */
            tI2c_tx_data_write[tIndex++] = BIN2BCD(update_time.mSecond);	 //Sec
            tI2c_tx_data_write[tIndex++] = BIN2BCD(update_time.minute);	 //Min
            tI2c_tx_data_write[tIndex++] = BIN2BCD(update_time.mHour);  //Hour
            tI2c_tx_data_write[tIndex++] = BIN2BCD(update_time.mDate);   //Date
            tI2c_tx_data_write[tIndex++] = BIN2BCD(0x06); //Dummy
            tI2c_tx_data_write[tIndex++] = BIN2BCD(update_time.month);  //Month
            tI2c_tx_data_write[tIndex] = BIN2BCD(update_time.myear);  //Year
            
            ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, tI2c_tx_data_write[0], &tI2c_tx_data_write[1], tIndex);
            if(ret != QL_I2C_SUCCESS)
            {
                QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
            }
            else
            {
                QL_APP_I2C_LOG("ql_I2cWrite OK ret=%d",ret);
            }
            tIndex = 0;
            tI2c_tx_data_write[tIndex++] = 0x2E; 			/* Stop_enable register address */
            tI2c_tx_data_write[tIndex] = 0x00;			/* Start RTC clock, to update time, Time starts counting from this point */	
            
            ret = ql_I2cWrite(i2c_1, SalveAddr_w_8bit, tI2c_tx_data_write[0], &tI2c_tx_data_write[1], tIndex);
            if(ret != QL_I2C_SUCCESS)
            {
                QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d",ret);
            }
            else
            { 
				QL_APP_I2C_LOG("RTC Clock Started");
                QL_APP_I2C_LOG("ql_I2cWrite OK ret=%d",ret);
            }
        }
		
		tI2c_tx_data_write[0] = 0x00; /* 100th_seconds register address */
        ret = ql_I2cRead(i2c_1, SalveAddr_r_8bit, tI2c_tx_data_write[0], tI2c_rx_data_read, 8);
        if(ret != QL_I2C_SUCCESS)
        {
            QL_APP_I2C_LOG("ql_I2cRead fail ret=%d",ret);
        }
        else
        {
			QL_APP_I2C_LOG("ql_I2cRead OK tI2c_rx_data_read:%x",tI2c_rx_data_read[0]);

            rtc_time.mDate   = BCD2DEC(tI2c_rx_data_read[4] & 0x3F);
            rtc_time.month   = BCD2DEC(tI2c_rx_data_read[6] & 0x1F);
            rtc_time.myear   = BCD2DEC(tI2c_rx_data_read[7]);
            rtc_time.mHour   = BCD2DEC(tI2c_rx_data_read[3] & 0x3F);
            rtc_time.minute  = BCD2DEC(tI2c_rx_data_read[2] & 0x7F);
            rtc_time.mSecond = BCD2DEC(tI2c_rx_data_read[1] & 0x7F);
            QL_APP_I2C_LOG("I2C read_data = Date=%02d/%02d/20%02d, Time=%02d:%02d:%02d", 
                                            rtc_time.mDate,rtc_time.month,rtc_time.myear, 
                                            rtc_time.mHour,rtc_time.minute,rtc_time.mSecond);
        }

        tI2c_tx_data_write[0] = 0x25;
      /*  ret = ql_I2cRead(i2c_1, SalveAddr_r_8bit, tI2c_tx_data_write[0], tI2c_rx_data_read, 3);			// review: not required 
        if(ret != QL_I2C_SUCCESS)
        {
            QL_APP_I2C_LOG("ql_I2cRead fail ret=%d",ret);
        }
        else
        {
            QL_APP_I2C_LOG("ql_I2cRead OK tI2c_rx_data_read:%x",tI2c_rx_data_read[0]);
            QL_APP_I2C_LOG("ql_I2cRead OK tI2c_rx_data_read:%x",tI2c_rx_data_read[1]);
            QL_APP_I2C_LOG("ql_I2cRead OK tI2c_rx_data_read:%x",tI2c_rx_data_read[2]);
        }
*/
		if(!b.progset)
		{
            if(modbusminute != rtc_time.minute)
            {
				/* Run Hour increment at every minutes */
				if((0 == DI_Final_value[0]) && (1 == DI_Final_value[15]))	// Power ON and Ckt1 ON
				{
					++(EPROM4HISTORY.TotalMinuteckt1);
				}
				if((0 == DI_Final_value[2]) && (1 == DI_Final_value[15]) && (EPROM_General.NoofCkt == 2))	// Power ON and Ckt2 ON
				{
					++(EPROM4HISTORY.TotalMinuteckt2);
				}
			}
		}

/* updated hours and min*/

		if(modbusminute != rtc_time.minute)
		{
			// Run Hour, RunHour 
			modbusminute = rtc_time.minute;

			if(0 == DI_Final_value[0])
			{
				int hours = EPROM4HISTORY.TotalMinuteckt1 / 60;
				int minutes = EPROM4HISTORY.TotalMinuteckt1 % 60;
				gFinalAnaValF[99] = (float)hours + (float)minutes / 100.0;
			}
			if(0 == DI_Final_value[2])
			{
				int hours = EPROM4HISTORY.TotalMinuteckt2 / 60;
				int minutes = EPROM4HISTORY.TotalMinuteckt2 % 60;
				gFinalAnaValF[100] = (float)hours + (float)minutes / 100.0;
			}
			if((EPROM4HISTORY.TotalMinuteckt1 - g_flashRunTimeParaSturct.TotalMinuteckt1 >= 15)  ||
			(EPROM4HISTORY.TotalMinuteckt2 - g_flashRunTimeParaSturct.TotalMinuteckt2 >= 15))
				ExtFlash_write_RuntimePara();
			QL_APP_I2C_LOG("Runhour ckt1:%d ckt2:%d", EPROM4HISTORY.TotalMinuteckt1,
						EPROM4HISTORY.TotalMinuteckt2);
		}
		
		if((23 == rtc_time.mHour) && (59 == rtc_time.minute)) // send data at 23 : 59
		{
			EPROM_General.DailySMS = 0;
			EPROM_General.SMSSend_Exceed = 0;
			// SaveToRunParameter();
			// DataSendignSMS = 1; 
			// SendMissngRequest();
		}
		if(EPROM4HISTORY.Runptr != rtc_time.mDate)
		{
			EPROM4HISTORY.Runptr = rtc_time.mDate;
			EPROM4HISTORY.TotalMinuteckt1 = 0;
			EPROM4HISTORY.TotalMinuteckt2 = 0;
			ExtFlash_write_RuntimePara();
		}
        ql_rtos_task_sleep_s(1);
    }
}

void ql_i2c_RTC_LCD_init(void)
{
    QlI2CStatus err = QL_OSI_SUCCESS;
    ql_task_t i2c_task = NULL;
    static ql_task_t i2c_RTC_task = NULL;
        
    err = ql_rtos_task_create(&i2c_task, QL_I2C_TASK_STACK_SIZE, QL_I2C_TASK_PRIO, "I2C LCD", ql_i2c_LCD_thread, NULL, QL_I2C_TASK_EVENT_CNT);
    if (err != QL_OSI_SUCCESS)
    {
        QL_APP_I2C_LOG("i2ctest1 LCD task created failed");
    }

    err = ql_rtos_task_create(&i2c_RTC_task, QL_I2C_TASK_STACK_SIZE, QL_I2C_TASK_PRIO, "I2CRTC", ql_i2c_RTC_thread, NULL, QL_I2C_TASK_EVENT_CNT);
    if (err != QL_OSI_SUCCESS)
    {
        QL_APP_I2C_LOG("i2ctest RTC Task created failed");
        return;
    }
}

///routine to display characters at particular loc.
void DispAt(unsigned char loc, const char *tstring)
{
    unsigned char lineno,pos;

    if(loc == 0) 
    {
        loc = 1;
    }
    lineno = loc/17;

    if(lineno==1)
    {
        pos = loc-17;
    }
    else
    {
        pos = loc-1;
    }
    lcd_set_cursor(++lineno, pos);
    lcd_display_string(tstring);
}
void Disp_main_menu(void)
{
	unsigned char L1, i = 0;
	Pcbplc_Display = 0;
	char tempPrintReg[16] = {0};

	if(KBPage.main_page > KBPage.main_page_limit)
	{
		KBPage.main_page = 1;
	}
	
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	switch(KBPage.main_page)
	{
		case 1:
		{
				sprintf((char*)&disp_buffer1[0], "TIME = %02d:%02d:%02d ",rtc_time.mHour,rtc_time.minute,rtc_time.mSecond);			 
				sprintf((char*)&disp_buffer2[0], "DATE = %02d/%02d/%02d ",rtc_time.mDate,rtc_time.month,rtc_time.myear);
				break;
		}
		case 2:
		{
				if(DI_Final_value[0])
				{
					sprintf((char*)&disp_buffer1[0],"DI-1 = ON");			 
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"DI-1 = OFF");			 
				}
				if(DI_Final_value[1])
				{
					sprintf((char*)&disp_buffer2[0],"DI-2 = ON");			 
				}
				else
				{
					sprintf((char*)&disp_buffer2[0],"DI-2 = OFF");			 
				}
				break;
		}
		case 3:
		{
				if(DI_Final_value[2])
				{
					sprintf((char*)&disp_buffer1[0],"DI-3 = ON");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"DI-3 = OFF");
				}
				if(DI_Final_value[3])
				{
					sprintf((char*)&disp_buffer2[0],"DI-4 = ON");
				}
				else
				{
					sprintf((char*)&disp_buffer2[0],"DI-4 = OFF");
				}
				break;
		}
		case 4:
		{
				if(DI_Final_value[4])
				{
					sprintf((char*)&disp_buffer1[0],"DI-5 = ON");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"DI-5 = OFF");
				}
				if(DI_Final_value[5])
				{
					sprintf((char*)&disp_buffer2[0],"DI-6 = ON");
				}
				else
				{
					sprintf((char*)&disp_buffer2[0],"DI-6 = OFF");
				}
				break;
		}
		case 5:
		{
				if(DI_Final_value[15] == 1)
				{
					sprintf((char*)disp_buffer1,"AC=ON"); 
				}
				else
				{
					sprintf((char*)disp_buffer1,"AC=OFF "); 
				}
				if(DI_Final_value[15] == 0)
				{
					if(DI_Final_value[14])  // it was 11 as per sanket
					{
						strcat((char*)&disp_buffer2[0]," BAT=LOW");
					}
					else
					{
						strcat((char*)&disp_buffer2[0]," BAT=HIGH");
					}
				}
				break;
		}
		case 6:	
		{
				sprintf((char*)&disp_buffer1[0],"SUNRISE: %02d:%02d", (unsigned char)gFinalAnaValF[SUNRISE_HOUR_gFinalAnaValF], (unsigned char)gFinalAnaValF[SUNRISE_MIN_gFinalAnaValF]);
				sprintf((char*)&disp_buffer2[0],"SUNSET : %02d:%02d", (unsigned char)gFinalAnaValF[SUNSET_HOUR_gFinalAnaValF], (unsigned char)gFinalAnaValF[SUNSET_MIN_gFinalAnaValF]);
				break;
		}
		case 7:
		{
				if(card_status == 0)
				{
					strcpy(tempPrintReg,"OK");
					//add log for card inserted
					QL_APP_I2C_LOG("Card Inserted");
				}
				else
				{
					strcpy(tempPrintReg,"NO");
					//add log for card not inserted
					QL_APP_I2C_LOG("Card Not Inserted");
				}
				sprintf((char*)&disp_buffer1[0],"SIM:%s", tempPrintReg);

				if(fNetworkRegistered)
				{
					strcat((char*)&disp_buffer1[0]," Reg OK");
					//add log for network registered
					QL_APP_I2C_LOG("Network Registered");
				}
				else
				{
					strcat((char*)&disp_buffer1[0]," Not Reg");
					//add log for network not registered
					QL_APP_I2C_LOG("Network Not Registered");
				}
				sprintf((char*)&disp_buffer2[0],"CSQ:%02d", csq_sim);

				if(mqtt_connected == 1)
				{
					strcat((char*)&disp_buffer2[0]," Server Connect");
				}
				else
				{
					strcat((char*)&disp_buffer2[0]," Server Disconnect");
				}
				break;
		}
		case 8:
		{
				DisplayRTU();
                break;					
		}
		case 9:
		{	
			sprintf((char*)&disp_buffer1[0],"I1: %f", gFinalAnaValF[450]);
			sprintf((char*)&disp_buffer2[0],"V1: %f", gFinalAnaValF[451]);
			break;				
		}
		case 10:
		{	
			sprintf((char*)&disp_buffer1[0],"I2: %f", gFinalAnaValF[452]);
			sprintf((char*)&disp_buffer2[0],"V2: %f", gFinalAnaValF[453]);
			break;				
		}
		case 11:
		{	
			sprintf((char*)&disp_buffer1[0],"I3: %f", gFinalAnaValF[454]);
			sprintf((char*)&disp_buffer2[0],"V3: %f", gFinalAnaValF[455]);
			break;				
		}
		case 12:
		{	
			sprintf((char*)&disp_buffer1[0],"PF: %f", gFinalAnaValF[456]);
			sprintf((char*)&disp_buffer2[0],"KW: %f", gFinalAnaValF[457]);
			break;				
		}
		case 13:
		{	
			sprintf((char*)&disp_buffer1[0],"Frq: %f", gFinalAnaValF[458]);
			sprintf((char*)&disp_buffer2[0],"KWh: %f", gFinalAnaValF[459]);
			break;				
		}
	}

	DispAt(0,(char*)disp_buffer1);
	L1=strlen(&disp_buffer1[0]);

	for(i=L1;i<16;i++)
	{
		DispAt(i+1,(char*)" ");
	}

	if(Pcbplc_Display == 0)
	{
		DispAt(17,(char*)disp_buffer2);
		L1=strlen(&disp_buffer2[0]);
		for(i=L1;i<16;i++)
		{
			DispAt(i+17,(char*)" ");
		}
	}
}
void Disp_prog_menu(void)
{
	if(1 == KBPage.enter_count)
	{
		shour	= rtc_time.mHour;
		sminute = rtc_time.minute;
		ssecond = rtc_time.mSecond;
		sdate 	= rtc_time.mDate;
		smonth 	= rtc_time.month;
		syear 	= rtc_time.myear;
		Case1EnterCout();
	}
	else if((KBPage.enter_count >= 2) && (KBPage.enter_count <= 20))
	{
		switch(KBPage.prog_page)
		{
            case 1:
			{
				Case1pro_page();
				break;
			}
			case 2:
			{
				Case2setpoint_page(); 
				break;
			}
			case 3:
			{
				Case3schedule_page();
				break;
			}
			case 4:
			{
				Case4alarm_page();
				break;
			}
			case 5:
			{
				Case5timer_page();
				break;
			}
			case 6:
			{
				Case6Energy_page();
				break;
			}
			case 7:
			{
				Case7version_page();
				break;
			}
			case 8:
			{
				Case8do_page();
				break;
			}
			case 9:
			{
				Case9mode_page();
				break;
			}
			case 10:
			{
				Case10maxsms_page();
				break;
			}
			case 11:
			{
				Case11gprs_page();
				break;
			}
			case 12:
			{
				Case12streetlight_page();
				break;
			}
			case 13:
			{
				// Case13daywisedim_page();
				break;
			}
        }
    }
}

//display routine for first ENTER case
void Case1EnterCout(void)
{
	unsigned int i,j;
	
	switch(KBPage.prog_page)
	{
		case 1:
		{
			QL_APP_I2C_LOG("I2C Setup_ID[%d]=%d",KBPage.enter_count-3,Setup_ID[KBPage.enter_count-3]);
			sprintf((char*)&disp_buffer1[0],"SetUp Config?   ");
			sprintf((char*)&disp_buffer2[0],">>              ");
		   	KBPage.setup_page = 1;

			sprintf((char*)dispStr,"%04d", EPROM_General.Rtu_Detail.RTUId);
			Setup_ID[0]=dispStr[0]-'0';
			Setup_ID[1]=dispStr[1]-'0';
			Setup_ID[2]=dispStr[2]-'0';
			Setup_ID[3]=dispStr[3]-'0';

			sprintf((char*)dispStr,"%04d", EPROM_General.Cust_Detail.Client_Id);
			Setup_cd[0]=dispStr[0]-'0';
			Setup_cd[1]=dispStr[1]-'0';
			Setup_cd[2]=dispStr[2]-'0';
			Setup_cd[3]=dispStr[3]-'0';

			sprintf((char*)dispStr,"%04d", EPROM_General.Cust_Detail.Reader_Id);
			Setup_rd[0]=dispStr[0]-'0';
			Setup_rd[1]=dispStr[1]-'0';
			Setup_rd[2]=dispStr[2]-'0';
			Setup_rd[3]=dispStr[3]-'0';

			sprintf((char*)dispStr,"%04d", EPROM_General.LogRate);
			Setup_log[0]=dispStr[0]-'0';
			Setup_log[1]=dispStr[1]-'0';
			Setup_log[2]=dispStr[2]-'0';
			Setup_log[3]=dispStr[3]-'0';
			break;
		}
		case 2:
		{
			sprintf((char*)&disp_buffer1[0],"Setpoint Config?");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.setpoint_page = 1;
			break;
	   	}
		case 3:
		{
			sprintf((char*)&disp_buffer1[0],"Schedule Config?");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.schedual_page = 1;
			break;
	   	}
		case 4:
		{
			EPROM_General.Mo_Comm.Mo_Com_Int = 0;
			if(EPROM_General.Mo_Comm.Mo_Com_Int == 0)
			{
				sprintf((char*)&disp_buffer1[0],"Alarm Config?   ");
				sprintf((char*)&disp_buffer2[0],">>              ");
				strcpy((char*)Alarm_MO[0],(char*)EPROM_General.Mo_Comm.mobMCS[0]);
				strcpy((char*)Alarm_MO[1],(char*)EPROM_General.Mo_Comm.mobMCS[1]);
				strcpy((char*)Alarm_MO[2],(char*)EPROM_General.Mo_Comm.mobMCS[2]);
				strcpy((char*)Alarm_MO[3],(char*)EPROM_General.Mo_Comm.mobMCS[3]);
				KBPage.alarm_page = 1;
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"Alarm Config?   ");
				sprintf((char*)&disp_buffer2[0],"Not applicable  ");
				KBPage.alarm_page = 1;
			}
			break;
	   	}
		case 5:
		{
			sprintf((char*)&disp_buffer1[0],"Timer Config?   ");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.timer_page = 1;
			break;
		}
		case 6:
		{
			sprintf((char*)&disp_buffer1[0],"Energy Config?  ");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.energy_page = 1;
			break;
		}
		case 7:
		{
			sprintf((char*)&disp_buffer1[0],"Version Info?   ");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.version_page = 1;
			break;
		}
		case 8:
		{
			sprintf((char*)&disp_buffer1[0],"DO Settings?    ");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.do_page = 1;
			break;
		}
		case 9:
		{
			sprintf((char*)&disp_buffer1[0],"Mode Settings?  ");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.mode_page = 1;
			break;
		}
		case 10:
		{
			sprintf((char*)&disp_buffer1[0],"MAX SMS Config? ");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.MaxSMS_page = 1;
			// sprintf(dispStr,"%04d",EPROM_General.MaxofSMS);
			// MAXSMS_ID[0]=dispStr[0]-'0';
			// MAXSMS_ID[1]=dispStr[1]-'0';
			// MAXSMS_ID[2]=dispStr[2]-'0';
			// MAXSMS_ID[3]=dispStr[3]-'0';
			break;
		}
		case 11:
		{
			sprintf((char*)&disp_buffer1[0],"GPRS Config?    ");
			sprintf((char*)&disp_buffer2[0],">>              ");
			KBPage.gprs_page = 1;

			sprintf((char *)dispStr,"%04d",EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port);
			GPRSPort[0]=dispStr[0]-'0';
			GPRSPort[1]=dispStr[1]-'0';
			GPRSPort[2]=dispStr[2]-'0';
			GPRSPort[3]=dispStr[3]-'0';

			memset(CommaStr,0,sizeof(CommaStr));
			for(i=0,CommPosition=0,j=0;i<strlen(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP);i++)
			{
				if(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP[i]==',' || EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP[i]==':' || EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP[i]=='.')
				{
					CommPosition++;
					j=0;
				}
				else
				{
					CommaStr[CommPosition][j]=EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP[i];
					j++;
				}
			}
			GPRSLiveIPbufVar[0]=atoi((char *)&CommaStr[0][0]);
			GPRSLiveIPbufVar[1]=atoi((char *)&CommaStr[1][0]);
			GPRSLiveIPbufVar[2]=atoi((char *)&CommaStr[2][0]);
			GPRSLiveIPbufVar[3]=atoi((char *)&CommaStr[3][0]);
			sprintf((char *)GPRSLiveIPbuf,"%03d.%03d.%03d.%03d", GPRSLiveIPbufVar[0], GPRSLiveIPbufVar[1], GPRSLiveIPbufVar[2],GPRSLiveIPbufVar[3]);
			break;
		}
		case 12:
		{
			sprintf((char*)&disp_buffer1[0],"Street Light    ");
			sprintf((char*)&disp_buffer2[0],"Settings?    >> ");
			KBPage.StreetLight_page = 1;
			memcpy(&update_time, &rtc_time, sizeof(DataTime_t));
			break;
		}
		case 13:
		{
			sprintf((char*)&disp_buffer1[0],"DayWise DIMSched");
			sprintf((char*)&disp_buffer2[0],"Settings?    >> ");
			KBPage.DSschedual_page = 1;
			break;
		}
		default: 
		{
			break;
		}
	}
	DispAt(1,disp_buffer1);
	uint8_t L1=strlen(&disp_buffer2[0]);
	for(uint8_t i=L1;i<16;i++)
	{
		DispAt(i+1,(char*)" ");
	}
	DispAt(17,disp_buffer2);
	L1=strlen(&disp_buffer2[0]);
	for(uint8_t i=L1;i<16;i++)
	{
		DispAt(i+17,(char*)" ");
	}
}

void Case1pro_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));
	switch (KBPage.setup_page)
	{
		case 1:
	   	{
			if((KBPage.enter_count == 3 || KBPage.enter_count==15) && (b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ", Setup_ID[0], Setup_ID[1], Setup_ID[2], Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d%d%d    ", Setup_cd[0], Setup_cd[1], Setup_cd[2], Setup_cd[3], Setup_rd[0], Setup_rd[1], Setup_rd[2], Setup_rd[3]);
			}
			else if((KBPage.enter_count==3) && (!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID= %d%d%d    ", Setup_ID[1], Setup_ID[2], Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d%d%d    ", Setup_cd[0], Setup_cd[1], Setup_cd[2], Setup_cd[3], Setup_rd[0], Setup_rd[1], Setup_rd[2], Setup_rd[3]);
			}
			else if((KBPage.enter_count==4)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d %d%d    ",Setup_ID[0],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d%d%d    ", Setup_cd[0], Setup_cd[1], Setup_cd[2], Setup_cd[3], Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==5)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d %d    ",Setup_ID[0],Setup_ID[1],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d%d%d    ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==6)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d     ",Setup_ID[0],Setup_ID[1],Setup_ID[2]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d%d%d    ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==7)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID: %d%d%d %d%d%d%d    ",Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==8)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d %d%d %d%d%d%d    ",Setup_cd[0],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==9)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d %d %d%d%d%d    ",Setup_cd[0],Setup_cd[1],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==10)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d  %d%d%d%d    ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			// Reader 
			else if((KBPage.enter_count==11)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d  %d%d%d    ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==12)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d %d%d    ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[2],Setup_rd[3]);
			}
			else if((KBPage.enter_count==13)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d %d    ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[3]);
			}
			else if((KBPage.enter_count==14)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d%d     ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[2]);
			}
			else
			{
				sprintf((char*)&disp_buffer1[0], "UNIT ID=%d%d%d%d    ",Setup_ID[0], Setup_ID[1], Setup_ID[2], Setup_ID[3]);
				sprintf((char*)&disp_buffer2[0], "CID:%d%d%d%d %d%d%d%d    ",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			}
			break;
		}
		case 2:
		{
			if((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d%d%d%d   ", Setup_log[0], Setup_log[1], Setup_log[2], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else if ((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE= %d%d%d   ", Setup_log[1], Setup_log[2], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else if((KBPage.enter_count==4)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d%d%d%d   ", Setup_log[0], Setup_log[1], Setup_log[2], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else if ((KBPage.enter_count==4)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d %d%d   ", Setup_log[0], Setup_log[2], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else if((KBPage.enter_count==5)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d%d%d%d   ", Setup_log[0], Setup_log[1], Setup_log[2], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else if ((KBPage.enter_count==5)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d%d %d   ", Setup_log[0], Setup_log[1], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else if((KBPage.enter_count==6)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d%d%d%d   ", Setup_log[0], Setup_log[1], Setup_log[2], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else if ((KBPage.enter_count==6)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d%d%d    ", Setup_log[0], Setup_log[1], Setup_log[2]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"LOG RATE=%d%d%d%d    ", Setup_log[0], Setup_log[1], Setup_log[2], Setup_log[3]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-1440 min.");
			}
			break;
		}
		case 3:
		{
			if((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"NO OF DO=%2d     ", EPROM_General.AI_DI_DO_Detail.Total_Do);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if ((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"NO OF DO=       ");
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"NO OF DO=%2d     ", EPROM_General.AI_DI_DO_Detail.Total_Do);
				sprintf((char*)&disp_buffer2[0],"                "); 	
			}
			break;
		}
		case 4:
		{
			if((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"NO OF DI=%2d     ", EPROM_General.AI_DI_DO_Detail.Total_Di);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if ((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"NO OF DI=       ");
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"NO OF DI=%2d     ", EPROM_General.AI_DI_DO_Detail.Total_Di);
				sprintf((char*)&disp_buffer2[0],"                "); 	
			}
			break;
		}
		case 5:
		{
			if((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"COMM TYPE : GPRS");
				sprintf((char*)&disp_buffer2[0],"DataOn WEBSCANET");
			}
			else if ((KBPage.enter_count==3) && (!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"COMM TYPE :     ");
				sprintf((char*)&disp_buffer2[0],"DataOn WEBSCANET");
			}
			break;
		}
	}
	// DispAt(1,disp_buffer1);
	// DispAt(17,disp_buffer2);
	DispAt(1,disp_buffer1);
	// uint8_t L1=strlen(&disp_buffer2[0]);
	// for(uint8_t i=L1;i<16;i++)
	// {
	// 	DispAt(i+1,(char*)" ");
	// }

	DispAt(17,disp_buffer2);
	// L1=strlen(&disp_buffer2[0]);
	// for(uint8_t i=L1;i<16;i++)
	// {
	// 	DispAt(i+17,(char*)" ");
	// }
}

void Case2setpoint_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	if((KBPage.setpoint_page >= 1) && (KBPage.setpoint_page < 4))
	{
		if((KBPage.enter_count == 2) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "LHB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LHB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 3) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "LHB:HV%02d:        ", KBPage.setpoint_page);
			sprintf((char*)&disp_buffer2[0], "LHB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 4) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "LHB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LHB:LV%02d:        ", KBPage.setpoint_page);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0], "LHB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LHB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
	}
	else if((KBPage.setpoint_page >= 4) && (KBPage.setpoint_page < 7))
	{
		if((KBPage.enter_count == 2) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "HEB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);	
			sprintf((char*)&disp_buffer2[0], "HEB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 3) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "HEB:HV%02d:        ", KBPage.setpoint_page);
			sprintf((char*)&disp_buffer2[0], "HEB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 4) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "HEB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "HEB:LV%02d:        ", KBPage.setpoint_page);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0], "HEB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "HEB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
	}
	else if(KBPage.setpoint_page == 7)
	{
		if((KBPage.enter_count == 2) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "LEB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LEB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 3) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "LEB:HV%02d:        ", KBPage.setpoint_page);
			sprintf((char*)&disp_buffer2[0], "LEB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 3) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "LEB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LEB:LV%02d:        ", KBPage.setpoint_page);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0], "LEB:HV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LEB:LV%02d:%03.02f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
	}
	else if(KBPage.setpoint_page >=8 && KBPage.setpoint_page < 11)
	{
		if((KBPage.setpoint_page == 8) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IR%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LED:RP%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 3) && (KBPage.setpoint_page == 8) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IR%02d:        ", KBPage.setpoint_page);
			sprintf((char*)&disp_buffer2[0], "LED:RP%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 4) && (KBPage.setpoint_page == 8) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IR%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LED:RP%02d:        ", KBPage.setpoint_page);
		}		
		else if((KBPage.setpoint_page == 9) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IY%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LED:YP%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 3) && (KBPage.setpoint_page == 9) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IY%02d:         ", KBPage.setpoint_page);
			sprintf((char*)&disp_buffer2[0], "LED:YP%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}else if((KBPage.enter_count == 4) && (KBPage.setpoint_page == 9) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IY%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LED:YP%02d:        ", KBPage.setpoint_page);
		}		
		else if((KBPage.setpoint_page == 10) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IB%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LED:BP%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 3) && (KBPage.setpoint_page == 10) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IB%02d:        ", KBPage.setpoint_page);
			sprintf((char*)&disp_buffer2[0], "LED:BP%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.lo_value[KBPage.setpoint_page-1]);
		}
		else if((KBPage.enter_count == 4) && (KBPage.setpoint_page == 10) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0], "Tot:IB%02d:%03.0f   ", KBPage.setpoint_page, EPROM_General.sp.hi_value[KBPage.setpoint_page-1]);
			sprintf((char*)&disp_buffer2[0], "LED:BP%02d:        ", KBPage.setpoint_page);
		}
	}
	DispAt(1, disp_buffer1);
	DispAt(17, disp_buffer2);
}


void Case3schedule_page(void)
{
	int8_t SChno;//, temp_sc;

	QL_APP_I2C_LOG("temp_sc:%d", EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di);

	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	SChno = KBPage.schedual_page-1;

	if((KBPage.enter_count == 3) && (b.blink))
	{
		if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di==DISABLE)
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"DIS: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"ENB: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
	}
	else if((KBPage.enter_count == 3) && (!b.blink))
	{
		if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di==DISABLE)
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"   : STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"   : STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
	}
	else if((KBPage.enter_count == 4) && (!b.blink))
	{
		if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di==DISABLE)
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:  .%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"DIS: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:  .%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"ENB: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
	}
	else if((KBPage.enter_count == 5) && (!b.blink))
	{
		if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di==DISABLE)
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.  ", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH);
			sprintf((char*)&disp_buffer2[0],"DIS: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.  ", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH);
			sprintf((char*)&disp_buffer2[0],"ENB: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
	}
	else if((KBPage.enter_count == 6) && (!b.blink))
	{
		if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di==DISABLE)
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"DIS: STOP :  .%02d", EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"ENB: STOP :  .%02d", EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
	}
	else if((KBPage.enter_count == 7) && (!b.blink))
	{
		if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di==DISABLE)
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"DIS: STOP :%02d.  ", EPROM_Schedule.Schedule[SChno].Stop_HH);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"ENB: STOP :%02d.  ", EPROM_Schedule.Schedule[SChno].Stop_HH);
		}
	}
	else
	{
		if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di==DISABLE)
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"DIS: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"SC%d= START:%02d.%02d", KBPage.schedual_page, EPROM_Schedule.Schedule[SChno].Start_HH, EPROM_Schedule.Schedule[SChno].Start_Min);
			sprintf((char*)&disp_buffer2[0],"ENB: STOP :%02d.%02d", EPROM_Schedule.Schedule[SChno].Stop_HH, EPROM_Schedule.Schedule[SChno].Stop_Min);
		}
	}
	DispAt(1, disp_buffer1);
	DispAt(17, disp_buffer2);
}

void Case4alarm_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

			if(!b.blink)
			{
				sprintf((char*)&disp_buffer1[0],"Mobile No%d:   ",KBPage.alarm_page);
				sprintf((char*)&disp_buffer2[0],"%s" , Alarm_MO[KBPage.alarm_page-1]);
			}
			else if((KBPage.enter_count>=3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Mobile No%d:   ",KBPage.alarm_page);
				sprintf((char*)&disp_buffer2[0],"%s" , Alarm_MO[KBPage.alarm_page-1]);
				disp_buffer2[KBPage.enter_count-3] = ' ';
			}
	DispAt(1, disp_buffer1);
	DispAt(17, disp_buffer2);
}

void Case5timer_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	if((KBPage.timer_page >= 1) && (KBPage.timer_page <= 10))
	{
		if((KBPage.enter_count == 3) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0],"TMR%02d:%04ld   ", KBPage.timer_page, EPROM_General.Def_timer[KBPage.timer_page-1]);
			sprintf((char*)&disp_buffer2[0],"Limit:1-2024   ");
		}
		else if((KBPage.enter_count == 3) && (!b.blink))
		{
			sprintf((char*)&disp_buffer1[0],"TMR%02d:        ", KBPage.timer_page);
			sprintf((char*)&disp_buffer2[0],"Limit:1-2024   ");
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"TMR%02d:%04ld   ", KBPage.timer_page, EPROM_General.Def_timer[KBPage.timer_page-1]);
			sprintf((char*)&disp_buffer2[0],"Limit:1-2024   ");
		}
	}
	DispAt(1, disp_buffer1);
	DispAt(17, disp_buffer2);
}

void Case6Energy_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	switch(KBPage.energy_page)
	{
		case 1:
		{
			if(KBPage.enter_count == 3 && b.blink)
			{
				if(EPROM_General.MeterMake == METER_HPL_NORMAL)
				{
					sprintf((char*)&disp_buffer1[0],"MeterMake:      ");
					sprintf((char*)&disp_buffer2[0],"  HPL NORMAL    ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"MeterMake:      ");
					sprintf((char*)&disp_buffer2[0],"  HPL DLMS      ");
				}
			}
			else if(KBPage.enter_count == 3 && !b.blink)
			{
				if(EPROM_General.MeterMake == METER_HPL_NORMAL)
				{
					sprintf((char*)&disp_buffer1[0],"MeterMake:      ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"MeterMake:      ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
			}
			else
			{
				if(EPROM_General.MeterMake == METER_HPL_NORMAL)
				{
					sprintf((char*)&disp_buffer1[0],"MeterMake:      ");
					sprintf((char*)&disp_buffer2[0],"  HPL NORMAL    ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"MeterMake:      ");
					sprintf((char*)&disp_buffer2[0],"  HPL DLMS      ");
				}
			}
			break;
		}
		case 2:
		{
			if(KBPage.enter_count == 3 && b.blink)
			{
				if(EPROM_General.MeterType == THREEPHASE)
				{
					sprintf((char*)&disp_buffer1[0],"MeterType:3PHASE");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"MeterType:1PHASE");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
			}
			else if(KBPage.enter_count == 3 && !b.blink)
			{
				if(EPROM_General.MeterType == THREEPHASE)
				{
					sprintf((char*)&disp_buffer1[0],"MeterType:      ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"MeterType:      ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
			}
			else
			{
				if(EPROM_General.MeterType == THREEPHASE)
				{
					sprintf((char*)&disp_buffer1[0],"MeterType:3PHASE");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"MeterType:1PHASE");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
			}
			break;
		}
	}
	DispAt(1, disp_buffer1);
	DispAt(17, disp_buffer2);
}
void Case7version_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	switch(KBPage.version_page)
	{
		case 1:
		{
			sprintf((char*)&disp_buffer1[0],"VER=%s       ", DEFAULT_FV_VERSION);
			sprintf((char*)&disp_buffer2[0],"                ");
			break;
		}
		case 2:
		{
			sprintf((char*)&disp_buffer1[0],"NO OF DI= %02d    ", MAX_DI_CHANNEL);
			sprintf((char*)&disp_buffer2[0],"                ");
			break;
		}
		case 3:
		{
			sprintf((char*)&disp_buffer1[0],"NO OF DO= %02d    ", MAX_DO_CHANNEL);
			sprintf((char*)&disp_buffer2[0],"                ");
			break;
		}
		case 4:
		{
			sprintf((char*)&disp_buffer1[0],"HW VER=%s    ", EPROM_General.Rtu_Detail.HW_Version);
			sprintf((char*)&disp_buffer2[0],"                ");
			break;
		}
	}

	DispAt(1, disp_buffer1);
	DispAt(17, disp_buffer2);
}

void Case8do_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	switch(KBPage.do_page)
	{
		case 1:	
		{
			if((KBPage.enter_count==3)&&(!b.blink))
			{	
				sprintf((char*)&disp_buffer1[0],"DO1 =           ");
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else 
			{
				// if(EPROM_General.AI_DI_DO_Detail.DOSignal[KBPage.do_page]==1)
				if(b.DO1_temp == 1)
				{
					sprintf((char*)&disp_buffer1[0],"DO1 = ON        ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"DO1 = OFF       ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
			}
			break;
		}
		case 2:
		{
			if((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"DO2 =           ");
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else
			{
				//if(EPROM_General.AI_DI_DO_Detail.DOSignal[KBPage.do_page]==1)
				if(b.DO2_temp == 1)
				{
					sprintf((char*)&disp_buffer1[0],"DO2 = ON        ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"DO2 = OFF       ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
			}
			break;
		}
		case 3:
		{
			if((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"DO3 =           ");
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else
			{
				// if(EPROM_General.AI_DI_DO_Detail.DOSignal[KBPage.do_page]==1)
				if(b.DO3_temp == 1)
				{
					sprintf((char*)&disp_buffer1[0],"DO3 = ON        ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
				else
				{
					sprintf((char*)&disp_buffer1[0],"DO3 = OFF       ");
					sprintf((char*)&disp_buffer2[0],"                ");
				}
			}
			break;
		}
	}
	DispAt(1,disp_buffer1);
	DispAt(17,disp_buffer2);
}

void Case9mode_page(void)
{
	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));

	switch(KBPage.mode_page)
	{
		case 1:	
		{
			if((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"MODE =           ");
				sprintf((char*)&disp_buffer2[0],"                 ");
			}
			else 
			{
				if(EPROM_General.DoModeDetails.Do_Mode_temp==RTU_DO_MODE_MANUAL)
				{
					sprintf((char*)&disp_buffer1[0],"MODE = MANUAL  ");
					sprintf((char*)&disp_buffer2[0],"               ");
				}
				else if(EPROM_General.DoModeDetails.Do_Mode_temp==RTU_DO_MODE_PHOTO)
				{
					sprintf((char*)&disp_buffer1[0],"MODE = PHOTO   ");
					sprintf((char*)&disp_buffer2[0],"     N.A       ");
				}
				else if(EPROM_General.DoModeDetails.Do_Mode_temp==RTU_DO_MODE_AUTO)
				{
					sprintf((char*)&disp_buffer1[0],"MODE = SCHEDULE");
					sprintf((char*)&disp_buffer2[0],"               ");
				}
				else if(EPROM_General.DoModeDetails.Do_Mode_temp==RTU_DO_MODE_ASTROTIME_GEO)
				{
					sprintf((char*)&disp_buffer1[0],"MODE = ASTRO   ");
					sprintf((char*)&disp_buffer2[0],"               ");
				}
			}
			break;
		}
	}
	DispAt(1,disp_buffer1);
	DispAt(17,disp_buffer2);
}

/*unsigned char* CaseDecimalEnter(void)
{
	memset(dispDecimal,0,sizeof(dispDecimal));
	if(Deci.CheckDecimalEnable==1)
	{
		if(b.blink)
		{
			switch(Deci.enter_count_Decimal)
			{
				case 1:
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
				}
				break;
				case 2: 
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
				}
				break;
				case 3:
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
				}
				break;
				case 4:
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
					Deci.CheckDecimalValue=1;
				}
				break;
				case 5:
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
					Deci.CheckDecimalValue=1;
				}
				break;
				case 6:
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
					Deci.CheckDecimalValue=1;
				}
				break;
				default : break;
			}
		}
		else 
		{
			switch(Deci.enter_count_Decimal)
			{
				case 1:
				{
					sprintf((char*)&dispDecimal[0]," %d%d%d",Deci.value[1],Deci.value[2],Deci.value[3]);
				} 
				break;
				case 2:
				{
					sprintf((char*)&dispDecimal[0],"%d %d%d",Deci.value[0],Deci.value[2],Deci.value[3]);
				}
				break;
				case 3:
				{
					sprintf((char*)&dispDecimal[0],"%d%d %d",Deci.value[0],Deci.value[1],Deci.value[3]);
				} 
				break;
				case 4: 
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d ",Deci.value[0],Deci.value[1],Deci.value[2]);
					Deci.CheckDecimalValue=1;
				}
				break;
				case 5:
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
					Deci.CheckDecimalValue=1;
				}
				break;
				case 6:
				{
					sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
					Deci.CheckDecimalValue=1;
				}
				break;
				default : break;																													
			}
		}
	}
	return &dispDecimal[0];
}*/

void Case10maxsms_page(void)
{
	unsigned char i = 0, L1;

	memset(&disp_buffer1[0], '\0', sizeof(disp_buffer1));
	memset(&disp_buffer2[0], '\0', sizeof(disp_buffer2));
	
#if 0
	if(KBPage.MaxSMS_page == 1)
	{
		if((KBPage.enter_count == 3) && (b.blink))
		{
			sprintf((char*)&disp_buffer1[0],"MAX SMS = %02d  ",EPROM_General.MaxofSMS);
			sprintf((char*)&disp_buffer2[0],"Lim: 1-64       ");		
		}
		else if((KBPage.enter_count == 3)&&(!b.blink))
		{
			sprintf((char*)&disp_buffer1[0],"MAX SMS =       ");
			sprintf((char*)&disp_buffer2[0],"Lim: 1-64       ");
		}
		else
		{
			sprintf((char*)&disp_buffer1[0],"MAX SMS = %02d  ",EPROM_General.MaxofSMS);
			sprintf((char*)&disp_buffer2[0],"Lim: 1-64      ");
		}
	}
	else if((KBPage.MaxSMS_page >= 2) && (KBPage.MaxSMS_page <= EPROM_General.MaxofSMS+1))
	{
		if((KBPage.enter_count==3)&&(b.blink))
		{
			sprintf((char*)&disp_buffer1[0],"%02d = %s       ",KBPage.MaxSMS_page-1,CaseDecimalEnter());
			sprintf((char*)&disp_buffer2[0],"                ");
		}
		else if((KBPage.enter_count==3)&&(!b.blink))
		{
			sprintf((char*)&disp_buffer1[0],"%02d = %s       ",KBPage.MaxSMS_page-1,CaseDecimalEnter());
			sprintf((char*)&disp_buffer2[0],"                ");
		}
		else
		{
			Deci.enter_count_Decimal=1;
			sprintf((char*)&disp_buffer1[0],"%04d",EPROM_General.ModSMSList[KBPage.MaxSMS_page-2]);
			for(i=0;i<4;i++)
			{
				Deci.value[i]=disp_buffer1[i]-'0';
			}
			sprintf((char*)&disp_buffer1[0],"%02d = %04d     ",KBPage.MaxSMS_page-1,EPROM_General.ModSMSList[KBPage.MaxSMS_page-2]);
			sprintf((char*)&disp_buffer2[0],"                ");
		}
	// 	sprintf((char *)dispStr,"%04d",EPROM_General.ModSMSList[KBPage.MaxSMS_page-2]);
	// 	MAXSMS_ID[0]=dispStr[0]-'0';
	// 	MAXSMS_ID[1]=dispStr[1]-'0';
	// 	MAXSMS_ID[2]=dispStr[2]-'0';
	// 	MAXSMS_ID[3]=dispStr[3]-'0';

	// 	if((KBPage.enter_count == 3) && (b.blink))
	// 	{
	// 		sprintf((char*)&disp_buffer1[0], "%02d=%d%d%d%d   ", KBPage.MaxSMS_page-1, MAXSMS_ID[0], MAXSMS_ID[1], MAXSMS_ID[2], MAXSMS_ID[3]);
	// 		sprintf((char*)&disp_buffer2[0],"                ");
	// 	}
	// 	else if((KBPage.enter_count==3) && (!b.blink))
	// 	{
	// 		sprintf((char*)&disp_buffer1[0], "%02d= %d%d%d    ", KBPage.MaxSMS_page-1, MAXSMS_ID[1], MAXSMS_ID[2], MAXSMS_ID[3]);
	// 		sprintf((char*)&disp_buffer2[0],"                ");
	// 	}
	// 	else if((KBPage.enter_count==4)&&(!b.blink))
	// 	{
	// 		sprintf((char*)&disp_buffer1[0], "%02d=%d %d%d    ", KBPage.MaxSMS_page-1, MAXSMS_ID[0],MAXSMS_ID[2],MAXSMS_ID[3]);
	// 		sprintf((char*)&disp_buffer2[0],"                ");
	// 	}
	// 	else if((KBPage.enter_count==5)&&(!b.blink))
	// 	{
	// 		sprintf((char*)&disp_buffer1[0], "%02d=%d%d %d    ", KBPage.MaxSMS_page-1, MAXSMS_ID[0],MAXSMS_ID[1],MAXSMS_ID[3]);
	// 		sprintf((char*)&disp_buffer2[0],"                ");
	// 	}
	// 	else if((KBPage.enter_count==6)&&(!b.blink))
	// 	{
	// 		sprintf((char*)&disp_buffer1[0], "%02d=%d%d%d     ", KBPage.MaxSMS_page-1, MAXSMS_ID[0],MAXSMS_ID[1],MAXSMS_ID[2]);
	// 		sprintf((char*)&disp_buffer2[0],"                ");
	// 	}
	// 	else
	// 	{
	// 		sprintf((char*)&disp_buffer1[0], "%02d=%d%d%d%d   ", KBPage.MaxSMS_page-1, MAXSMS_ID[0], MAXSMS_ID[1], MAXSMS_ID[2], MAXSMS_ID[3]);
	// 		sprintf((char*)&disp_buffer2[0],"                ");
	// 	}
	}
	// DispAt(1,disp_buffer1);
	// DispAt(17,disp_buffer2);
#endif
	switch (KBPage.MaxSMS_page)
	{
		case 1:
		{
			// sprintf((char*)&disp_buffer2[0],"                ");
			// if((KBPage.enter_count>=3)&&(!b.blink))
			// {
            //     sprintf((char*)&disp_buffer1[0],"MAX SMS = %d%d%d%d",MAXSMS_ID[0],MAXSMS_ID[1],MAXSMS_ID[2],MAXSMS_ID[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
            //     disp_buffer1[7+KBPage.enter_count]=' ';
			// 	sprintf((char*)&disp_buffer2[0],"Lim: 1-64     ");
			// }
			// else
			// {
            //     sprintf((char*)&disp_buffer1[0],"MAX SMS = %d%d%d%d",MAXSMS_ID[0],MAXSMS_ID[1],MAXSMS_ID[2],MAXSMS_ID[3],Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
			// 	sprintf((char*)&disp_buffer2[0],"Lim: 1-64     ");
			// }
				// break;
			if((KBPage.enter_count == 3) && (b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"MAX SMS = %02d  ",EPROM_General.MaxofSMS);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-64       ");
			}
			else if((KBPage.enter_count == 3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"MAX SMS =       ");
				sprintf((char*)&disp_buffer2[0],"Lim: 1-64       ");
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"MAX SMS = %02d  ",EPROM_General.MaxofSMS);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-64      ");
			}
			break;
		}
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
		case 48:
		case 49:
		case 50:
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		case 58:
		case 59:
		case 60:
		case 61:
		case 62:
		case 63:
		case 64:
		case 65:
		{
			sprintf((char*)&disp_buffer2[0],"                ");
			if((KBPage.enter_count>=3 && KBPage.enter_count<=6))
			{
				sprintf((char*)&disp_buffer1[0],"%02d = %d%d%d%d",KBPage.MaxSMS_page-1,Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
                if((!b.blink))
				{
					disp_buffer1[2+KBPage.enter_count]=' ';
				}
				sprintf((char*)&disp_buffer2[0],"Lim: 1-9999     ");
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"%04d",EPROM_General.ModSMSList[KBPage.MaxSMS_page-2]);
				QL_APP_I2C_LOG("disp_buffer1:%s",disp_buffer1);
				for(i=0;i<4;i++)
				{
					Deci.value[i]=disp_buffer1[i]-'0';
				}
				sprintf((char*)&disp_buffer1[0],"%02d = %04d",KBPage.MaxSMS_page-1,EPROM_General.ModSMSList[KBPage.MaxSMS_page-2]);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-9999     ");
			}
			break;
		}
		default:
			break;
	}

	DispAt(0,(char*)disp_buffer1);
	L1=strlen(&disp_buffer1[0]);

	for(i=L1;i<16;i++)
	{
		DispAt(i+1,(char*)" ");
	}

	if(Pcbplc_Display == 0)
	{
		DispAt(17,(char*)disp_buffer2);
		L1=strlen(&disp_buffer2[0]);
		for(i=L1;i<16;i++)
		{
			DispAt(i+17,(char*)" ");
		}
	}
}

void Case11gprs_page(void)
{
	unsigned char L1,i=0;			

	switch(KBPage.gprs_page)
	{
		case 1:
		{
			if((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"APN Name:       ");
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"APN Name:       ");
				sprintf((char*)&disp_buffer2[0],"%s", EPROM_General.Mo_Comm.Mo_APN);
			}
			break;
		}
		case 2:
		{
			// if((KBPage.enter_count==3)&&(!b.blink))
			// {
			//    sprintf((char*)&disp_buffer1[0],"Live Port=             ");
			// }
			// else
			// {
			// 	sprintf((char*)&disp_buffer1[0],"Live Port=%d   ", EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port);
			// 	sprintf((char*)&disp_buffer2[0],"                ");
			// }
			if((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d%d%d%d   ", GPRSPort[0], GPRSPort[1], GPRSPort[2], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if ((KBPage.enter_count==3)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port= %d%d%d   ", GPRSPort[1], GPRSPort[2], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if((KBPage.enter_count==4)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d%d%d%d   ", GPRSPort[0], GPRSPort[1], GPRSPort[2], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if ((KBPage.enter_count==4)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d %d%d   ", GPRSPort[0], GPRSPort[2], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if((KBPage.enter_count==5)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d%d%d%d   ", GPRSPort[0], GPRSPort[1], GPRSPort[2], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if ((KBPage.enter_count==5)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d%d %d   ", GPRSPort[0], GPRSPort[1], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if((KBPage.enter_count==6)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d%d%d%d   ", GPRSPort[0], GPRSPort[1], GPRSPort[2], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else if ((KBPage.enter_count==6)&&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d%d%d    ", GPRSPort[0], GPRSPort[1], GPRSPort[2]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			else
			{
				sprintf((char*)&disp_buffer1[0],"Live Port=%d%d%d%d    ", GPRSPort[0], GPRSPort[1], GPRSPort[2], GPRSPort[3]);
				sprintf((char*)&disp_buffer2[0],"                ");
			}
			break;
		}
		case 3:
		{
			if(KBPage.enter_count>=3 && KBPage.enter_count<=17 &&(!b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Live IP            ");
                sprintf((char*)&disp_buffer2[0],"%s",GPRSLiveIPbuf);
                disp_buffer2[KBPage.enter_count-3]=' ';
			}
			else 
			{
				sprintf((char*)&disp_buffer1[0],"Live IP            ");
                sprintf((char*)&disp_buffer2[0],"%s",GPRSLiveIPbuf);
            }
			break;
		}
		default:
		{
	 		break;
		}
	}
	// DispAt(1,disp_buffer1);
	// DispAt(17,disp_buffer2);	
	DispAt(0,(char*)disp_buffer1);
	L1=strlen(&disp_buffer1[0]);

	for(i=L1;i<16;i++)
	{
		DispAt(i+1,(char*)" ");
	}

	if(Pcbplc_Display == 0)
	{
		DispAt(17,(char*)disp_buffer2);
		L1=strlen(&disp_buffer2[0]);
		for(i=L1;i<16;i++)
		{
			DispAt(i+17,(char*)" ");
		}
	}
}
#if 0
void Case12streetlight_page(void)
{
    // Initialize display buffers
    switch(KBPage.StreetLight_page)
    {
        case 1:
        {
            // Format latitude and longitude strings
            sprintf((char*)&disp_buffer1[0], "Lat : %0.4f", EPROM_General.Cust_Detail.Lattitude);
            sprintf((char*)&disp_buffer2[0], "Log : %0.4f", EPROM_General.Cust_Detail.Longitude);

            if (b.blink)
            {
                // Blink specific digits based on enter_count
                int index = KBPage.enter_count - 3;
                if (index >= 0 && index < sizeof(disp_buffer1))
                {
                    disp_buffer1[6 + index] = ' ';
                }
                else if (index >= sizeof(disp_buffer1) && index < sizeof(disp_buffer1) + sizeof(disp_buffer2))
                {
                    disp_buffer2[index - sizeof(disp_buffer1)] = ' ';
                }
            }
            break;
        }
        case 2:
        {
            char timezone_str[20];
            sprintf(timezone_str, "%c%02d:%02d", EPROM_General.Cust_Detail.Timezone_sign ? '-' : '+', 
                    EPROM_General.Cust_Detail.Timezone_hours, EPROM_General.Cust_Detail.Timezone_minutes);

            if (!b.blink || KBPage.enter_count < 3)
            {
                sprintf((char*)&disp_buffer1[0], "UTC TIME: %s", timezone_str);
                sprintf((char*)&disp_buffer2[0], "Lim: 1-24 Hr");
            }
            else
            {
                switch(KBPage.enter_count)
                {
                    case 4:
                        sprintf((char*)&disp_buffer1[0], "UTC TIME:  %02d:%02d", 
                                EPROM_General.Cust_Detail.Timezone_hours, EPROM_General.Cust_Detail.Timezone_minutes);
                        break;
                    case 5:
                        sprintf((char*)&disp_buffer1[0], "UTC TIME: %c  :%02d", 
                                EPROM_General.Cust_Detail.Timezone_sign ? '-' : '+', 
                                EPROM_General.Cust_Detail.Timezone_minutes);
                        break;
                    case 6:
                        sprintf((char*)&disp_buffer1[0], "UTC TIME: %c%02d:  ", 
                                EPROM_General.Cust_Detail.Timezone_sign ? '-' : '+', 
                                EPROM_General.Cust_Detail.Timezone_hours);
                        break;
                    default:
                        // Handle other cases or add default behavior here
                        break;
                }
                sprintf((char*)&disp_buffer2[0], "Lim: 1-24 Hr");
            }
            break;
        }
        case 3:
        {
            if (!b.blink || KBPage.enter_count < 3)
            {
                sprintf((char*)&disp_buffer1[0], "Sunrise Sunset ");
                sprintf((char*)&disp_buffer2[0], "Offset: %d", EPROM_General.Cust_Detail.Offset_Value);
            }
            else
            {
                sprintf((char*)&disp_buffer1[0], "Sunrise Sunset ");
                sprintf((char*)&disp_buffer2[0], "Offset: ");
            }
            break;
        }
        default:
            // Handle other cases or add default behavior here
            break;
    }

    // Clear any remaining characters on the display
    DispAt(0, (char*)disp_buffer1);
    int len1 = strlen((char*)disp_buffer1);
    for(int i = len1; i < 16; i++)
    {
        DispAt(i + 1, (char*)" ");
    }

    if(Pcbplc_Display == 0)
    {
        DispAt(17, (char*)disp_buffer2);
        int len2 = strlen((char*)disp_buffer2);
        for(int i = len2; i < 16; i++)
        {
            DispAt(i + 17, (char*)" ");
        }
    }
}

#endif



void Case12streetlight_page(void)
{
	unsigned char L1,i=0;			

	switch(KBPage.StreetLight_page)
	{
		case 1:
		{
			sprintf((char*)&disp_buffer1[0],"Lat : %0.4f", EPROM_General.Cust_Detail.Lattitude);
			sprintf((char*)&disp_buffer2[0],"Log : %0.4f", EPROM_General.Cust_Detail.Longitude);
			if((KBPage.enter_count==3)&&(b.blink))
			{
				if (disp_buffer1[6]>= 0)
				{
				//sprintf((char*)&disp_buffer1[0],"Lat : %0.4f", EPROM_General.Cust_Detail.Lattitude);
				disp_buffer1[6] = ' ';			// to blink specific digit 
				}
				else
				 {
					disp_buffer1[6] = ' ';
					disp_buffer1[7] = ' ';
				 }
				//sprintf((char*)&disp_buffer2[0],"Log : %0.4f", EPROM_General.Cust_Detail.Longitude);
			}
			else if((KBPage.enter_count==4)&&(b.blink))
			{
				if (disp_buffer1[6]>= 0)
				{
				//sprintf((char*)&disp_buffer1[0],"Lat : %0.4f", EPROM_General.Cust_Detail.Lattitude);
					disp_buffer1[7] = ' ';			// to blink specific digit 
				}
				else
				{
					disp_buffer1[8] = ' ';
				}
				
			}
			else if((KBPage.enter_count==5)&&(b.blink))
			{
				if (disp_buffer1[6]>= 0)
				{
					disp_buffer1[9] = ' ';
				}
				else
				{				
				disp_buffer1[10] = ' ';
				}
				
			}
			else if((KBPage.enter_count==6)&&(b.blink))
			{
				if (disp_buffer1[6]>= 0)
				{
					disp_buffer1[10] = ' ';
				}
				else
				{				
					disp_buffer1[11] = ' ';
				}
				
			}
			else if((KBPage.enter_count==7)&&(b.blink))
			{
				if (disp_buffer1[6]>= 0)
				{
					disp_buffer1[11] = ' ';
				}
				else
				{				
					disp_buffer1[12] = ' ';
				}
			}
			else if((KBPage.enter_count==8)&&(b.blink))
			{
				if (disp_buffer1[6]>= 0)
				{
					disp_buffer1[12] = ' ';
				}

				else
				{				
					disp_buffer1[12] = ' ';
				}
				
			}
			else if((KBPage.enter_count==9)&&(b.blink))
			{
				if (disp_buffer2[6]>= 0)
				{
				
					disp_buffer2[6] = ' ';			// to blink specific digit 
				}
				else
				 {
					disp_buffer2[6] = ' ';
					disp_buffer2[7] = ' ';
				 }		
					
			}
			else if((KBPage.enter_count==10)&&(b.blink))
			{                                          // 1
				if (disp_buffer2[6]>= 0)
				{
				
					disp_buffer2[7] = ' ';			// to blink specific digit 1
				}
				else
				 {
					disp_buffer2[7] = ' ';
					disp_buffer2[8] = ' ';
				 }				
			}
			else if((KBPage.enter_count==11)&&(b.blink))
			{										                   
				if (disp_buffer2[6]>= 0)
				{
				
					disp_buffer2[9] = ' ';			// to blink specific digit 1
				}
				else
				 {
					disp_buffer2[10] = ' ';
					
				 }		
			}
			else if((KBPage.enter_count==12)&&(b.blink))
			{												//0
				
				if (disp_buffer2[6]>= 0)
				{
				
					disp_buffer2[10] = ' ';			// to blink specific digit 1
				}
				else
				 {
					disp_buffer2[11] = ' ';
					
				 }	
			}
			else if((KBPage.enter_count==13)&&(b.blink))
			{
				if (disp_buffer2[6]>= 0)
				disp_buffer2[11] = ' ';
				else
				 {
					disp_buffer2[12] = ' ';
					
				 }
			}
			else if((KBPage.enter_count==14)&&(b.blink))
			{
				if (disp_buffer2[6]>= 0)
				disp_buffer2[12] = ' ';
				else
				 {
					disp_buffer2[13] = ' ';
					
				 }
			}							
			break;
		}
		case 2:
		{
			if(!b.blink)
			{
				sprintf((char*)&disp_buffer1[0],"UTC TIME: %c%02d:%02d", EPROM_General.Cust_Detail.Timezone_sign?'-':'+', EPROM_General.Cust_Detail.Timezone_hours, EPROM_General.Cust_Detail.Timezone_minutes);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-24 Hr");
			}
			else if ((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"UTC TIME:  %02d:%02d",EPROM_General.Cust_Detail.Timezone_hours, EPROM_General.Cust_Detail.Timezone_minutes);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-24 Hr");
			}
			else if ((KBPage.enter_count==4)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"UTC TIME: %c  :%02d",EPROM_General.Cust_Detail.Timezone_sign?'-':'+', EPROM_General.Cust_Detail.Timezone_minutes);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-24 Hr");
			}
			else if ((KBPage.enter_count==5)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"UTC TIME: %c%02d:  ", EPROM_General.Cust_Detail.Timezone_sign?'-':'+', EPROM_General.Cust_Detail.Timezone_hours);
				sprintf((char*)&disp_buffer2[0],"Lim: 1-24 Hr");
			}
			break;
		}
		case 3:
		{
			if((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"Sunrise Sunset ");
				sprintf((char*)&disp_buffer2[0],"Offset: ");
			}
			else if (!b.blink)
			{
				sprintf((char*)&disp_buffer1[0],"Sunrise Sunset ");
				sprintf((char*)&disp_buffer2[0],"Offset: %d",  EPROM_General.Cust_Detail.Offset_Value);
			}
			break;
		}
		case 4:
		{
			if((KBPage.enter_count==3)&&(b.blink))
			{
				sprintf((char*)&disp_buffer1[0],"No of CKT:  ");
			}
			else if (!b.blink)
			{
				sprintf((char*)&disp_buffer1[0],"No of CKT: %d", EPROM_General.NoofCkt);	
			}	
			sprintf((char*)&disp_buffer2[0],"Lim: 1-2");			
			break;
		}
		case 5:
		{
			if(!b.blink)
			{		
				sprintf((char*)&disp_buffer1[0], "TIME = %02d:%02d:%02d ",update_time.mHour,update_time.minute,update_time.mSecond);			 
				sprintf((char*)&disp_buffer2[0], "DATE = %02d/%02d/%02d ",update_time.mDate,update_time.month,update_time.myear);
			}			
			else if ((KBPage.enter_count==3)&&(b.blink))
			{
				disp_buffer1[7] = ' ';
				disp_buffer1[8] = ' ';
			}			
			else if ((KBPage.enter_count==4)&&(b.blink))
			{
				disp_buffer1[10] = ' ';
				disp_buffer1[11] = ' ';
			}
			else if ((KBPage.enter_count==5)&&(b.blink))
			{
				disp_buffer1[13] = ' ';
				disp_buffer1[14] = ' ';
			}
			else if ((KBPage.enter_count==6)&&(b.blink))
			{
				disp_buffer2[7] = ' ';
				disp_buffer2[8] = ' ';
			}
			else if ((KBPage.enter_count==7)&&(b.blink))
			{
				disp_buffer2[10] = ' ';
				disp_buffer2[11] = ' ';
			}
			else if ((KBPage.enter_count==8)&&(b.blink))
			{
				disp_buffer2[13] = ' ';
				disp_buffer2[14] = ' ';
			}		
			break;
		}
	}
	DispAt(0,(char*)disp_buffer1);
	L1=strlen(&disp_buffer1[0]);

	for(i=L1;i<16;i++)
	{
		DispAt(i+1,(char*)" ");
	}

	if(Pcbplc_Display == 0)
	{
		DispAt(17,(char*)disp_buffer2);
		L1=strlen(&disp_buffer2[0]);
		for(i=L1;i<16;i++)
		{
			DispAt(i+17,(char*)" ");
		}
	}
}

void DisplayRTU(void)
{
    static char Displaycnt = 1;    
	char Disp_Mode[6][3] = {"M","P","S","A","a","C",};
    
	if(b.DO1 == 1)
	{
		sprintf((char*)&disp_buffer1[0],"%s:ON ",&DOname[0][0]);
	}
	else
	{
		sprintf((char*)&disp_buffer1[0],"%s:OFF ",&DOname[0][0]);
	}
	// if(b.DO2 == 1)
	// {
	// 	sprintf((char*)&disp_buffer2[0],"%s:ON ",&DOname[1][0]);
	// }
	// else
	// {
	// 	sprintf((char*)&disp_buffer2[0],"%s:OF ",&DOname[1][0]);
	// }
	// if(b.DO3 == 1)
	// {
	// 	sprintf((char*)&disp_buffer3[0],"%s:ON ",&DOname[2][0]);
	// }
	// else
	// {
	// 	sprintf((char*)&disp_buffer3[0],"%s:OF ",&DOname[2][0]);
	// }

	// strcat(disp_buffer2,disp_buffer3);
	// strcat(disp_buffer1,disp_buffer2);

    if(EPROM_General.DoModeDetails.Do_Mode >= 6)
    {
        EPROM_General.DoModeDetails.Do_Mode = 0;
    }
    strcat(disp_buffer1,&Disp_Mode[EPROM_General.DoModeDetails.Do_Mode][0]);
    switch(Displaycnt)
    {
        case 1:
            sprintf(disp_buffer2,"%s", DI_Final_value[0] ? "CKT1 OFF        " : "CKT1 ON         ");
            ++Displaycnt;
			++Displaycnt;
            break;
        case 2:
            sprintf(disp_buffer2,"%s", DI_Final_value[1] ? "PHOTOCELL ON    " : "");
            ++Displaycnt;
            break;
        case 3:
            sprintf(disp_buffer2,"%s", DI_Final_value[2] ? "CKT2 OFF        " : "CKT2 ON         ");
            ++Displaycnt;
            break;
        case 4:
            sprintf(disp_buffer2,"%s", DI_Final_value[3] ? "LOCAL MODE      " : "REMOTE MODE     ");
            ++Displaycnt;
            break;
        case 5:
            sprintf(disp_buffer2,"%s", DI_Final_value[4] ? "DOOR OPEN       " : "DOOR CLOSE      ");
            ++Displaycnt;
			++Displaycnt;
            break;
        case 6:
            sprintf(disp_buffer2,"%s", DI_Final_value[5] ?  "ELR TRIP        " : "");
            ++Displaycnt;
            break;
        case 7:
            sprintf(disp_buffer2,"%s", DI_Final_value[15] ? "POWER NORMAL    " : "POWER FAIL      ");
            ++Displaycnt;
            break;
        case 8:
        {
            // sprintf(disp_buffer2,"%s", DI_Final_value[12] ? "OverCurrent Trip" : "");
			sprintf(disp_buffer2, "%s", DI_Final_value[22] ? (DI_Final_value[24] ? (DI_Final_value[26] ? "OverCurrent Trip" : "") : "") : "");
            ++Displaycnt;
            break;
        }
        case 9:
        {
            if(RUN_Timer[0]!=0) 
            {
                sprintf(disp_buffer2,"ON_DELAY %d ",RUN_Timer[0]);
            }
            else if(DI_Final_value[8]!=0)   
            {
                sprintf(disp_buffer2,"FEEDBACK Error  ");
            }
            ++Displaycnt;
            break;
        }
        default:
        {
            Displaycnt = 1;
            break;
        }
    }
	ql_rtos_task_sleep_ms(500);
}