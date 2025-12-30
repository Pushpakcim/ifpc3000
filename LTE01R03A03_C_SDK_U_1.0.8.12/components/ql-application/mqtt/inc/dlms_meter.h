/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Cimcon

  @File Name
    dlsms_meter.h
 */
/* ************************************************************************** */

#ifndef _DLMS_METER_H    /* Guard against multiple inclusion */
#define _DLMS_METER_H

/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */

 /* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
#define THREEPHASE			3
#define SINGLEPHASE		  1
#define NOPHASE				  0
#define NOOFCKT         1       // ckt 1 or 2 

#define METER_HPL_DLMS      0
#define METER_HPL_NORMAL    1

#define SEND_INIT           0
#define SEND_CURRENTL1 		  1
#define SEND_VOLTAGEL1 		  2
#define SEND_CURRENTL2      3
#define SEND_VOLTAGEL2      4
#define SEND_CURRENTL3      5
#define SEND_VOLTAGEL3      6
#define SEND_POWER_FACTOR   7
#define SEND_ACTIVE_POWER   8
#define SEND_CURRENTL11     9
#define SEND_VOLTAGEL11     10
#define SEND_CURRENTL22     11
#define SEND_VOLTAGEL22     12
#define SEND_CURRENTL33     13
#define SEND_VOLTAGEL33     14
#define SEND_FREQUECY       15
#define SEND_CUM_ENERGY     16

#define SEND_CURRENTL_1PHASE 		  1
#define SEND_VOLTAGEL_1PHASE 		  2
#define SEND_POWER_FACTOR_1PHASE  3
#define SEND_ACTIVE_POWER_1PHASE  4	
#define SEND_FREQUECY_1PHASE      5
#define SEND_CUM_ENERGY_1PHASE    6
#define SEND_CURRENTL_1PHASE1		  7
#define SEND_VOLTAGEL_1PHASE1		  8


/* ************************************************************************** */
// Section: Data Types
/* ************************************************************************** */
union DLMSData {
    uint8_t c[4];
    int8_t c_char[4];
    uint32_t i;
    int32_t i_int;
    float f;
};

extern uint8_t cim_recv_buff[];
extern uint8_t DLMS_init_flag;
extern char DLMS_Status;
extern uint8_t ReceiveTimer;

/* ************************************************************************** */
// Section: Interface Functions
/* ************************************************************************** */
void init_DLMS(void);
void fillGprsBuffer(unsigned char* gprs_buff2, const char* hex_data, int length);
void DLMS_Get_data(void);

// 3-Phase Declaration
void DLMS_Current_L1(void);
void DLMS_Voltage_L1(void);
void DLMS_Current_L2(void);
void DLMS_Voltage_L2(void);
void DLMS_Current_L3(void);
void DLMS_Voltage_L3(void);
void DLMS_Power_Factor(void);
void DLMS_Active_Power(void);
void DLMS_Frequency(void);
void DLMS_Cum_Energy(void);

// 1-Phase Declaration
void DLMS_Voltage_1PHASE(void);
void DLMS_Current_1PHASE(void);
void DLMS_Power_Factor_1PHASE(void);
void DLMS_Active_Power_1PHASE(void);
void DLMS_Frequency_1PHASE(void);
void DLMS_Cum_Energy_1PHASE(void);
void DLMS_Voltage_1PHASE1(void);
void DLMS_Current_1PHASE1(void);
void Check_AnaLog_parameter(void);

#endif /* _DLMS_METER_H */

/* *****************************************************************************
 End of File
 */
