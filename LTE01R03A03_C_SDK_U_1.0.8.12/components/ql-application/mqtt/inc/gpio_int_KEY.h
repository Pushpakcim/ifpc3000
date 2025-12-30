/*=================================================================

						EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN			  WHO		  WHAT, WHERE, WHY
------------	 -------	 -------------------------------------------------------------------------------

=================================================================*/


#ifndef _GPIOINTDEMO_H
#define _GPIOINTDEMO_H

#include "ql_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define MAX_SCH_LG 		100
/*===========================================================================
 * Enum
 ===========================================================================*/
typedef enum
{
    INT_CB01,
    INT_CB02,
    INT_CB03,
    INT_CB04
} ql_IntSel;

struct PAGE_KB
{
	unsigned char main_page;
	unsigned char main_page_limit;

	unsigned char prog_page;
	unsigned char prog_page_limit;
	
    unsigned char enter_count;
	unsigned char setup_page;
    unsigned char setup_page_limit;

    unsigned char schedual_page;
	unsigned char schedual_page_limit;

	unsigned char setpoint_page;
	unsigned char setpoint_page_limit;

	unsigned char do_page;
    unsigned char do_page_limit;

    unsigned char mode_page;
    unsigned char mode_page_limit;

	unsigned char am_page;
	unsigned char am_page_limit;
	
    unsigned char scail_page;
	unsigned char scail_page_limit;

	unsigned char alarm_page;
	unsigned char alarm_page_limit;

	unsigned char timer_page;
	unsigned char timer_page_limit;

	unsigned char pumpondelay;
	unsigned char energy_page;
	unsigned char energy_page_limit;
	
	unsigned char version_page;
	unsigned char version_page_limit;

	unsigned char cal_page;
	unsigned char cal_page_limit;

	unsigned char cal_page_prog;
	unsigned char cal_page_prog_limit;
	
	unsigned int RecVar_page;
	unsigned int RecVar_page_limit;

	unsigned int MaxSMS_page;
	unsigned int MaxSMS_page_limit;	

	unsigned char gprs_page;
	unsigned char gprs_page_limit;

	unsigned char StreetLight_page;
	unsigned char StreetLight_page_limit;

	unsigned char DSschedual_page;
	unsigned char DSschedual_page_limit;

	unsigned char dst_page;
	unsigned char dst_page_limit;

	unsigned char Modbus_page;
	unsigned char Modbus_page_limit;
};
extern struct PAGE_KB KBPage;

struct bits 
{
    unsigned char keybit		:1;

    unsigned char DO3			:1;
    unsigned char DO2			:1;
    unsigned char DO1			:1;
    unsigned char DO3_temp		:1;
    unsigned char DO2_temp		:1;
    unsigned char DO1_temp		:1;

    unsigned char progset		:1;
    unsigned char enter			:1;
    unsigned char blink			:1;
    unsigned char installation 	:1;
    unsigned char datatype		:1;
    unsigned char energy_reset	:1;
    unsigned char eeprom_reset	:1;
    unsigned char calib			:1;
    unsigned char reset			:1;
    unsigned char storee		:1;
    unsigned char setlog		:1;
    unsigned char log_data		:1;
    unsigned char string_detect	:1;
    unsigned char to201m		:1;
    unsigned char to202m		:1;

    unsigned char sc1			:1;
    unsigned char sc2			:1;
    unsigned char sc3			:1;
    unsigned char sc4			:1;
    unsigned char sc5			:1;
    unsigned char sc6			:1;
    unsigned char sc7			:1;
    unsigned char sc8			:1;
    unsigned char sc9			:1;
    unsigned char sc10			:1;
    unsigned char sc11			:1;
    unsigned char sc12			:1;

    unsigned char demand		:1;
    unsigned char demandon		:1;
    unsigned char demandreset	:1;
    unsigned char Flash_reset	:1;
    unsigned char pcbplc		:1;
};
extern struct bits b ;


extern uint8_t FlagswitchInt1;
extern uint8_t FlagswitchInt2;
extern uint8_t FlagswitchInt3;
extern uint8_t FlagswitchInt4;

extern uint8_t ENTER, UP, DOWN, PROG, PROGSET, Fact_def;
extern uint8_t KEYPAD_DATA_IN, keybyte;
extern int L_port;
extern float PFValueHigh, PFValueLow;

/*===========================================================================
 * Functions declaration
 ===========================================================================*/
void ql_gpioint_app_key_init(void);
void initialParameter(void);
void check_key(void);
void keyboard(void);
void KeyPROGONLY(void);
void KeyPROG(void);
void KeyDOWN(void);
void KeyUP(void);
void KeyENTER(void);


#ifdef __cplusplus
} /*"C" */
#endif

#endif /* _GPIOINTDEMO_H */


