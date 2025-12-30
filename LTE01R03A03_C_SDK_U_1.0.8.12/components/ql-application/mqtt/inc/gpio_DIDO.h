/*=================================================================

						EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN			  WHO		  WHAT, WHERE, WHY
------------	 -------	 -------------------------------------------------------------------------------

=================================================================*/


#ifndef _GPIODEMO_H
#define _GPIODEMO_H

#include "ql_gpio.h"
#include "define.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Macro Definition
 ===========================================================================*/

#define TURNON    1
#define TURNOFF   0
#define MYSCH     4 
/*===========================================================================
 * Struct
 ===========================================================================*/
typedef struct
{
    ql_GpioNum      gpio_num;
    ql_GpioDir      gpio_dir;
    ql_PullMode     gpio_pull;    //for input only
    ql_LvlMode      gpio_lvl;     //for output only
} ql_gpio_cfg;

/** FlagStatus, ITStatus   */
typedef enum
{
  RESET = 0,
  SET = !RESET
} FlagStatus, ITStatus;

extern uint8_t DO_Final_value[MAX_DO_CHANNEL];
extern uint8_t DI_Final_value[MAX_DI_CHANNEL];
extern uint8_t Old_DI_Final_value[MAX_DI_CHANNEL];
extern uint16_t RUN_Timer[MAXMTR];
extern char Schedule[5];
/*===========================================================================
 * Functions declaration
 ===========================================================================*/
void ql_gpio_app_dido_init(void);
void _ql_gpio_dido_init(void);
void ScanDI(void);
void UpdateRunTimer(void);
void check_timer_DO_onoff_Mode(void);
void ManualModeOperations(void);
void ScheduleModeOperations(void);
void AstroModeOperations(void);
void Sync_RTC_datetime(void) ;

uint32_t Calculate_SunsetSunrise(unsigned char direction);//, struct tm fine_time);
double DegreesToAngle(double degrees, double minutes, double seconds);
double Deg2Rad(double angle);
double Rad2Deg(double angle);
double FixValue(double value, double min, double max);
void Get_Astro_time(void);
int get_offset_UTC(void);
uint8_t bcdToDec(uint8_t val);
double DegreesToAngle(double degrees, double minutes, double seconds);

#ifdef __cplusplus
} /*"C" */
#endif

#endif /* _GPIODEMO_H */


