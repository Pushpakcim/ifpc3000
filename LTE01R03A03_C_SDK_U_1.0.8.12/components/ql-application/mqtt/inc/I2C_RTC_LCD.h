/**  @file
  I2C_demo.h

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

#ifndef _I2C_DEMO_H
#define _I2C_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * include files
 ===========================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "ql_i2c.h"
/*===========================================================================
 * Macro Definition
 ===========================================================================*/

typedef int    QlI2CStatus;
typedef void * ql_task_t;

#define BUFFERSIZE    512
#define PROG_TIMEOUT  30

#define ENABLE 1
#define DISABLE 0
#define NUM_MOBILE_NUMBERS 4
#define DIGITS_PER_NUMBER 14
/*===========================================================================
 * Struct
 ===========================================================================*/
typedef struct datetime
{
	/*    test
	char mDate;
	char month;
	int  myear;
	char mHour;
	char minute;
	char mSecond;
*/
	uint8_t mDate;     // Day of the month (1-31)
    uint8_t month;     // Month (1-12)
    uint16_t myear;    // Year (e.g., 2024)
    uint8_t mHour;     // Hour (0-23)
    uint8_t minute;    // Minutes (0-59)
    uint8_t mSecond;   // Seconds (0-59)
}DataTime_t;
/*===========================================================================
 * Enum
 ===========================================================================*/
struct Decimal 
{
	unsigned char value[5];
	unsigned char status		:1;
	unsigned char status_temp	:1;
	unsigned char Decimal_page;
	unsigned int Decimal;
	unsigned char enter_count_Decimal;
	unsigned char CheckDecimalValue;
	unsigned char CheckDecimalEnable;
	unsigned char h_shift;
};
extern struct Decimal Deci ;

struct SaveHistory
{
    unsigned char Hourptr;		// Future use
    unsigned int TotalDays;		// Future use
    unsigned char Runptr;
    unsigned int TotalMinuteckt1;		// Runhour 
    unsigned int TotalMinuteckt2;
};
extern struct SaveHistory EPROM4HISTORY;
/*===========================================================================
 * Variate
 ===========================================================================*/
extern DataTime_t rtc_time,update_time;
extern uint8_t d[4];
extern uint8_t rtc_intialized;
extern uint8_t lcd_intialized;
extern uint8_t keypad_initialized;

extern unsigned char 	shour,sminute,ssecond;
extern unsigned char sdate,smonth,syear;
extern char OldSecomdRec,OldclearSecomdRec;
extern unsigned char dispStr[BUFFERSIZE];
extern char disp_buffer1[200], disp_buffer2[200];
extern unsigned char CommaStr[22][25], CommPosition, ColonPositionFound;
extern char Setup_ID[5], Setup_rd[5], Setup_cd[5], Setup_log[5], MAXSMS_ID[5];
extern char Alarm_MO[NUM_MOBILE_NUMBERS][DIGITS_PER_NUMBER];
extern char GPRSPort[5];
extern int GPRSLiveIPbufVar[5];
extern unsigned char GPRSLiveIPbuf[20];
extern uint8_t Meter_reset, Pcbplc_Display;
extern unsigned long prog_page_count;
extern uint8_t modbusminute;
extern unsigned char DOname[3][5];
/*===========================================================================
 * Functions
 ===========================================================================*/

void ql_i2c_RTC_LCD_init(void);

ql_errcode_i2c_e lcd_write_command(uint8_t command);
ql_errcode_i2c_e lcd_write_data(uint8_t data) ;
ql_errcode_i2c_e lcd_clear(void);
ql_errcode_i2c_e lcd_display_string(const char *string);
ql_errcode_i2c_e lcd_initialize(void);
ql_errcode_i2c_e lcd_set_cursor(uint8_t row, uint8_t  col);
ql_errcode_i2c_e lcd_float_print(float value, int precision);
void DispAt(unsigned char loc, const char *tstring);
void Disp_main_menu(void);
void Disp_prog_menu(void);
void Case1EnterCout(void);
void Case1pro_page(void);
void Case2setpoint_page(void);
void Case3schedule_page(void);
void Case4alarm_page(void);
void Case5timer_page(void);
void Case6Energy_page(void);
void Case7version_page(void);
void Case8do_page(void);
void Case9mode_page(void);
//unsigned char* CaseDecimalEnter(void);
void Case10maxsms_page(void);
void Case11gprs_page(void);
void DisplayRTU(void);
void Case12streetlight_page(void);

#ifdef __cplusplus
    } /*"C" */
#endif
    
#endif /* _I2C_DEMO_H */
