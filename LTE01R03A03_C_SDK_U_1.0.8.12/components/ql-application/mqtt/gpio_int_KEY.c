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
#include <string.h>
#include <stdlib.h>

#include "ql_api_osi.h"
#include "ql_log.h"
#include "ql_pin_cfg.h"

#include "gpio_DIDO.h"
#include "gpio_int_KEY.h"
#include "I2C_RTC_LCD.h"
#include "configuration.h"
#include "dlms_meter.h"
#include "cfw.h"	// lat long  
/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define QL_GPIOINTDEMO_LOG_LEVEL             QL_LOG_LEVEL_INFO
#define QL_GPIOINTDEMO_LOG(msg, ...)         QL_LOG(QL_GPIOINTDEMO_LOG_LEVEL, "ql_GPIOINTDEMO", msg, ##__VA_ARGS__)
#define QL_GPIOINTDEMO_LOG_PUSH(msg, ...)    QL_LOG_PUSH("ql_GPIOINTDEMO", msg, ##__VA_ARGS__)


// #define KEYPAD_DATA_IN     ((FIO2PIN)  & 0x3c)
#define DEBOUNCE  10
/*===========================================================================
 * Variate
 ===========================================================================*/
ql_IntSel ql_intsel_flg = INT_CB01;
uint8_t FlagswitchInt1;
uint8_t FlagswitchInt2;
uint8_t FlagswitchInt3;
uint8_t FlagswitchInt4;
uint8_t ENTER, UP, DOWN, PROG, PROGSET, Fact_def;
uint8_t KEYPAD_DATA_IN, keybyte;
// unsigned int debounce_cnt;
unsigned char Disp_refresh, Menu_refresh, Key_refresh;
bool keybit;
struct bits b;
struct PAGE_KB KBPage;
unsigned char disp_buffer[70];
unsigned char dispDecimal[70];
int L_port;
float PFValueHigh, PFValueLow;
uint8_t Longitude[20];
uint8_t Latitude[20];

/*===========================================================================
 * Functions
 ===========================================================================*/
static void _gpioint_callback01(void *ctx)
{
    ql_LvlMode  gpio_lvl;

    ql_gpio_get_level(GPIO_28, &gpio_lvl);
    QL_GPIOINTDEMO_LOG("gpio[%d] int lvl:[%d]", GPIO_0, gpio_lvl);

    QL_GPIOINTDEMO_LOG("Welcome to Quectel!");
    FlagswitchInt1 = 1;
    KEYPAD_DATA_IN = 0x34;
	prog_page_count = 0;
}

static void _gpioint_callback02(void *ctx)
{
    ql_LvlMode  gpio_lvl;

    ql_gpio_get_level(GPIO_27, &gpio_lvl);
    QL_GPIOINTDEMO_LOG("gpio[%d] int lvl:[%d]", GPIO_1, gpio_lvl);

    QL_GPIOINTDEMO_LOG("Quectel is No.1!");
    FlagswitchInt2 = 1;
    KEYPAD_DATA_IN = 0x38;
	prog_page_count = 0;
}

static void _gpioint_callback03(void *ctx)
{
    ql_LvlMode  gpio_lvl;

    ql_gpio_get_level(GPIO_26, &gpio_lvl);
    QL_GPIOINTDEMO_LOG("gpio[%d] int lvl:[%d]", GPIO_0, gpio_lvl);

    QL_GPIOINTDEMO_LOG("Welcome to Quectel_1!");
    FlagswitchInt3 = 1;
    KEYPAD_DATA_IN = 0x2C;
	prog_page_count = 0;
}

static void _gpioint_callback04(void *ctx)
{
    ql_LvlMode  gpio_lvl;

    ql_gpio_get_level(GPIO_25, &gpio_lvl);
    QL_GPIOINTDEMO_LOG("gpio[%d] int lvl:[%d]", GPIO_1, gpio_lvl);

    QL_GPIOINTDEMO_LOG("Quectel is No.1__1!");
    FlagswitchInt4 = 1;
    KEYPAD_DATA_IN = 0x1C;
	prog_page_count = 0;
}

void initialParameter(void)
{
    UP = 0x34;
	ENTER = 0x1C;
	DOWN = 0x38;
	PROG = 0x2C;
	PROGSET = 0x0C;
	Fact_def = 0x28;

	b.pcbplc = 1;
	// ps.status=0;
	// scroll_sec=1;
	b.setlog = 0;

	b.progset = 0;
	b.blink = 0;
	b.enter = 0;
	// rec_count=0;
	// debounce_cnt = DEBOUNCE;

	KBPage.enter_count = 1;

	KBPage.main_page = 1;
	KBPage.main_page_limit = 13; //37;

	KBPage.prog_page = 1;
	KBPage.prog_page_limit = 13;

	KBPage.setup_page = 1;
    KBPage.setup_page_limit = 4;

	KBPage.setpoint_page = 1;
	KBPage.setpoint_page_limit = 10;

	KBPage.schedual_page = 1;
	KBPage.schedual_page_limit = 4;

	KBPage.alarm_page = 1;
	KBPage.alarm_page_limit=4;

	KBPage.timer_page = 1;
	KBPage.timer_page_limit = 10;

    KBPage.energy_page = 1;
	KBPage.energy_page_limit = 2;

	KBPage.version_page = 1;
    KBPage.version_page_limit = 4;

	KBPage.do_page = 1;
    KBPage.do_page_limit = 3;

	KBPage.mode_page = 1;
	KBPage.mode_page_limit = 1;

	KBPage.MaxSMS_page = 1;
	KBPage.MaxSMS_page_limit = 65;

	KBPage.gprs_page = 1;
	KBPage.gprs_page_limit = 3; 

	KBPage.StreetLight_page = 1;
	KBPage.StreetLight_page_limit = 5;

	KBPage.DSschedual_page = 1;
	KBPage.DSschedual_page_limit = 28;
}

static void ql_gpioint_demo_thread(void *param)
{
    QL_GPIOINTDEMO_LOG("gpio int demo thread enter, param 0x%x", param);

    ql_event_t event;
	ql_rtos_task_sleep_s(5);

    ql_pin_set_func(QL_TEST1_PIN_GPIO28, QL_TEST1_PIN_GPIO28_FUNC_GPIO);      // set GPIO28
    ql_pin_set_func(QL_TEST1_PIN_GPIO29, QL_TEST1_PIN_GPIO29_FUNC_GPIO);      // set GPIO29
    ql_pin_set_func(QL_TEST1_PIN_GPIO30, QL_TEST1_PIN_GPIO30_FUNC_GPIO);      // set GPIO30
    ql_pin_set_func(QL_TEST1_PIN_GPIO31, QL_TEST1_PIN_GPIO31_FUNC_GPIO);      // set GPIO31

    ql_int_register(GPIO_28, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_RISING, PULL_NONE, _gpioint_callback01, NULL);
    //ql_int_register(GPIO_27, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_FALLING, PULL_UP, _gpioint_callback02, NULL);
    ql_int_register(GPIO_27, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_RISING, PULL_NONE, _gpioint_callback02, NULL);
    ql_int_register(GPIO_26, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_RISING, PULL_NONE, _gpioint_callback03, NULL);
    ql_int_register(GPIO_25, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_RISING, PULL_NONE, _gpioint_callback04, NULL);

    ql_int_enable(GPIO_28);
    ql_int_enable(GPIO_27);
    ql_int_enable(GPIO_26);
    ql_int_enable(GPIO_25);

    initialParameter();

    while(1)
    {
        ql_event_wait(&event, 1);

        if( ql_intsel_flg == INT_CB01 )
        {
            if( ql_int_disable(GPIO_28) == 0 )
            {
                QL_GPIOINTDEMO_LOG("gpio[%d] int disable, enable gpio[%d] int", GPIO_0, GPIO_1);
                ql_int_enable(GPIO_28);
                //ql_int_register(GPIO_30, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_FALLING, PULL_UP, _gpioint_callback02, NULL);
                ql_intsel_flg = INT_CB02;
            }
        }
        else if( ql_intsel_flg == INT_CB02 )
        {
            if( ql_int_disable(GPIO_27) == 0 )
            {
                QL_GPIOINTDEMO_LOG("gpio[%d] int disable, enable gpio[%d] int", GPIO_1, GPIO_0);
                ql_int_enable(GPIO_27);
                //ql_int_register(GPIO_29, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_RISING, PULL_DOWN, _gpioint_callback01, NULL);
                ql_intsel_flg = INT_CB03;
            }
        }
        else if( ql_intsel_flg == INT_CB03 )
        {
            if( ql_int_disable(GPIO_26) == 0 )
            {
                QL_GPIOINTDEMO_LOG("gpio[%d] int disable, enable gpio[%d] int", GPIO_1, GPIO_0);
                ql_int_enable(GPIO_26);
                //ql_int_register(GPIO_29, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_RISING, PULL_DOWN, _gpioint_callback01, NULL);
                ql_intsel_flg = INT_CB04;
            }
        }
        else/* ql_intsel_flg == INT_CB04 */
        {
            if( ql_int_disable(GPIO_25) == 0 )
            {
                QL_GPIOINTDEMO_LOG("gpio[%d] int disable, enable gpio[%d] int", GPIO_1, GPIO_0);
                ql_int_enable(GPIO_25);
                //ql_int_register(GPIO_29, EDGE_TRIGGER, DEBOUNCE_EN, EDGE_RISING, PULL_DOWN, _gpioint_callback01, NULL);
                ql_intsel_flg = INT_CB01;
            }
        }
        ql_rtos_task_sleep_s(20);
    }

    ql_rtos_task_delete(NULL);
}

void ql_gpioint_app_key_init(void)
{
    QlOSStatus err = QL_OSI_SUCCESS;
    ql_task_t gpioint_task = NULL;

    err = ql_rtos_task_create(&gpioint_task, 3*1024, APP_PRIORITY_NORMAL, "ql_gpiointdemo", ql_gpioint_demo_thread, NULL, 1);
    if( err != QL_OSI_SUCCESS )
    {
        QL_GPIOINTDEMO_LOG("gpio int demo task created failed");
    }
}

//key check routine...called in while loop
void check_key(void) // KEY-CHECK ROUTINE.. 
{
	// KEYPAD_ALL_DIR_IN

	keybyte = KEYPAD_DATA_IN;
	keybyte &= 0x3c;

	QL_GPIOINTDEMO_LOG("keybyte:%X",keybyte);

	if((keybyte==0x0000003c))
	{
		QL_GPIOINTDEMO_LOG("if keybit:%d",keybit);
  		if(!keybit)
		{
	// 		debounce_cnt--;
	// 		if(!debounce_cnt)
	// 		{
	// 			debounce_cnt=DEBOUNCE;
				keybit=1;
	// 		}
		}
	}
	else if((keybyte==PROGSET)||(keybyte==UP)||(keybyte==DOWN)||(keybyte==ENTER)||(keybyte==PROG))
	{
		//Key_refresh=1;
		QL_GPIOINTDEMO_LOG("else if keybit:%d",keybit);
		// if(keybit)
		{
			
			//debounce_cnt--;
			//if(!debounce_cnt)
			{
			//	debounce_cnt=DEBOUNCE;
				keybit=0;
				keyboard();
			}
	    }
		KEYPAD_DATA_IN = 0x3c;
    }
    else
    {
		QL_GPIOINTDEMO_LOG("else keybit:%d",keybit);
        // debounce_cnt=DEBOUNCE;
		KEYPAD_DATA_IN = 0x3c;
    }
}

/** keypress action routine */
void keyboard(void)
{
	if(keybyte == PROGSET)
	{
		QL_GPIOINTDEMO_LOG("PROGSET_KEY");
		// KeyPROGSET();
		prog_page_count = 0;
        // DispAt(1, "PROGSET_KEY ");
	}
	else if(keybyte == DOWN)
	{
		QL_GPIOINTDEMO_LOG("DOWN_KEY");
		KeyDOWN();
		prog_page_count = 0;
        // DispAt(1, "DOWN_KEY ");
	}
	else if(keybyte == UP)
	{
		QL_GPIOINTDEMO_LOG("UP_KEY");
		KeyUP();
		prog_page_count = 0;
        // DispAt(1, "UP_KEY ");		
	}
	else if(keybyte == ENTER)
	{
		QL_GPIOINTDEMO_LOG("ENTER_KEY");
		KeyENTER();
		prog_page_count = 0;
        // DispAt(1, "ENTER_KEY ");
	}
	else if(keybyte == PROG)
	{
		QL_GPIOINTDEMO_LOG("PROG_KEY");
		if((!b.progset) && (KBPage.enter_count < 2))
		{
			 KeyPROGONLY();
		}
		else
		{
			KeyPROG();
		}
		prog_page_count = 0;
        // DispAt(1, "PROG_KEY ");
	}
}

void KeyPROGONLY(void)
{
	if(KBPage.enter_count < 2)
	{
		lcd_clear();
		// DispAt(1," POGRAMME KEY ");
		b.progset ^=1;
	}
    // ql_rtos_task_sleep_ms(100);

	if((b.progset)&&(KBPage.enter_count < 2))
	{
		KBPage.prog_page = 1;
		QL_GPIOINTDEMO_LOG("PROGRAMMING MENU");
		// DOOR_OPCL = 1;
	}
}
void KeyPROG(void)
{
	// DispAt(1, "PROG_KEY ");
	if((b.progset) && (KBPage.enter_count > 1) && (KBPage.enter_count < 3))
	{
        if((KBPage.enter_count == 1) || (KBPage.enter_count == 2))
        {
            KBPage.setup_page=1;
			KBPage.setpoint_page=1;
            KBPage.schedual_page=1;
            KBPage.alarm_page=1;
            KBPage.timer_page=1;
            KBPage.energy_page=1;
            KBPage.version_page=1;
			KBPage.do_page=1;
			KBPage.mode_page=1;
			KBPage.MaxSMS_page=1;
            KBPage.gprs_page=1;
			KBPage.StreetLight_page=1;
			KBPage.DSschedual_page=1;
        }
		KBPage.enter_count--;
    }
}

void KeyDOWN(void)
{
    if((!b.calib)&&(b.progset)&&(KBPage.enter_count == 1))
    {
        KBPage.prog_page--;
    }

	if((!b.calib)&&(KBPage.prog_page == 0)&&(b.progset))
    {
        KBPage.prog_page = KBPage.prog_page_limit;
    }

	if((!b.calib)&&(!b.progset))
	{
		KBPage.main_page--;
		lcd_clear();
	}

    if((!b.calib)&&(KBPage.main_page == 0)&&(!b.progset))
    {
        KBPage.main_page = KBPage.main_page_limit;
    }

	if((KBPage.enter_count == 2) &&(b.progset))
	{
		if(KBPage.prog_page == 1)
		{
			KBPage.setup_page--;
			if(KBPage.setup_page == 0)
			{
				KBPage.setup_page = KBPage.setup_page_limit;
			}
		}
		else if(KBPage.prog_page == 2)
		{
			KBPage.setpoint_page--;
			if(KBPage.setpoint_page == 0)
			{
				KBPage.setpoint_page = KBPage.setpoint_page_limit;
			}
		}
		else if(KBPage.prog_page == 3)
		{
			KBPage.schedual_page--;
			if(KBPage.schedual_page == 0)
			{
				KBPage.schedual_page = KBPage.schedual_page_limit;
			}
		}
		else if(KBPage.prog_page == 4)
		{
			KBPage.alarm_page--;
			if(KBPage.alarm_page == 0)
			{
				KBPage.alarm_page = KBPage.alarm_page_limit;
			}
		}
		else if(KBPage.prog_page == 5)
		{
			KBPage.timer_page--;
			if(KBPage.timer_page == 0)
			{
				KBPage.timer_page = KBPage.timer_page_limit;
			}
		}
		else if(KBPage.prog_page == 6)
		{
			KBPage.energy_page--;
			if(KBPage.energy_page == 0)
			{
				KBPage.energy_page = KBPage.energy_page_limit;
			}
		}
		else if(KBPage.prog_page == 7)
		{
			KBPage.version_page--;
			if(KBPage.version_page == 0)
			{
				KBPage.version_page = KBPage.version_page_limit;
			}
		}
		else if(KBPage.prog_page == 8)
		{
			KBPage.do_page--;
			if(KBPage.do_page == 0)
			{
				KBPage.do_page = KBPage.do_page_limit;
			}
		}
		else if(KBPage.prog_page == 9)
		{
			KBPage.mode_page--;
			if(KBPage.mode_page == 0)
			{
				KBPage.mode_page = KBPage.mode_page_limit;
			}
		}
		else if(KBPage.prog_page == 10)
		{
			KBPage.MaxSMS_page--;
			if(KBPage.MaxSMS_page == 0)
			{
				KBPage.MaxSMS_page = EPROM_General.MaxofSMS+1;
			}
		}
		else if(KBPage.prog_page == 11)
		{
			KBPage.gprs_page--;
			if(KBPage.gprs_page == 0)
			{
				KBPage.gprs_page = KBPage.gprs_page_limit;
			}
		}
		else if(KBPage.prog_page == 12)
		{
			KBPage.StreetLight_page--;
			if(KBPage.StreetLight_page == 0)
			{
				KBPage.StreetLight_page = KBPage.StreetLight_page_limit;
			}
		}
		else if(KBPage.prog_page == 13)
		{
			KBPage.DSschedual_page--;
			if(KBPage.DSschedual_page == 0)
			{
				KBPage.DSschedual_page = KBPage.DSschedual_page_limit;
			}
		}
	}

	if((b.progset) && (KBPage.prog_page==1))
	{
		if((KBPage.setup_page==1))
		{
			// QL_GPIOINTDEMO_LOG("Setup_ID[%d]=%d",KBPage.enter_count-3,Setup_ID[KBPage.enter_count-3]);
			if(KBPage.enter_count==3 ||  KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				Setup_ID[KBPage.enter_count-3]--;
				if(Setup_ID[KBPage.enter_count-3] > 9)
				{
					Setup_ID[KBPage.enter_count-3] = 9;
				}
				QL_GPIOINTDEMO_LOG("Setup_ID[%d]=%d",KBPage.enter_count-3,Setup_ID[KBPage.enter_count-3]);
			}
			if(KBPage.enter_count==7 ||  KBPage.enter_count==8 || KBPage.enter_count==9 || KBPage.enter_count==10)
			{
				Setup_cd[KBPage.enter_count-7]--;
				if(Setup_cd[KBPage.enter_count-7] > 9)
				{
					Setup_cd[KBPage.enter_count-7] = 9;
				}
				QL_GPIOINTDEMO_LOG("Setup_cd[%d]=%d",KBPage.enter_count-7,Setup_cd[KBPage.enter_count-7]);
			}
			if(KBPage.enter_count==11 ||  KBPage.enter_count==12 || KBPage.enter_count==13 || KBPage.enter_count==14)
			{
				Setup_rd[KBPage.enter_count-11]--;
				if(Setup_rd[KBPage.enter_count-11] > 9)
				{
					Setup_rd[KBPage.enter_count-11] = 9;
				}
				QL_GPIOINTDEMO_LOG("Setup_rd[%d]=%d",KBPage.enter_count-11,Setup_rd[KBPage.enter_count-11]);
			}
		}
		else if((KBPage.setup_page == 2))
		{
			if(KBPage.enter_count==3 || KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				Setup_log[KBPage.enter_count-3]--;
				if(Setup_log[KBPage.enter_count-3] > 9)
				{
					Setup_log[KBPage.enter_count-3] = 9;
				}
			}
		}
		else if((KBPage.setup_page == 3) && (KBPage.enter_count == 3))
		{
			EPROM_General.AI_DI_DO_Detail.Total_Do--;
			if(EPROM_General.AI_DI_DO_Detail.Total_Do > MAX_DO_CHANNEL)
			{
				EPROM_General.AI_DI_DO_Detail.Total_Do = MAX_DO_CHANNEL;
			}
		}
		else if((KBPage.setup_page == 4) && (KBPage.enter_count == 3))
		{
			EPROM_General.AI_DI_DO_Detail.Total_Di--;
			if(EPROM_General.AI_DI_DO_Detail.Total_Di > MAX_DI_CHANNEL)
			{
				EPROM_General.AI_DI_DO_Detail.Total_Di = MAX_DI_CHANNEL;
			}
		}
		else if((KBPage.setup_page == 5) && (KBPage.enter_count == 3))
		{
			EPROM_General.Mo_Comm.Mo_Com_Int--;
			if(EPROM_General.Mo_Comm.Mo_Com_Int > 3)
			{
				EPROM_General.Mo_Comm.Mo_Com_Int = 2;
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==2) && KBPage.setpoint_page >=1)
	{
		if((KBPage.enter_count==3) && ((KBPage.setpoint_page>=1) && (KBPage.setpoint_page<=3)))		// voltage high
		{
			EPROM_General.sp.hi_value[KBPage.setpoint_page-1] -=5;
			if(EPROM_General.sp.hi_value[KBPage.setpoint_page-1] < 0)
			{
				EPROM_General.sp.hi_value[KBPage.setpoint_page-1] = 600;
			}
		}
		else if((KBPage.enter_count==4) && ((KBPage.setpoint_page>=1) && (KBPage.setpoint_page<=3)))  // Voltage low 
		{
			EPROM_General.sp.lo_value[KBPage.setpoint_page-1] -=5;
			if(EPROM_General.sp.lo_value[KBPage.setpoint_page-1] < 0)
			{
				EPROM_General.sp.lo_value[KBPage.setpoint_page-1] = 600;
			}
		}
		else if((KBPage.enter_count==3) && (KBPage.setpoint_page==7))			// PF HIGH
		{
			 EPROM_General.sp.hi_value[KBPage.setpoint_page-1]-=0.01;		
			if( EPROM_General.sp.hi_value[KBPage.setpoint_page-1] < 0 )
			{
				 EPROM_General.sp.hi_value[KBPage.setpoint_page-1]= 1.0;
			}
		}
		else if((KBPage.enter_count==4) && (KBPage.setpoint_page==7))		// PF LOW
		{
			EPROM_General.sp.lo_value[KBPage.setpoint_page-1]-=0.01;
			if(EPROM_General.sp.lo_value[KBPage.setpoint_page-1] < 0.25)
			{
				EPROM_General.sp.lo_value[KBPage.setpoint_page-1]= 1;
			}
		}
		else if((KBPage.enter_count==3) && ((KBPage.setpoint_page>=4) && (KBPage.setpoint_page<=10)))    	//current High & bulb
		{
			EPROM_General.sp.hi_value[KBPage.setpoint_page-1]--;
			if(EPROM_General.sp.hi_value[KBPage.setpoint_page-1] < 0)
			{
				EPROM_General.sp.hi_value[KBPage.setpoint_page-1] = 99;
			}
		}
		else if((KBPage.enter_count==4) && ((KBPage.setpoint_page>=4) && (KBPage.setpoint_page<=10)))		//current Low & LED
		{
			EPROM_General.sp.lo_value[KBPage.setpoint_page-1]--;
			if(EPROM_General.sp.lo_value[KBPage.setpoint_page-1] < 0)
			{
				EPROM_General.sp.lo_value[KBPage.setpoint_page-1] = 99;
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==3) && KBPage.schedual_page >=1)
	{
		if((b.progset)&&(KBPage.enter_count==3)&&(KBPage.schedual_page>=1))
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di ^= 1; //~EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di;
		}
	
		if(KBPage.enter_count==4)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH--;
			if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH < 0 )//|| EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH > 23)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH = 23;
			}
		}
		else if(KBPage.enter_count==5)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min--;
			if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min < 0 )//|| EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min > 59)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min = 59;
			}
		}
		else if(KBPage.enter_count==6)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH--;
			if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH < 0)// || EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH > 23)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH = 23;
			}
		}
		else if(KBPage.enter_count==7)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min--;
			if(EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min < 0)// || EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min > 59)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min = 59;
			}
		}
	}
	
	else if((b.progset)&&(KBPage.prog_page==4) && KBPage.alarm_page >=1)
	{
		if(KBPage.enter_count>=3)
		{
			int mobile_index = KBPage.alarm_page-1;
			int digit_index = KBPage.enter_count-3;
			if (mobile_index < NUM_MOBILE_NUMBERS)  // Ensure we don't exceed the number of mobile numbers
			{
				if(KBPage.enter_count == 3)
				{
					// Set the first digit or a special marker like '+' if needed
					Alarm_MO[mobile_index][digit_index] = '+';  // Starting symbol or first entry handling
				}
				else
				{
					// Decrement the current digit if necessary
					Alarm_MO[mobile_index][digit_index]--;
					QL_GPIOINTDEMO_LOG("%c,%d,%d",Alarm_MO[mobile_index][digit_index], mobile_index, digit_index);
					// Ensure the digit stays within the range of '0' to '9'
					if (Alarm_MO[mobile_index][digit_index] < '0')
					{
						Alarm_MO[mobile_index][digit_index] = '9';  // Wrap-around to '9' if it goes below '0'
					}
				}
			}
        }
	}
	else if((b.progset)&&(KBPage.prog_page==5) && KBPage.timer_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			EPROM_General.Def_timer[KBPage.timer_page-1]--;           // decrement by 20
			if(EPROM_General.Def_timer[KBPage.timer_page-1] == 0 || EPROM_General.Def_timer[KBPage.timer_page-1] > 2024)
			{
				EPROM_General.Def_timer[KBPage.timer_page-1] = 2024;
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==6) && KBPage.energy_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			if(KBPage.energy_page==1)
			{
				if(EPROM_General.MeterMake == METER_HPL_DLMS)
				{
					EPROM_General.MeterMake = METER_HPL_NORMAL;
				}
				else
				{
					EPROM_General.MeterMake = METER_HPL_DLMS;
				}
			}
			else if(KBPage.energy_page==2)
			{
				if(EPROM_General.MeterType == THREEPHASE)
				{
					EPROM_General.MeterType = SINGLEPHASE;
				}
				else if(EPROM_General.MeterType == SINGLEPHASE)
				{
					EPROM_General.MeterType = THREEPHASE;
				}
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==7) && KBPage.version_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			//GPS_LAT_gFinalAnaValF=
			//EPROM_General.Cust_Detail.Lattitude = LAT->valuedouble;
		}
	}
	else if((b.progset)&&(KBPage.prog_page==8) && KBPage.do_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			if(KBPage.do_page==1)
			{
				if(b.DO1_temp==1)
				{
					b.DO1_temp=0;
				}
				else if(b.DO1_temp==0)
				{
					b.DO1_temp=1;
				}
			}
			else if(KBPage.do_page==2)
			{
				if(b.DO2_temp==1)
				{
					b.DO2_temp=0;
				}
				else if(b.DO2_temp==0)
				{
					b.DO2_temp=1;
				}
			}
			else if(KBPage.do_page==3)
			{
				if(b.DO3_temp==1)
				{
					b.DO3_temp=0;
				}
				else if(b.DO3_temp==0)
				{
					b.DO3_temp=1;
				}
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==9) && KBPage.mode_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			EPROM_General.DoModeDetails.Do_Mode_temp--;
			if((EPROM_General.DoModeDetails.Do_Mode_temp < RTU_DO_MODE_MANUAL) || (EPROM_General.DoModeDetails.Do_Mode_temp > RTU_DO_MODE_ASTROTIME_GEO))
			{
				EPROM_General.DoModeDetails.Do_Mode_temp = RTU_DO_MODE_ASTROTIME_GEO;
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==10) && KBPage.MaxSMS_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			if(KBPage.MaxSMS_page==1)
			{
				EPROM_General.MaxofSMS--;
				if(EPROM_General.MaxofSMS  < 0 || EPROM_General.MaxofSMS > 64)
				{
					EPROM_General.MaxofSMS = 64;
				}
			}
		}
		if(KBPage.MaxSMS_page>=2 && KBPage.MaxSMS_page <= EPROM_General.MaxofSMS+1)
		{
			if(KBPage.enter_count==3 || KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				Deci.value[KBPage.enter_count-3]--;
				if(Deci.value[KBPage.enter_count-3] > 9 || Deci.value[KBPage.enter_count-3] < 0)
				{
					Deci.value[KBPage.enter_count-3] = 9;
				}
				QL_GPIOINTDEMO_LOG("Deci.value[%d]=%d",KBPage.enter_count-3,Deci.value[KBPage.enter_count-3]);
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==11) && KBPage.gprs_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			if(KBPage.gprs_page == 1)
			{
				if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"airtelgprs.com")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"www");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"www")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"internet");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"internet")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"JioNet");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"JioNet")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"IPV4V6");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"IPV4V6")==0)			
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"bsnlnet");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"bsnlnet")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"airtelgprs.com");
				}
				else
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"www");
				}
			}
		}
		if(KBPage.gprs_page == 2)
		{
			// L_port--;
			// sprintf(dispStr,"%d",L_port);
			// strcpy(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port, dispStr);
			// EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port--;
			if(KBPage.enter_count==3 ||  KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				GPRSPort[KBPage.enter_count-3]--;
				if(GPRSPort[KBPage.enter_count-3] > 9)
				{
					GPRSPort[KBPage.enter_count-3] = 9;
				}
				QL_GPIOINTDEMO_LOG("GPRSPort[%d]=%d",KBPage.enter_count-3,GPRSPort[KBPage.enter_count-3]);
			}
		}
		if(KBPage.gprs_page == 3)
		{
			if(GPRSLiveIPbuf[KBPage.enter_count-3]!='.')
			{
				if(GPRSLiveIPbuf[KBPage.enter_count-3]=='0')
				{
					GPRSLiveIPbuf[KBPage.enter_count-3]='9';
				}
				else
				{
					GPRSLiveIPbuf[KBPage.enter_count-3]--;
				}
			}
			// if(GPRSLiveIPbufVar[KBPage.enter_count-3]!='.')
			// {
			// 	if(GPRSLiveIPbufVar[KBPage.enter_count-3]<'0' && GPRSLiveIPbufVar[KBPage.enter_count-3]>'9')
			// 	{
			// 		GPRSLiveIPbufVar[KBPage.enter_count-3]='9';
			// 	}
			// 	else
			// 	{
			// 		GPRSLiveIPbufVar[KBPage.enter_count-3]--;
			// 	}
			// }
			QL_GPIOINTDEMO_LOG("GPRSLiveIPbuf[%d]=%d",KBPage.enter_count-3, GPRSLiveIPbuf[KBPage.enter_count-3]);
		}
	}
	else if((b.progset)&&(KBPage.prog_page==12) && KBPage.StreetLight_page >=1)
	{
		if(KBPage.enter_count == 3)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude -= 10;
				if(EPROM_General.Cust_Detail.Lattitude < -90)
				{
					EPROM_General.Cust_Detail.Lattitude = 90;
				}				
			}			
			else if(KBPage.StreetLight_page == 2)
			{
				EPROM_General.Cust_Detail.Timezone_sign--;
				if (EPROM_General.Cust_Detail.Timezone_sign < 0)
				{
					EPROM_General.Cust_Detail.Timezone_sign = 1;
				}
			}
			else if(KBPage.StreetLight_page == 3)
			{
				EPROM_General.Cust_Detail.Offset_Value--;
				if(EPROM_General.Cust_Detail.Offset_Value < -60)
				{
					EPROM_General.Cust_Detail.Offset_Value = 60;
				}
			}
			else if(KBPage.StreetLight_page == 4)
			{
				EPROM_General.NoofCkt--;
				if(EPROM_General.NoofCkt < 1)				
				{
					EPROM_General.NoofCkt = 2;
				}
			}
			else if(KBPage.StreetLight_page == 5)    // hour 
			{
				
				if(update_time.mHour == 0)
				{
					update_time.mHour = 23;
				}
				else
				{
					update_time.mHour--;
				}
			}
		}
		else if(KBPage.enter_count == 4)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude --;
				if(EPROM_General.Cust_Detail.Lattitude < -90)
				{
					EPROM_General.Cust_Detail.Lattitude = 90;
				}
			}
			else if(KBPage.StreetLight_page == 2)
			{
				EPROM_General.Cust_Detail.Timezone_hours--;
				if (EPROM_General.Cust_Detail.Timezone_hours < 0)
				{
					EPROM_General.Cust_Detail.Timezone_hours = 23;
				}
			}	
			else if(KBPage.StreetLight_page == 5)    // min 
			{
				
				if(update_time.minute == 0)
				{
					update_time.minute = 59;
				}
				else
				{
					update_time.minute--;
				}
			}		
		}

		else if(KBPage.enter_count == 5)   
		{
			if((KBPage.StreetLight_page == 1) && (EPROM_General.Cust_Detail.Lattitude !='.'))
			{
				EPROM_General.Cust_Detail.Lattitude -= 0.1;
				if(EPROM_General.Cust_Detail.Lattitude < -90)
				{
					EPROM_General.Cust_Detail.Lattitude = 90;
				}
			}
			else if((KBPage.StreetLight_page == 1) && (EPROM_General.Cust_Detail.Lattitude ='.'))
			{
				EPROM_General.Cust_Detail.Lattitude -= 0.1;
				if(EPROM_General.Cust_Detail.Lattitude < -90)
				{
					EPROM_General.Cust_Detail.Lattitude = 90;
				}
			}

			else if(KBPage.StreetLight_page == 2)
			{
				EPROM_General.Cust_Detail.Timezone_minutes--;
				if (EPROM_General.Cust_Detail.Timezone_minutes < 0)
				{
					EPROM_General.Cust_Detail.Timezone_minutes = 59;
				}
			}			
			else if(KBPage.StreetLight_page == 5)    //sec 
			{
				
				if(update_time.mSecond ==0)
				{
					update_time.mSecond = 59;
				}
				else
				{
					update_time.mSecond--;
				}
			}		
				
		}
	
		else if(KBPage.enter_count == 6)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude -= 0.01;
				if(EPROM_General.Cust_Detail.Lattitude < -90)
				{
					EPROM_General.Cust_Detail.Lattitude = 90;
				}
			}
			else if(KBPage.StreetLight_page == 5)    //date
			{
				update_time.mDate--;
				if(update_time.mDate < 1)
				{
					update_time.mDate = 31;
				}
			}
		}
		else if(KBPage.enter_count == 7)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude -= 0.001;
				if(EPROM_General.Cust_Detail.Lattitude < -90)
				{
					EPROM_General.Cust_Detail.Lattitude = 90;
				}
			}
			else if(KBPage.StreetLight_page == 5)   // Month
			{
				update_time.month--;
				if(update_time.month < 1)
				{
					update_time.month = 12;
				}
			}
		}
		else if(KBPage.enter_count == 8)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude -= 0.0001;
				if(EPROM_General.Cust_Detail.Lattitude < -90)
				{
					EPROM_General.Cust_Detail.Lattitude = 90;
				}
			}
			else if(KBPage.StreetLight_page == 5)     //year       
			{
				update_time.myear--;
				if(update_time.myear < 1)
				{
					update_time.myear = 99;
				}
			}
		}
#if 1
		else if(KBPage.enter_count == 9)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude -= 10;
				if(EPROM_General.Cust_Detail.Longitude < -180)
				{
					EPROM_General.Cust_Detail.Longitude = 180;
				}
			}
		}
		
		else if(KBPage.enter_count == 10)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude --;
				if(EPROM_General.Cust_Detail.Longitude < -180)
				{
					EPROM_General.Cust_Detail.Longitude = 180;
				}
			}
		}
		else if(KBPage.enter_count == 11)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude -= 0.1;
				if(EPROM_General.Cust_Detail.Longitude < -180)
				{
					EPROM_General.Cust_Detail.Longitude = 180;
				}
			}
		}
		else if(KBPage.enter_count == 12)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude -= 0.01;
				if(EPROM_General.Cust_Detail.Longitude < -180)
				{
					EPROM_General.Cust_Detail.Longitude = 180;
				}
			}
		}
		else if(KBPage.enter_count == 13)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude -= 0.001;
				if(EPROM_General.Cust_Detail.Longitude < -180)
				{
					EPROM_General.Cust_Detail.Longitude = 180;
				}
			}
		}
		else if(KBPage.enter_count == 14)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude -= 0.0001;
				if(EPROM_General.Cust_Detail.Longitude < -180)
				{
					EPROM_General.Cust_Detail.Longitude = 180;
				}
			}
		}

#endif
	}
}


void KeyUP(void)
{
	if((!b.progset))		
	{
		lcd_clear();
		KBPage.main_page++;
		if(KBPage.main_page > KBPage.main_page_limit)
		{
        	KBPage.main_page = 1;
    	}
	}

	if((b.progset)&&(KBPage.enter_count == 1))
    {
        KBPage.prog_page++;
		if(KBPage.prog_page > KBPage.prog_page_limit)
		{
			KBPage.prog_page = 1;
		}
    }
	

	if((KBPage.enter_count == 2) && (b.progset))
	{
		if(KBPage.prog_page==1)
		{
			KBPage.setup_page++;
			if(KBPage.setup_page > KBPage.setup_page_limit)
			{
				KBPage.setup_page = 1;
			}
		}
		else if(KBPage.prog_page==2)
		{
			KBPage.setpoint_page++;
			if(KBPage.setpoint_page > KBPage.setpoint_page_limit)
			{
				KBPage.setpoint_page = 1;
			}
		}
	
		else if(KBPage.prog_page==3)		//Schedule
	
		{
			KBPage.schedual_page++;
			if(KBPage.schedual_page > KBPage.schedual_page_limit)
			{
				KBPage.schedual_page = 1;
			}
		}
	
		else if(KBPage.prog_page==4)
		{
			KBPage.alarm_page++;
			if(KBPage.alarm_page > KBPage.alarm_page_limit)
			{
				KBPage.alarm_page = 1;
			}
		}
		else if(KBPage.prog_page==5)
		{
			KBPage.timer_page++;
			if(KBPage.timer_page > KBPage.timer_page_limit)
			{
				KBPage.timer_page = 1;
			}
		}
		else if(KBPage.prog_page==6)
		{
			KBPage.energy_page++;
			if(KBPage.energy_page > KBPage.energy_page_limit)
			{
				KBPage.energy_page = 1;
			}
		}
		else if(KBPage.prog_page==7)
		{
			KBPage.version_page++;
			if(KBPage.version_page > KBPage.version_page_limit)
			{
				KBPage.version_page = 1;
			}
		}
		else if(KBPage.prog_page==8)
		{
			KBPage.do_page++;
			if(KBPage.do_page > KBPage.do_page_limit)
			{
				KBPage.do_page = 1;
			}
		}
		else if(KBPage.prog_page==9)
		{
			KBPage.mode_page++;
			if(KBPage.mode_page > KBPage.mode_page_limit)
			{
				KBPage.mode_page = 1;
			}
		}
		else if(KBPage.prog_page==10)
		{
			KBPage.MaxSMS_page++;
			if(KBPage.MaxSMS_page > EPROM_General.MaxofSMS)
			{
				KBPage.MaxSMS_page = 1;
			}
		}
		else if(KBPage.prog_page==11)
		{
			KBPage.gprs_page++;
			if(KBPage.gprs_page > KBPage.gprs_page_limit)
			{
				KBPage.gprs_page = 1;
			}
		}
		else if(KBPage.prog_page==12)
		{
			KBPage.StreetLight_page++;
			if(KBPage.StreetLight_page > KBPage.StreetLight_page_limit)
			{
				KBPage.StreetLight_page = 1;
			}
		}
		else if(KBPage.prog_page==13)
		{
			KBPage.DSschedual_page++;
			if(KBPage.DSschedual_page > KBPage.DSschedual_page_limit)
			{
				KBPage.DSschedual_page = 1;
			}
		}
	}

	if((b.progset)&&(KBPage.prog_page==1))
	{
		if((KBPage.setup_page==1))
		{
			if(KBPage.enter_count==3 ||  KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				Setup_ID[KBPage.enter_count-3]++;
				if(Setup_ID[KBPage.enter_count-3] > 9)
				{
					Setup_ID[KBPage.enter_count-3] = 0;
				}
			}
			if(KBPage.enter_count==7 ||  KBPage.enter_count==8 || KBPage.enter_count==9 || KBPage.enter_count==10)
			{
				Setup_cd[KBPage.enter_count-7]++;
				if(Setup_cd[KBPage.enter_count-7] > 9)
				{
					Setup_cd[KBPage.enter_count-7] = 0;
				}
			}
			if(KBPage.enter_count==11 ||  KBPage.enter_count==12 || KBPage.enter_count==13 || KBPage.enter_count==14)
			{
				Setup_rd[KBPage.enter_count-11]++;
				if(Setup_rd[KBPage.enter_count-11] > 9)
				{
					Setup_rd[KBPage.enter_count-11]=0;
				}
			}
		}
		else if((KBPage.setup_page == 2))
		{
			if(KBPage.enter_count==3 ||  KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				Setup_log[KBPage.enter_count-3]++;
				if(Setup_log[KBPage.enter_count-3] > 9)
				{
					Setup_log[KBPage.enter_count-3] = 0;
				}
			}
		}
		else if((KBPage.setup_page == 3) && (KBPage.enter_count == 3))
		{
			EPROM_General.AI_DI_DO_Detail.Total_Do++;
			if(EPROM_General.AI_DI_DO_Detail.Total_Do > MAX_DO_CHANNEL)
			{
				EPROM_General.AI_DI_DO_Detail.Total_Do = 1;
			}
		}
		else if((KBPage.setup_page == 4) && (KBPage.enter_count == 3))
		{
			EPROM_General.AI_DI_DO_Detail.Total_Di++;
			if(EPROM_General.AI_DI_DO_Detail.Total_Di > MAX_DI_CHANNEL)
			{
				EPROM_General.AI_DI_DO_Detail.Total_Di = 1;
			}
		}
		else if((KBPage.setup_page == 5) && (KBPage.enter_count == 3))
		{
			EPROM_General.Mo_Comm.Mo_Com_Int++;
			if(EPROM_General.Mo_Comm.Mo_Com_Int > 2)
			{
				EPROM_General.Mo_Comm.Mo_Com_Int = 0;
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==2) && KBPage.setpoint_page >=1)
	{
		if((KBPage.enter_count==3) && ((KBPage.setpoint_page>=1) && (KBPage.setpoint_page<=3)))		// voltage High 
		{
			EPROM_General.sp.hi_value[KBPage.setpoint_page-1] +=5;
			if(EPROM_General.sp.hi_value[KBPage.setpoint_page-1] > 600)
			{
				EPROM_General.sp.hi_value[KBPage.setpoint_page-1] = 0;
			}
		}
		else if((KBPage.enter_count==4) && ((KBPage.setpoint_page>=1) && (KBPage.setpoint_page<=3)))		//Voltage Low 
		{
			EPROM_General.sp.lo_value[KBPage.setpoint_page-1] +=5;
			if(EPROM_General.sp.lo_value[KBPage.setpoint_page-1] > 600)
			{
				EPROM_General.sp.lo_value[KBPage.setpoint_page-1] = 0;
			}
		}
		else if((KBPage.enter_count==3) && (KBPage.setpoint_page==7))  		//power factor HIGH
		{
			EPROM_General.sp.hi_value[KBPage.setpoint_page-1] +=0.01;
			if(EPROM_General.sp.hi_value[KBPage.setpoint_page-1] > 1.0)
			{
				EPROM_General.sp.hi_value[KBPage.setpoint_page-1]= 0;
			}
		}
		else if((KBPage.enter_count==4) && (KBPage.setpoint_page==7))		//PF LOW
		{
			EPROM_General.sp.lo_value[KBPage.setpoint_page-1] +=0.01;
			if(EPROM_General.sp.lo_value[KBPage.setpoint_page-1] > 1.0)
			{
				EPROM_General.sp.lo_value[KBPage.setpoint_page-1]= 0;
			}
		}
		else if((KBPage.enter_count==3) && ((KBPage.setpoint_page>=4) && (KBPage.setpoint_page<=10))) 		//current high  & bulb
		{
			EPROM_General.sp.hi_value[KBPage.setpoint_page-1]++;
			if(EPROM_General.sp.hi_value[KBPage.setpoint_page-1] > 99)
			{
				EPROM_General.sp.hi_value[KBPage.setpoint_page-1] = 0;
			}
		}
		else if((KBPage.enter_count==4) && ((KBPage.setpoint_page>=4) && (KBPage.setpoint_page<=10)))		//current Low & led
		{
			EPROM_General.sp.lo_value[KBPage.setpoint_page-1]++;
			if(EPROM_General.sp.lo_value[KBPage.setpoint_page-1] > 99)
			{
				EPROM_General.sp.lo_value[KBPage.setpoint_page-1] = 0;
			}
		}
		
	}
	else if((b.progset)&&(KBPage.prog_page==3) && KBPage.schedual_page >=1)
	{
		if((b.progset)&&(KBPage.enter_count==3)&&(KBPage.schedual_page>=1))
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di ^=1; // ~EPROM_Schedule.Schedule[KBPage.schedual_page-1].Sch_En_Di;
		}
	
		if(KBPage.enter_count==4)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH++;
			if(/*EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH < 0 ||*/ EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH > 23)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_HH = 0;
			}
		}
		else if(KBPage.enter_count==5)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min++;
			if(/*EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min < 0 ||*/ EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min > 59)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Start_Min = 0;
			}
		}
		else if(KBPage.enter_count==6)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH++;
			if(/*EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH < 0 ||*/ EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH > 23)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_HH = 0;
			}
		}
		else if(KBPage.enter_count==7)
		{
			EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min++;
			if(/*EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min < 0 ||*/ EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min > 59)
			{
				EPROM_Schedule.Schedule[KBPage.schedual_page-1].Stop_Min = 0;
			}
		}
	}
		
	else if((b.progset)&&(KBPage.prog_page==4) && KBPage.alarm_page >=1)
	{
		if(KBPage.enter_count>=3)
		{
			int mobile_index = KBPage.alarm_page-1;
			int digit_index = KBPage.enter_count-3;
			if ((mobile_index) < (NUM_MOBILE_NUMBERS))  // Ensure we don't exceed the number of mobile numbers
			{
				if(KBPage.enter_count == 3)
				{
					// Set the first digit or a special marker like '+' if needed
					Alarm_MO[mobile_index][digit_index] = '+';  // Starting symbol or first entry handling
				}
				else
				{
					// Increment the current digit if necessary
					Alarm_MO[mobile_index][digit_index]++;
					QL_GPIOINTDEMO_LOG("%c,%d,%d",Alarm_MO[mobile_index][digit_index], mobile_index, digit_index);
					// Ensure the digit stays within the range of '0' to '9'
					if (Alarm_MO[mobile_index][digit_index] > '9')
					{
						Alarm_MO[mobile_index][digit_index] = '0';  // Wrap-around to '9' if it goes below '0'
					}
				}
			}
        }
    }
	else if((b.progset)&&(KBPage.prog_page==5) && KBPage.timer_page >=1)
	{
		if((b.progset)&&(KBPage.enter_count==3)&&(KBPage.timer_page>=1))
		{
			EPROM_General.Def_timer[KBPage.timer_page-1]++;             // timer +20
			if(EPROM_General.Def_timer[KBPage.timer_page-1] < 0 || EPROM_General.Def_timer[KBPage.timer_page-1] > 2024)
			{
				EPROM_General.Def_timer[KBPage.timer_page-1] = 1;
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==6) && KBPage.energy_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			if(KBPage.energy_page==1)
			{
				if(EPROM_General.MeterMake == METER_HPL_DLMS)
				{
					EPROM_General.MeterMake = METER_HPL_NORMAL;
				}
				else
				{
					EPROM_General.MeterMake = METER_HPL_DLMS;
				}
			}
			else if(KBPage.energy_page==2)
			{
				if(EPROM_General.MeterType == THREEPHASE)
				{
					EPROM_General.MeterType = SINGLEPHASE;
				}
				else if(EPROM_General.MeterType == SINGLEPHASE)
				{
					EPROM_General.MeterType = THREEPHASE;
				}
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==7) && KBPage.version_page >=1)
	{

	}
	else if((b.progset)&&(KBPage.prog_page==8) && KBPage.do_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			if(KBPage.do_page==1)
			{
				if(b.DO1_temp==1)
				{
					b.DO1_temp=0;
				}
				else if(b.DO1_temp==0)
				{
					b.DO1_temp=1;
				}
			}
			else if(KBPage.do_page==2)
			{
				if(b.DO2_temp==1)
				{
					b.DO2_temp=0;
				}
				else if(b.DO2_temp==0)
				{
					b.DO2_temp=1;
				}
			}
			else if(KBPage.do_page==3)
			{
				if(b.DO3_temp==1)
				{
					b.DO3_temp=0;
				}
				else if(b.DO3_temp==0)
				{
					b.DO3_temp=1;
				}
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==9) && KBPage.mode_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			EPROM_General.DoModeDetails.Do_Mode_temp++;
			if((EPROM_General.DoModeDetails.Do_Mode_temp < RTU_DO_MODE_MANUAL) || (EPROM_General.DoModeDetails.Do_Mode_temp > RTU_DO_MODE_ASTROTIME_GEO))
			{
				EPROM_General.DoModeDetails.Do_Mode_temp = RTU_DO_MODE_MANUAL;
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==10) && KBPage.MaxSMS_page >=1)
	{
		if(KBPage.enter_count==3)
		{
			if(KBPage.MaxSMS_page==1)
			{
				EPROM_General.MaxofSMS++;
				if(EPROM_General.MaxofSMS  < 0 || EPROM_General.MaxofSMS > 64)
				{
					EPROM_General.MaxofSMS = 0;
				}
			}
		}
		if(KBPage.MaxSMS_page>=2 && KBPage.MaxSMS_page <= EPROM_General.MaxofSMS+1)
		{
			if(KBPage.enter_count==3 ||  KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				Deci.value[KBPage.enter_count-3]++;
				if(Deci.value[KBPage.enter_count-3] > 9 || Deci.value[KBPage.enter_count-3] < 0)
				{
					Deci.value[KBPage.enter_count-3] = 0;
				}
			}
		}
	}
	else if((b.progset)&&(KBPage.prog_page==11) && KBPage.gprs_page >=1)
	{
		if(KBPage.enter_count == 3)
		{
			if(KBPage.gprs_page == 1)
			{
				if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"airtelgprs.com")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"www");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"www")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"internet");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"internet")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"JioNet");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"JioNet")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"IPV4V6");
				}
				else if(strcmp(EPROM_General.Mo_Comm.Mo_APN,"IPV4V6")==0)
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"airtelgprs.com");
				}
				else
				{
					strcpy(EPROM_General.Mo_Comm.Mo_APN,"www");
				}			
			}
		}
		if(KBPage.gprs_page == 2)
		{
			// L_port = L_port + 15;
			// sprintf(dispStr,"%d",L_port);
			// strcpy(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port,dispStr);		
			// EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port++;	
			if(KBPage.enter_count==3 ||  KBPage.enter_count==4 || KBPage.enter_count==5 || KBPage.enter_count==6)
			{
				GPRSPort[KBPage.enter_count-3]++;
				if(GPRSPort[KBPage.enter_count-3] > 9)
				{
					GPRSPort[KBPage.enter_count-3] = 9;
				}
				QL_GPIOINTDEMO_LOG("GPRSPort[%d]=%d",KBPage.enter_count-3,GPRSPort[KBPage.enter_count-3]);
			}		
		}
		else if(KBPage.gprs_page == 3)
		{
			if(GPRSLiveIPbuf[KBPage.enter_count-3]!='.')
			{
				GPRSLiveIPbuf[KBPage.enter_count-3]++;
				if(GPRSLiveIPbuf[KBPage.enter_count-3]>'9')
				{
					GPRSLiveIPbuf[KBPage.enter_count-3]='0';
				}
			}
			// if(GPRSLiveIPbufVar[KBPage.enter_count-3]!='.')
			// {
			// 	if(GPRSLiveIPbufVar[KBPage.enter_count-3]<'0' && GPRSLiveIPbufVar[KBPage.enter_count-3]>'9')
			// 	{
			// 		GPRSLiveIPbufVar[KBPage.enter_count-3]='0';
			// 	}
			// 	else
			// 	{
			// 		GPRSLiveIPbufVar[KBPage.enter_count-3]++;
			// 	}
			// }
		}
	}
	else if((b.progset)&&(KBPage.prog_page==12) && KBPage.StreetLight_page >=1)

	{
		if(KBPage.enter_count == 3)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude += 10;
				if(EPROM_General.Cust_Detail.Lattitude > 90)
				{
					EPROM_General.Cust_Detail.Lattitude = -90;
				}

			}
			else if(KBPage.StreetLight_page == 2)
			{
				EPROM_General.Cust_Detail.Timezone_sign++;
				if (EPROM_General.Cust_Detail.Timezone_sign > 1)
				{
					EPROM_General.Cust_Detail.Timezone_sign = 0;
				}
			}
			else if(KBPage.StreetLight_page == 3)
			{
				EPROM_General.Cust_Detail.Offset_Value++;
				if(EPROM_General.Cust_Detail.Offset_Value > 60)
				{
					EPROM_General.Cust_Detail.Offset_Value = -60;
				}
			}
			else if(KBPage.StreetLight_page == 4)
			{
				EPROM_General.NoofCkt++;
				if(EPROM_General.NoofCkt > 2)				
				{
					EPROM_General.NoofCkt = 1;
				}
			}
			else if(KBPage.StreetLight_page == 5)    // hour 
			{
				update_time.mHour++;
				if(update_time.mHour > 23)
				{
					update_time.mHour = 0;
				}
			}
		}
		else if(KBPage.enter_count == 4)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude++;
				if(EPROM_General.Cust_Detail.Lattitude > 90)
				{
					EPROM_General.Cust_Detail.Lattitude = -90;
				}
			}
			else if(KBPage.StreetLight_page == 2)
			{
				EPROM_General.Cust_Detail.Timezone_hours++;
				if (EPROM_General.Cust_Detail.Timezone_hours > 23)
				{
					EPROM_General.Cust_Detail.Timezone_hours = 0;
				}
			}	
			else if(KBPage.StreetLight_page == 5)    // min 
			{
				update_time.minute++;
				if(update_time.minute > 59)
				{
					update_time.minute = 0;
				}
			}	
		}

		else if(KBPage.enter_count == 5)   
		{
			if((KBPage.StreetLight_page == 1) && (EPROM_General.Cust_Detail.Lattitude !='.'))
			{
				EPROM_General.Cust_Detail.Lattitude += 0.1;
				if(EPROM_General.Cust_Detail.Lattitude > 90)
				{
					EPROM_General.Cust_Detail.Lattitude = -90;
				}
			}
			else if((KBPage.StreetLight_page == 1) && (EPROM_General.Cust_Detail.Lattitude ='.'))
			{
				EPROM_General.Cust_Detail.Lattitude += 0.1;
				if(EPROM_General.Cust_Detail.Lattitude > 90)
				{
					EPROM_General.Cust_Detail.Lattitude = -90;
				}
			}

			else if(KBPage.StreetLight_page == 2)
			{
				EPROM_General.Cust_Detail.Timezone_minutes++;
				if (EPROM_General.Cust_Detail.Timezone_minutes > 59)
				{
					EPROM_General.Cust_Detail.Timezone_minutes = 0;
				}
			}	
			else if(KBPage.StreetLight_page == 5)    //sec 
			{
				update_time.mSecond++;
				if(update_time.mSecond > 59)
				{
					update_time.mSecond = 0;
				}
			}		
				
		}
	
		else if(KBPage.enter_count == 6)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude += 0.01;
				if(EPROM_General.Cust_Detail.Lattitude > 90)
				{
					EPROM_General.Cust_Detail.Lattitude = -90;
				}
			}
			
			else if(KBPage.StreetLight_page == 5)    //date
			{
				update_time.mDate++;
				if(update_time.mDate > 31)
				{
					update_time.mDate = 1;
				}
			}
		}
		else if(KBPage.enter_count == 7)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude += 0.001;
				if(EPROM_General.Cust_Detail.Lattitude > 90)
				{
					EPROM_General.Cust_Detail.Lattitude = -90;
				}
			}
			else if(KBPage.StreetLight_page == 5)   // Month
			{
				update_time.month++;
				if(update_time.month > 12)
				{
					update_time.month = 1;
				}
			}
		}
		else if(KBPage.enter_count == 8)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Lattitude += 0.0001;
				if(EPROM_General.Cust_Detail.Lattitude > 90)
				{
					EPROM_General.Cust_Detail.Lattitude = -90;
				}
			}
			else if(KBPage.StreetLight_page == 5)     //year       
			{
				update_time.myear++;
				if(update_time.myear > 99)
				{
					update_time.myear = 1;
				}
			}
		}

		else if(KBPage.enter_count == 9)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude += 10;
				if(EPROM_General.Cust_Detail.Longitude > 180)
				{
					EPROM_General.Cust_Detail.Longitude = -180;
				}
			}
		}
		
		else if(KBPage.enter_count == 10)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude ++;
				if(EPROM_General.Cust_Detail.Longitude > 180)
				{
					EPROM_General.Cust_Detail.Longitude = -180;
				}
			}
		}
		else if(KBPage.enter_count == 11)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude += 0.1;
				if(EPROM_General.Cust_Detail.Longitude > 180)
				{
					EPROM_General.Cust_Detail.Longitude = -180;
				}
			}
		}
		else if(KBPage.enter_count == 12)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude += 0.01;
				if(EPROM_General.Cust_Detail.Longitude > 180)
				{
					EPROM_General.Cust_Detail.Longitude = -180;
				}
			}
		}
		else if(KBPage.enter_count == 13)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude += 0.001;
				if(EPROM_General.Cust_Detail.Longitude > 180)
				{
					EPROM_General.Cust_Detail.Longitude = -180;
				}
			}
		}
		else if(KBPage.enter_count == 14)
		{
			if(KBPage.StreetLight_page == 1)
			{
				EPROM_General.Cust_Detail.Longitude += 0.0001;
				if(EPROM_General.Cust_Detail.Longitude > 180)
				{
					EPROM_General.Cust_Detail.Longitude = -180;
				}
			}
		}
	}
}

void KeyENTER(void)
{
	if(b.progset)
	{
		KBPage.enter_count++;
	}

	if(KBPage.prog_page == 1)		//SETUP
	{
		if(KBPage.setup_page == 1)
		{
			if((KBPage.enter_count == 7))
			{
				sprintf((char*)&dispDecimal[0],"%d%d%d%d",Setup_ID[0],Setup_ID[1],Setup_ID[2],Setup_ID[3]);
				EPROM_General.Rtu_Detail.RTUId = atoi((char*)dispDecimal);
			}
			if((KBPage.enter_count == 11))
			{
				sprintf((char*)&dispDecimal[0],"%d%d%d%d",Setup_cd[0],Setup_cd[1],Setup_cd[2],Setup_cd[3]);
				EPROM_General.Cust_Detail.Client_Id = atoi((char*)dispDecimal);	
			}
			if((KBPage.enter_count == 15))
			{
				KBPage.enter_count = 2;
				sprintf((char*)&dispDecimal[0],"%d%d%d%d",Setup_rd[0],Setup_rd[1],Setup_rd[2],Setup_rd[3]);
				EPROM_General.Cust_Detail.Reader_Id = atoi((char*)dispDecimal);
				syncExtFlashVariableWithPCBPLCVariable();
				flag_flashUpdateEPROM_General = 1;
            	flag_flashUpdateEPROM_General_WaitCounter=5;
			}
		}
		else if(KBPage.setup_page == 2)
		{
			if(KBPage.enter_count == 7)
			{
				KBPage.enter_count = 2;
				sprintf((char*)&dispDecimal[0],"%d%d%d%d", Setup_log[0], Setup_log[1], Setup_log[2], Setup_log[3]);
				EPROM_General.LogRate = atoi((char*)dispDecimal);
				syncExtFlashVariableWithPCBPLCVariable();
				flag_flashUpdateEPROM_General = 1;
            	flag_flashUpdateEPROM_General_WaitCounter=5;
			}
		}
		else if(KBPage.setup_page == 3)
		{
			if(KBPage.enter_count > 3)
			{
				KBPage.enter_count = 2;
				flag_flashUpdateEPROM_General = 1;
            	flag_flashUpdateEPROM_General_WaitCounter=5;
			}
		}
	}
	else if(KBPage.prog_page == 2)		//SETpoint
	{
		if(KBPage.enter_count >= 5)
		{
			KBPage.enter_count = 2;
			flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter=5;
		}
	}
	else if(KBPage.prog_page == 3)		//Schedule
	{
		if(KBPage.enter_count >= 8)
		{
			KBPage.enter_count = 2;
			flag_flashUpdateEPROM_Schedule = 1;
			flag_flashUpdateEPROM_Schedule_WaitCounter = 5;
		}
	}
	else if(KBPage.prog_page == 4)		//Alarm
	{
		if(KBPage.enter_count > 15)
		{
			KBPage.enter_count = 2;
			strcpy((char*)EPROM_General.Mo_Comm.mobMCS[KBPage.alarm_page-1],(char*)Alarm_MO[KBPage.alarm_page-1]);
			syncExtFlashVariableWithPCBPLCVariable();
			flag_flashUpdateEPROM_General = 1;
			flag_flashUpdateEPROM_General_WaitCounter=5;
		}
	}
	else if(KBPage.prog_page == 5)		//Timer
	{
		if(KBPage.enter_count > 3)
		{
			KBPage.enter_count = 2;
			flag_flashUpdateEPROM_General = 1;
			flag_flashUpdateEPROM_General_WaitCounter=5;
		}
	}
	else if(KBPage.prog_page == 6)		//EnergyCONFIG
	{
		QL_GPIOINTDEMO_LOG("%d",KBPage.enter_count);
		if(KBPage.enter_count > 3)
		{
			KBPage.enter_count = 2;
			if(SINGLEPHASE == EPROM_General.MeterType)
        	{
				if(METER_HPL_DLMS == EPROM_General.MeterMake)
				{				
					EPROM_General.ModSMSList[0] = 903; // Voltage
					EPROM_General.ModSMSList[1] = 0;   // Spare
					EPROM_General.ModSMSList[2] = 0;   // Spare
					EPROM_General.ModSMSList[3] = 919; // KWH
					EPROM_General.ModSMSList[4] = 901; // Current
					EPROM_General.ModSMSList[5] = 1221; // No of CKT
					EPROM_General.ModSMSList[6] = 0;   // Spare
					EPROM_General.ModSMSList[7] = 0;   // Spare
					EPROM_General.ModSMSList[8] = 1237; // Mode(618)
					EPROM_General.ModSMSList[9] = 915; // Active Power-KW
					EPROM_General.ModSMSList[10]= 913; // PF
					EPROM_General.ModSMSList[11]= 917; // Frequency
					EPROM_General.ModSMSList[12]= 199; // Run Hour CKT1
					EPROM_General.ModSMSList[13]= 201; // Run Hour CKT2
					EPROM_General.ModSMSList[14]= 691; // No of LampR(345) 
					EPROM_General.ModSMSList[15]= 0;   // Spare
					EPROM_General.ModSMSList[16]= 0;   // Spare 
					EPROM_General.ModSMSList[17]= 0;   // Spare
					EPROM_General.ModSMSList[18]= 1363;   // Start Hour
					EPROM_General.ModSMSList[19]= 1365;   // start Minute
					EPROM_General.ModSMSList[20]= 1357;   // Stop Hour
					EPROM_General.ModSMSList[21]= 1359;   // Stop Minute
					EPROM_General.ModSMSList[22]= 1213;   // Lograte
					EPROM_General.ModSMSList[23]= 1321;   // LAT
					EPROM_General.ModSMSList[24]= 1323;   // LONG
					EPROM_General.ModSMSList[25]= 1225;   // Meter Type
				
					for(uint8_t i=26;i<EPROM_General.MaxofSMS;i++)
					{
						EPROM_General.ModSMSList[i]= 0;
					}
					QL_GPIOINTDEMO_LOG("1Phase");
				}
        	}
			else if(THREEPHASE == EPROM_General.MeterType)
			{
				if(METER_HPL_DLMS == EPROM_General.MeterMake)
				{
					EPROM_General.ModSMSList[0] = 903; // VRN
					EPROM_General.ModSMSList[1] = 907; // VYN
					EPROM_General.ModSMSList[2] = 911; // VBN
					EPROM_General.ModSMSList[3] = 919; // KWH
					EPROM_General.ModSMSList[4] = 901; // IR
					EPROM_General.ModSMSList[5] = 1221; // No of CKT
					EPROM_General.ModSMSList[6] = 905; // IY
					EPROM_General.ModSMSList[7] = 909; // IB
					EPROM_General.ModSMSList[8] = 1237; // iFPC Mode(618)
					EPROM_General.ModSMSList[9] = 915; // 3-Phase Active Power (456)
					EPROM_General.ModSMSList[10]= 913; // Average PF                        
					EPROM_General.ModSMSList[11]= 917; //Frequency (462)
					EPROM_General.ModSMSList[12]= 199; // Run HOUR CKT1
					EPROM_General.ModSMSList[13]= 201; // RUN HOUR CKT2
					EPROM_General.ModSMSList[14]= 691; // R-PHASE LAMP FAILURE(345)
					EPROM_General.ModSMSList[15]= 693; // Y-PHASE LAMP FAILURE(346)
					EPROM_General.ModSMSList[16]= 695; // B-PHASE LAMP FAILURE(347)
					EPROM_General.ModSMSList[17]= 0;   // Spare
					EPROM_General.ModSMSList[18]= 1363; // Start Hour sunset
					EPROM_General.ModSMSList[19]= 1365; // start Minute sunset
					EPROM_General.ModSMSList[20]= 1357; // Stop Hour sunrise
					EPROM_General.ModSMSList[21]= 1359; // Stop Minute sunrise
					EPROM_General.ModSMSList[22]= 1213;   // Lograte
					EPROM_General.ModSMSList[23]= 1321;   // LAT
					EPROM_General.ModSMSList[24]= 1323;   // LONG
					EPROM_General.ModSMSList[25]= 1225;   // Meter Type

					for(uint8_t i=26;i<EPROM_General.MaxofSMS;i++)
					{
						EPROM_General.ModSMSList[i]= 0;
					}
					QL_GPIOINTDEMO_LOG("3Phase");
				}
			}
			flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter=5;
		}
	}
	else if(KBPage.prog_page == 7)		//version info 
	{
		if(KBPage.enter_count >= 3)
		{
			KBPage.enter_count = 2;
		}
	}
	else if(KBPage.prog_page == 8)		//DO Setting
	{
		if(KBPage.enter_count > 3)
		{
			if(RTU_DO_MODE_MANUAL == EPROM_General.DoModeDetails.Do_Mode)
			{
				if(KBPage.do_page==1)
				{
					b.DO1 = b.DO1_temp;
					if(b.DO1==1)
					{
						EPROM_General.AI_DI_DO_Detail.DOSignal[0] = 1;
					}
					else if(b.DO1==0)
					{
						EPROM_General.AI_DI_DO_Detail.DOSignal[0]  = 0;
					}
					flag_flashUpdateEPROM_General = 1;
					flag_flashUpdateEPROM_General_WaitCounter=5;
				}
				else if(KBPage.do_page==2)
				{
					b.DO2 = b.DO2_temp;
					if(b.DO2==1)
					{
						EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 1;
						EPROM_General.AI_DI_DO_Detail.OLDDOSignal[1] = 0;
					}
					//else if(b.DO2==0)
					{
						//EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 0;
					}
				}
				else if(KBPage.do_page==3)
				{
					b.DO3 = b.DO3_temp;
					if(b.DO3==1)
					{
						EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 1;
						EPROM_General.AI_DI_DO_Detail.OLDDOSignal[2] = 0;
					}
					//else if(b.DO3==0)
					{
						//EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 0;
					}
				}
			}
			KBPage.enter_count = 2;
		}
	}
	else if(KBPage.prog_page == 9)		// Mode SEtting
	{
		if(KBPage.enter_count > 3)
		{
			KBPage.enter_count = 2;
			EPROM_General.DoModeDetails.Do_Mode = EPROM_General.DoModeDetails.Do_Mode_temp;
			RUN_Timer[2] = 1;	// Will generate event
			flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter=5;
		}
	}
	else if(KBPage.prog_page == 10)		// MAX SMS Settings
	{
		if((KBPage.MaxSMS_page == 1) && (KBPage.enter_count > 3))
		{
			KBPage.enter_count = 2;
			flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter=5;
		}
		if((KBPage.MaxSMS_page >=2) && (KBPage.enter_count >= 7))
		{
			if((KBPage.enter_count == 7))
			{
				sprintf((char*)&dispDecimal[0],"%d%d%d%d",Deci.value[0],Deci.value[1],Deci.value[2],Deci.value[3]);
				EPROM_General.ModSMSList[KBPage.MaxSMS_page-2]=atoi((char*)dispDecimal);
				KBPage.enter_count = 2;
				syncExtFlashVariableWithPCBPLCVariable();
				flag_flashUpdateEPROM_General = 1;
            	flag_flashUpdateEPROM_General_WaitCounter=5;
			}
		}
	}
	else if(KBPage.prog_page == 11)		//GPRS configuration
	{
		if((KBPage.gprs_page == 1) && (KBPage.enter_count>=4))
		{
			KBPage.enter_count = 2;
			flag_modem_MQTT_Reconnect = 1;
			flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter=5;
		}
		else if((KBPage.gprs_page == 2) && (KBPage.enter_count>=7))
		{
			sprintf((char*)&dispDecimal[0],"%d%d%d%d",GPRSPort[0], GPRSPort[1], GPRSPort[2], GPRSPort[3]);
			EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port=atoi((char*)dispDecimal);
			KBPage.enter_count = 2;
			flag_modem_MQTT_Reconnect = 1;
			flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter=5;
		}
		else if	((KBPage.gprs_page == 3) && (KBPage.enter_count>17))
		{
			KBPage.enter_count = 2;
			sprintf(&EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP[0],"%s",GPRSLiveIPbuf);
			flag_modem_MQTT_Reconnect = 1;
			syncExtFlashVariableWithPCBPLCVariable();
			flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter=5;
		}
	}
	
	else if (KBPage.prog_page == 12) // Streetlight Setting
	{	
		if ((KBPage.StreetLight_page == 1)  && (KBPage.enter_count >= 14))// Latitude and Longitude Setting
		{
			KBPage.enter_count = 2;
			syncExtFlashVariableWithPCBPLCVariable();
			flag_flashUpdateEPROM_General = 1;
			flag_flashUpdateEPROM_General_WaitCounter = 5;
		}
		else if (KBPage.StreetLight_page == 2 && KBPage.enter_count >= 6) // Timezone Setting
		{
			KBPage.enter_count = 2;
			syncExtFlashVariableWithPCBPLCVariable();
			flag_flashUpdateEPROM_General = 1;
			flag_flashUpdateEPROM_General_WaitCounter = 5;
		}
		else if (KBPage.StreetLight_page == 3 && KBPage.enter_count >= 4) // Astro Offset Time Setting
		{
			KBPage.enter_count = 2;
			syncExtFlashVariableWithPCBPLCVariable();
			flag_flashUpdateEPROM_General = 1;
			flag_flashUpdateEPROM_General_WaitCounter = 5;
		}
		else if (KBPage.StreetLight_page == 4 && KBPage.enter_count >= 4) // no of CKT 
		{
			KBPage.enter_count = 2;
			syncExtFlashVariableWithPCBPLCVariable();
			flag_flashUpdateEPROM_General = 1;
			flag_flashUpdateEPROM_General_WaitCounter = 5;
		}
		else if (KBPage.StreetLight_page == 5 && KBPage.enter_count >= 9) // Time setting  
		{
			KBPage.enter_count = 2;
			syncExtFlashVariableWithPCBPLCVariable();
			rtc_intialized = 1;
		}
	}
	else if ( KBPage.prog_page == 13)		// Daywise DiM schedule 
	{
		if(KBPage.enter_count > 3)
		{
			KBPage.enter_count = 2;
		}
	}
}