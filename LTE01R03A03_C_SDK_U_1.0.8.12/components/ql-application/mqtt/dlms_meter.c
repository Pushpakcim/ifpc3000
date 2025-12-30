/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Cimcon

  @File Name
    DLMS_HPL.c
*/

/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
#include <stdint.h>
#include <math.h>
#include "dlms_meter.h"

#include "gpio_DIDO.h"
#include "ql_api_osi.h"
#include "ql_log.h"
#include "ql_uart.h"
#include "ql_gpio.h"
#include "ql_pin_cfg.h"
#include "ql_usb.h"
#include "i2c_RTC_LCD.h"
#include "ql_api_sim.h"
#include "json_parser_sp.h"
#include "configuration.h"
#include "mqtt_demo.h"
#include "rs232_UART.h"
#include "sms.h"

/* **************************************************************************/
/* define */
 /******************************************************************************/
#define QL_DLMS_LOG_LEVEL			QL_LOG_LEVEL_INFO
#define QL_DLMS_LOG(msg, ...)		QL_LOG(QL_DLMS_LOG_LEVEL, "ql_dlms", msg, ##__VA_ARGS__)

/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
extern uint16_t Rec_char;
char DLMS_Status;
union DLMSData myData;
uint8_t ReceiveTimer = 2;
uint16_t write_len = 0;
unsigned char Check_Count_lo[MAXANA_VAR],aTollo[MAXANA_VAR],aTolhi[MAXANA_VAR],Check_Count_hi[MAXANA_VAR],OldaTollo[MAXANA_VAR],OldaTolhi[MAXANA_VAR];

/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
void init_DLMS(void)
{
    unsigned char DLMS_buffer[128];
    const char* hex_data1 = "7EA00703415356A27E"; // Disconnect request
    const char* hex_data2 = "7EA0070341935A647E"; // Send SNRM request.
    const char* hex_data3 = "7EA04C0341106B04E6E600603EA1090607608574050801018A0207808B0760857405080201AC12801031313131313131313131313131313131BE10040E01000000065F1F0400621E5DFFFF64C77E"; // Send AARQ request.
    int length1 = 18;
    int length2 = 18;
    int length3 = 156;

    memset(&gFinalAnaValF[450], 0, sizeof(float)*9);    // Reset L1, L2, L3 - Vol, Curr, PF, KWh, F, W
    //DLMS- Send Disconnect request
    fillGprsBuffer(DLMS_buffer, hex_data1, length1);
    QL_DLMS_LOG("Send Disconnect request ");
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 9);
	QL_DLMS_LOG("write_len:%d", write_len);      
    ql_rtos_task_sleep_ms(500);
    QL_DLMS_LOG("REC_1:%d", Rec_char);
    
    if((Rec_char == 0)||(ProductionMode == 1))
    {
        DLMS_init_flag = 1;
        return;
    }
//    else
//	{
//    	if((cim_recv_buff[0] == 0x7F)&&(cim_recv_buff[1] == 0x7E)&&(cim_recv_buff[2] == 0x7F)&&(cim_recv_buff[3] == 0x7E))
//    	{
//    		ProductionMode = 1;
//            DLMS_init_flag = 1;
//            return;
//    	}
//	}
    Rec_char=0;  
    
    //DLMS- Send SNRM request
    fillGprsBuffer(DLMS_buffer, hex_data2, length2);
    QL_DLMS_LOG("Send SNRM request. ");
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 9);
	QL_DLMS_LOG("write_len:%d", write_len); 
    ql_rtos_task_sleep_ms(1000);      
    if(Rec_char == 0)
    {
        DLMS_init_flag = 1;
        DI_Final_value[9]=0;
        return;
    }
    Rec_char=0;  
    
    //DLMS- Send AARQ request
    fillGprsBuffer(DLMS_buffer, hex_data3, length3);
    QL_DLMS_LOG("Send AARQ request. ");
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 78);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(2000);
    // QL_DLMS_LOG("Rec_char=%d, cim_recv_buff=%s", Rec_char, cim_recv_buff);
    if(Rec_char < 10)
    {
        // QL_DLMS_LOG("Rec_char:%d", Rec_char);
        DLMS_init_flag = 1;
        DI_Final_value[9]=0;
    }    
    memset(cim_recv_buff, 0, 2048);
    Rec_char=0;
}

void DLMS_Get_data()
{   
    ReceiveTimer--;
	if(ReceiveTimer > 20)
    {
        ReceiveTimer = 5;
    }
    if(ReceiveTimer == 0)
    {
        QL_DLMS_LOG("ReceiveTimer:%d MeterType:%d  DLMS_Status:%d",ReceiveTimer, EPROM_General.MeterType, DLMS_Status);
    
        if(EPROM_General.MeterType == THREEPHASE)
        {
            QL_DLMS_LOG("Query No:%d", DLMS_Status);
            
            // First 8 Frame Start of 3Phase
            if(DLMS_Status == SEND_CURRENTL1)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL1;
                DLMS_Current_L1(); 
            }
            else if(DLMS_Status == SEND_VOLTAGEL1)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL2;
                DLMS_Voltage_L1();
            }
            else if(DLMS_Status == SEND_CURRENTL2)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL2;
                DLMS_Current_L2();
            }
            else if(DLMS_Status == SEND_VOLTAGEL2)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL3;
                DLMS_Voltage_L2();
            }
            else if(DLMS_Status == SEND_CURRENTL3)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL3;
                DLMS_Current_L3();
            }
            else if(DLMS_Status == SEND_VOLTAGEL3)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_POWER_FACTOR;
                DLMS_Voltage_L3();
            }
            else if(DLMS_Status == SEND_POWER_FACTOR)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_ACTIVE_POWER;
                DLMS_Power_Factor();
            }
            else if(DLMS_Status == SEND_ACTIVE_POWER)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL11;
                DLMS_Active_Power();
            }
            
            // Second 8 Frame start of 3Phase
            if(DLMS_Status == SEND_CURRENTL11)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL11;
                DLMS_Current_L1();
            }
            else if(DLMS_Status == SEND_VOLTAGEL11)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL22;
                DLMS_Voltage_L1();
            }
            else if(DLMS_Status == SEND_CURRENTL22)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL22;
                DLMS_Current_L2();
            }
            else if(DLMS_Status == SEND_VOLTAGEL22)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL33;
                DLMS_Voltage_L2();
            }
            else if(DLMS_Status == SEND_CURRENTL33)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL33;
                DLMS_Current_L3();
            }
            else if(DLMS_Status == SEND_VOLTAGEL33)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_FREQUECY;
                DLMS_Voltage_L3();
            }
            else if(DLMS_Status == SEND_FREQUECY)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CUM_ENERGY;
                DLMS_Frequency();
            }
            else if(DLMS_Status == SEND_CUM_ENERGY)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL1;
                DLMS_Cum_Energy();
            }
        }
        else if(EPROM_General.MeterType == SINGLEPHASE)
        {
            // 8 Frame Start of 1Phase
            if(DLMS_Status == SEND_CURRENTL_1PHASE)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL_1PHASE;
                DLMS_Current_1PHASE();
            }
            else if(DLMS_Status == SEND_VOLTAGEL_1PHASE)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_POWER_FACTOR_1PHASE;
                DLMS_Voltage_1PHASE();
            }
            else if(DLMS_Status == SEND_POWER_FACTOR_1PHASE)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_ACTIVE_POWER_1PHASE;
                DLMS_Power_Factor_1PHASE();
            }
            else if(DLMS_Status == SEND_ACTIVE_POWER_1PHASE)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_FREQUECY_1PHASE;
                DLMS_Active_Power_1PHASE();
            }
            else if(DLMS_Status == SEND_FREQUECY_1PHASE)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CUM_ENERGY_1PHASE;
                DLMS_Frequency_1PHASE();
            }
            else if(DLMS_Status == SEND_CUM_ENERGY_1PHASE)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL_1PHASE1;
                DLMS_Cum_Energy_1PHASE();
            }
            else if(DLMS_Status == SEND_CURRENTL_1PHASE1)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_VOLTAGEL_1PHASE1;
                DLMS_Current_1PHASE1();
                
            }
            else if(DLMS_Status == SEND_VOLTAGEL_1PHASE1)
            {
                ReceiveTimer = 1;
                DLMS_Status = SEND_CURRENTL_1PHASE;
                DLMS_Voltage_1PHASE1();
            }
        }
        memset(cim_recv_buff, 0, 2048);
        Rec_char = 0;
    }
}


//*******3 Phase Declaration******** */
void DLMS_Current_L1(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341523CDEE6E600C001C1000301001F0700FF0200DC797E";
    int length = 54;
    int result = 0;
    float CurrentL1 = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }    
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    CurrentL1 = ((float)result)/100; 
    gFinalAnaValF[450] = CurrentL1;
    QL_DLMS_LOG("result: %d , REC Current L1:%f ",result, CurrentL1);
    
    Rec_char=0;
}
void DLMS_Voltage_L1(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA019034174089AE6E600C001C100030100200700FF020085837E";
    int length = 54;
    int result = 0;
    float voltageL1 = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[3] = cim_recv_buff[16];
    myData.c[2] = cim_recv_buff[17];
    myData.c[1] = cim_recv_buff[18];
    myData.c[0] = cim_recv_buff[19];
    
    // Convert the string to an integer
    result = (myData.i);
    voltageL1 = ((float)result)/100; 
    gFinalAnaValF[451] = voltageL1;
    QL_DLMS_LOG("result: %d , REC Voltage L1:%f ",result, voltageL1);
    
    Rec_char=0;
}
void DLMS_Current_L2(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA019034196145EE6E600C001C100030100330700FF020048CD7E";
    int length = 54;
    int result = 0;
    float CurrentL2 = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    CurrentL2 = ((float)result)/100;
    gFinalAnaValF[452] = CurrentL2;
    QL_DLMS_LOG("result: %d , REC Current L2:%f ",result, CurrentL2);
    
    Rec_char=0;
}
void DLMS_Voltage_L2(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341B86896E6E600C001C100030100340700FF020099D17E";
    int length = 54;
    int result = 0;
    float voltageL2 = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }    
    myData.c[3] = cim_recv_buff[16];
    myData.c[2] = cim_recv_buff[17];
    myData.c[1] = cim_recv_buff[18];
    myData.c[0] = cim_recv_buff[19];
    
    // Convert the string to an integer
    result = (myData.i);
    voltageL2 = ((float)result)/100;
    gFinalAnaValF[453] = voltageL2;
    QL_DLMS_LOG("result: %d , REC Voltage L2:%f ",result, voltageL2);
    
    Rec_char=0;
}
void DLMS_Current_L3(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341DA7CD6E6E600C001C100030100470700FF0200E5187E";
    int length = 54;
    int result = 0;
    float CurrentL3 = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    CurrentL3 = ((float)result)/100;
    gFinalAnaValF[454] = CurrentL3;
    QL_DLMS_LOG("result: %d , REC Current L3:%f ",result, CurrentL3);
    
    Rec_char=0;
}
void DLMS_Voltage_L3(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341FC4892E6E600C001C100030100480700FF02006C257E";
    int length = 54;
    int result = 0;
    float voltageL3 = 0;

    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }    
    myData.c[3] = cim_recv_buff[16];
    myData.c[2] = cim_recv_buff[17];
    myData.c[1] = cim_recv_buff[18];
    myData.c[0] = cim_recv_buff[19];
    
    // Convert the string to an integer
    result = (myData.i);
    voltageL3 = ((float)result)/100;
    gFinalAnaValF[455] = voltageL3;
    QL_DLMS_LOG("result: %d , REC Voltage L3:%f ",result, voltageL3);
    
    Rec_char=0;
}
void DLMS_Power_Factor(void)
{
    unsigned char DLMS_buffer[30];
    const char* hex_data = "7EA01903411E5456E6E600C001C1000301000D0700FF02003A337E";
    
    int length = 54;
    float powerFactor = 0.0;
    int16 twos_complement = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }

    QL_DLMS_LOG("PF: %x %x", cim_recv_buff[16], cim_recv_buff[17]);

    twos_complement = ((cim_recv_buff[16] << 8) + cim_recv_buff[17]);
    powerFactor = ((float)twos_complement)/1000;
    gFinalAnaValF[456] = fabs(powerFactor);
    QL_DLMS_LOG("REC Power Factor:%f ",powerFactor);
    
    Rec_char=0;
}
void DLMS_Active_Power(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA019034130289EE6E600C001C100030100010700FF0200CE027E";
    int length = 54;
    int result = 0;
    float activeFactor = 0;

    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[3] = cim_recv_buff[16];
    myData.c[2] = cim_recv_buff[17];
    myData.c[1] = cim_recv_buff[18];
    myData.c[0] = cim_recv_buff[19];
    
    // Convert the string to an integer
    result = (myData.i);
    activeFactor = ((float)result)/1000;
    gFinalAnaValF[457] = activeFactor;
    QL_DLMS_LOG("result: %d , REC Active Power:%f ",result, activeFactor);
    
    Rec_char=0;
}
void DLMS_Frequency(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA01903411E5456E6E600C001C1000301000E0700FF0200473F7E";
    int length = 54;
    int result = 0;
    float Frequency = 0;

    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;

    // Convert the string to an integer
    result = (myData.i);
    Frequency = ((float)result)/100;
    gFinalAnaValF[458] = Frequency;
    QL_DLMS_LOG("result: %d , REC Frequency:%f ",result, Frequency);
    
    Rec_char=0;
}
void DLMS_Cum_Energy(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA019034130289EE6E600C001C100030100010800FF020032687E";
    int length = 54;
    int result = 0;
    float Cum_Energy = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[3] = cim_recv_buff[16];
    myData.c[2] = cim_recv_buff[17];
    myData.c[1] = cim_recv_buff[18];
    myData.c[0] = cim_recv_buff[19];

    // Convert the string to an integer
    result = (myData.i);
    Cum_Energy = ((float)result)/1000;
    gFinalAnaValF[459] = Cum_Energy;
    QL_DLMS_LOG("result: %d , REC Cum_Energy:%f ",result, Cum_Energy);
    
    Rec_char=0;
}
void fillGprsBuffer(unsigned char* gprs_buff, const char* hex_data, int length) {
    int i, j = 0;
    unsigned int byte = 0;

    for (i = 0; i < length; i += 2) 
    {
        sscanf(hex_data + i, "%2X", &byte);
        gprs_buff[j++] = (unsigned char)byte;
    }
}

/****************************************************** */
/******************1-PHASE DLMS************************ */
/****************************************************** */

void DLMS_Current_1PHASE(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341323ABDE6E600C001C1000301000B0700FF0200C02B7E";
    int length = 54;
    int result = 0;
    float CurrentL1 = 0;

    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    CurrentL1 = ((float)result)/100; 
    gFinalAnaValF[450] = CurrentL1;
    QL_DLMS_LOG("result: %d , REC Current:%f ",result, CurrentL1);
    
    Rec_char=0;
}

void DLMS_Voltage_1PHASE(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341540ABBE6E600C001C1000301000C0700FF020011377E";
    int length = 54;
    int result = 0;
    float voltage = 0;

    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    voltage = ((float)result)/10;
    gFinalAnaValF[451] = voltage;
    QL_DLMS_LOG("result: %d , REC Voltage:%f ",result, voltage);
    
    Rec_char=0;
}
void DLMS_Power_Factor_1PHASE(void)
{
    unsigned char DLMS_buffer[100], flagPowerFactor = 0;
    const char* hex_data = "7EA0190341761AB9E6E600C001C1000301000D0700FF02003A337E";
    int length = 54;
    int result;
    float powerFactor = 0;
    unsigned char ones_complement = 0, num =0;
    unsigned char twos_complement = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
#if 0
    myData.c[0] = cim_recv_buff[16];
    myData.c[1] = 0;
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    powerFactor = ((float)(result))/100;
    
    num = (char)cim_recv_buff[16];
    powerFactor = ((float)num)/100;
#endif

    num = cim_recv_buff[16];
    if(((num & 0x80) == 0x80) && (num != 0xFF))
    {
        ones_complement = ~cim_recv_buff[16];  // 1's complement
        twos_complement = ones_complement + 1;  // 2's complement
        flagPowerFactor = 1;
    }
    else
    {
        twos_complement = cim_recv_buff[16];
        flagPowerFactor = 0;
    }
    myData.c[1] = 0x00;
    myData.c[0] = twos_complement;//cim_recv_buff[16];
    myData.c[2] = 0x00;
    myData.c[3] = 0X00;
    
    result = (myData.i);
    powerFactor = ((float)result)/100;

    if(flagPowerFactor == 1)
    {
        powerFactor = powerFactor * (-1);
    }
    gFinalAnaValF[456] = fabs(powerFactor);
    QL_DLMS_LOG("result: %d , REC Power Factor:%f ",num, powerFactor);
    
    Rec_char=0;
}
void DLMS_Active_Power_1PHASE(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341986AB7E6E600C001C100030100010700FF0200CE027E";
    int length = 54;
    int result = 0;
    float activeFactor = 0;

    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[3] = cim_recv_buff[16];
    myData.c[2] = cim_recv_buff[17];
    myData.c[1] = cim_recv_buff[18];
    myData.c[0] = cim_recv_buff[19];
    
    // Convert the string to an integer
    result = (myData.i);
    activeFactor = ((float)result)/1000;
    gFinalAnaValF[457] = activeFactor;
    QL_DLMS_LOG("result: %d , REC Active Power:%f ",result, activeFactor);
    
    Rec_char=0;
}
void DLMS_Frequency_1PHASE(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341BA7AB5E6E600C001C1000301000E0700FF0200473F7E";
    int length = 54;
    int result = 0;
    float Frequency = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    Frequency = ((float)result)/10;
    gFinalAnaValF[458] = Frequency;
    QL_DLMS_LOG("result: %d , REC Frequency:%f ",result, Frequency);
    
    Rec_char=0;
}
void DLMS_Cum_Energy_1PHASE(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341DC4AB3E6E600C001C100030100010800FF020032687E";
    int length = 54;
    int result = 0;
    float Cum_Energy = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    QL_DLMS_LOG("Send Cum_Energy request. ");
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[3] = cim_recv_buff[16];
    myData.c[2] = cim_recv_buff[17];
    myData.c[1] = cim_recv_buff[18];
    myData.c[0] = cim_recv_buff[19];

    // Convert the string to an integer
    result = (myData.i);
    Cum_Energy = ((float)result)/100;
    gFinalAnaValF[459] = Cum_Energy;
    QL_DLMS_LOG("result: %d , REC Cum_Energy:%f ",result, Cum_Energy);
    
    Rec_char=0;
}

void DLMS_Current_1PHASE1(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341FE5AB1E6E600C001C1000301000B0700FF0200C02B7E";
    int length = 54;
    int result = 0;
    float CurrentL1 = 0;
   
    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    CurrentL1 = ((float)result)/100; 
    gFinalAnaValF[450] = CurrentL1;
    QL_DLMS_LOG("result: %d , REC Current:%f ",result, CurrentL1);
    
    Rec_char=0;
}
void DLMS_Voltage_1PHASE1(void)
{
    unsigned char DLMS_buffer[100];
    const char* hex_data = "7EA0190341102ABFE6E600C001C1000301000C0700FF020011377E";
    int length = 54;
    int result = 0;
    float voltage = 0;

    fillGprsBuffer(DLMS_buffer, hex_data, length);
    write_len = ql_uart_write(QL_UART_PORT_1, DLMS_buffer, 27);
	QL_DLMS_LOG("write_len:%d", write_len);
    ql_rtos_task_sleep_ms(500);
    if(Rec_char < 10)
    {
        DLMS_init_flag = 1;
    }
    myData.c[1] = cim_recv_buff[16];
    myData.c[0] = cim_recv_buff[17];
    myData.c[2] = 0;
    myData.c[3] = 0;
    
    // Convert the string to an integer
    result = (myData.i);
    voltage = ((float)result)/10;
    gFinalAnaValF[451] = voltage;
    QL_DLMS_LOG("result: %d , REC Voltage:%f ",result, voltage);
    
    Rec_char=0;
}

void Check_AnaLog_parameter(void)
{
	uint8_t i;
    /* Alarm Setting Details 
        EPROM_General.sp.status[]:
            1 = Only Low Voltage/Current/PF Alarm
            2 = only High Voltage/Current/PF Alarm
            3 = Both low and high voltage/current/PF Alarm
    */
    if(EPROM_General.MeterType == THREEPHASE)
    {
        for(i=0;i<7;i++)
        {
             uint16_t index;
            if(i <= 2) 
                index = 451 + 2*i;    // R,Y,B Vol                
            else if(i <= 5)
                index = 450 + 2*(i-3);    // Current
            else
                index = 456;    // PF (Power)
            if((EPROM_General.sp.status[i] == 1 || EPROM_General.sp.status[i] == 3) && (gFinalAnaValF[index] != 0)) //Under Voltage
            {
                if(gFinalAnaValF[index] < EPROM_General.sp.lo_value[i])
                {
                    Check_Count_lo[i]++;
                    if(Check_Count_lo[i] >=5)
                    {
                        aTollo[i] = 1;
                    }
                }
                if(gFinalAnaValF[index] >= (EPROM_General.sp.lo_value[i] + ((EPROM_General.sp.lo_value[i] * 10)/100))) // 10% tolerance
                {
                    aTollo[i] = 0;
                    Check_Count_lo[i] = 0;
                }
            }
            else
            {
                aTollo[i]=0;
                Check_Count_lo[i] = 0;
            }

            if((EPROM_General.sp.status[i] == 2 || EPROM_General.sp.status[i] == 3 ) && (gFinalAnaValF[index] != 0))
            {
                // if( (i == 0) || (i == 1) || (i == 3))
                {
                    if(gFinalAnaValF[index] > EPROM_General.sp.hi_value[i])
                    {
                        Check_Count_hi[i]++;
                        if(Check_Count_hi[i] >= 5)
                        {
                            aTolhi[i] = 1;
                        }
                    }

                    if(gFinalAnaValF[index] <= (EPROM_General.sp.hi_value[i] - ((EPROM_General.sp.hi_value[i] * 10)/100)))
                    {
                        aTolhi[i] = 0;	
                        Check_Count_hi[i] = 0;
                    }
                }
            }
            else
            {
                aTolhi[i]=0;
                Check_Count_hi[i] = 0;
            }
        }
    }
    else if(EPROM_General.MeterType == SINGLEPHASE)
    {
        for(i=0;i<7;i++)
        {
            uint16_t index;
            if(i == 0) 
                index = 451;    // Vol
            else if(i == 3)
                index = 450;    // Current
            else
                index = 456;    // PF
            if(i == 0 || i == 3 || i == 6)  // voltage, currnt, PF
            {
                if((EPROM_General.sp.status[i] == 1 || EPROM_General.sp.status[i] == 3) && gFinalAnaValF[index] != 0)       //LHB and LEB
                {                                  
                    if(gFinalAnaValF[index] < EPROM_General.sp.lo_value[i])
                    {
                        Check_Count_lo[i]++;
                        if(Check_Count_lo[i] >=5)
                        {
                            aTollo[i] = 1;
                        }
                    }
                    if(gFinalAnaValF[index] >= (EPROM_General.sp.lo_value[i] + ((EPROM_General.sp.lo_value[i] * 10)/100)))				
                    {
                        aTollo[i] = 0;
                        Check_Count_lo[i] = 0;
                    }
                }
                else
                {
                    aTollo[i]=0;
                    Check_Count_lo[i] = 0;
                }

                if((EPROM_General.sp.status[i] == 2 || EPROM_General.sp.status[i] == 3 ) && gFinalAnaValF[index] != 0)
                {
                    if(gFinalAnaValF[index] > EPROM_General.sp.hi_value[i])
                    {
                        Check_Count_hi[i]++;
                        if(Check_Count_hi[i] >= 5)
                        {
                            aTolhi[i] = 1;
                        }                      
                    }
                    if(gFinalAnaValF[index] <= (EPROM_General.sp.hi_value[i] - ((EPROM_General.sp.hi_value[i] * 10)/100)))
                    {
                        aTolhi[i] = 0;
                        Check_Count_hi[i] = 0;
                    }
                }
                else
                {
                    aTolhi[i]=0;
                    Check_Count_hi[i] = 0;
                }
            }
        }
    }
    /* It starts Timer-2 on Under voltage / current / PF, Over voltage / current / PF */
    for(i = 0 ; i < 11 ; ++i)
    {
        if(aTollo[i] != OldaTollo[i])
        {
            OldaTollo[i] = aTollo[i];
            RUN_Timer[2] = 1;
        }
        if(aTolhi[i] != OldaTolhi[i])
        {
            OldaTolhi[i] = aTolhi[i];
            RUN_Timer[2] = 1;
        }
    }

    QL_DLMS_LOG("aTollo:%d aTolhi:%d",aTollo[0],aTolhi[0]);

    for(uint8_t tDiNum = 0 ; tDiNum < EPROM_General.AI_DI_DO_Detail.Total_Di ; ++tDiNum)
    {
        if(DI_Final_value[tDiNum] != Old_DI_Final_value[tDiNum])
        {
            Old_DI_Final_value[tDiNum] = DI_Final_value[tDiNum];
            
            RUN_Timer[2] = 1;
            QL_DLMS_LOG("Run1");
            // tDIStatusChangeforSMS[tDiNum] = 1;
            if(0 == tDiNum || 2 == tDiNum || 3 == tDiNum || 4 == tDiNum || 15 == tDiNum)
            {
                // gSendDataOnDcuConChanged = 1;
                flagMqttPubLogData = 1;
                
            }

            if (15 == tDiNum)
            {
                SMSAlarmType = 4;
            }
            if (3 == tDiNum)
            {
                SMSAlarmType = 3;
            }
            if(4 == tDiNum )
            {
                SMSAlarmType = 2;
            }
            if(6 == tDiNum || 8 == tDiNum)
            {
                SMSAlarmType = 1;
            }
            
            // SaveToRunParameter();
        }
    }
    if(RUN_Timer[2] != 0)  
    {
        if(RUN_Timer[2] > EPROM_General.Def_timer[2])
        {
            QL_DLMS_LOG("Data Send timer expired");
            // DebugInfo("\n Flag1");
            RUN_Timer[2] = 0;

            DI_Final_value[17] = aTollo[0];     // R Vol
            DI_Final_value[16] = aTolhi[0];

            DI_Final_value[19] = aTollo[1];
            DI_Final_value[18] = aTolhi[1];

            DI_Final_value[21] = aTollo[2];
            DI_Final_value[20] = aTolhi[2];

            DI_Final_value[23] = aTollo[3];     // IR 
            DI_Final_value[22] = aTolhi[3];

            DI_Final_value[25] = aTollo[4];
            DI_Final_value[24] = aTolhi[4];

            DI_Final_value[27] = aTollo[5];
            DI_Final_value[26] = aTolhi[5];
               
             DI_Final_value[29] = aTollo[6];     //PF
             DI_Final_value[28] = aTolhi[6];
           
        //    flagMqttPubLogData = 1;    //  check 
           // SMSAlarmType = 1;
            // GVar.DataSendignSMS = 1;
            // SaveToRunParameter();
        }
    }
}
/* *****************************************************************************
 End of File
 */
