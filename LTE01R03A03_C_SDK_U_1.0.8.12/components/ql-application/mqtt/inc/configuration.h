/*
 * configuration.h
 *
 *  Created on: Jun 10, 2024
 *      Author: Sanket
	updated By Pushpak _2025
 */

#ifndef INC_CONFIGURATION_H_
#define INC_CONFIGURATION_H_

#include <stdint.h>
#include "mqtt_demo.h"
#include "define.h"

/**************************************************************************//**
 * define
 *****************************************************************************/

#define DEFAULT_HW_VERSION					"6.2"     //"x.x.x"
#define DEFAULT_FV_VERSION					"1.0.9_1P" // 1.0.9
#define DEFAULT_PLC_VERSION					"x.x.x"
#define DEFAULT_REC_VERSION					"x.x.x"
#define DEFAULT_LOGRATE						15
#define DEFAULT_RTUID						2

#define DEFAULT_CLIENTID					3
#define DEFAULT_READERID					4
#define DEFAULT_PROJECT_CODE  				"xyz"
#define DEFAULT_SITE_NAME	  				"xyz"
#define DEFAULT_TIME_ZONE	  				"+5:30"
#define DEFAULT_TIME_ZONE_SIGN				0
#define DEFAULT_TIME_ZONE_HOURS				5
#define DEFAULT_TIME_ZONE_MINUTES			30
#define DEFAULT_DAY_NIGHT_REBOOT			0
#define DEFAULT_LAT							9.9312 		//23.04981//23.4511
#define DEFAULT_LOGITUDE					76.26737 	//72.50129//72.5689
#define DEFAULT_DI							6
#define DEFAULT_DO							3

#define DEFAULT_OFFSET_VALUE				0

#define DEFAULT_RS232_1_ENABLE				1
#define DEFAULT_RS232_1_PROTCOL				1
#define DEFAULT_RS232_1_MASTER_SLAVE		3
#define DEFAULT_RS232_1_BAUDRATE			9600
#define DEFAULT_RS232_1_PORT_ID				COM_RS232_1
#define DEFAULT_RS232_1_POLL_FRQ			100 //ms

#define DEFAULT_RS232_2_ENABLE				1
#define DEFAULT_RS232_2_PROTCOL				1
#define DEFAULT_RS232_2_MASTER_SLAVE		5
#define DEFAULT_RS232_2_BAUDRATE			9600
#define DEFAULT_RS232_2_PORT_ID				COM_RS232_2
#define DEFAULT_RS232_2_POLL_FRQ			100

#define DEFAULT_RS485_1_ENABLE				1
#define DEFAULT_RS485_1_PROTCOL				1
#define DEFAULT_RS485_1_MASTER_SLAVE		4
#define DEFAULT_RS485_1_BAUDRATE			9600
#define DEFAULT_RS485_1_PORT_ID				COM_RS485_1
#define DEFAULT_RS485_1_POLL_FRQ			100

#define DEFAULT_RS485_2_ENABLE				1
#define DEFAULT_RS485_2_PROTCOL				1
#define DEFAULT_RS485_2_MASTER_SLAVE		3
#define DEFAULT_RS485_2_BAUDRATE			9600
#define DEFAULT_RS485_2_PORT_ID				COM_RS485_2
#define DEFAULT_RS485_2_POLL_FRQ			100

#define DEFAULT_ETHERNET_ENABLE  				1
#define DEFAULT_ETHERNET_DHCP_ENABLE  			0
#define DEFAULT_ETHERNET_MODBUS_TCP_ENABLE  	1
#define DEFAULT_ETHERNET_MODBUS_TCP_SER_CLIENT  0

#define DEFAULT_ETHERNET_IP_0  					199
#define DEFAULT_ETHERNET_IP_1  					199
#define DEFAULT_ETHERNET_IP_2  					51
#define DEFAULT_ETHERNET_IP_3  					228

#define DEFAULT_ETHERNET_SUBNET_0  				255
#define DEFAULT_ETHERNET_SUBNET_1  				255
#define DEFAULT_ETHERNET_SUBNET_2  				254
#define DEFAULT_ETHERNET_SUBNET_3  				0

#define DEFAULT_ETHERNET_GATEWAY_0  			199
#define DEFAULT_ETHERNET_GATEWAY_1  			199
#define DEFAULT_ETHERNET_GATEWAY_2  			50
#define DEFAULT_ETHERNET_GATEWAY_3  			3

#define DEFAULT_ETHERNET_DNS1_0  				0
#define DEFAULT_ETHERNET_DNS1_1  				0
#define DEFAULT_ETHERNET_DNS1_2  				0
#define DEFAULT_ETHERNET_DNS1_3  				0

#define DEFAULT_ETHERNET_DNS2_0  				0
#define DEFAULT_ETHERNET_DNS2_1  				0
#define DEFAULT_ETHERNET_DNS2_2  				0
#define DEFAULT_ETHERNET_DNS2_3  				0

#define DEFAULT_ETHERNET_MODBUS_TCP_PORT  		502
#define DEFAULT_ETHERNET_MODBUS_TCP_POLL_FRQ    100 //ms
#define DEFAULT_MODEM_ENABLE 					1
#define DEFAULT_MODEM_SERIAL_USB 				0
#define DEFAULT_MODEM_PROTOCOL 					0
#define DEFAULT_MODEM_MQTT_BROKER_IP 			"14.102.161.101"
#define DEFAULT_MODEM_MQTT_BROKER_PORT 			1883
#define DEFAULT_MQTT_COMM_MODE					0
#define DEFAULT_MODEM_MQTT_USR_NAME 			"cimcon"
#define DEFAULT_MODEM_MQTT_USR_PASS 			"cimcon"
#define DEFAULT_MODEM_MQTT_CLIENTID 			"cimcon12345"
#define DEFAULT_MODEM_MQTT_PUB_TOPIC 			"12345678"
#define DEFAULT_MODEM_MQTT_SUB_TOPIC 			"12345678"
#define DEFAULT_MODEM_APN 						"www"
#define DEFAULT_MODEM_MQTT_LIVE_FRQ				5  //sec
#define DEFAULT_MODEM_MOB_MCS					"+919111111111"

#define DEFAULT_BLE_ENABLE 						1

#define DEFAULT_BLE_MAC_0 						1
#define DEFAULT_BLE_MAC_1 						1
#define DEFAULT_BLE_MAC_2 						1
#define DEFAULT_BLE_MAC_3 						1
#define DEFAULT_BLE_MAC_4 						1
#define DEFAULT_BLE_MAC_5 						1

#define DEFAULT_GPS_ENABLE 						1
#define DEFAULT_GPS_POLL_FRQ 					2 //min
#define DEFAULT_SD_ENABLE 						0
#define	DEFAULT_SD_SIZE 						0
#define DEFAULT_DO_MODE 						3     // 0 = Manual, 1 = Photo, 2 = schedule , 3 = Astrotime, 5 = Local  
#define DEFAULT_DO_OFF 							0

#define DEFAULT_SCHEDULE_TOTAL_NO 					4
#define DEFAULT_SCHEDULE_DISABLE 					0
#define DEFAULT_SCHEDULE_START_HH 					1
#define DEFAULT_SCHEDULE_START_MIN 					1
#define DEFAULT_SCHEDULE_STOP_HH					1
#define DEFAULT_SCHEDULE_STOP_MIN					1

#define DEFAULT_MODBUS_QUARY_DETAIL_TOTAL_NO  			0
#define DEFAULT_MODBUS_QUARY_DETAIL_RETRY_COUNT  		0
#define DEFAULT_MODBUS_QUARY_DETAIL_TOTAL_PARA_NO		0
#define DEFAULT_MAX_DATA_TAG							0
#define MAX_AI_CHANNEL   								6

// #define THREEPHASE			3
// #define SINGLEPHASE		    1

// #define METER_HPL_DLMS 1
// #define METER_GENERIC 2

#define EPROM_GENERAL_START_ADDRESS			0x00000000
#define	EPROM_GENERAL_END_ADDRESS			0x00001FFF

#define EPROM_SCHEDULE_START_ADDRESS		0x00002000
#define	EPROM_SCHEDULE_END_ADDRESS			0x00003FFF

#define EPROM_PERMANENT_START_ADDRESS		0x00004000
#define	EPROM_PERMANENT_END_ADDRESS			0x00005FFF

#define HISTORY_DATA_FILE_START_ADDRESS		0x0000F000
#define HISTORY_DATA_FILE_END_ADDRESS		0x003F0FFF

#define HISTORY_DATA_RUN_TIME_PARA_START_ADDRESS	0x003FF000
#define HISTORY_DATA_RUN_TIME_PARA_END_ADDRESS		0x003FFFFF

#define	MAX_HISTORY_DATA_PACKETS			3000

/**************************************************************************//**
 * enum
 *****************************************************************************/

// ********************** configuration command Name (CMDDtate)****************************
typedef enum
{
	RTU_INFO = 1,
	CUST_INFO,
	AI_DI_DO_INFO,
	SERIAL_INFO,
	ETHERNET_INFO,
	CELL_MODEM_INFO,
	BLE_INFO,
	GPS_INFO,
	SD_CARD_INFO,
	RTC_INFO,
	HISTORY_LOGRATE_INFO,
	DO_MODE_INFO,
	SCHEDULE_INFO,
	MODBUS_INFO,
} Configuration_command;

typedef enum
{
	WRITE_CMD = 1,
	READ_CMD  = 2,
} Read_write;


// ********************** RTU Details ****************************
struct RtuDetails //Command 1 for RTU basic Details
{
	int  RTUId ;
	char HW_Version[30];
	char PLC_Version[30];
	char REC_Version[30];
	char Hex_Version[30];
	unsigned char forFutureUse[30];
};

// ********************** Customer Details ***********************
struct CustDetails //Command 2 for Customer basic Details
{
	char Proj_Code[30];
	char Site_Name[30];
	char Time_zone[100];

	int  Client_Id;
	int  Reader_Id;

	double Lattitude;
	double Longitude;
	int Offset_Value;

	char reboot_day_night;
	char Timezone_sign;
	char Timezone_hours;
	char Timezone_minutes;

	unsigned char forFutureUse[26];
};

// ********************** DI/DO  Details ***********************

struct AIDIDODetails
{
	int Total_Di;
	int Total_Do;
	uint8_t DOSignal[MAX_DO_CHANNEL];
	uint8_t OLDDOSignal[MAX_DO_CHANNEL];
	uint8_t dig_bit_array[MAX_DI_CHANNEL];
	uint8_t Old_dig_bit_array[MAX_DI_CHANNEL];
	uint8_t forFutureUse[100];
};

// **********************Communication  Details ***********************
typedef enum
{
	CONF_DISABLE = 0,
	CONF_ENABLE  = 1,
} Comm_En_Dis;

// **********************Serial Communication  Details ***********************
typedef enum
{
	S_MODBUS_RTU   = 1,
	S_MODBUS_ASCII = 2,
	S_SER_DEB      = 3,
} SComm_Proto;

// ********************** Ethernet Communication Details ***********************
typedef enum
{
	E_STATIC = 0,
	E_DHCP   = 1,
} EComm_Mode;

typedef enum
{
	E_SERVER = 1,
	E_CLIENT = 0,
} EComm_Ser_cli;

struct ECommunication
{
	Comm_En_Dis E_Co_En_Di;
	EComm_Mode E_Mode;
	Comm_En_Dis E_Mod_TCP;
	EComm_Ser_cli E_Ser_cli;
	char E_IP_Add[4];
	char E_Subnet_Add[4];
	char E_Gateway_Add[4];
	char E_Preferred_DNS[4];
	char E_Alternate_DNS[4];

	int  E_TCP_Port;
	int  E_Poll_Freq;
	char E_MAC_Add[6];
	unsigned char forFutureUse[94];
};

// ********************** cell Modem Communication Details ***********************
typedef enum
{
	MO_SERIAL = 0,
	MO_USB    = 1,
} MO_Comm_Interface;

typedef enum
{
	MO_MQTT   = 0,
	MO_UDP    = 1,
	MO_DNP3   = 2,
	MO_MQTTS  = 3,
	MO_CUSTOM = 4,
} MO_Comm_Protocol;

struct MQTT_Connection
{
	char MQTT_Bro_IP[20];
	char MQTT_Us_Name[20];
	char MQTT_Us_Pass[20];
	char MQTT_PUB_Topic[50];
	char MQTT_Sub_Topic[50];
	char MQTT_Cli_Id[20];
	int  MQTT_Bro_Port;
	char MQTT_Comm_Mode;
	unsigned char forFutureUse[92];
};

struct ModemCommunication
{
	Comm_En_Dis Mo_Co_En_Di;
	MO_Comm_Interface Mo_Com_Int;
	MO_Comm_Protocol Mo_Proto;
	struct MQTT_Connection MQTT_Conn;
	char Mo_APN[30];
	int MQTT_LiveFreq;
	unsigned char mobMCS[4][14];
};

// ********************** BLE/GPS/SD card /Do mode Details ***********************
//typedef enum
//{
//	DO_AUTO = 0,
//	DO_NOT_APPLICABLE,
//	DO_MANUAL,
//} DO_Mode_AU_MA;

struct BLE_Details
{
	Comm_En_Dis BLE_Co_En_Di;
	char BLE_MAC_Add[6];
	unsigned char forFutureUse[20];
};

struct GPS_Details
{
	Comm_En_Dis GPS_Co_En_Di;
	int GPS_Poll_Freq;
	unsigned char forFutureUse[20];
};

struct SD_Card_Details
{
	Comm_En_Dis SD_Card_Co_En_Di;
	int SD_Card_Size;
	unsigned char forFutureUse[20];
};

typedef enum
{
    RTU_DO_MODE_MANUAL          =   0,
    RTU_DO_MODE_PHOTO           =   1,
    RTU_DO_MODE_AUTO            =   2,
    RTU_DO_MODE_ASTROTIME_GEO   =   3,
    RTU_DO_MODE_TWILIGHT_GEO    =   4,
    RTU_DO_MODE_LOCAL           =   5
}RtuDoMode_e;
struct DO_Mode_Details
{
	RtuDoMode_e Do_Mode;
	char DO_Value[35];
	unsigned char forFutureUse[19];
	RtuDoMode_e Do_Mode_temp;
};

// **********************Schedule Configuration ***********************
struct Schedule_Data
{
	Comm_En_Dis Sch_En_Di;
	char Sch_Id;
	signed char Start_HH;
	signed char Start_Min;
	signed char Stop_HH;
	signed char Stop_Min;
	unsigned char forFutureUse[12];
};

/**************************************************************************//**
 * struct
 *****************************************************************************/
struct setpoint 
{
	uint8_t status[11];			  //for low
	float hi_value[11];				//MAX_SP = 11
	float lo_value[11];
};

struct Save_Para_General
{
	unsigned char checkbyte;							//1
	unsigned char ChecksumOfStuct;						//1+1=2
	uint16_t SizeOfStuct;								//2+2=4
	unsigned char forFutureUse1[11];
	uint8_t MeterMake;
	uint8_t NoofCkt;
	uint16_t MaxofSMS;
	uint16_t SMSSend_Exceed;
	uint16_t DailySMS;
	uint8_t flashOTA;
	unsigned int rebootCount;
	unsigned int LogRate;
	Comm_En_Dis History_En_Di;
	struct RtuDetails Rtu_Detail;
	struct CustDetails Cust_Detail;
	struct AIDIDODetails AI_DI_DO_Detail;
	struct ECommunication E_Comm;
	struct ModemCommunication Mo_Comm;
	struct BLE_Details bleDetails;
	struct GPS_Details gpsDetails;
	struct SD_Card_Details sdCardDetails;
	struct DO_Mode_Details DoModeDetails;
	unsigned char DeviceID[30];
	unsigned char pro_CheckByte;
	
	uint8_t MeterType;
	
	uint32_t Def_timer[10]; // MAXSETPT = 10
	uint8_t mIsInterlockEnable;
    uint8_t mIsDiInterlockEnable;
	uint8_t EventDayStored;
	uint8_t DataHourlyStored;
	struct setpoint sp ;
	uint16_t ModSMSList[64];

};

struct Save_Para_Schedule_Configuration
{
	unsigned char checkbyte;							//1
	unsigned char ChecksumOfStuct;						//1+1=2
	uint16_t SizeOfStuct;								//2+2=4
	unsigned char forFutureUse1[20];					//4+20=24
	int Total_No_Schedule;
	struct Schedule_Data Schedule[85];
};

struct Save_Para_PermanentData
{
	unsigned char checkbyte;							//1
	unsigned char ChecksumOfStuct;						//1+1=2
	uint16_t SizeOfStuct;								//2+2=4
	unsigned char forFutureUse1[20];					//4+20=24
	char HW_Version[30];
	unsigned char DeviceID[30];
	unsigned char pro_CheckByte;
};

typedef struct {
    uint8_t ip[4]; /*!< IPv4 address */
} lwgsm_ip_t;
/**************************************************************************//**
 * extern
 *****************************************************************************/

extern struct Save_Para_General EPROM_General;
extern struct Save_Para_Schedule_Configuration EPROM_Schedule;
extern struct Save_Para_PermanentData EPROM_PermanentData;
extern struct Configuration Config;

extern uint8_t flag_flashUpdateEPROM_General;
extern uint8_t flag_flashUpdateEPROM_General_WaitCounter;
extern uint8_t flag_flashUpdateEPROM_Schedule;
extern uint8_t flag_flashUpdateEPROM_Schedule_WaitCounter;
extern uint8_t flag_flashUpdateEPROM_PermanentData;
extern uint8_t flag_flashUpdateEPROM_PermanentData_WaitCounter;

extern unsigned char pro_MQTT_Broker_IP[30];
extern unsigned int pro_MQTT_Broker_Port;
extern unsigned char pro_MQTT_Client_ID[30];
extern unsigned char pro_APN[30];
extern unsigned char Pro_Application_flag,pro_DO_DI_TestFinish;
extern unsigned char pro_RS232_1_state,pro_RS485_1_state;
extern unsigned char pro_DO_State[3],pro_DI_State[6];
extern unsigned char DO_Opration,DO_Opration_count;
extern unsigned char pro_Flash_State,pro_I2C1_State,pro_I2C2_State;
extern unsigned short int proTestRequest;
/**************************************************************************//**
 * Function Declaration
 *****************************************************************************/
void ExtFlash_update_EPROM_General(void);
void ExtFlash_Read_EPROM_Schedule(unsigned char makeDefault);
void ExtFlash_update_EPROM_Schedule(void);
void ExtFlash_Read_EPROM_PermanentData(unsigned char makeDefault);
void ExtFlash_update_EPROM_PermanentData(void);
void syncExtFlashVariableWithPCBPLCVariable(void);
int FindSubstr(char *listPointer, char *itemPointer);
int32_t lwgsmi_parse_number(const char** str);
uint8_t lwgsmi_parse_ip(const char** src, lwgsm_ip_t* ip);

#endif /* INC_CONFIGURATION_H_ */
