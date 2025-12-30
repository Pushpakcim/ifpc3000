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
#include <time.h>
#include <math.h>

#include "ql_api_osi.h"
#include "ql_log.h"
#include "ql_pin_cfg.h"

#include "gpio_DIDO.h"
#include "define.h"
#include "configuration.h"
#include "json_parser_sp.h"
#include "I2C_RTC_LCD.h"
#include "gpio_int_KEY.h"
#include "dlms_meter.h"
/*===========================================================================
 * Macro Definition
 ===========================================================================*/
#define QL_GPIODEMO_LOG_LEVEL             QL_LOG_LEVEL_INFO
#define QL_GPIODEMO_LOG(msg, ...)         QL_LOG(QL_GPIODEMO_LOG_LEVEL, "ql_GPIODIDO", msg, ##__VA_ARGS__)
#define QL_GPIODEMO_LOG_PUSH(msg, ...)    QL_LOG_PUSH("ql_GPIODIDO", msg, ##__VA_ARGS__)

/*===========================================================================
 * Struct
 ===========================================================================*/
static ql_gpio_cfg _ql_gpio_cfg[] =
{   /* gpio_num   gpio_dir      gpio_pull      gpio_lvl    */
    {  GPIO_29,   GPIO_OUTPUT,  0xff,         LVL_LOW },   // set output low-level DO -1
    {  GPIO_30,   GPIO_OUTPUT,  0xff,         LVL_LOW },   // set output low-level DO -2
    {  GPIO_8,    GPIO_OUTPUT,  0xff,         LVL_LOW },   // set output low-level DO -3
    {  GPIO_13,   GPIO_OUTPUT,  0xff,         LVL_LOW },   // set output low-level GSM LED DO
    {  GPIO_22,   GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none DI -1
    {  GPIO_23,   GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none DI -2
    {  GPIO_2,    GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none DI -3
    {  GPIO_3,    GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none DI -4
    {  GPIO_1,    GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none DI -5
    {  GPIO_0,    GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none DI -6
    {  GPIO_21,   GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none Battery Status DI
    {  GPIO_18,   GPIO_INPUT,   PULL_NONE,    0xff    },   // set input pull-none AC Status DI
    {  GPIO_31,   GPIO_OUTPUT,  0xff,         LVL_LOW }    // set output low-level Watchdog DO
};

/*===========================================================================
 * Variate
 ===========================================================================*/
uint8_t DO_Final_value[MAX_DO_CHANNEL];
uint8_t DI_Final_value[MAX_DI_CHANNEL];
uint8_t Old_DI_Final_value[MAX_DI_CHANNEL];
uint32_t count_DO;
uint16_t RUN_Timer[MAXMTR]={0,};
DataTime_t gTimeInfo = {0,};
bool flag_timezone_sign;
uint8_t tPmpOnStatus = 0;
uint8_t tPmpOnStatus_relay[MYSCH];
extern uint8_t ProductionMode;
struct Schedule_Data Schedule_astro;
extern uint8_t DI_Final_value[];
//static bool isRebootFlag = true;
//extern bool isRebootFlag; // Declare the flag from json_parser_sp.c
//static int skipAstroCounter = 0; // Add persistent counter for skipping Astro logic

/*===========================================================================
 * Functions
 ===========================================================================*/

/***************************************************************
This Function Init On Board DIDO *
								 *
								 *
Input:	void					 *
Return	void					 *
****************************************************************/
void _ql_gpio_dido_init(void)
{
    uint16_t num;
    for( num = 0; num < sizeof(_ql_gpio_cfg)/sizeof(_ql_gpio_cfg[0]); num++ )
    {
        // if(num==3)
        // {

        // }
        // else
        {
            QL_GPIODEMO_LOG("init %d, %d",_ql_gpio_cfg,_ql_gpio_cfg[0]);
            ql_gpio_deinit(_ql_gpio_cfg[num].gpio_num);
            ql_gpio_init(_ql_gpio_cfg[num].gpio_num, _ql_gpio_cfg[num].gpio_dir, _ql_gpio_cfg[num].gpio_pull, _ql_gpio_cfg[num].gpio_lvl);
        }
    }
}

/***************************************************************
Function Name : DO 											   *
Inputs:			OnBoard , TurnOn/Off , BitNumber)	   *
OutPuts:		It Alter a Bit on specified DO				   *
****************************************************************/
void DO_On_Off(short State,short BitNumber)
{
	if(State==SET)
	{
		//DoTurnOn(BitNumber);
        QL_GPIODEMO_LOG("DO SET %d,%d",BitNumber,State);
        ql_gpio_set_direction(_ql_gpio_cfg[BitNumber-1].gpio_num, GPIO_OUTPUT);
        ql_gpio_set_level(_ql_gpio_cfg[BitNumber-1].gpio_num, LVL_HIGH);
	}
	else if(State==RESET)
	{
        QL_GPIODEMO_LOG("DO RESET %d,%d",BitNumber,State);
		//DoTurnOff(BitNumber);
        ql_gpio_set_direction(_ql_gpio_cfg[BitNumber-1].gpio_num, GPIO_OUTPUT);
        ql_gpio_set_level(_ql_gpio_cfg[BitNumber-1].gpio_num, LVL_LOW);
	}
	else
	{
        ql_rtos_task_sleep_ms(1);
	}
}

/***************************************************************
This Function Scans(Read) On Board DI for all DI   *
									   *
															   *
Input:	void											   *
Return	void									   *
****************************************************************/
void ScanDI(void)
{
    DI_Final_value[7]=1; // For Communication
    for(uint8_t num = 4; num < 10; num++ )
    {
        /* set input pull-down */
        // ql_gpio_set_direction(_ql_gpio_cfg[num].gpio_num, gpio_dir);
        // ql_gpio_set_pull(_ql_gpio_cfg[num].gpio_num, gpio_pull);

        /* get input pull-down */
        // ql_gpio_get_direction(_ql_gpio_cfg[num].gpio_num, &gpio_dir);
        // ql_gpio_get_pull(_ql_gpio_cfg[num].gpio_num, &gpio_pull);
        ql_gpio_get_level(_ql_gpio_cfg[num].gpio_num, &DI_Final_value[num-4]);

        if(ProductionMode ==1)
        {
            //DI_Final_value[num-4] ^= 1;
        }
        else
        {
            DI_Final_value[num-4] ^= 1;
        }
        // if(DI_Final_value[num-4] == LVL_HIGH)
        // {
        //     DI_Final_value[num-4] = 0;
        // }
        // else
        // {
        //     DI_Final_value[num-4] = 1;
        // }
    }
    for(uint8_t num = 10; num < 12; num++ )
    {
        /* get input pull-down */
        // ql_gpio_get_direction(_ql_gpio_cfg[num].gpio_num, &gpio_dir);
        // ql_gpio_get_pull(_ql_gpio_cfg[num].gpio_num, &gpio_pull);
        ql_gpio_get_level(_ql_gpio_cfg[num].gpio_num, &DI_Final_value[num+4]);
        // DI_Final_value[num+4] ^= 1;
        // if(DI_Final_value[num+4] == LVL_HIGH)
        // {
        //     DI_Final_value[num+4] = 0;
        // }
        // else
        // {
        //     DI_Final_value[num+4] = 1;
        // }
    }
  // Add logging here to check DI1 and DI3 values
  QL_GPIODEMO_LOG("DI1 (DI_Final_value[0]): %d, DI3 (DI_Final_value[2]): %d",DI_Final_value[0], DI_Final_value[2]);  // Log DI1 and DI3 values

   
   QL_GPIODEMO_LOG("in the particular mode DI1 will changed from %d to %d, DI3 will changed from %d to %d",
                        Old_DI_Final_value[0], DI_Final_value[0],
                        Old_DI_Final_value[2], DI_Final_value[2]);

    // Add logging here to check DI1 and DI3 values
    QL_GPIODEMO_LOG("DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
    
    QL_GPIODEMO_LOG("DI_Final_value[9] :  %d ,  and gFinalAnaValF[450] :  %d",DI_Final_value[9],gFinalAnaValF[450]);
    if(DI_Final_value[15] == 0 && DI_Final_value[9]==0 && gFinalAnaValF[450] == 0)     // Power OFF    DIfinal[9] = em con/ disconnected 
    {
        QL_GPIODEMO_LOG("DI_Final_value[9] :  %d ,  and gFinalAnaValF[450] :  %d",DI_Final_value[9],gFinalAnaValF[450]); //add log of difinal[9] = 0 and gFinalAnaValF[450] = 0
        QL_GPIODEMO_LOG("Power OFF state activated. EM Disconnected");
		 QL_GPIODEMO_LOG(" if pow off DI1 (DI_Final_value[0]): %d, DI3 (DI_Final_value[2]): %d",DI_Final_value[0], DI_Final_value[2]);  // Log DI1 and DI3 values
		
        DI_Final_value[0] = DI_Final_value[2] = 1;   // Ckt1 & 2 OFF
        tPmpOnStatus = 0;
        QL_GPIODEMO_LOG("tPmpOnStatus in POWER off : %d", tPmpOnStatus); // Log tPmpOnStatus value
       
        QL_GPIODEMO_LOG(" di1 and 3 set to 1 DI1 (DI_Final_value[0]): %d, DI3 (DI_Final_value[2]): %d",DI_Final_value[0], DI_Final_value[2]);  // Log DI1 and DI3 values
    }
    //add the logic to check snrm response is 1 then only set the DI1 and DI3 to 1
    // if(DI_Final_value[15] == 1)
    // {
    //     if(DI_Final_value[0] == 1 && DI_Final_value[2] == 1) 
    // {
    //     tPmpOnStatus = 0;
    //     QL_GPIODEMO_LOG("Power ON state activated.");
    // }
    // }
  //add log for tPmponstatus
    QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value
    //comment 
    if ((sizeof(DI_Final_value) > 15) && 
        (((DI_Final_value[0] == 0) || (DI_Final_value[2] == 0 && EPROM_General.NoofCkt == 2)) && (DI_Final_value[15] == 1)))  // To set state ON if someone manually turn ON
   //     if ((DI_Final_value[0] == 0) || (DI_Final_value[2] == 0 && EPROM_General.NoofCkt == 2))   // To set state ON if someone manually turn ON
      {
        tPmpOnStatus = 1;
      }

    QL_GPIODEMO_LOG("Updated tPmpOnStatus: %d", tPmpOnStatus);
#if 1
        QL_GPIODEMO_LOG("Old_DI_Final_value: DI1:%d DI2:%d DI3:%d DI4:%d DI5:%d DI6:%d DI8:%d DI9:%d DI10:%d DI11:%d DI12:%d DI13:%d DI14:%d DI15:%d DI16:%d",                    
                    Old_DI_Final_value[0],
                    Old_DI_Final_value[1],
                    Old_DI_Final_value[2],
                    Old_DI_Final_value[3],
                    Old_DI_Final_value[4],
                    Old_DI_Final_value[5],
                    Old_DI_Final_value[7],
                    Old_DI_Final_value[8],
                    Old_DI_Final_value[9],
                    Old_DI_Final_value[10],
                    Old_DI_Final_value[11],
                    Old_DI_Final_value[12],
                    Old_DI_Final_value[13],
                    Old_DI_Final_value[14],
                    Old_DI_Final_value[15]);
    
    QL_GPIODEMO_LOG("    DI_Final_value: DI1:%d DI2:%d DI3:%d DI4:%d DI5:%d DI6:%d DI8:%d DI9:%d DI10:%d DI11:%d DI12:%d DI13:%d DI14:%d DI15:%d DI16:%d",
                    DI_Final_value[0],
                    DI_Final_value[1],
                    DI_Final_value[2],
                    DI_Final_value[3],
                    DI_Final_value[4],
                    DI_Final_value[5],
                    DI_Final_value[7],
                    DI_Final_value[8],
                    DI_Final_value[9],
                    DI_Final_value[10],
                    DI_Final_value[11],
                    DI_Final_value[12],
                    DI_Final_value[13],
                    DI_Final_value[14],
                    DI_Final_value[15]);
#endif
}

void pro_checkDIDOState()
{
	unsigned char i,pro_DI_State_off[6],pro_DI_State_on[6];

	if(pro_DO_DI_TestFinish == 0)
	{
		for(unsigned char i=1;i<=3;i++)
		{
			DO_On_Off(SET, i);
			//DoTurnOn(i); // it will make pin low
            QL_GPIODEMO_LOG("TurnON");
		}
		ql_rtos_task_sleep_ms(1500);
		ScanDI();
		for(i=0;i<6;i++)
		{
			pro_DI_State_off[i]=DI_Final_value[i];
		}
		for(i=1;i<=3;i++)
		{
			DO_On_Off(RESET, i);
            QL_GPIODEMO_LOG("TurningOFF");
			//DoTurnOff(i); // it will make pin high
		}
		ql_rtos_task_sleep_ms(1500);
		ScanDI();
		for(i=0;i<6;i++)
		{
			pro_DI_State_on[i]=DI_Final_value[i];
		}

		for(i=0;i<3;i++)
		{
			if(((pro_DI_State_off[i] == 0)&&(pro_DI_State_on[i] == 1))||((pro_DI_State_off[i+3] == 0)&&(pro_DI_State_on[i+3] == 1)))
			{
				pro_DO_State[i] = 1;
				if((pro_DI_State_off[i] == 0)&&(pro_DI_State_on[i] == 1))
				{
					pro_DI_State[i] = 1;
				}
				else
				{
					pro_DI_State[i] = 0;
				}

				if((pro_DI_State_off[i+3] == 0)&&(pro_DI_State_on[i+3] == 1))
				{
					pro_DI_State[i+3] = 1;
				}
				else
				{
					pro_DI_State[i+3] = 0;
				}
			}
			else
			{
				pro_DO_State[i] = 0;
				pro_DI_State[i] = 0;
				pro_DI_State[i+3] = 0;
			}
			pro_DO_DI_TestFinish = 1;
		}
	}
	else
	{
		// Todo : Write LCD, LED , KEY pad test LOGIC Here

		if(DO_Opration == 2)
		{
			if(DO_Opration_count++ >= 2)
			{
				DO_Opration = 3;
				DO_Opration_count = 0;
				for(i=1;i<=4;i++)
				{
					DO_On_Off(RESET, i);
				}
			}
		}
		if(DO_Opration == 1)
		{
			DO_Opration = 2;
			DO_Opration_count = 0;
			for(unsigned char i=1;i<=4;i++)
			{
				DO_On_Off(SET, i);
			}
		}
	}
}
static void ql_gpio_dido_thread(void *param)
{
    uint8_t wdToggle = 0;//, Set_to_switching = 0;
    ql_event_t event;
    // uint32_t count_DO_manual = 0;
    // uint8_t flag_count_DO_manual = 0;
      ql_rtos_task_sleep_s(6);   
    QL_GPIODEMO_LOG("gpio demo thread enter, param 0x%x", param);
    /* init demo gpio array */
    ql_pin_set_func(QL_TEST1_PIN_GPIO1, QL_TEST1_PIN_GPIO1_FUNC_GPIO);      // TEST1_PIN set GPIO29
    ql_pin_set_func(QL_TEST1_PIN_GPIO2, QL_TEST1_PIN_GPIO2_FUNC_GPIO);      // TEST1_PIN set GPIO30
    ql_pin_set_func(QL_TEST1_PIN_GPIO3, QL_TEST1_PIN_GPIO3_FUNC_GPIO);      // TEST1_PIN set GPIO8

    ql_pin_set_func(QL_TEST1_PIN_GPIO6,  QL_TEST1_PIN_GPIO6_FUNC_GPIO);      // TEST1_PIN set GPIO22
    ql_pin_set_func(QL_TEST1_PIN_GPIO23, QL_TEST1_PIN_GPIO23_FUNC_GPIO);      // TEST1_PIN set GPIO23
    ql_pin_set_func(QL_TEST1_PIN_GPIO24, QL_TEST1_PIN_GPIO24_FUNC_GPIO);      // TEST1_PIN set GPIO2

    ql_pin_set_func(QL_TEST1_PIN_GPIO25, QL_TEST1_PIN_GPIO25_FUNC_GPIO);      // TEST1_PIN set GPIO3
    ql_pin_set_func(QL_TEST1_PIN_GPIO26, QL_TEST1_PIN_GPIO26_FUNC_GPIO);      // TEST1_PIN set GPIO1
    ql_pin_set_func(QL_TEST1_PIN_GPIO27, QL_TEST1_PIN_GPIO27_FUNC_GPIO);      // TEST1_PIN set GPIO0

    ql_pin_set_func(QL_TEST1_PIN_GPIO65, QL_TEST1_PIN_GPIO65_FUNC_GPIO);      // TEST1_PIN set GPIO18
    ql_pin_set_func(QL_TEST1_PIN_GPIO66, QL_TEST1_PIN_GPIO66_FUNC_GPIO);      // TEST1_PIN set GPIO21

    _ql_gpio_dido_init();

    RUN_Timer[0]=0;
    RUN_Timer[1]=0;
    RUN_Timer[2]=0;
    RUN_Timer[3]=0;
    RUN_Timer[4]=0;
    RUN_Timer[5]=0;

    for(uint8_t i=0;i<12;i++)
    {
        Old_DI_Final_value[i] = DI_Final_value[i];
    }

    EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 0;
    EPROM_General.AI_DI_DO_Detail.OLDDOSignal[1] = 0;
    EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 0;
    EPROM_General.AI_DI_DO_Detail.OLDDOSignal[2] = 0;
    
    ql_gpio_set_level(GPIO_31, LVL_HIGH);         
    //ql_rtos_task_sleep_s(5);
QL_GPIODEMO_LOG("Before calling Get_Astro_time function");
	Get_Astro_time();
    // Log after calling Get_Astro_time
    QL_GPIODEMO_LOG("After calling Get_Astro_time: Sunrise - %02d:%02d:%02d, Sunset - %02d:%02d:%02d",
        (int)gFinalAnaValF[SUNRISE_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_SEC_gFinalAnaValF],
        (int)gFinalAnaValF[SUNSET_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_SEC_gFinalAnaValF]);
 QL_GPIODEMO_LOG("After calling Get_Astro_time function");
    ql_gpio_set_level(GPIO_31, LVL_LOW);
    
    while(1)
    {
        count_DO++;
        ql_event_wait(&event, 1);
        //add log
        QL_GPIODEMO_LOG("Main Thread: count_DO = %d", count_DO);
        // Scan DI state
        // QL_GPIODEMO_LOG("Main Thread: Scanning DI state...");

        QL_GPIODEMO_LOG("Before ScanDI: DI1 (DI_Final_value[0]): %d, DI3 (DI_Final_value[2]): %d", DI_Final_value[0], DI_Final_value[2]);
        ScanDI();
        QL_GPIODEMO_LOG("After ScanDI: DI1 (DI_Final_value[0]): %d, DI3 (DI_Final_value[2]): %d", DI_Final_value[0], DI_Final_value[2]);
    

        
    // Sync RTC datetime
    QL_GPIODEMO_LOG("Main Thread: Syncing RTC datetime...");

        // Log RTC values before syncing
        QL_GPIODEMO_LOG("Before Sync_RTC_datetime: RTC Time - %02d:%02d:%02d %02d-%02d-%04d",
            rtc_time.mHour, rtc_time.minute, rtc_time.mSecond,
            rtc_time.mDate, rtc_time.month, rtc_time.myear + 2000);

        Sync_RTC_datetime();
        
    // Log the new RTC value after sync
    QL_GPIODEMO_LOG("after: RTC Sync Completed - Current Time: %02d:%02d:%02d %02d-%02d-%04d",
        gTimeInfo.mHour, gTimeInfo.minute, gTimeInfo.mSecond,
        gTimeInfo.mDate, gTimeInfo.month, gTimeInfo.myear + 2000);

		if(proTestRequest==2)
		{

		}
		else
		{
	        //*** Watchdog Timer ***//
	        if(wdToggle == 0)
	        {
	            wdToggle = 1;
            	ql_gpio_set_level(GPIO_31, LVL_HIGH);
	        }
	        else
	        {
	            wdToggle = 0;
	            ql_gpio_set_level(GPIO_31, LVL_LOW);
	        }
		}

		if(ProductionMode == 1)
		{
			pro_checkDIDOState();
		}
		else
		{
			//Status_led_BLE_GPS();
			UpdateRunTimer();
        	check_timer_DO_onoff_Mode();
            Check_AnaLog_parameter();
		}
        ql_rtos_task_sleep_s(1);
    }
    ql_rtos_task_delete(NULL);
}

void ql_gpio_app_dido_init(void)
{
    QlOSStatus err = QL_OSI_SUCCESS;
    ql_task_t gpio_task = NULL;

    err = ql_rtos_task_create(&gpio_task, 3*1024, APP_PRIORITY_NORMAL, "ql_gpiodido", ql_gpio_dido_thread, NULL, 1);
    if( err != QL_OSI_SUCCESS )
    {
        QL_GPIODEMO_LOG("gpio dido task created failed");
    }
}
                                                                                                                                                                                                 
void check_timer_DO_onoff_Mode(void)
{
    bool DO1_JustPressCommand = false;
    if(RUN_Timer[0] != 0)
    {
        QL_GPIODEMO_LOG("Case-1");
        
        if(RUN_Timer[0] >= EPROM_General.Def_timer[0])
        {
            RUN_Timer[0] = 0;
        }
    }
    // Do not perform any logic when Local
    else if(DI_Final_value[3]==1)
    {
        QL_GPIODEMO_LOG("Case-2 / local");    
        //  DI_Final_value[6]=0;
        //  DI_Final_value[8]=0;
       //DI_Final_value[8]=0;
        RUN_Timer[1]=0;
        RUN_Timer[8]=0;
       QL_GPIODEMO_LOG("Clearing feedback error \r\n");    
    }
    else if(RTU_DO_MODE_MANUAL == EPROM_General.DoModeDetails.Do_Mode)
    {
        ManualModeOperations();
    }
    else if(RTU_DO_MODE_PHOTO == EPROM_General.DoModeDetails.Do_Mode)
    {

    }
    else if(RTU_DO_MODE_AUTO == EPROM_General.DoModeDetails.Do_Mode)
    {
        ScheduleModeOperations();
    }
    else if(RTU_DO_MODE_ASTROTIME_GEO == EPROM_General.DoModeDetails.Do_Mode)
    {
        // Proceed with normal Astro mode operations
        AstroModeOperations();
    }
    else if(RTU_DO_MODE_TWILIGHT_GEO == EPROM_General.DoModeDetails.Do_Mode)
    {

    }
    else if(RTU_DO_MODE_LOCAL == EPROM_General.DoModeDetails.Do_Mode)
    {
        // CivilModeOperations();      
    //    DI_Final_value[6]=0;
    //    DI_Final_value[8]=0;
       RUN_Timer[1]=0;
       RUN_Timer[8]=0;
       QL_GPIODEMO_LOG("Clearing feedback error\r\n");
    }
    if((RUN_Timer[6]!=0) && (EPROM_General.AI_DI_DO_Detail.DOSignal[1]==1))
    {
        if(RUN_Timer[6] >= EPROM_General.Def_timer[6])
        {
            DO_On_Off(RESET, 2);
            DO_On_Off(RESET, 3);
            RUN_Timer[1] = 1;
            RUN_Timer[6] = 0;
            EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 0;
            EPROM_General.AI_DI_DO_Detail.OLDDOSignal[1] = 0;
            b.DO2_temp = 0;
            b.DO2 = 0;
        }
    }
    else if((RUN_Timer[7]!=0) && (EPROM_General.AI_DI_DO_Detail.DOSignal[2]==1))
    {
        if(RUN_Timer[7] >= EPROM_General.Def_timer[7])
        {
            DO_On_Off(RESET, 2);
            DO_On_Off(RESET, 3);
            RUN_Timer[7] = 0;
            EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 0;
            EPROM_General.AI_DI_DO_Detail.OLDDOSignal[2] = 0;
            b.DO3_temp = 0;
            b.DO3 = 0;
        }
    }
    if((RUN_Timer[1]!=0) && (DI_Final_value[15] == 1)) // if Feed back time for DI Checking 
    {
        if(DI_Final_value[0]==0) // Feedback comes and checking more further 
        {
            DI_Final_value[6]=0;
            QL_GPIODEMO_LOG("Clearing feedback error\r\n");
        }
        else if(RUN_Timer[1]>=EPROM_General.Def_timer[1]) // Feed back not received 
        {
            DI_Final_value[6]=1;// Feedback DI not received 
            if(EPROM_General.NoofCkt == 1)           // todo
            {
                RUN_Timer[1]=0;
                RUN_Timer[8]=1;
            }
            QL_GPIODEMO_LOG("Setting feedback error\r\n");
        }

        if(DI_Final_value[2]==0) // Feedback comes and checking more further 
        {
            DI_Final_value[8]=0;
            QL_GPIODEMO_LOG("Clearing feedback error\r\n");
        }
        else if(RUN_Timer[1]>=EPROM_General.Def_timer[1] && EPROM_General.NoofCkt == 2) // Feed back not received 
        {
            RUN_Timer[1]=0;
            RUN_Timer[8]=1;
            DI_Final_value[8]=1;// Feedback DI not received 
            QL_GPIODEMO_LOG("Setting feedback error\r\n");
        }

        if(DI_Final_value[0]==0 && ((DI_Final_value[2]==0 && EPROM_General.NoofCkt == 2) || EPROM_General.NoofCkt == 1))    // If both ckt not trip then reset timer
        {
            RUN_Timer[1]=1;
            RUN_Timer[8]=0;
            QL_GPIODEMO_LOG("both ckt not trip reset\r\n");
        }
    }
    
    if(RUN_Timer[8]!=0 && (DI_Final_value[15] == 1)) // if Feed back time for DI Checking 
    {
        if(DI_Final_value[0]==1) // Feedback not received
        {
            RUN_Timer[8]=1;             // 
            DI_Final_value[6]=1;
            QL_GPIODEMO_LOG("Setting feedback error\r\n");
        }
        else if(RUN_Timer[8] >= EPROM_General.Def_timer[8]) // Feed clear
        {
            DI_Final_value[6]=0;
            if(EPROM_General.NoofCkt == 1)
            {
                RUN_Timer[8]=1;
            }
            QL_GPIODEMO_LOG("Clearing feedback error\r\n");
        }

        if(DI_Final_value[2]==1) // Feedback not received 
        {
            RUN_Timer[8]=1;
            DI_Final_value[8]=1;
            QL_GPIODEMO_LOG("Setting feedback error\r\n");
        }
        else if(RUN_Timer[8] >= EPROM_General.Def_timer[8] && EPROM_General.NoofCkt == 2) // Feed clear
        {
            RUN_Timer[8]=1;
            DI_Final_value[8]=0;
            QL_GPIODEMO_LOG("Clearing feedback error\r\n");
        }
    }

    if(RUN_Timer[5]!=0 && EPROM_General.AI_DI_DO_Detail.DOSignal[1]==1) // if Feed back time for DI Checking 
    {
        QL_GPIODEMO_LOG("iFPC EPROM.DOSignal[1] %d RUN_Timer[5]%d\r\n", EPROM_General.AI_DI_DO_Detail.DOSignal[1], RUN_Timer[5]);
        if(DO1_JustPressCommand == 1)  // maulin
        {
            if(RUN_Timer[5] >= EPROM_General.Def_timer[5])
            {
                EPROM_General.AI_DI_DO_Detail.DOSignal[2-1] = 0;
                RUN_Timer[5] = 0;
                RUN_Timer[1] = 1;
                RUN_Timer[8] = 1;
                // set_off(1);
                DO_On_Off(RESET, 1);
                DO1_JustPressCommand = 0;
            }
        }
    }
}

void UpdateRunTimer(void)
{
    if(RUN_Timer[0] != 0)
    {
        RUN_Timer[0]++;
    }
    if(RUN_Timer[1] != 0)
    {
        RUN_Timer[1]++;
    }
    if(RUN_Timer[2] != 0)
    {
        RUN_Timer[2]++;
    }
    if(RUN_Timer[3] != 0)
    {
        RUN_Timer[3]++;
    }
    if(RUN_Timer[4] != 0)
    {
        RUN_Timer[4]++;
    }
    if(RUN_Timer[5] != 0)
    {
        RUN_Timer[5]++;
    }
    if(RUN_Timer[6] != 0)
    {
        RUN_Timer[6]++;
    }
    if(RUN_Timer[7] != 0)
    {
        RUN_Timer[7]++;
    }
    if(RUN_Timer[8] != 0)
    {
        RUN_Timer[8]++;
    }
    QL_GPIODEMO_LOG( "RUN_Timer [0]:%02d/%02d [1]:%02d/%02d [2]:%02d/%02d [3]:%02d/%02d [4]:%02d/%02d [5]:%02d/%02d [6]:%02d/%02d [7]:%02d/%02d [8]:%02d/%02d DI_Final_value[8]:%d\r\n",
                            RUN_Timer[0], EPROM_General.Def_timer[0],
                            RUN_Timer[1], EPROM_General.Def_timer[1],
                            RUN_Timer[2], EPROM_General.Def_timer[2],
                            RUN_Timer[3], EPROM_General.Def_timer[3],
                            RUN_Timer[4], EPROM_General.Def_timer[4],
                            RUN_Timer[5], EPROM_General.Def_timer[5],
                            RUN_Timer[6], EPROM_General.Def_timer[6],
                            RUN_Timer[7], EPROM_General.Def_timer[7],
                            RUN_Timer[8], EPROM_General.Def_timer[8],
                            DI_Final_value[8]);
}

// This function handles manual mode operations for digital outputs (DO) based on the current and previous DO signal states.
// It iterates over the number of pumps and logs the DO signal changes.
// If there is a change in the DO signal, it updates the old DO signal and adjusts the output state accordingly.
// Timers are set based on the updated DO signal states.

void ManualModeOperations(void)
{
    // Add logging here to check DI1 and DI3 values
    QL_GPIODEMO_LOG(" Manual Mode : \nDI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
    QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value
    uint8_t tDO_Num;
    for(tDO_Num = 0; tDO_Num < EPROM_General.AI_DI_DO_Detail.Total_Do ; ++tDO_Num)
    {
        if(tDO_Num == 0)
        {
            QL_GPIODEMO_LOG("MANUAL_MODE - DO(%d) ,%d-->%d\r\n", tDO_Num, EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num], EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num]);
            if(EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] != EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num])
            {
                EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num] = EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num];
                if(1 == EPROM_General.AI_DI_DO_Detail.DOSignal[0])
                {
                                        // Add log here to check DI1 and DI3 values
    QL_GPIODEMO_LOG("Manual mode do0=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
  
                    DO_On_Off(SET, 1);
                    RUN_Timer[1]=1;
                    RUN_Timer[0]=0;
                    EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 1;      // To turn ON lamp
                    EPROM_General.AI_DI_DO_Detail.OLDDOSignal[1] = 0;
                    tPmpOnStatus = 1;
                }
                else
                {
                                      // Add log here to check DI1 and DI3 values
    QL_GPIODEMO_LOG("Manual mode do0=0 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
  
                    DO_On_Off(RESET, 1);
                    RUN_Timer[1]=0;
                    RUN_Timer[0]=1;
                    EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 1;      // To turn OFF lamp
                    EPROM_General.AI_DI_DO_Detail.OLDDOSignal[2] = 0;
                    tPmpOnStatus = 0;
                }
            }
        }
        else if(tDO_Num == 1 || tDO_Num == 2)
        {
            QL_GPIODEMO_LOG("MANUAL_MODE - DO(%d) ,%d-->%d\r\n", tDO_Num, EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num], EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num]);
            if(EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] != EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num])
            {
                EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num] = EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num];
                    // Add log here to check DI1 and DI3 values
                    QL_GPIODEMO_LOG("Manual mode do 1 & 2 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
  
                if(1 == EPROM_General.AI_DI_DO_Detail.DOSignal[1])
                {
                                        // Add log here to check DI1 and DI3 values
    QL_GPIODEMO_LOG("Manual mode do1=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
  
                    DO_On_Off(RESET, 3);
                    DO_On_Off(SET, 2);
                    DO_On_Off(SET, 1);  
                    RUN_Timer[1]=1;
                    RUN_Timer[0]=0;
                    RUN_Timer[6]=1;
                    tPmpOnStatus = 1;
                }
                else if(1 == EPROM_General.AI_DI_DO_Detail.DOSignal[2])
                {
                                        // Add log here to check DI1 and DI3 values
    QL_GPIODEMO_LOG("Manual mode do2=1   : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
  
                    DO_On_Off(SET, 3);
                    DO_On_Off(RESET, 2);  
                    DO_On_Off(RESET, 1);
                    RUN_Timer[1]=0; 
                    RUN_Timer[0]=0;
                    RUN_Timer[7]=1;
                    tPmpOnStatus = 0;
                    QL_GPIODEMO_LOG("TurningOFF");
                }
            }
        }
    }

    for(tDO_Num = 0 ; tDO_Num < EPROM_General.AI_DI_DO_Detail.Total_Do ; ++tDO_Num)
    {
        if(2 == EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num])
        {
            EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] = 1; 
            RUN_Timer[1]=1;
            RUN_Timer[0]=1; 
        }
    }
}

void ScheduleModeOperations(void)
{
    //                                 // Add log here to check DI1 and DI3 values
    // QL_GPIODEMO_LOG("schedule mode pwr on : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
 
    //tPmpOnStatus = 0;
    unsigned int tDO_Num;

    for(tDO_Num = 0 ; tDO_Num < EPROM_General.AI_DI_DO_Detail.Total_Do ; ++tDO_Num)
    {
        if(tDO_Num == 1 || tDO_Num == 2)                    //todo
        {
            QL_GPIODEMO_LOG("Schedule - DO(%d) ,%d-->%d\r\n", tDO_Num, EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num], EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num]);
            if((EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] != EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num]) 
                && (DI_Final_value[15] == 1 && DI_Final_value[6] == 0 && DI_Final_value[8] == 0))
            {

                // Add log here to check DI0 and DI2 values
                QL_GPIODEMO_LOG("Schedule Mode: DI0 Final Value: %d, DI2 Final Value: %d", DI_Final_value[0], DI_Final_value[2]);
               
                EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num] = EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num];
                if(tDO_Num == 0)
				{ 
                    //add log here to check DI1 and DI3 values
                    QL_GPIODEMO_LOG("Schedule mode do0=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3 
                    continue;
                }
	
                    
                if(1 == EPROM_General.AI_DI_DO_Detail.DOSignal[1])// && (tPmpOnStatus == 0))
                {
                    // Add log here to check DI1 and DI3 values
                    QL_GPIODEMO_LOG("Schedule mode do1=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                    
                    DO_On_Off(RESET, 3);
                    DO_On_Off(SET, 2);
                    DO_On_Off(SET, 1);
                    RUN_Timer[1]=1;
                    RUN_Timer[0]=0;
                    RUN_Timer[6]=1;
                    tPmpOnStatus = 1;
                    // add log of di0 and di2 values
                    QL_GPIODEMO_LOG("Schedule mode do1=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3 
                }
                else if(1 == EPROM_General.AI_DI_DO_Detail.DOSignal[2])// && (tPmpOnStatus == 1))
                {

                    // Add log here to check DI1 and DI3 values
                    QL_GPIODEMO_LOG("Schedule mode do2=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                    DO_On_Off(SET, 3);
                    DO_On_Off(RESET, 2);
                    DO_On_Off(RESET, 1);
                    RUN_Timer[1]=0; 
                    RUN_Timer[0]=0; 
                    RUN_Timer[7]=1;
                    tPmpOnStatus = 0;
                    // EPROM_General.AI_DI_DO_Detail.dig_bit_array[2] = 1;
                    //add log of di0 and di2 values
                    QL_GPIODEMO_LOG("Schedule mode do2=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                }
            }
        }
    }
    for(uint8_t tSchNo = 0 ; tSchNo < MYSCH ; ++tSchNo)
    {
        // add RTC time log
        QL_GPIODEMO_LOG("Schedule mode: RTC Time - %02d:%02d:%02d %02d-%02d-%04d",
            gTimeInfo.mHour, gTimeInfo.minute, gTimeInfo.mSecond,
            gTimeInfo.mDate, gTimeInfo.month, gTimeInfo.myear + 2000); 
        if(EPROM_Schedule.Schedule[tSchNo].Sch_En_Di) // New Logic for 23:59 Midnight time schedule
        {
            if(((EPROM_Schedule.Schedule[tSchNo].Stop_HH) == 23) && ((EPROM_Schedule.Schedule[tSchNo].Stop_Min) == 59))
            {
                if ((gTimeInfo.mHour > EPROM_Schedule.Schedule[tSchNo].Start_HH) ||
                    ((gTimeInfo.mHour == EPROM_Schedule.Schedule[tSchNo].Start_HH) &&
                    (gTimeInfo.minute >= EPROM_Schedule.Schedule[tSchNo].Start_Min)))
                {
                    // Current time is equal to or after the start time
                    if ((gTimeInfo.mHour < EPROM_Schedule.Schedule[tSchNo].Stop_HH) ||
                        ((gTimeInfo.mHour == EPROM_Schedule.Schedule[tSchNo].Stop_HH) &&
                            (gTimeInfo.minute <= EPROM_Schedule.Schedule[tSchNo].Stop_Min)))
                    {
                        // Current time is before or equal to the stop time
                        if(tPmpOnStatus == 0)
                        {
                            // add di log here to check DI1 and DI3 values
                            QL_GPIODEMO_LOG("Schedule mode do3=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                            tPmpOnStatus_relay[tSchNo] = 1;
                            //add di log here to check DI1 and DI3 values
                            QL_GPIODEMO_LOG("Schedule mode do3=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                            break;
                        }
                    }
                    else
                    {
                        if(tPmpOnStatus == 1)
                        {
                            tPmpOnStatus_relay[tSchNo] = 0;
                        }
                    }
                }
                else
                {
                    if(tPmpOnStatus == 1)
                    {
                        tPmpOnStatus_relay[tSchNo] = 0;
                    }
                }
            }
            else
            {
                if ((gTimeInfo.mHour > EPROM_Schedule.Schedule[tSchNo].Start_HH) ||
                    ((gTimeInfo.mHour == EPROM_Schedule.Schedule[tSchNo].Start_HH) &&
                        (gTimeInfo.minute >= EPROM_Schedule.Schedule[tSchNo].Start_Min)))
                {
                    QL_GPIODEMO_LOG("lora: %d", tSchNo);
                    // Current time is equal to or after the start time
                    if ((EPROM_Schedule.Schedule[tSchNo].Stop_HH >= EPROM_Schedule.Schedule[tSchNo].Start_HH) && 
                        ((gTimeInfo.mHour < EPROM_Schedule.Schedule[tSchNo].Stop_HH) ||
                            ((gTimeInfo.mHour == EPROM_Schedule.Schedule[tSchNo].Stop_HH) &&
                                (gTimeInfo.minute < EPROM_Schedule.Schedule[tSchNo].Stop_Min))))
                    {
                        // Current time is before or equal to the stop time
                        tPmpOnStatus_relay[tSchNo] = 1;
                        break;
                    }                                                                                              
                    else
                    {
                        if(EPROM_Schedule.Schedule[tSchNo].Stop_HH < EPROM_Schedule.Schedule[tSchNo].Start_HH)
                        {
                    QL_GPIODEMO_LOG("lora: %d", tSchNo);
                            // Current time is before or equal to the stop time
                            tPmpOnStatus_relay[tSchNo] = 1;
                            break;
                        }
                        else if(tPmpOnStatus == 1)
                        {
                    QL_GPIODEMO_LOG("lora: %d", tSchNo);
                            tPmpOnStatus_relay[tSchNo] = 0;
                        }
                    }
                }
                else
                {
                    if ((EPROM_Schedule.Schedule[tSchNo].Stop_HH < EPROM_Schedule.Schedule[tSchNo].Start_HH) && 
                            ((gTimeInfo.mHour < EPROM_Schedule.Schedule[tSchNo].Stop_HH) ||
                                ((gTimeInfo.mHour == EPROM_Schedule.Schedule[tSchNo].Stop_HH) &&
                                    (gTimeInfo.minute < EPROM_Schedule.Schedule[tSchNo].Stop_Min))))
                    {
                    QL_GPIODEMO_LOG("lora: %d", tSchNo);
                        // Current time is before or equal to the stop time
                        tPmpOnStatus_relay[tSchNo] = 1;
                        break;
                    }
                    else if(tPmpOnStatus == 1)
                    {
                        tPmpOnStatus_relay[tSchNo] = 0;
                    }
                }
            }
            QL_GPIODEMO_LOG("Enable: %d %d %d", tSchNo, tPmpOnStatus, EPROM_Schedule.Schedule[tSchNo].Sch_En_Di);    
            QL_GPIODEMO_LOG("Start Time: %d %d :%d", tSchNo, EPROM_Schedule.Schedule[tSchNo].Start_HH, EPROM_Schedule.Schedule[tSchNo].Start_Min);      
            QL_GPIODEMO_LOG("Stop Time: %d %d :%d", tSchNo, EPROM_Schedule.Schedule[tSchNo].Stop_HH, EPROM_Schedule.Schedule[tSchNo].Stop_Min);
        }
    }
    QL_GPIODEMO_LOG("tPmpOnStatus_relay: %d  %d  %d  %d",tPmpOnStatus_relay[0],tPmpOnStatus_relay[1],tPmpOnStatus_relay[2],tPmpOnStatus_relay[3]);

    if((tPmpOnStatus_relay[0] == 1 || tPmpOnStatus_relay[1] == 1 || tPmpOnStatus_relay[2] == 1 
    || tPmpOnStatus_relay[3] == 1) && tPmpOnStatus == 0 )//|| DI_Final_value[0] == 1))        //turnON Condition
    {
        //log here to check DI1 and DI3 values
        QL_GPIODEMO_LOG("Schedule mode do0=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
        EPROM_General.AI_DI_DO_Detail.DOSignal[0] = 1;
        EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 1;
        EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 0;
        EPROM_General.AI_DI_DO_Detail.OLDDOSignal[1] = 0;
    }
    else if((tPmpOnStatus_relay[0] == 0 && tPmpOnStatus_relay[1] == 0 && tPmpOnStatus_relay[2] == 0 
    && tPmpOnStatus_relay[3] == 0) && tPmpOnStatus == 1)        //turnoff condition
    {
        EPROM_General.AI_DI_DO_Detail.DOSignal[0] = 0;
        EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 0;
        EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 1;
        EPROM_General.AI_DI_DO_Detail.OLDDOSignal[2] = 0;
    }

    // QL_GPIODEMO_LOG("\n OLD DO = %d %d",EPROM_General.AI_DI_DO_Detail.DOSignal[0],EPROM_General.AI_DI_DO_Detail.OLDDOSignal[0]);
    for(tDO_Num = 0 ; tDO_Num < EPROM_General.AI_DI_DO_Detail.Total_Do ; ++tDO_Num)
    {
        if(tDO_Num == 1 || tDO_Num == 2)
        {
            if(EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] != EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num])
            {
                if(EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] == 1)
                {
                    RUN_Timer[1]=1;
                    RUN_Timer[0]=0;
                }
                else
                {
                    RUN_Timer[1]=0;
                    RUN_Timer[0]=0;
                }                            
            }
        }
    }
}

void AstroModeOperations(void)
{
    QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value




    // if (isReboot)
    // {
    //     QL_GPIODEMO_LOG("Astro Mode: Skipping activation due to reboot");
    //     isReboot = false;
    //     return;
    // }

    // Normal Astro Mode logic here
     
    //tPmpOnStatus = 0;
    unsigned int tDO_Num;

    for(tDO_Num = 0 ; tDO_Num < EPROM_General.AI_DI_DO_Detail.Total_Do ; ++tDO_Num)
    {
        if(tDO_Num == 1 || tDO_Num == 2)
        {
            QL_GPIODEMO_LOG("AutoMode - DO(%d) ,%d-->%d\r\n", tDO_Num, EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num], EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num]);
            if((EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] != EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num]) 
                && (DI_Final_value[15] == 1 && DI_Final_value[6] == 0 && DI_Final_value[8] == 0))

    
            {
                EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num] = EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num];

                if(tDO_Num == 0)
                       continue;
                    // Add log here to check DI1 and DI3 values
                    QL_GPIODEMO_LOG("astro mode do0 continue : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
  
                if(1 == EPROM_General.AI_DI_DO_Detail.DOSignal[1]) 
                {
                    // add log here to check DI1 and DI3 values
                    QL_GPIODEMO_LOG("astro mode do1=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                    
                    DO_On_Off(RESET, 3);
                    DO_On_Off(SET, 2);
                    DO_On_Off(SET, 1);
                    RUN_Timer[1]=1;
                    RUN_Timer[0]=0;
                    RUN_Timer[6]=1;
                    tPmpOnStatus = 1;
                    QL_GPIODEMO_LOG("\n DO_on in astro  1_do1 ");
                    QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value

                    QL_GPIODEMO_LOG("astro mode do1=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]);
                }
                else if(1 == EPROM_General.AI_DI_DO_Detail.DOSignal[2])
                {
                    // add log here to check DI1 and DI3 values
                    QL_GPIODEMO_LOG("astro mode do2=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
   
                    DO_On_Off(SET, 3);
                    DO_On_Off(RESET, 2);
                    DO_On_Off(RESET, 1);
                    RUN_Timer[1]=0; 
                    RUN_Timer[0]=0; 
                    RUN_Timer[7]=1;
                    tPmpOnStatus = 0;
                    QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value
                    QL_GPIODEMO_LOG("\n DO_on in astro  1_do2 ");
                    QL_GPIODEMO_LOG("astro mode do2=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                }
            }
           
        }
    }
    //add astrogeo time log
    QL_GPIODEMO_LOG("Astro mode: RTC Time - %02d:%02d:%02d %02d-%02d-%04d",
        gTimeInfo.mHour, gTimeInfo.minute, gTimeInfo.mSecond,
        gTimeInfo.mDate, gTimeInfo.month, gTimeInfo.myear + 2000);


    Schedule_astro.Sch_En_Di =1;

   // add astrogeo time log
    QL_GPIODEMO_LOG("Astro mode: RTC Time - %02d:%02d:%02d %02d-%02d-%04d",
        gTimeInfo.mHour, gTimeInfo.minute, gTimeInfo.mSecond,
        gTimeInfo.mDate, gTimeInfo.month, gTimeInfo.myear + 2000);

    Schedule_astro.Start_HH = (unsigned char)gFinalAnaValF[SUNSET_HOUR_gFinalAnaValF];
    Schedule_astro.Start_Min  = (unsigned char)gFinalAnaValF[SUNSET_MIN_gFinalAnaValF];
    Schedule_astro.Stop_HH  = (unsigned char)gFinalAnaValF[SUNRISE_HOUR_gFinalAnaValF];
    Schedule_astro.Stop_Min   = (unsigned char)gFinalAnaValF[SUNRISE_MIN_gFinalAnaValF];

    if ((gTimeInfo.mHour > Schedule_astro.Start_HH) ||
        ((gTimeInfo.mHour == Schedule_astro.Start_HH) &&
            (gTimeInfo.minute >= Schedule_astro.Start_Min)))
    {
        // Current time is equal to or after the start time
        if (Schedule_astro.Stop_HH >= Schedule_astro.Start_HH && ((gTimeInfo.mHour < Schedule_astro.Stop_HH) ||
            ((gTimeInfo.mHour == Schedule_astro.Stop_HH) &&
                (gTimeInfo.minute < Schedule_astro.Stop_Min))))
        {
            QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value
            // Current time is before or equal to the stop time
            tPmpOnStatus_relay[0] = 1;
        }
        else
        {
            if(Schedule_astro.Stop_HH <Schedule_astro.Start_HH)
            {
                QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value
                // add log here to check DI1 and DI3 values
                QL_GPIODEMO_LOG("astro mode do0=1 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                tPmpOnStatus_relay[0] = 1;
            }
            else if(tPmpOnStatus == 1)
            {
                QL_GPIODEMO_LOG("tPmpOnStatus: %d", tPmpOnStatus); // Log tPmpOnStatus value
                // add log here to check DI1 and DI3 values
                QL_GPIODEMO_LOG("astro mode do0=0 : \n DI1 Final Value: %d, DI3 Final Value: %d", DI_Final_value[0], DI_Final_value[2]); // Additional logging for DI1 and DI3
                tPmpOnStatus_relay[0] = 0;
            }
        }
    }
    else
    {
        if ((Schedule_astro.Stop_HH < Schedule_astro.Start_HH) && ((gTimeInfo.mHour < Schedule_astro.Stop_HH) ||
            ((gTimeInfo.mHour == Schedule_astro.Stop_HH) && (gTimeInfo.minute < Schedule_astro.Stop_Min))))
        {
            tPmpOnStatus_relay[0] = 1;
        }
        else if(tPmpOnStatus == 1)
        {
            tPmpOnStatus_relay[0] = 0;
        }
    }
   
    QL_GPIODEMO_LOG("Enable:%d %d %d ", tPmpOnStatus, EPROM_General.AI_DI_DO_Detail.DOSignal[0], Schedule_astro.Sch_En_Di);    
    QL_GPIODEMO_LOG("Start Time: %d %d :%d", EPROM_General.AI_DI_DO_Detail.DOSignal[0], Schedule_astro.Start_HH, Schedule_astro.Start_Min);      
    QL_GPIODEMO_LOG("Stop Time: %d %d :%d", EPROM_General.AI_DI_DO_Detail.DOSignal[0], Schedule_astro.Stop_HH, Schedule_astro.Stop_Min);

   
    if(tPmpOnStatus_relay[0] == 1 && tPmpOnStatus == 0)
    {
        EPROM_General.AI_DI_DO_Detail.DOSignal[0] = 1;
        EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 1;
        EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 0;
        EPROM_General.AI_DI_DO_Detail.OLDDOSignal[1] = 0;
        QL_GPIODEMO_LOG("\n DO_on in astro  Do_0 = 1 ");
    }
    else if(tPmpOnStatus_relay[0] == 0 && tPmpOnStatus == 1)
    {
        EPROM_General.AI_DI_DO_Detail.DOSignal[0] = 0;
        EPROM_General.AI_DI_DO_Detail.DOSignal[1] = 0;
        EPROM_General.AI_DI_DO_Detail.DOSignal[2] = 1;
        EPROM_General.AI_DI_DO_Detail.OLDDOSignal[2] = 0;
        QL_GPIODEMO_LOG("\n DO_off in astro  do_0 = 0");
    }

     QL_GPIODEMO_LOG("\n OLD DO = %d %d",EPROM_General.AI_DI_DO_Detail.DOSignal[1],EPROM_General.AI_DI_DO_Detail.OLDDOSignal[1]);


     //*****************
     //check

     for(tDO_Num = 0 ; tDO_Num < EPROM_General.AI_DI_DO_Detail.Total_Do ; ++tDO_Num)
    {
        if(tDO_Num == 1 || tDO_Num == 2)
        {
            if(EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] != EPROM_General.AI_DI_DO_Detail.OLDDOSignal[tDO_Num])
            {
                if(EPROM_General.AI_DI_DO_Detail.DOSignal[tDO_Num] == 1)
                {
                    RUN_Timer[1]=1;
                    RUN_Timer[0]=0;
                }
                else
                {
                    RUN_Timer[1]=0;
                    RUN_Timer[0]=0;
                }                            
            }
        }
    }
  
}

uint32_t Calculate_SunsetSunrise(unsigned char direction)//, struct tm fine_time)
{
	int i=0;
	double lngHour =0;
    double t=0;
	double M=0 ;
	double L =0;
	double RA=0;
    double Lquadrant=0;
    double RAquadrant =0;
    double sinDec =0;
    double cosDec =0;
    double cosH =0;
    double H=0;
    double T =0;
    double UT=0;
	int N = 0;
    double  RadacosTempcosH=0;
    float temp1,temp2;

	for(i=1;i<bcdToDec(rtc_time.month);i++)
	{
		if(i==1 || i==3 || i==5 || i==7 || i==9 || i==11)
		{
			N=N+31;
		}
		else if(i==4 || i==6 || i==8 || i==10|| i==12)
		{
			N=N+30;
		}
		if(i==2)
		{
			if((bcdToDec(rtc_time.myear) + 2000 - 1900)%4 == 0)
			N=N+29;
			else
			N=N+28;
		}
	}
	N=N+bcdToDec(rtc_time.mDate);
    /* appr. time (t) */
	lngHour = (double)EPROM_General.Cust_Detail.Longitude / 15.0;
    if (direction == SUNRISE)
        t = N + ((6.0 - lngHour) / 24.0);
    else
        t = N + ((18.0 - lngHour) / 24.0);
    /* mean anomaly (M) */
    M = (0.9856 * t) - 3.289;
    temp1=Deg2Rad(M);
    temp1=sinf(temp1);
    temp2=Deg2Rad(2 * M);
    temp2=sinf(temp2);
	/* true gGpsLongitude (L) */
    L = M + (1.916 * temp1 ) + (0.020 * temp2) + 282.634;

    L = FixValue(L, 0, 360);
    /* right asc (RA) */
    RA = Rad2Deg(atan(0.91764 * tanf(Deg2Rad(L))));
    RA = FixValue(RA, 0, 360);
    /* adjust quadrant of RA */
    temp2 =(L / 90.0);
    temp2 =(int)(temp2);
    Lquadrant =  temp2* 90.0;
    temp2 =(RA / 90.0);
    temp2 =(int)(temp2);
    RAquadrant =  temp2* 90.0;
    RA = RA + (Lquadrant - RAquadrant);
    RA = RA / 15.0;
    /* sin cos DEC (sinDec / cosDec) */
     sinDec = 0.39782 * sinf(Deg2Rad(L));
     cosDec = cosf(asinf(sinDec));
    /* local hour angle (cosH) */
    cosH = (cos(Deg2Rad(zenith / 1000.0)) - (sinDec * sin(Deg2Rad(EPROM_General.Cust_Detail.Lattitude)))) / (cosDec * cos(Deg2Rad(EPROM_General.Cust_Detail.Lattitude)));
    RadacosTempcosH=Rad2Deg(acosf((float)cosH));
    if (direction == SUNRISE)
        H = 360.0 - RadacosTempcosH;
    else
        H = RadacosTempcosH;
    H = H / 15.0;
    /* time (T) */
     T = H + RA - (0.06571 * t) - 6.622;
    /* universal time (T) */
    UT = T - lngHour;
    UT = FixValue(UT, 0, 24);
    UT = UT*100;
	return (((((int)UT * 3600) / 100)));// + utcOffset); // Convert to seconds
}

void Get_Astro_time(void)
{
    // static int skipAstroCounter = 0;

    // // Check RTC time after reboot
    // if (isRebootFlag)
    // {
    //     QL_GPIODEMO_LOG("RTC after reboot: %02d:%02d:%02d", 
    //                     gTimeInfo.mHour, gTimeInfo.minute, gTimeInfo.mSecond);
    
    //     if (gTimeInfo.mHour == 0 && gTimeInfo.minute == 0 && gTimeInfo.mSecond == 0)
    //     {
    //         skipAstroCounter = 2;  // Skip Astro logic for 2 iterations
    //         QL_GPIODEMO_LOG("RTC shows default time (00:00:00). Skipping Astro logic for 2 cycles.");
    //     }
    
    //     // Reboot flag handled; reset it
    //     isRebootFlag = false;
    // }
    
    // // Skip Astro logic if counter is active
    // if (skipAstroCounter > 0)
    // {
    //     QL_GPIODEMO_LOG("Astro Mode: Skipping execution, remaining skips: %d", skipAstroCounter);
    //     skipAstroCounter--;
    //     return;
    // }
    
    // Proceed with normal Astro mode operations
    //AstroModeOperations();
    



    
	time_t current_time = 0, timezone_UTC;
	// pointer
	struct tm* ptime;
	timezone_UTC = get_offset_UTC();
	if(flag_timezone_sign == 1)
	{
		current_time = Calculate_SunsetSunrise(SUNRISE)-timezone_UTC;
	}
	else
	{
		current_time = Calculate_SunsetSunrise(SUNRISE)+timezone_UTC;
	}
    current_time += EPROM_General.Cust_Detail.Offset_Value;
	ptime = gmtime(&current_time);
	gFinalAnaValF[SUNRISE_HOUR_gFinalAnaValF] = (ptime->tm_hour)%24;
	gFinalAnaValF[SUNRISE_MIN_gFinalAnaValF]  = ptime->tm_min;
	gFinalAnaValF[SUNRISE_SEC_gFinalAnaValF]  = ptime->tm_sec;

	if(flag_timezone_sign == 1)
	{
		current_time = Calculate_SunsetSunrise(SUNSET)-timezone_UTC;
	}
	else
	{
		current_time = Calculate_SunsetSunrise(SUNSET)+timezone_UTC;
	}
    current_time += EPROM_General.Cust_Detail.Offset_Value;
	ptime = gmtime(&current_time);
	gFinalAnaValF[SUNSET_HOUR_gFinalAnaValF] = (ptime->tm_hour)%24;
	gFinalAnaValF[SUNSET_MIN_gFinalAnaValF]  = ptime->tm_min;
	gFinalAnaValF[SUNSET_SEC_gFinalAnaValF]  = ptime->tm_sec;
}

void Sync_RTC_datetime(void)
{

    // Log the current RTC time before syncing
    QL_GPIODEMO_LOG("Sync_RTC_datetime: RTC Time Before Sync - %02d:%02d:%02d %02d-%02d-%04d",
        rtc_time.mHour, rtc_time.minute, rtc_time.mSecond,
        rtc_time.mDate, rtc_time.month, rtc_time.myear + 2000);

    gTimeInfo.mHour = rtc_time.mHour;  // set change
	gTimeInfo.minute = rtc_time.minute;// set change
	gTimeInfo.mSecond = rtc_time.mSecond;
	gTimeInfo.mDate = rtc_time.mDate;
	gTimeInfo.month = rtc_time.month;
	gTimeInfo.myear = rtc_time.myear;
	//gTimeInfo.mDayofWeek = rtc_time.mweek;
    
     // Log the updated gTimeInfo structure
     QL_GPIODEMO_LOG("Sync_RTC_datetime: Updated gTimeInfo - %02d:%02d:%02d %02d-%02d-%04d",
        gTimeInfo.mHour, gTimeInfo.minute, gTimeInfo.mSecond,
        gTimeInfo.mDate, gTimeInfo.month, gTimeInfo.myear + 2000);


    gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF+0] = gTimeInfo.mDate;
	gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF+1] = gTimeInfo.month;
	gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF+2] = gTimeInfo.myear + 2000;
	gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF+3] = gTimeInfo.mHour;
	gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF+4] = gTimeInfo.minute;
	gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF+5] = gTimeInfo.mSecond;

       // Log the updated gFinalAnaValF array
       QL_GPIODEMO_LOG("Sync_RTC_datetime: Updated gFinalAnaValF - Date: %02d, Month: %02d, Year: %04d, Time: %02d:%02d:%02d",
        (int)gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF + 0],
        (int)gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF + 1],
        (int)gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF + 2],
        (int)gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF + 3],
        (int)gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF + 4],
        (int)gFinalAnaValF[RTC_TIME_DATE_gFinalAnaValF + 5]);
}

uint8_t decToBcd(uint8_t val)
{
  return( (val/10*16) + (val%10) );
}

uint8_t bcdToDec(uint8_t val)
{
  return( (val/16*10) + (val%16) );
}

double DegreesToAngle(double degrees, double minutes, double seconds)
{
	if (degrees < 0)
		return ((double)(degrees - (minutes / 60.0) - (seconds / 3600.0)));
	else
		return ((double)(degrees + (minutes / 60.0) + (seconds / 3600.0)));
}

double Deg2Rad(double angle)
{
    return (PI * angle / 180.0);
}

double Rad2Deg(double angle)
{
    return (180.0 * angle / PI);
}

double FixValue(double value, double min, double max)
{
    while (value < min)
    {
        value += (max - min);
    }

    while (value >= max)
    {
        value -= (max - min);
    }

    return value;
}

int get_offset_UTC(void)
{
	int utcOffset_hour=0, utcOffset_min=0;
	//char temp_hour[3]={0},temp_min[3]={0};
	if(EPROM_General.Cust_Detail.Timezone_sign == 1)
	{
		flag_timezone_sign = 1;
	}
	else
	{
		flag_timezone_sign = 0;
	}

	utcOffset_hour = (int)(EPROM_General.Cust_Detail.Timezone_hours);
	utcOffset_min = (int)(EPROM_General.Cust_Detail.Timezone_minutes);
	return ((utcOffset_hour*3600)+(utcOffset_min*60));
}