/*
 * Configuration.c
 *
 *  Created on: Jan 9, 2023
 *      Author: Shreyanss 
 updated BY : Pushpak in 2025
 */

/**************************************************************************//**
 * Includes
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Configuration.h"
#include "ql_log.h"
#include "spi_flash_at25ff.h"
#include "define.h"
#include "json_parser_sp.h"
#include "dlms_meter.h"
#include "OTA.h"
#include "gpio_int_KEY.h"
#include "gpio_DIDO.h"

/**************************************************************************//**
 * define
 *****************************************************************************/
#define QL_CONFIG_LOG_LEVEL				QL_LOG_LEVEL_INFO
#define QL_CONFIG_LOG(msg, ...)			QL_LOG(QL_CONFIG_LOG_LEVEL, "ql_flash_config", msg, ##__VA_ARGS__)
#define QL_CONFIG_LOG_PUSH(msg, ...)	QL_LOG_PUSH("ql_flash_config", msg, ##__VA_ARGS__)


/**************************************************************************//**
 * extern
 *****************************************************************************/

/**************************************************************************//**
 * Global variables 
 *****************************************************************************/
struct Save_Para_General EPROM_General;
struct Save_Para_Schedule_Configuration EPROM_Schedule;
struct Save_Para_PermanentData EPROM_PermanentData;
struct Configuration Config;
unsigned char PDP_Context_APN[30] = "www";

uint8_t flag_flashUpdateEPROM_General = 1;
uint8_t flag_flashUpdateEPROM_General_WaitCounter = 5;
uint8_t flag_flashUpdateEPROM_Schedule = 1;
uint8_t flag_flashUpdateEPROM_Schedule_WaitCounter = 5;
uint8_t flag_flashUpdateEPROM_PermanentData = 1;
uint8_t flag_flashUpdateEPROM_PermanentData_WaitCounter = 5;

int32_t Fream_id;
uint8_t print[256];

unsigned char pro_MQTT_Broker_IP[30]="203.88.128.141";
unsigned int pro_MQTT_Broker_Port = 1884;
unsigned char pro_MQTT_Client_ID[30]="Production_Test";
unsigned char pro_APN[30]="airtelgprs.com";
unsigned char Pro_Application_flag=0,pro_DO_DI_TestFinish = 0;
unsigned char pro_RS232_1_state=0,pro_RS485_1_state=0;
unsigned char pro_DO_State[3],pro_DI_State[6];
unsigned char DO_Opration = 0,DO_Opration_count = 0;
unsigned char pro_Flash_State=0,pro_I2C1_State=0,pro_I2C2_State=0;
unsigned short int proTestRequest=0;
/**************************************************************************//**
 * Functions
 *****************************************************************************/
void WriteLog(uint8_t LogEnable,const char *pData,uint8_t logType)
{
    QL_CONFIG_LOG("%s",pData);
}

unsigned char calculateCheckSumOfStruct(unsigned char *structAddress,uint16_t sizeofstruct)
{
	unsigned char sum=0;
	unsigned char *cksum_ptr=structAddress;
	for(int i=4;i<sizeofstruct;i++)
	{
		sum+=cksum_ptr[i];
	}
	return 255-sum;
}

void ExtFlash_Read_EPROM_General(unsigned char makeDefault)
{
	unsigned char Set_default_Flash=0,i=0;
    at25dfx_read((uint8_t*) &EPROM_General, sizeof(EPROM_General), EPROM_GENERAL_START_ADDRESS);
    
	sprintf((char *)print,"ExtFlash_Read_EPROM_General:Mode+:%d, Meter_type: %d,\r\n",EPROM_General.DoModeDetails.Do_Mode,EPROM_General.AI_DI_DO_Detail.Total_Do);
	//MeterType
	WriteLog(1, (char *)print, 1);

	if((EPROM_General.checkbyte != 0xAB)||(EPROM_General.SizeOfStuct==0xFFFF))
	{
		Set_default_Flash=1;
		sprintf((char *)print,"ExtFlash_Read_EPROM_General:EPROM_General.checkbyte:%d, %d\r\n",EPROM_General.checkbyte,EPROM_General.SizeOfStuct);
		WriteLog(1, (char *)print, 1);
	}
	else if(EPROM_General.SizeOfStuct<sizeof(EPROM_General))
	{
		if(EPROM_General.ChecksumOfStuct==calculateCheckSumOfStruct((unsigned char*)&EPROM_General,EPROM_General.SizeOfStuct))
		{
			Set_default_Flash=0;
		}
		else
		{
			Set_default_Flash=1;

			sprintf((char *)print,"Stuct Size Change CRC mismatch : %d,%d,%d\r\n",EPROM_General.checkbyte,EPROM_General.SizeOfStuct,sizeof(EPROM_General));
			WriteLog(1, (char *)print, 1);
		}
	}
	else
	{
		if(EPROM_General.ChecksumOfStuct==calculateCheckSumOfStruct((unsigned char*)&EPROM_General,sizeof(EPROM_General)))
		{
			Set_default_Flash=0;
		}
		else
		{
			Set_default_Flash=1;
			sprintf((char *)print,"same size but CRC mismatch : %d,%d,%d\r\n",EPROM_General.checkbyte,EPROM_General.SizeOfStuct,sizeof(EPROM_General));
			WriteLog(1, (char *)print, 1);
		}
	}
	//Set_default_Flash = 1; // To Eprom Save    //flash erase
	if((Set_default_Flash==1)||(makeDefault == 1))
	{
		sprintf((char *)print,"ExtFlash_Read_EPROM_General: Defaulting\r\n");
		WriteLog(1, (char *)print, 1);
		EPROM_General.checkbyte = 0xAB;
		EPROM_General.ChecksumOfStuct = calculateCheckSumOfStruct((unsigned char*)&EPROM_General,sizeof(EPROM_General));
		EPROM_General.SizeOfStuct = sizeof(EPROM_General);
		memset(EPROM_General.forFutureUse1,0,sizeof(EPROM_General.forFutureUse1));
		EPROM_General.rebootCount = 0;
		EPROM_General.LogRate = DEFAULT_LOGRATE;		// Min
		EPROM_General.History_En_Di = CONF_ENABLE;

		strcpy((char *)EPROM_General.Rtu_Detail.HW_Version,DEFAULT_HW_VERSION);
		strcpy((char *)EPROM_General.Rtu_Detail.Hex_Version,DEFAULT_FV_VERSION);
		strcpy((char *)EPROM_General.Rtu_Detail.PLC_Version,DEFAULT_PLC_VERSION);
		strcpy((char *)EPROM_General.Rtu_Detail.REC_Version,DEFAULT_REC_VERSION);
		EPROM_General.Rtu_Detail.RTUId = DEFAULT_RTUID; 
		memset(EPROM_General.Rtu_Detail.forFutureUse,0,sizeof(EPROM_General.Rtu_Detail.forFutureUse));

		strcpy((char *)EPROM_General.Cust_Detail.Proj_Code,DEFAULT_PROJECT_CODE);
		strcpy((char *)EPROM_General.Cust_Detail.Site_Name,DEFAULT_SITE_NAME);
		strcpy((char *)EPROM_General.Cust_Detail.Time_zone,DEFAULT_TIME_ZONE);
		EPROM_General.Cust_Detail.Timezone_sign = DEFAULT_TIME_ZONE_SIGN;
		EPROM_General.Cust_Detail.Timezone_hours = DEFAULT_TIME_ZONE_HOURS;
		EPROM_General.Cust_Detail.Timezone_minutes = DEFAULT_TIME_ZONE_MINUTES;

		EPROM_General.Cust_Detail.reboot_day_night = DEFAULT_DAY_NIGHT_REBOOT;
		EPROM_General.Cust_Detail.Client_Id = DEFAULT_CLIENTID;
		EPROM_General.Cust_Detail.Reader_Id = DEFAULT_READERID;
		EPROM_General.Cust_Detail.Lattitude = DEFAULT_LAT;
		EPROM_General.Cust_Detail.Longitude = DEFAULT_LOGITUDE;
		EPROM_General.Cust_Detail.Offset_Value = DEFAULT_OFFSET_VALUE;
		memset(EPROM_General.Cust_Detail.forFutureUse,0,sizeof(EPROM_General.Cust_Detail.forFutureUse));

		EPROM_General.AI_DI_DO_Detail.Total_Di = MAX_DI_CHANNEL;
		EPROM_General.AI_DI_DO_Detail.Total_Do = MAX_DO_CHANNEL;
		memset(EPROM_General.AI_DI_DO_Detail.forFutureUse,0,sizeof(EPROM_General.AI_DI_DO_Detail.forFutureUse));

		EPROM_General.E_Comm.E_Co_En_Di = DEFAULT_ETHERNET_ENABLE;
		EPROM_General.E_Comm.E_Mode = DEFAULT_ETHERNET_DHCP_ENABLE;
		EPROM_General.E_Comm.E_Mod_TCP = DEFAULT_ETHERNET_MODBUS_TCP_ENABLE;
		EPROM_General.E_Comm.E_Ser_cli = DEFAULT_ETHERNET_MODBUS_TCP_SER_CLIENT;
		EPROM_General.E_Comm.E_IP_Add[0] = DEFAULT_ETHERNET_IP_0;
		EPROM_General.E_Comm.E_IP_Add[1] = DEFAULT_ETHERNET_IP_1;
		EPROM_General.E_Comm.E_IP_Add[2] = DEFAULT_ETHERNET_IP_2;
		EPROM_General.E_Comm.E_IP_Add[3] = DEFAULT_ETHERNET_IP_3;
		EPROM_General.E_Comm.E_Subnet_Add[0] = DEFAULT_ETHERNET_SUBNET_0;
		EPROM_General.E_Comm.E_Subnet_Add[1] = DEFAULT_ETHERNET_SUBNET_1;
		EPROM_General.E_Comm.E_Subnet_Add[2] = DEFAULT_ETHERNET_SUBNET_2;
		EPROM_General.E_Comm.E_Subnet_Add[3] = DEFAULT_ETHERNET_SUBNET_3;
		EPROM_General.E_Comm.E_Gateway_Add[0] = DEFAULT_ETHERNET_GATEWAY_0;
		EPROM_General.E_Comm.E_Gateway_Add[1] = DEFAULT_ETHERNET_GATEWAY_1;
		EPROM_General.E_Comm.E_Gateway_Add[2] = DEFAULT_ETHERNET_GATEWAY_2;
		EPROM_General.E_Comm.E_Gateway_Add[3] = DEFAULT_ETHERNET_GATEWAY_3;
		EPROM_General.E_Comm.E_Preferred_DNS[0] = DEFAULT_ETHERNET_DNS1_0;
		EPROM_General.E_Comm.E_Preferred_DNS[1] = DEFAULT_ETHERNET_DNS1_1;
		EPROM_General.E_Comm.E_Preferred_DNS[2] = DEFAULT_ETHERNET_DNS1_2;
		EPROM_General.E_Comm.E_Preferred_DNS[3] = DEFAULT_ETHERNET_DNS1_3;
		EPROM_General.E_Comm.E_Alternate_DNS[0] = DEFAULT_ETHERNET_DNS2_0;
		EPROM_General.E_Comm.E_Alternate_DNS[1] = DEFAULT_ETHERNET_DNS2_1;
		EPROM_General.E_Comm.E_Alternate_DNS[2] = DEFAULT_ETHERNET_DNS2_2;
		EPROM_General.E_Comm.E_Alternate_DNS[3] = DEFAULT_ETHERNET_DNS2_3;
		EPROM_General.E_Comm.E_TCP_Port = DEFAULT_ETHERNET_MODBUS_TCP_PORT;
		EPROM_General.E_Comm.E_Poll_Freq = DEFAULT_ETHERNET_MODBUS_TCP_POLL_FRQ;
		memset(EPROM_General.E_Comm.forFutureUse,0,sizeof(EPROM_General.E_Comm.forFutureUse));

		EPROM_General.Mo_Comm.Mo_Co_En_Di = DEFAULT_MODEM_ENABLE;
		EPROM_General.Mo_Comm.Mo_Com_Int = DEFAULT_MODEM_SERIAL_USB;
		EPROM_General.Mo_Comm.Mo_Proto = DEFAULT_MODEM_PROTOCOL;
		strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP,DEFAULT_MODEM_MQTT_BROKER_IP);
		EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port = DEFAULT_MODEM_MQTT_BROKER_PORT;
		EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Comm_Mode = DEFAULT_MQTT_COMM_MODE;
		strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Name,DEFAULT_MODEM_MQTT_USR_NAME);
		strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Pass,DEFAULT_MODEM_MQTT_USR_PASS);
		strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Cli_Id,DEFAULT_MODEM_MQTT_CLIENTID);
		strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_PUB_Topic,DEFAULT_MODEM_MQTT_PUB_TOPIC);
		strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Sub_Topic,DEFAULT_MODEM_MQTT_SUB_TOPIC);
		memset(EPROM_General.Mo_Comm.MQTT_Conn.forFutureUse,0,sizeof(EPROM_General.Mo_Comm.MQTT_Conn.forFutureUse));
		strcpy((char *)EPROM_General.Mo_Comm.Mo_APN,DEFAULT_MODEM_APN);
		EPROM_General.Mo_Comm.MQTT_LiveFreq = DEFAULT_MODEM_MQTT_LIVE_FRQ;
		memcpy((char*)EPROM_General.Mo_Comm.mobMCS[0], DEFAULT_MODEM_MOB_MCS,14);
		memcpy((char*)EPROM_General.Mo_Comm.mobMCS[1], "+919111111112",14);
		memcpy((char*)EPROM_General.Mo_Comm.mobMCS[2], "+919111111113",14);
		memcpy((char*)EPROM_General.Mo_Comm.mobMCS[3], "+919111111114",14);

		EPROM_General.bleDetails.BLE_Co_En_Di = DEFAULT_BLE_ENABLE;
		EPROM_General.bleDetails.BLE_MAC_Add[0] = DEFAULT_BLE_MAC_0;
		EPROM_General.bleDetails.BLE_MAC_Add[1] = DEFAULT_BLE_MAC_1;
		EPROM_General.bleDetails.BLE_MAC_Add[2] = DEFAULT_BLE_MAC_2;
		EPROM_General.bleDetails.BLE_MAC_Add[3] = DEFAULT_BLE_MAC_3;
		EPROM_General.bleDetails.BLE_MAC_Add[4] = DEFAULT_BLE_MAC_4;
		EPROM_General.bleDetails.BLE_MAC_Add[5] = DEFAULT_BLE_MAC_5;
		memset(EPROM_General.bleDetails.forFutureUse,0,sizeof(EPROM_General.bleDetails.forFutureUse));

		EPROM_General.gpsDetails.GPS_Co_En_Di = DEFAULT_GPS_ENABLE;
		EPROM_General.gpsDetails.GPS_Poll_Freq = DEFAULT_GPS_POLL_FRQ;
		memset(EPROM_General.gpsDetails.forFutureUse,0,sizeof(EPROM_General.gpsDetails.forFutureUse));

		EPROM_General.sdCardDetails.SD_Card_Co_En_Di = DEFAULT_SD_ENABLE;
		EPROM_General.sdCardDetails.SD_Card_Size = DEFAULT_SD_SIZE;
		memset(EPROM_General.sdCardDetails.forFutureUse,0,sizeof(EPROM_General.sdCardDetails.forFutureUse));


		EPROM_General.DoModeDetails.Do_Mode = DEFAULT_DO_MODE;
		i=0;
		for(i=0;i<35;i++)
		{
			EPROM_General.DoModeDetails.DO_Value[i] = DEFAULT_DO_OFF;
		}
		EPROM_General.DoModeDetails.Do_Mode_temp = DEFAULT_DO_MODE;
		
		memset(EPROM_General.DoModeDetails.forFutureUse,0,sizeof(EPROM_General.DoModeDetails.forFutureUse));
		memset(EPROM_General.DeviceID,0,sizeof(EPROM_General.DeviceID));

		for(uint8_t i=0;i<MAX_DI_CHANNEL;i++)
		{
			EPROM_General.AI_DI_DO_Detail.Old_dig_bit_array[i] = 0;
			EPROM_General.AI_DI_DO_Detail.dig_bit_array[i] = 0;
		}

		EPROM_General.MeterMake = METER_HPL_DLMS;
		EPROM_General.MeterType = SINGLEPHASE;				 // SINGLEPHASE , THREEPHASE  
		EPROM_General.NoofCkt = NOOFCKT;

		for(uint8_t i=0;i<MAX_DO_CHANNEL;i++) 
		{
			//if(EPROM_General.automanualstatus != RTU_DO_MODE_MANUAL)
			{
				EPROM_General.AI_DI_DO_Detail.OLDDOSignal[i] = 0;
				EPROM_General.AI_DI_DO_Detail.DOSignal[i] = 0;
			}
			// EPROM_General.AI_DI_DO_Detail.OLDDOSignal[i]=0;
		}

    	EPROM_General.Def_timer[0]=1;
    	EPROM_General.Def_timer[1]=400;
    	EPROM_General.Def_timer[2]=15;
    	EPROM_General.Def_timer[3]=5;
    	EPROM_General.Def_timer[4]=15;
		EPROM_General.Def_timer[5]=10;
    	EPROM_General.Def_timer[6]=2;
    	EPROM_General.Def_timer[7]=2;
    	EPROM_General.Def_timer[8]=10;

		EPROM_General.DataHourlyStored = 0;
		EPROM_General.EventDayStored = 0;
		EPROM_General.mIsInterlockEnable = 0;
		
		for(uint8_t i=0;i<11;i++)
		{
			EPROM_General.sp.status[i]=0;
			EPROM_General.sp.hi_value[i]=0.0;
			EPROM_General.sp.lo_value[i]=0.0;
		}
    	EPROM_General.sp.status[0]=3;   EPROM_General.sp.hi_value[0]=270.0;  EPROM_General.sp.lo_value[0]=180.0;
    	EPROM_General.sp.status[1]=3;   EPROM_General.sp.hi_value[1]=270.0;  EPROM_General.sp.lo_value[1]=180.0;
    	
		EPROM_General.sp.status[2]=3;   EPROM_General.sp.hi_value[2]=270.0;  EPROM_General.sp.lo_value[2]=180.0;
    	EPROM_General.sp.status[3]=3;   EPROM_General.sp.hi_value[3]=30.0;   EPROM_General.sp.lo_value[3]=0.0;
    	
		EPROM_General.sp.status[4]=3;   EPROM_General.sp.hi_value[4]=30.0;   EPROM_General.sp.lo_value[4]=0.0;
    	EPROM_General.sp.status[5]=3;   EPROM_General.sp.hi_value[5]=30.0;   EPROM_General.sp.lo_value[5]=0.0;

		EPROM_General.sp.status[6]=3;   EPROM_General.sp.hi_value[6]=0.00;   EPROM_General.sp.lo_value[6]=0.8;

		EPROM_General.sp.hi_value[7] = 000.00;		// Total expected load in R1 output.
		EPROM_General.sp.lo_value[7] = 000;			// Number of bulb connected in R1 output line.

		EPROM_General.sp.hi_value[8] = 000.00;		// Total expected load in Y1 output.
		EPROM_General.sp.lo_value[8] = 000;			// Number of bulb connected in Y1 output line.

		EPROM_General.sp.hi_value[9] = 000.00;		// Total expected load in B2 output.
		EPROM_General.sp.lo_value[9] = 000;			// Number of bulb connected in B1 output line.
		

		EPROM_General.MaxofSMS = 26;
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
				EPROM_General.ModSMSList[18]= 1363; // Start Hour
				EPROM_General.ModSMSList[19]= 1365; // start Minute
				EPROM_General.ModSMSList[20]= 1357; // Stop Hour
				EPROM_General.ModSMSList[21]= 1359; // Stop Minute
				EPROM_General.ModSMSList[22]= 1213;   // Lograte
				EPROM_General.ModSMSList[23]= 1321;   // LAT
				EPROM_General.ModSMSList[24]= 1323;   // LONG
				EPROM_General.ModSMSList[25]= 1225;   // Meter Type

				for(uint8_t i=26;i<EPROM_General.MaxofSMS;i++)
				{
					EPROM_General.ModSMSList[i]= 0;
				}
			}
        }
		EPROM_General.flashOTA = 0;
		EPROM_General.pro_CheckByte = 0;
		ExtFlash_update_EPROM_General();
	}
	else
	{
		EPROM_General.rebootCount++;
		gFinalAnaValF[REBOOT_COUNT_gFinalAnaValF]=EPROM_General.rebootCount;
		sprintf((char *)print,"EPROM_General.rebootCount : %d,%f,%d\r\n",EPROM_General.rebootCount,gFinalAnaValF[REBOOT_COUNT_gFinalAnaValF],sizeof(EPROM_General));
		WriteLog(1, (char *)print, 1);
		//if(xSemaphoreTake(sendExternalFlashSemaphore, 500) == pdTRUE )
		//{
		//	ExtFlash_update_EPROM_General();
		//	//xSemaphoreGive(sendExternalFlashSemaphore);
		//}
		if(EPROM_General.flashOTA == 1)
		{
			EPROM_General.flashOTA = 0;
			ql_fota_http_app_init1();
		}
		if(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Comm_Mode < 0 || EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Comm_Mode > 1)
		{
			EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Comm_Mode = 0;
		}

		if(EPROM_General.Cust_Detail.Timezone_sign < 0 || EPROM_General.Cust_Detail.Timezone_sign > 1)
		{
			EPROM_General.Cust_Detail.Timezone_sign = 0;
		}

		if(EPROM_General.Cust_Detail.reboot_day_night < 0 || EPROM_General.Cust_Detail.reboot_day_night > 3)
		{
			EPROM_General.Cust_Detail.reboot_day_night = 0;
		}

		if(EPROM_General.Cust_Detail.Timezone_hours < 0 || EPROM_General.Cust_Detail.Timezone_hours > 23)
		{
			EPROM_General.Cust_Detail.Timezone_hours = 5;    
		}

		if(EPROM_General.Cust_Detail.Timezone_minutes < 0 || EPROM_General.Cust_Detail.Timezone_minutes > 60)
		{
			EPROM_General.Cust_Detail.Timezone_minutes = 30;
		}


		flag_flashUpdateEPROM_General = 1;
		flag_flashUpdateEPROM_General_WaitCounter = 10;
	}
}

void ExtFlash_update_EPROM_General(void)
{
	struct Save_Para_General tempEPROM_General;
	memcpy(&tempEPROM_General, &EPROM_General, sizeof(EPROM_General));
	sprintf((char *)print, "ExtFlash_update_EPROM_General:\r\n");
	WriteLog(1, (char *)print, 1);
	tempEPROM_General.ChecksumOfStuct = calculateCheckSumOfStruct((unsigned char*)&tempEPROM_General,sizeof(tempEPROM_General));
	tempEPROM_General.SizeOfStuct = sizeof(tempEPROM_General);
	at25dfx_write((uint8_t*)&tempEPROM_General,sizeof(tempEPROM_General), EPROM_GENERAL_START_ADDRESS);
}

void ExtFlash_Read_EPROM_Schedule(unsigned char makeDefault)
{
	unsigned char Set_default_Flash=0,i;
	at25dfx_read((uint8_t*) &EPROM_Schedule, sizeof(EPROM_Schedule), EPROM_SCHEDULE_START_ADDRESS);

	if((EPROM_Schedule.checkbyte != 0xAB)||(EPROM_Schedule.SizeOfStuct==0xFFFF))
	{
		Set_default_Flash=1;
	    sprintf((char *)print, "ExtFlash_Read_EPROM_Schedule:checkbyte default\r\n");
	    WriteLog(1, (char *)print, 1);
	}
	else if(EPROM_Schedule.SizeOfStuct<sizeof(EPROM_Schedule))
	{
		if(EPROM_Schedule.ChecksumOfStuct==calculateCheckSumOfStruct((unsigned char*)&EPROM_Schedule,EPROM_Schedule.SizeOfStuct))
		{
			Set_default_Flash=0;
		}
		else
		{
			Set_default_Flash=1;
		    sprintf((char *)print, "ExtFlash_Read_EPROM_Schedule: Stuct Size Change CRC mismatch: %d, %d, %d\r\n",EPROM_Schedule.checkbyte,EPROM_Schedule.SizeOfStuct,sizeof(EPROM_Schedule));
		    WriteLog(1, (char *)print, 1);
		}
	}
	else
	{
		if(EPROM_Schedule.ChecksumOfStuct==calculateCheckSumOfStruct((unsigned char*)&EPROM_Schedule,sizeof(EPROM_Schedule)))
		{
			Set_default_Flash=0;
		}
		else
		{
			Set_default_Flash=1;
		    sprintf((char *)print, "ExtFlash_Read_EPROM_Schedule:size same but CRC mitchmatch: %d, %d, %d\r\n",EPROM_Schedule.checkbyte,EPROM_Schedule.SizeOfStuct,sizeof(EPROM_Schedule));
		    WriteLog(1, (char *)print, 1);
		}
	}
	// Set_default_Flash = 1;
	if((Set_default_Flash==1)||(makeDefault == 1))
	{
		EPROM_Schedule.checkbyte = 0xAB;
		EPROM_Schedule.ChecksumOfStuct = calculateCheckSumOfStruct((unsigned char*)&EPROM_Schedule,sizeof(EPROM_Schedule));
		EPROM_Schedule.SizeOfStuct = sizeof(EPROM_Schedule);
		memset(EPROM_Schedule.forFutureUse1,0,sizeof(EPROM_Schedule.forFutureUse1));

		EPROM_Schedule.Total_No_Schedule = DEFAULT_SCHEDULE_TOTAL_NO;

		for(i=0;i<84;i++)
		{
			EPROM_Schedule.Schedule[i].Sch_Id = i+1;
			EPROM_Schedule.Schedule[i].Sch_En_Di = DEFAULT_SCHEDULE_DISABLE;
			EPROM_Schedule.Schedule[i].Start_HH = DEFAULT_SCHEDULE_START_HH;
			EPROM_Schedule.Schedule[i].Start_Min = DEFAULT_SCHEDULE_START_MIN;
			EPROM_Schedule.Schedule[i].Stop_HH = DEFAULT_SCHEDULE_STOP_HH;
			EPROM_Schedule.Schedule[i].Stop_Min = DEFAULT_SCHEDULE_STOP_MIN;
			memset(EPROM_Schedule.Schedule[i].forFutureUse,0,sizeof(EPROM_Schedule.Schedule[i].forFutureUse));
		}
		ExtFlash_update_EPROM_Schedule();
	}
	else
	{

	}
}

void ExtFlash_update_EPROM_Schedule(void)
{
	EPROM_Schedule.ChecksumOfStuct = calculateCheckSumOfStruct((unsigned char*)&EPROM_Schedule,sizeof(EPROM_Schedule));
	EPROM_Schedule.SizeOfStuct = sizeof(EPROM_Schedule);
	at25dfx_write((uint8_t *)&EPROM_Schedule, sizeof(EPROM_Schedule), EPROM_SCHEDULE_START_ADDRESS);
}

void ExtFlash_Read_EPROM_PermanentData(unsigned char makeDefault)
{
	unsigned char Set_default_Flash=0;
	at25dfx_read((uint8_t*) &EPROM_PermanentData, sizeof(EPROM_PermanentData), EPROM_PERMANENT_START_ADDRESS);
    EPROM_General.AI_DI_DO_Detail.OLDDOSignal[0] = 0;

	if((EPROM_PermanentData.checkbyte != 0xAB)||(EPROM_PermanentData.SizeOfStuct==0xFFFF))
	{
		Set_default_Flash=1;
	    sprintf((char *)print, "ExtFlash_Read_EPROM_PermanentData:checkbyte default\r\n");
	    WriteLog(1, (char *)print, 1);
	}
	else if(EPROM_PermanentData.SizeOfStuct<sizeof(EPROM_PermanentData))
	{
		if(EPROM_PermanentData.ChecksumOfStuct==calculateCheckSumOfStruct((unsigned char*)&EPROM_PermanentData,EPROM_PermanentData.SizeOfStuct))
		{
			Set_default_Flash=0;
		}
		else
		{
			Set_default_Flash=1;
		    sprintf((char *)print, "ExtFlash_Read_EPROM_PermanentData: Stuct Size Change CRC mismatch: %d, %d, %d\r\n",EPROM_PermanentData.checkbyte,EPROM_PermanentData.SizeOfStuct,sizeof(EPROM_PermanentData));
		    WriteLog(1, (char *)print, 1);
		}
	}
	else
	{
		if(EPROM_PermanentData.ChecksumOfStuct==calculateCheckSumOfStruct((unsigned char*)&EPROM_PermanentData,sizeof(EPROM_PermanentData)))
		{
			Set_default_Flash=0;
		}
		else
		{
			Set_default_Flash=1;
		    sprintf((char *)print, "ExtFlash_Read_EPROM_PermanentData:size same but CRC mitchmatch: %d, %d, %d\r\n",EPROM_PermanentData.checkbyte,EPROM_PermanentData.SizeOfStuct,sizeof(EPROM_PermanentData));
		    WriteLog(1, (char *)print, 1);
		}
	}
	// Set_default_Flash = 1;
	if((Set_default_Flash==1)||(makeDefault == 1))
	{
		EPROM_PermanentData.checkbyte = 0xAB;
		EPROM_PermanentData.ChecksumOfStuct = calculateCheckSumOfStruct((unsigned char*)&EPROM_PermanentData,sizeof(EPROM_PermanentData));
		EPROM_PermanentData.SizeOfStuct = sizeof(EPROM_PermanentData);
		memset(EPROM_PermanentData.forFutureUse1,0,sizeof(EPROM_PermanentData.forFutureUse1));

		memset(EPROM_PermanentData.DeviceID,0,sizeof(EPROM_PermanentData.DeviceID));
		memset(EPROM_PermanentData.DeviceID,0x30,sizeof(EPROM_PermanentData.DeviceID));

		memset(EPROM_PermanentData.HW_Version,0,sizeof(EPROM_PermanentData.HW_Version));
		memset(EPROM_PermanentData.HW_Version,0x30,sizeof(EPROM_PermanentData.HW_Version));

		EPROM_PermanentData.pro_CheckByte = 0;

		ExtFlash_update_EPROM_PermanentData();
		Get_Astro_time();
	}
	else
	{

	}
}

void ExtFlash_update_EPROM_PermanentData(void)
{
	EPROM_PermanentData.ChecksumOfStuct = calculateCheckSumOfStruct((unsigned char*)&EPROM_PermanentData,sizeof(EPROM_PermanentData));
	EPROM_PermanentData.SizeOfStuct = sizeof(EPROM_PermanentData);
	at25dfx_write((uint8_t *)&EPROM_PermanentData, sizeof(EPROM_PermanentData), EPROM_PERMANENT_START_ADDRESS);
}

void syncExtFlashVariableWithPCBPLCVariable(void)
{
	unsigned int i=0;
    lwgsm_ip_t ip_t;
	unsigned char *buf_t;
	//============================================================================================
	//============================================================================================
	//============================================================================================	

	//EPROM_General.History_En_Di = CONF_ENABLE;
	//strcpy((char *)EPROM_General.Rtu_Detail.HW_Version,DEFAULT_HW_VERSION);
	//strcpy((char *)EPROM_General.Rtu_Detail.Hex_Version,DEFAULT_FV_VERSION);
	//strcpy((char *)EPROM_General.Rtu_Detail.PLC_Version,DEFAULT_PLC_VERSION);
	//strcpy((char *)EPROM_General.Rtu_Detail.REC_Version,DEFAULT_REC_VERSION);

	gFinalAnaValF[RTU_ID_gFinalAnaValF] = EPROM_General.Rtu_Detail.RTUId;
	gFinalAnaValF[LOG_RATE_gFinalAnaValF] = EPROM_General.LogRate;
	gFinalAnaValF[MAX_DO_gFinalAnaValF] = EPROM_General.AI_DI_DO_Detail.Total_Do;
	gFinalAnaValF[MAX_DI_gFinalAnaValF] = EPROM_General.AI_DI_DO_Detail.Total_Di;
	gFinalAnaValF[NO_OF_CKT_gFinalAnaValF] = EPROM_General.NoofCkt;
	gFinalAnaValF[MAX_NO_OF_SMSTAG_gFinalAnaValF] = EPROM_General.MaxofSMS;
	gFinalAnaValF[METER_MAKE_gFinalAnaValF] = EPROM_General.MeterMake;
	gFinalAnaValF[METER_TYPE_gFinalAnaValF] = EPROM_General.MeterType;

	//memset(EPROM_General.Rtu_Detail.forFutureUse,0,sizeof(EPROM_General.Rtu_Detail.forFutureUse));

	//strcpy((char *)EPROM_General.Cust_Detail.Proj_Code,DEFAULT_PROJECT_CODE);
	//strcpy((char *)EPROM_General.Cust_Detail.Site_Name,DEFAULT_SITE_NAME);
	//strcpy((char *)EPROM_General.Cust_Detail.Time_zone,DEFAULT_TIME_ZONE);
	gFinalAnaValF[CLIENT_ID_gFinalAnaValF] = EPROM_General.Cust_Detail.Client_Id;
	gFinalAnaValF[READER_ID_gFinalAnaValF] = EPROM_General.Cust_Detail.Reader_Id;
	
	gFinalAnaValF[MODE_AUTO_MANUAL_gFinalAnaValF] = EPROM_General.DoModeDetails.Do_Mode;
	b.DO1 = EPROM_General.AI_DI_DO_Detail.DOSignal[0];
	b.DO1_temp = b.DO1;

	// gFinalAnaValF[ETHERNET_IP_0_gFinalAnaValF] = EPROM_General.E_Comm.E_IP_Add[0];
	// gFinalAnaValF[ETHERNET_IP_1_gFinalAnaValF] = EPROM_General.E_Comm.E_IP_Add[1];
	// gFinalAnaValF[ETHERNET_IP_2_gFinalAnaValF] = EPROM_General.E_Comm.E_IP_Add[2];
	// gFinalAnaValF[ETHERNET_IP_3_gFinalAnaValF] = EPROM_General.E_Comm.E_IP_Add[3];

	// gFinalAnaValF[ETHERNET_SUBNET_0_gFinalAnaValF] = EPROM_General.E_Comm.E_Subnet_Add[0];
	// gFinalAnaValF[ETHERNET_SUBNET_1_gFinalAnaValF] = EPROM_General.E_Comm.E_Subnet_Add[1];
	// gFinalAnaValF[ETHERNET_SUBNET_2_gFinalAnaValF] = EPROM_General.E_Comm.E_Subnet_Add[2];
	// gFinalAnaValF[ETHERNET_SUBNET_3_gFinalAnaValF] = EPROM_General.E_Comm.E_Subnet_Add[3];
	// gFinalAnaValF[ETHERNET_GATEWAY_0_gFinalAnaValF] = EPROM_General.E_Comm.E_Gateway_Add[0];
	// gFinalAnaValF[ETHERNET_GATEWAY_1_gFinalAnaValF] = EPROM_General.E_Comm.E_Gateway_Add[1];
	// gFinalAnaValF[ETHERNET_GATEWAY_2_gFinalAnaValF] = EPROM_General.E_Comm.E_Gateway_Add[2];
	// gFinalAnaValF[ETHERNET_GATEWAY_3_gFinalAnaValF] = EPROM_General.E_Comm.E_Gateway_Add[3];

	buf_t = (unsigned char*)&EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP;
    lwgsmi_parse_ip((const char**)&buf_t, &ip_t);
	
    gFinalAnaValF[MODEM_MQTT_BROKER_IP_0_gFinalAnaValF] = ip_t.ip[0];
    gFinalAnaValF[MODEM_MQTT_BROKER_IP_1_gFinalAnaValF] = ip_t.ip[1];
    gFinalAnaValF[MODEM_MQTT_BROKER_IP_2_gFinalAnaValF] = ip_t.ip[2];
    gFinalAnaValF[MODEM_MQTT_BROKER_IP_3_gFinalAnaValF] = ip_t.ip[3];
    gFinalAnaValF[MODEM_MQTT_BROKER_PORT_gFinalAnaValF] = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port;
    
	// gFinalAnaValF[COMM_MODE_ETHER_GPRS_gFinalAnaValF] = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Comm_Mode;
    gFinalAnaValF[TIMEZONE_SIGN_gFinalAnaValF] = EPROM_General.Cust_Detail.Timezone_sign;
    gFinalAnaValF[TIMEZONE_HOUR_gFinalAnaValF] = EPROM_General.Cust_Detail.Timezone_hours;
    gFinalAnaValF[TIMEZONE_MIN_gFinalAnaValF] = EPROM_General.Cust_Detail.Timezone_minutes;
    gFinalAnaValF[DEVICE_REBOOT_gFinalAnaValF] = 0.0;																// reboot 
    gFinalAnaValF[DEVICE_REBOOT_TIME_DAY_NIGHT_gFinalAnaValF] = EPROM_General.Cust_Detail.reboot_day_night;
	//strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Name,DEFAULT_MODEM_MQTT_USR_NAME);
	//strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Pass,DEFAULT_MODEM_MQTT_USR_PASS);
	//strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Cli_Id,DEFAULT_MODEM_MQTT_CLIENTID);
	//strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_PUB_Topic,DEFAULT_MODEM_MQTT_PUB_TOPIC);
	//strcpy((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Sub_Topic,DEFAULT_MODEM_MQTT_SUB_TOPIC);

    gFinalAnaValF[BLE_MAC_0_gFinalAnaValF] = EPROM_General.bleDetails.BLE_MAC_Add[0];
    gFinalAnaValF[BLE_MAC_1_gFinalAnaValF] = EPROM_General.bleDetails.BLE_MAC_Add[1];
    gFinalAnaValF[BLE_MAC_2_gFinalAnaValF] = EPROM_General.bleDetails.BLE_MAC_Add[2];
    gFinalAnaValF[BLE_MAC_3_gFinalAnaValF] = EPROM_General.bleDetails.BLE_MAC_Add[3];
    gFinalAnaValF[BLE_MAC_4_gFinalAnaValF] = EPROM_General.bleDetails.BLE_MAC_Add[4];
    gFinalAnaValF[BLE_MAC_5_gFinalAnaValF] = EPROM_General.bleDetails.BLE_MAC_Add[5];


	gFinalAnaValF[GPS_LAT_gFinalAnaValF] = EPROM_General.Cust_Detail.Lattitude;
	gFinalAnaValF[GPS_Log_gFinalAnaValF] = EPROM_General.Cust_Detail.Longitude;

	for(i=0;i<4;i++)
	{
		//EPROM_Schedule.Schedule[i].Sch_Id = i;
		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 0 ] = EPROM_Schedule.Schedule[i].Sch_En_Di;
		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 1 ] = EPROM_Schedule.Schedule[i].Start_HH;
		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 2 ] = EPROM_Schedule.Schedule[i].Start_Min;
		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 3 ] = EPROM_Schedule.Schedule[i].Stop_HH;
		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 4 ] = EPROM_Schedule.Schedule[i].Stop_Min;
		//memset(EPROM_Schedule.Schedule[i].forFutureUse,0,sizeof(EPROM_Schedule.Schedule[i].forFutureUse));
	}
}

int FindSubstr(char *listPointer, char *itemPointer)
{
  int t;
  char *p, *p2;

  for(t=0; listPointer[t]; t++)
  {
    p = &listPointer[t];
    p2 = itemPointer;

    while(*p2 && *p2==*p)
	{
      p++;
      p2++;
    }
    if(!*p2) return t; /* 1st return */
  }
   return -1; /* 2nd return */
}

#define LWGSM_CHARISNUM(x)    ((x) >= '0' && (x) <= '9')

#define LWGSM_CHARTONUM(x)    ((x) - '0')

#define LWGSM_CHARISHEXNUM(x) (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))

#define LWGSM_CHARHEXTONUM(x)                                                                                          \
    (((x) >= '0' && (x) <= '9')                                                                                        \
         ? ((x) - '0')                                                                                                 \
         : (((x) >= 'a' && (x) <= 'f') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'F') ? ((x) - 'A' + 10) : 0)))

int32_t lwgsmi_parse_number(const char** str)
{
    int32_t val = 0;
    uint8_t minus = 0;
    const char* p = *str; /*  */

    if (*p == ' ') { /* Skip ' ' character */
        ++p;
    }
    if (*p == '"') { /* Skip leading quotes */
        ++p;
    }
    if (*p == ',') { /* Skip leading comma */
        ++p;
    }
    if (*p == '"') { /* Skip leading quotes */
        ++p;
    }
    if (*p == '/') { /* Skip '/' character, used in datetime */
        ++p;
    }
    if (*p == ':') { /* Skip ':' character, used in datetime */
        ++p;
    }
    if (*p == '+') { /* Skip '+' character, used in datetime */
        ++p;
    }
    if (*p == '-') { /* Check negative number */
        minus = 1;
        ++p;
    }
    //while (LWGSM_CHARISNUM(*p))
    while (((*p) >= '0' && (*p) <= '9'))
    {
    	/* Parse until character is valid number */
        val = val * 10 + LWGSM_CHARTONUM(*p);
        ++p;
    }
    if (*p == '"') { /* Skip trailling quotes */
        ++p;
    }
    *str = p; /* Save new pointer with new offset */

    return minus ? -val : val;
}

uint8_t lwgsmi_parse_ip(const char** src, lwgsm_ip_t* ip) {
    const char* p = *src;

    if (*p == ',') {
        ++p;
    }
    if (*p == '"') {
        ++p;
    }
    if (LWGSM_CHARISNUM(*p)) {
        ip->ip[0] = lwgsmi_parse_number(&p);
        ++p;
        ip->ip[1] = lwgsmi_parse_number(&p);
        ++p;
        ip->ip[2] = lwgsmi_parse_number(&p);
        ++p;
        ip->ip[3] = lwgsmi_parse_number(&p);
    }
    if (*p == '"') {
        ++p;
    }

    *src = p; /* Set new pointer */
    return 1;
}
