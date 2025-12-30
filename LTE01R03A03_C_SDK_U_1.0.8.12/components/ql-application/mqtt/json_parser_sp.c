/*
 * json_parser.c
 *
 *  Created on: Jun 10, 2024
 *      Author: Sanket
 */

/**************************************************************************//**
 * Includes
 *****************************************************************************/

#include "cJSON.h"
#include "json_parser_sp.h"
#include "lwip/api.h"
#include "mqtt_demo.h"
#include "ql_log.h"
#include "configuration.h"
#include "define.h"
#include "spi_flash_at25ff.h"
#include "I2C_RTC_LCD.h"
#include "gpio_DIDO.h"
#include "mqtt_demo.h"
#include "ql_uart.h"
#include "OTA.h"
#include "ql_power.h"
#include "ql_api_dev.h"
#include "dlms_meter.h"

/**************************************************************************//**
 * Variable
 *****************************************************************************/
#define QL_MQTT_LOG_LEVEL	            QL_LOG_LEVEL_INFO
#define QL_MQTT_LOG(msg, ...)			QL_LOG(QL_MQTT_LOG_LEVEL, "ql_JSON_Parse", msg, ##__VA_ARGS__)
#define QL_MQTT_LOG_PUSH(msg, ...)	    QL_LOG_PUSH("ql_JSON_Parse", msg, ##__VA_ARGS__)


CMD_TYPE current_cmd;
float gFinalAnaValF[2024];
uint8_t pubScheduleBlock;
uint8_t rtctime[20], rtcdate[20];
extern uint8_t SerialTXBuffer[2048];
bool OTA_flag = 0;
char mBinPath[256];

bool isRebootFlag = 1; // Define the flag and initialize it to false
#if 0
//modem_time_t rtc;
//extern struct OTA OTA_Data;
extern struct Configuration Config;
struct OTA OTA_Data;
char msg_payload[1400];
char print1[300];
unsigned int OTA_START=0,OTA_resetCouner=0;
CMD_TYPE current_cmd;
extern void Ota_File_write_ack(COM_TYPE com_mode,int fileType,int chunk_number,int chunk_lenth,char *data ,char * ACK_Response);
JSON_ERROR_RESPONSE response_ACK_JSON_frame(COM_TYPE com_mode ,CMD_TYPE CMD, OTA_FILE_ACK iACK,char * ACK_Response);
JSON_ERROR_RESPONSE Config_response_ACK_JSON_frame(COM_TYPE com_mode , CMD_TYPE CMD, char cmdState, char file_CMD_type, RES_ACK iACK, char * ACK_Response);
extern int send_OTA_status;
int OTAStart =0;
int CRC_value;
extern plcRecFlashInfo_t gPlcRecFlash;
int gPlcFileLength;
int gRecFileLength;
unsigned long int ghexFileLength;

unsigned char flagMQTTPubSchedule=0,flagMQTTPubGetMode = 0,flagMqttPubLogData = 0,flagMqttPubLiveData=0,
		flagMQTT_ID_First=0,flagMQTT_ID_afterPowerCycle = 0,flagMQTT_TestMethod_1_ACK = 0,flagMQTT_TestMethod_1_Result=0,
		flagMQTT_TestMethod_2_ACK = 0,flagMQTT_TestMethod_2_Result=0,
		flagMQTT_ID_AI_CALI_App=0,flagMQTT_AI_Channel_CaliResponse=0,flagMQTT_AI_Channel_Test_Result=0;


unsigned char flagLORAPubLogData;

unsigned char flagTCP_ID_First=0,flagTCP_ID_afterPowerCycle = 0,flagTCP_TestMethod_1_ACK = 0,flagTCP_TestMethod_1_Result=0,
		flagTCP_TestMethod_2_ACK = 0,flagTCP_TestMethod_2_Result=0,
		flagTCP_ID_AI_CALI_App=0,flagTCP_AI_Channel_CaliResponse=0,flagTCP_AI_Channel_Test_Result=0;

unsigned char pubScheduleBlock=0;
unsigned char gAI_Point = 0;
int Fream_id;

void WriteLog(uint8_t LogEnable,const char *pData,uint8_t logType);
#endif

unsigned char flagMQTTPubSchedule=0,flagMQTTPubGetMode = 0,flagMqttPubLogData = 0,flagMqttPubLiveData=0, flagMqttPubAlarmData = 0, flagMQTTPubGetLograte = 0,
		flagMQTT_ID_First=0,flagMQTT_ID_afterPowerCycle = 0,flagMQTT_TestMethod_1_ACK = 0,flagMQTT_TestMethod_1_Result=0, flagMQTTPubHistoryData = 0,
		flagMQTT_TestMethod_2_ACK = 0,flagMQTT_TestMethod_2_Result=0,
		flagMQTT_ID_AI_CALI_App=0,flagMQTT_AI_Channel_CaliResponse=0,flagMQTT_AI_Channel_Test_Result=0;

unsigned char flagSERIAL_ID_First=0,flagSERIAL_ID_afterPowerCycle = 0,flagSERIAL_TestMethod_1_ACK = 0,flagSERIAL_TestMethod_1_Result=0,
		flagSERIAL_TestMethod_2_ACK = 0,flagSERIAL_TestMethod_2_Result=0,
		flagSERIAL_ID_AI_CALI_App=0,flagSERIAL_AI_Channel_CaliResponse=0,flagSERIAL_AI_Channel_Test_Result=0;

extern char csq_sim;
extern uint8_t ram_buff[];

/**************************************************************************//**
 * Check MQTT Received JSON Frame
 *****************************************************************************/

// void Parse_IP_Address(char *input_string ,unsigned char * Ip_Address)
// {
// 	Ip_Address[0]= 0;
// 	Ip_Address[1]= 0;
// 	Ip_Address[2]= 0;
// 	Ip_Address[3]= 0;

// 	size_t index = 0;
// 	while (*input_string)
// 	{
// 		if (isdigit((unsigned char)*input_string))
// 		{
// 			Ip_Address[index] *= 10;
// 			Ip_Address[index] += *input_string - '0';
// 		} else
// 		{
// 			index++;
// 		}
// 		input_string++;
// 	}
// }

JSON_ERROR_RESPONSE parse_JSON_frame(COM_TYPE com_mode,char* jsonString, char * ACK_Response)
{
    JSON_ERROR_RESPONSE ret = JSON_SUCCESS;
	cJSON *Received_json = NULL;
	const cJSON *request = NULL;

    QL_MQTT_LOG("parse_JSON_frame Enter");

	Received_json = cJSON_Parse(jsonString);
    if (Received_json == NULL)
    {
        ret = FAILED_CREATE_JSON_OBJECT;
        cJSON_Delete(Received_json);
        QL_MQTT_LOG("parse_JSON_frame Null");
        return ret;
    }
	
    if (cJSON_HasObjectItem(Received_json, "CMD"))
    {
    	const cJSON *CMD = NULL;
        CMD = cJSON_GetObjectItemCaseSensitive(Received_json, "CMD");
        current_cmd = CMD->valueint;
        switch (current_cmd)
        {
        	case CMD_PRODUCTION: 	//6: //Set_RTC
    		{
    			const cJSON *CMDState = NULL;
    			if (cJSON_HasObjectItem(Received_json, "CMDState"))
    			{
    				CMDState = cJSON_GetObjectItemCaseSensitive(Received_json, "CMDState");
    				switch(CMDState->valueint)
    				{
    					case 1: // ID request
    					{
    						/*
    							{
									"CMD": 2,
									"CMDState": 1,
									"DeviceID": "abcd1234-12",
									"slotNo": 1,
									"HW_Version": "x.x.x",
									"DD": 9,
									"MM": 2,
									"YY": 2023,
									"HH": 10,
									"MN": 37,
									"SS": 50,
									"MQTTBrokerIP":"199.2.22.33",
									"MQTTBrokerPort":1883,
									"APN":"airtelgprs.com"
								}
    						 */
    						const cJSON *DeviceID = NULL;
    //						const cJSON *slotNo = NULL;
    						const cJSON *HW_Version = NULL;
    						const cJSON *date = NULL;
    						const cJSON *month = NULL;
    						const cJSON *year = NULL;
    						const cJSON *hour = NULL;
    						const cJSON *min = NULL;
    						const cJSON *sec = NULL;
    						const cJSON *MQTTBrokerIP = NULL;
    						const cJSON *MQTTBrokerPort = NULL;
							const cJSON *APN = NULL;
    						if (cJSON_HasObjectItem(Received_json, "DeviceID"))
    						{
    							DeviceID = cJSON_GetObjectItemCaseSensitive(Received_json, "DeviceID");
    							memset(EPROM_General.DeviceID,0,sizeof(EPROM_General.DeviceID));
    							strcpy((char *)EPROM_General.DeviceID , (const char *)DeviceID->valuestring);
								strcpy((char *)EPROM_PermanentData.DeviceID,(const char *)EPROM_General.DeviceID);
    							//memset(EPROM_AI_Calibration.DeviceID,0,sizeof(EPROM_AI_Calibration.DeviceID));
    							//memcpy(EPROM_AI_Calibration.DeviceID,EPROM_General.DeviceID,sizeof(EPROM_General.DeviceID));

    						}
    //						if (cJSON_HasObjectItem(Received_json, "slotNo"))
    //						{
    //							slotNo = cJSON_GetObjectItemCaseSensitive(Received_json, "slotNo");
    //						}
    						if (cJSON_HasObjectItem(Received_json, "HW_Version"))
    						{
    							HW_Version = cJSON_GetObjectItemCaseSensitive(Received_json, "HW_Version");
    							memset(EPROM_General.Rtu_Detail.HW_Version,0,sizeof(EPROM_General.Rtu_Detail.HW_Version));
    							strcpy(EPROM_General.Rtu_Detail.HW_Version , HW_Version->valuestring);
    							strcpy((char *)EPROM_PermanentData.HW_Version,(const char *)EPROM_General.Rtu_Detail.HW_Version);

    						}
    						if (cJSON_HasObjectItem(Received_json, "DD"))
    						{
    							date = cJSON_GetObjectItemCaseSensitive(Received_json, "DD");
    							update_time.mDate = (unsigned char)date->valueint;
    						}
    						if (cJSON_HasObjectItem(Received_json, "MM"))
    						{
    							month = cJSON_GetObjectItemCaseSensitive(Received_json, "MM");
    							update_time.month = (unsigned char)month->valueint;
    						}
    						if (cJSON_HasObjectItem(Received_json, "YY"))
    						{
    							year = cJSON_GetObjectItemCaseSensitive(Received_json, "YY");
    							update_time.myear = year->valueint-2000;
    						}
    						if (cJSON_HasObjectItem(Received_json, "HH"))
    						{
    							hour = cJSON_GetObjectItemCaseSensitive(Received_json, "HH");
    							update_time.mHour = (unsigned char)hour->valueint;
    						}
    						if (cJSON_HasObjectItem(Received_json, "MN"))
    						{
    							min = cJSON_GetObjectItemCaseSensitive(Received_json, "MN");
    							update_time.minute = (unsigned char)min->valueint;
    						}
    						if (cJSON_HasObjectItem(Received_json, "SS"))
    						{
    							sec = cJSON_GetObjectItemCaseSensitive(Received_json, "SS");
    							update_time.mSecond = (unsigned char)sec->valueint;
    						}
    						if (cJSON_HasObjectItem(Received_json, "MQTTBrokerIP"))
    						{
    							MQTTBrokerIP = cJSON_GetObjectItemCaseSensitive(Received_json, "MQTTBrokerIP");
    							strcpy((char *)pro_MQTT_Broker_IP,(const char *)MQTTBrokerIP->valuestring);
    						}
    						if (cJSON_HasObjectItem(Received_json, "MQTTBrokerPort"))
    						{
    							MQTTBrokerPort = cJSON_GetObjectItemCaseSensitive(Received_json, "MQTTBrokerPort");
    							pro_MQTT_Broker_Port = MQTTBrokerPort->valueint;
    						}
							if (cJSON_HasObjectItem(Received_json, "APN"))
							{
								APN = cJSON_GetObjectItemCaseSensitive(Received_json, "APN");
								strcpy((char *)pro_APN,(const char *)APN->valuestring);
							}

    						strcpy((char *)pro_MQTT_Client_ID,(const char *)EPROM_General.DeviceID);

    						rtc_intialized=1;

    						if(com_mode == SERIAL)
    						{
    							flagSERIAL_ID_First=1;
    						}
//    						if(com_mode == TCP)
//    						{
//    							flagTCP_ID_First=1;
//    						}
//    						else if(com_mode == MQTT)
//    						{
//    							flagMQTT_ID_First=1;
//    						}
    						flag_flashUpdateEPROM_General = 1;
    						flag_flashUpdateEPROM_General_WaitCounter = 5;
    						flag_flashUpdateEPROM_PermanentData = 1;
    						flag_flashUpdateEPROM_PermanentData_WaitCounter = 5;
    						//flag_flashUpdateEPROM_AI_Calibration = 1;
    						//flag_flashUpdateEPROM_AI_Calibration_WaitCounter = 5;
    						flag_modem_MQTT_Reconnect = 1;
    						break;
    					}
    					case 2: // Test Method 1 request
    					{
    						/*
    						 	{
    								"CMD": 2,
    								"CMDState": 2,
    								"DeviceID": "abcd1234-12",
    								"slotNo": 1,
    								"TestRequest":1
    							}
    						 */
    						const cJSON *TestRequest = NULL;
    						if (cJSON_HasObjectItem(Received_json, "TestRequest"))
    						{
    							TestRequest = cJSON_GetObjectItemCaseSensitive(Received_json, "TestRequest");
    							proTestRequest = TestRequest->valueint;
    						}

    						if(com_mode == SERIAL)
    						{
    							flagSERIAL_TestMethod_1_ACK=1;
    						}
//    						if(com_mode == TCP)
//    						{
//    							flagTCP_TestMethod_1_ACK = 1;
//    						}
//    						else if(com_mode == MQTT)
//    						{
//    							flagMQTT_TestMethod_1_ACK = 1;
//    						}

    						break;
    					}
    					case 3: // Test Method 1 result request
    					{
    						/*
    						 * {
    								"CMD": 2,
    								"CMDState": 3,
    								"DeviceID": "abcd1234-12",
    								"slotNo": 1,
    								"TestRequest":1
    							}
    						 */
    						if(com_mode == SERIAL)
    						{
    							flagSERIAL_TestMethod_1_Result=1;
    						}
//    						if(com_mode == TCP)
//    						{
//    							flagTCP_TestMethod_1_Result = 1;
//    						}
//    						else if(com_mode == MQTT)
//    						{
//    							flagMQTT_TestMethod_1_Result = 1;
//    						}
    						// TODO : Set flag to send Test Method 1 result
    						break;
    					}
    					case 4: // ID request after Power Cycle
    					{
    						/*
    						 * {
    								"CMD": 2,
    								"CMDState": 4,
    								"DeviceID": "abcd1234-12",
    								"slotNo": 1,
    								"HW_Version": "x.x.x",
									"DD": 9,
									"MM": 2,
									"YY": 2023,
									"HH": 10,
									"MN": 37,
									"SS": 50,
									"MQTTBrokerIP":"199.2.22.33",
									"MQTTBrokerPort":1883,
									"APN":"airtelgprs.com"
    							}
    						 */
//    						const cJSON *DeviceID = NULL;
//    						if (cJSON_HasObjectItem(Received_json, "DeviceID"))  // Todo: Remove This
//    						{
//    							DeviceID = cJSON_GetObjectItemCaseSensitive(Received_json, "DeviceID");
//    							memset(EPROM_General.DeviceID,0,sizeof(EPROM_General.DeviceID));
//    							strcpy((char *)EPROM_General.DeviceID , (const char *)DeviceID->valuestring);
//								strcpy((char *)EPROM_PermanentData.DeviceID,(const char *)EPROM_General.DeviceID);
//    							//memset(EPROM_AI_Calibration.DeviceID,0,sizeof(EPROM_AI_Calibration.DeviceID));
//    							//memcpy(EPROM_AI_Calibration.DeviceID,EPROM_General.DeviceID,sizeof(EPROM_General.DeviceID));
//
//    						}
    						if(com_mode == SERIAL)
    						{
    							flagSERIAL_ID_afterPowerCycle=1;
    						}
//    						if(com_mode == TCP)
//    						{
//    							flagTCP_ID_afterPowerCycle = 1;
//    						}
//    						else if(com_mode == MQTT)
//    						{
//    							flagMQTT_ID_afterPowerCycle = 1;
//    						}
    						// TODO : Set flag to send ID frame after power cycle
    						break;
    					}
    					case 5: // Test Method 2 request
    					{
    						/*
    						 * {
    								"CMD": 2,
    								"CMDState": 5,
    								"DeviceID": "abcd1234-12",
    								"slotNo": 1,
    								"TestRequest":2
    							}
    						 */
    						const cJSON *TestRequest = NULL;
    						if (cJSON_HasObjectItem(Received_json, "TestRequest"))
    						{
    							TestRequest = cJSON_GetObjectItemCaseSensitive(Received_json, "TestRequest");
    							proTestRequest = TestRequest->valueint;
    						}

    						if(com_mode == SERIAL)
    						{
    							flagSERIAL_TestMethod_2_ACK=1;
    						}
//    						if(com_mode == TCP)
//    						{
//    							flagTCP_TestMethod_2_ACK = 1;
//    						}
//    						else if(com_mode == MQTT)
//    						{
//    							flagMQTT_TestMethod_2_ACK = 1;
//    						}
    						break;
    					}
    					case 6: // Test Method 2 result request
    					{
    						/*
    						 * {
    								"CMD": 2,
    								"CMDState": 3,
    								"DeviceID": "abcd1234-12",
    								"slotNo": 1,
    								"TestRequest":2
    							}
    						 */
    						// TODO : Set flag to send Test Method 2 result
    						if(com_mode == SERIAL)
    						{
    							flagSERIAL_TestMethod_2_Result=1;
    						}
//    						if(com_mode == TCP)
//    						{
//    							flagTCP_TestMethod_2_Result = 1;
//    						}
//    						else if(com_mode == MQTT)
//    						{
//    							flagMQTT_TestMethod_2_Result = 1;
//    						}
    						break;
    					}
    					default:
    					{


    					}
    				}
    			}
    			break;
    		}
			default:
			{

			}
		}
    }



    request = cJSON_GetObjectItemCaseSensitive(Received_json, "request");
    if( cJSON_IsString(request))
    {
        /*
         * 	DO ON through web-scanet in Manual Mode
         *
    		Request:
    		{"request": "set_do_key_status","request_parameters": {"do_key_data": {"pin-no": 1,"value": 1}}}
    		Response:
    		NA

    		DO OFF through web-scanet in Manual Mode

    		Request:
    		{"request": "set_do_key_status","request_parameters": {"do_key_data": {"pin-no": 1,"value": 0}}}
    		Response:
    		NA
         */
    	if(strncmp(request->valuestring,"set_do_key_status",strlen("set_do_key_status"))==0)
    	{
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *do_key_data = NULL;
        	    do_key_data = cJSON_GetObjectItemCaseSensitive(request_parameters, "do_key_data");
        	    if( cJSON_IsObject(do_key_data))
        	    {
            	    const cJSON *pinNo = NULL;
            	    pinNo = cJSON_GetObjectItemCaseSensitive(do_key_data, "pin-no");

            	    const cJSON *pinValue = NULL;
            	    pinValue = cJSON_GetObjectItemCaseSensitive(do_key_data, "value");

					if(RTU_DO_MODE_MANUAL == EPROM_General.DoModeDetails.Do_Mode)
					{
						
						EPROM_General.AI_DI_DO_Detail.OLDDOSignal[pinNo->valueint-1] = EPROM_General.AI_DI_DO_Detail.DOSignal[pinNo->valueint-1];
						EPROM_General.AI_DI_DO_Detail.DOSignal[pinNo->valueint-1] = pinValue->valueint;
						if(pinNo->valueint == 2)		// DO2 to turn ON 
						{
							EPROM_General.AI_DI_DO_Detail.OLDDOSignal[0] = 0;
							EPROM_General.AI_DI_DO_Detail.DOSignal[0] = 1;
						}
						else if(pinNo->valueint == 3)		// DO3 to turn OFF
						{
							EPROM_General.AI_DI_DO_Detail.OLDDOSignal[0] = 1;
							EPROM_General.AI_DI_DO_Detail.DOSignal[0] = 0;
						}
						flag_flashUpdateEPROM_General = 1;
						flag_flashUpdateEPROM_General_WaitCounter=5;
											
						flagMqttPubLogData = 1;
					}
        	    }
    	    }
    	}
    	else if(strncmp(request->valuestring,"set_schedule",strlen("set_schedule"))==0)
    	{
    		/*
    		 * {"request": "set_schedule",
    		 * 	"request_parameters":
    		 * 		{ "schedule_num": 1,
    		 * 			"schedule":
    		 * 			[
    		 * 				{"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0},
    		 * 				{"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0},
			 * 				{"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0},
			 * 				{"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0}
    		 * 			]
    		 * 		}
    		 * 	}
    		 *
    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    // const cJSON *schedule_num = NULL;
        	    // schedule_num = cJSON_GetObjectItemCaseSensitive(request_parameters, "schedule_num");

        	    const cJSON *schedule = NULL;
        	    schedule = cJSON_GetObjectItemCaseSensitive(request_parameters, "schedule");

        	    if( cJSON_IsArray(schedule))
        	    {
					const cJSON *schedule_element = NULL;
					const cJSON *sch_enable = NULL;
        	    	const cJSON *sch_start_hour = NULL;
        	    	const cJSON *sch_start_minute = NULL;
        	    	const cJSON *sch_stop_hour = NULL;
        	    	const cJSON *sch_stop_minute = NULL;

        	    	int n = cJSON_GetArraySize(schedule);
        	    	for (unsigned char i = 0; i < n; i++)
        	    	{
        	    		schedule_element = cJSON_GetArrayItem(schedule, i);
        	    		sch_enable = cJSON_GetObjectItem(schedule_element, "enable");
        	    		sch_start_hour = cJSON_GetObjectItem(schedule_element, "start_hour");
        	    		sch_start_minute = cJSON_GetObjectItem(schedule_element, "start_minute");
        	    		sch_stop_hour = cJSON_GetObjectItem(schedule_element, "stop_hour");
        	    		sch_stop_minute = cJSON_GetObjectItem(schedule_element, "stop_minute");

        	    		EPROM_Schedule.Schedule[(i)].Sch_En_Di = sch_enable->valueint;
        	    		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 0 ] = sch_enable->valueint;
        	    		EPROM_Schedule.Schedule[i].Start_HH = sch_start_hour->valueint;
        	    		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 1 ] = sch_start_hour->valueint;
        	    		EPROM_Schedule.Schedule[i].Start_Min = sch_start_minute->valueint;
        	    		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 2 ] = sch_start_minute->valueint;
        	    		EPROM_Schedule.Schedule[i].Stop_HH = sch_stop_hour->valueint;
        	    		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 3 ] = sch_stop_hour->valueint;
        	    		EPROM_Schedule.Schedule[i].Stop_Min = sch_stop_minute->valueint;
        	    		gFinalAnaValF[SCHEDULE_gFinalAnaValF + (i*5) + 4 ] = sch_stop_minute->valueint;
        	    	}
        	    }
        	    flag_flashUpdateEPROM_Schedule=1;
        	    flag_flashUpdateEPROM_Schedule_WaitCounter=5;
    	    }
    	}

		else if(strncmp(request->valuestring,"set_Broker",strlen("set_Broker"))==0)
		{
			/*
				Set MQTT broker IP and port

				Request:
				{"request": "set_Broker","request_parameters": {"IP": "192.168.1.1", "PORT": 1883}}

				Response: NA
			*/
			const cJSON *request_parameters = NULL;
			request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
			if( cJSON_IsObject(request_parameters))
			{
				const cJSON *ip_json = NULL;
				const cJSON *port_json = NULL;
				//struct ip_addr ip_t;
			// unsigned char *buf_t;

				// Extract IP parameter
				ip_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "IP");
				if (cJSON_IsString(ip_json) && (ip_json->valuestring != NULL))
				{
					// Copy the IP string into EPROM, ensure null-termination
					strncpy(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP, ip_json->valuestring, sizeof(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP) - 1);
					EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP[sizeof(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP) - 1] = '\0';
				}

				// Extract PORT parameter
				port_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "PORT");
				if (cJSON_IsNumber(port_json))
				{
					EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port = port_json->valueint;
				}

				// Parse the new IP to update global variables
			//  buf_t = (unsigned char*)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP;
			// lwgsmi_parse_ip((const char**)&buf_t, &ip_t);

				// gFinalAnaValF[MODEM_MQTT_BROKER_IP_0_gFinalAnaValF] = ip_t.ip[0];
				// gFinalAnaValF[MODEM_MQTT_BROKER_IP_1_gFinalAnaValF] = ip_t.ip[1];
				// gFinalAnaValF[MODEM_MQTT_BROKER_IP_2_gFinalAnaValF] = ip_t.ip[2];
				// gFinalAnaValF[MODEM_MQTT_BROKER_IP_3_gFinalAnaValF] = ip_t.ip[3];
				gFinalAnaValF[MODEM_MQTT_BROKER_PORT_gFinalAnaValF] = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port;

				// Trigger EPROM save and data publish
				flag_flashUpdateEPROM_General = 1;
				flag_flashUpdateEPROM_General_WaitCounter = 5;
				flagMqttPubLogData = 1;
			}
		}

// to add  /client id /groupid 
		else if (strncmp(request->valuestring, "set_client_group", strlen("set_client_group")) == 0)
		{
			/*
				Set Client ID and Group ID

				Request:
				{"request": "set_client_group", "request_parameters": {"client_id": 3, "group_id": 4}}

				Response: NA
			*/
			const cJSON *request_parameters = NULL;
			request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
			if (cJSON_IsObject(request_parameters))
			{
				const cJSON *client_id_json = NULL;
				const cJSON *group_id_json = NULL;

				// Extract client_id parameter
				client_id_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "client_id");
				if (cJSON_IsNumber(client_id_json))
				{
					// Update the client_id in EPROM
					EPROM_General.Cust_Detail.Client_Id = client_id_json->valueint;
				}

				// Extract group_id parameter
				group_id_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "group_id");
				if (cJSON_IsNumber(group_id_json))
				{
					// Update the group_id in EPROM
					EPROM_General.Cust_Detail.Reader_Id = group_id_json->valueint;
				}

				// Log the changes
				QL_MQTT_LOG("Client ID updated to: %d", EPROM_General.Cust_Detail.Client_Id);
				QL_MQTT_LOG("Group ID updated to: %d", EPROM_General.Cust_Detail.Reader_Id);

				// Trigger EPROM save and data publish
				flag_flashUpdateEPROM_General = 1;
				flag_flashUpdateEPROM_General_WaitCounter = 5;
				flagMqttPubLogData = 1;
			}
		}
// add comment by pc for rtuid change through imei
		else if (strncmp(request->valuestring, "rtuid change through imei", strlen("rtuid change through imei")) == 0)
		{
			/*
				Request:
				{"request": "rtuid change through imei", "request_parameters": {"IMEI": "123456789012345", "RTU_ID": 1001}}

				Response: NA
			*/
			const cJSON *request_parameters = NULL;
			request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
			if (cJSON_IsObject(request_parameters))
			{
				const cJSON *imei_json = NULL;
				const cJSON *rtu_id_json = NULL;

				// Extract IMEI parameter
				imei_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "IMEI");
				if (cJSON_IsString(imei_json) && (imei_json->valuestring != NULL))
				{
					// Compare the received IMEI with the device's IMEI
					if (strncmp((const char *)IMEI, imei_json->valuestring, strlen((const char *)IMEI)) == 0)
					{
						// Extract RTU ID parameter
						rtu_id_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "RTU_ID");
						if (cJSON_IsNumber(rtu_id_json))
						{
							// Update the RTU ID in EPROM
							EPROM_General.Rtu_Detail.RTUId = rtu_id_json->valueint;

							// Log the RTU ID change
							QL_MQTT_LOG("RTU ID updated to: %d", EPROM_General.Rtu_Detail.RTUId);

							// Trigger EPROM save and data publish
							flag_flashUpdateEPROM_General = 1;
							flag_flashUpdateEPROM_General_WaitCounter = 5;
							flagMqttPubLogData = 1;
						}
						else
						{
							QL_MQTT_LOG("Invalid RTU ID in request");
						}
					}
					else
					{
						QL_MQTT_LOG("IMEI mismatch: Received IMEI: %s, Device IMEI: %s", imei_json->valuestring, IMEI);
					}
				}
				else
				{
					QL_MQTT_LOG("Invalid IMEI in request");
				}
			}
		}
		else if (strncmp(request->valuestring, "get_rtu_id", strlen("get_rtu_id")) == 0)
		{
			/*
				Get RTU ID

				Request: {"request": "get_rtu_id", "request_parameters": "all"}

				Response: {"client_id": 5, "group_id": 5, "rtu_id": 5}
			*/
			const cJSON *request_parameters = NULL;
			request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
			if (cJSON_IsString(request_parameters))
			{
				if (strncmp(request_parameters->valuestring, "all", strlen("all")) == 0)
				{
					if (com_mode == MQTT)
					{
						flagMQTTPubGetMode = 1;
					}
				}
			}
		}

		//***************** */
        else if(strncmp(request->valuestring, "set_mobMCS", strlen("set_mobMCS")) == 0)
		{
			/*
				Set mobile numbers for mobMCS
				Request:
				{"request": "set_mobMCS", "request_parameters": {"mobMCS": ["+919111111111", "+919111111112", "+919111111113", "+919111111114"]}}
				Response: NA
			*/
			const cJSON *request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
			if (cJSON_IsObject(request_parameters))
			{
				const cJSON *mobMCS_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "mobMCS");
				if (cJSON_IsArray(mobMCS_json))
				{
					int count = cJSON_GetArraySize(mobMCS_json);
					for (int i = 0; i < count && i < 4; i++)
					{
						const cJSON *num_json = cJSON_GetArrayItem(mobMCS_json, i);
						if (cJSON_IsString(num_json) && (num_json->valuestring != NULL))
						{
							strncpy((char*)EPROM_General.Mo_Comm.mobMCS[i], num_json->valuestring, 14);
							EPROM_General.Mo_Comm.mobMCS[i][13] = '\0'; // Ensure null-termination
						}
					}
					// Optionally, update the rest to empty if fewer than 4 provided
					for (int i = count; i < 4; i++)
					{
						EPROM_General.Mo_Comm.mobMCS[i][0] = '\0';
					}
					// Save to flash and trigger publish if needed
					flag_flashUpdateEPROM_General = 1;
					flag_flashUpdateEPROM_General_WaitCounter = 5;
					flagMqttPubLogData = 1;
				}
			}
		}


		
		//**************** *
    	else if(strncmp(request->valuestring,"get_schedule",strlen("get_schedule"))==0)
    	{
    		/*
				Request:
				{"request": "get_schedule","request_parameters": {"schedule_num": 1}}

				Response:
				{"client_id": 5, "group_id": 5, "rtu_id": 5, "schedule_num": 1,
				 "schedule":
					 [
						 {"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0},
						 {"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0}
					 ]
				 }

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *schedule_num = NULL;
        	    schedule_num = cJSON_GetObjectItemCaseSensitive(request_parameters, "schedule_num");
        	    if(com_mode == MQTT)
        	    {
        	    	flagMQTTPubSchedule = 1;
        	    	pubScheduleBlock =  schedule_num->valueint;
        	    }
        	    // TODO : Set flag to send schedule json based on  schedule_num
    	    }
    	}
    	else if(strncmp(request->valuestring,"get_mode",strlen("get_mode"))==0)
    	{
    		/*
				Get PCBPLC service mode

				Request: {"request": "get_mode","request_parameters": "all"}

				Response: {"client_id": 5, "group_id": 5, "rtu_id": 5, "pcbplc_mode": 0}

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsString(request_parameters))
    	    {
    	    	if(strncmp(request_parameters->valuestring,"all",strlen("all"))==0)
        	    {
            	    if(com_mode == MQTT)
            	    {
            	    	flagMQTTPubGetMode = 1;
            	    }
        	    }
    	    }
    	}
    	else if(strncmp(request->valuestring,"set_mode",strlen("set_mode"))==0)
    	{
    		/*
				Set PCBPLC service mode

				Request: {"request": "set_mode","request_parameters": {"pcbplc_mode": 0}}

				Response: NA

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *pcbplc_mode = NULL;
        	    pcbplc_mode = cJSON_GetObjectItemCaseSensitive(request_parameters, "pcbplc_mode");
        	    // TODO : Set mode here
            	EPROM_General.DoModeDetails.Do_Mode = pcbplc_mode->valueint;
				EPROM_General.DoModeDetails.Do_Mode_temp = EPROM_General.DoModeDetails.Do_Mode;
				RUN_Timer[2] = 1;
                flag_flashUpdateEPROM_General = 1;
                flag_flashUpdateEPROM_General_WaitCounter=5;
    	    }
    	}
    	else if(strncmp(request->valuestring,"Get_data",strlen("Get_data"))==0)
    	{
    		/*
				Get PCBPLC service mode

				Request: {"request": "Get_data","request_parameters": {"ReadData": 1}}

				Response: NA

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
				const cJSON *ReadData = NULL;
        	    ReadData = cJSON_GetObjectItemCaseSensitive(request_parameters, "ReadData");
				if(com_mode == MQTT)
				{
					flagMqttPubLogData = 1;
					if(ReadData->valueint == 1)
					{
						// flagMqttPubLogData = 1 ;
					}
				}
    	    }
    	}
		else if (strncmp(request->valuestring, "set_meter_type", strlen("set_meter_type")) == 0)
		{
			/*
				Set Meter Type Request

				Request:
				{"request": "set_meter_type", "request_parameters": {"meter_type": "THREEPHASE"}}
			*/
			const cJSON *request_parameters = NULL;
			request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
			if (cJSON_IsObject(request_parameters))
			{
				const cJSON *meter_type_json = NULL;

				// Extract meter_type parameter
				meter_type_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "meter_type");
				if (cJSON_IsString(meter_type_json) && (meter_type_json->valuestring != NULL))
				{
					if (strncmp(meter_type_json->valuestring, "SINGLEPHASE", strlen("SINGLEPHASE")) == 0)
					{
						EPROM_General.MeterType = SINGLEPHASE;
					}
					else if (strncmp(meter_type_json->valuestring, "THREEPHASE", strlen("THREEPHASE")) == 0)
					{
						EPROM_General.MeterType = THREEPHASE;
					}

					// Update gFinalAnaValF with the new meter type
					gFinalAnaValF[METER_TYPE_gFinalAnaValF] = EPROM_General.MeterType;

					// Log the change
					QL_MQTT_LOG("Meter Type updated to: %d", EPROM_General.MeterType);

					// Trigger EPROM save and data publish
					flag_flashUpdateEPROM_General = 1;
					flag_flashUpdateEPROM_General_WaitCounter = 5;
					flagMqttPubLogData = 1;
				}
				else
				{
					QL_MQTT_LOG("Invalid meter_type parameter in request");
				}
			}
			else
			{
				QL_MQTT_LOG("Invalid request_parameters in request");
			}
		}

    	else if(strncmp(request->valuestring,"soft_reboot",strlen("soft_reboot"))==0)
    	{
    		/*
				Set Reboot service mode

				Request: {"request": "soft_reboot","request_parameters": {"reboot_value": 1}}

				Response: NA

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *reboot_value = NULL;
        	    reboot_value = cJSON_GetObjectItemCaseSensitive(request_parameters, "reboot_value");
        	    // TODO : Set mode here     // bellow 4 lines were commented for soft reboot 
            //	EPROM_General.DoModeDetails.Do_Mode = pcbplc_mode->valueint;
             //   gpcbplcCnfg.mRtuDoMode = pcbplc_mode->valueint;
             //   flag_flashUpdateEPROM_General = 1;
            //    flag_flashUpdateEPROM_General_WaitCounter=5;
        	    gFinalAnaValF[DEVICE_REBOOT_gFinalAnaValF] = reboot_value->valueint;
					

					 if(gFinalAnaValF[DEVICE_REBOOT_gFinalAnaValF] == 1)
					 {
					 	//reboot_device_func();
						flag_flashUpdateEPROM_General = 1;
						flag_flashUpdateEPROM_General_WaitCounter = 5;
						flagMqttPubLogData = 1;
	QL_MQTT_LOG("I2C read_data = Date=%02d/%02d/20%02d, Time=%02d:%02d:%02d", 
								rtc_time.mDate,rtc_time.month,rtc_time.myear, 
								rtc_time.mHour,rtc_time.minute,rtc_time.mSecond);
						ql_power_reset(RESET_NORMAL);
					 }
    	    }
    	}
	//****

	else if (strncmp(request->valuestring, "set_lamp_info", strlen("set_lamp_info")) == 0)
{
    /*
        Set Total Lamp Current and Number of bulbs connected for outputs 7, 8, 9.
        Request:
        {
            "request": "set_lamp_info",
            "request_parameters": {
                "lamp_info": [
                    {"index": 7, "total_current": 123.45, "num_bulbs": 10},
                    {"index": 8, "total_current": 67.89, "num_bulbs": 5},
                    {"index": 9, "total_current": 33.21, "num_bulbs": 2}
                ]
            }
        }
        Response: NA
    */
    const cJSON *request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    if (cJSON_IsObject(request_parameters))
    {
        const cJSON *lamp_info_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "lamp_info");
        if (cJSON_IsArray(lamp_info_json))
        {
            int count = cJSON_GetArraySize(lamp_info_json);
            for (int i = 0; i < count; i++)
            {
                const cJSON *item = cJSON_GetArrayItem(lamp_info_json, i);
                if (cJSON_IsObject(item))
                {
                    const cJSON *index_json = cJSON_GetObjectItemCaseSensitive(item, "index");
                    const cJSON *current_json = cJSON_GetObjectItemCaseSensitive(item, "total_current");
                    const cJSON *bulbs_json = cJSON_GetObjectItemCaseSensitive(item, "num_bulbs");
                    if (cJSON_IsNumber(index_json) && cJSON_IsNumber(current_json) && cJSON_IsNumber(bulbs_json))
                    {
                        int idx = index_json->valueint;
                        if (idx >= 7 && idx <= 9)
                        {
                            EPROM_General.sp.hi_value[idx] = current_json->valuedouble;
                            EPROM_General.sp.lo_value[idx] = bulbs_json->valueint;
                        }
                    }
                }
            }
            // Save to flash and trigger publish if needed
            flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter = 5;
            flagMqttPubLogData = 1;
        }
    }
}
//***************** */
	else if (strncmp(request->valuestring, "set_sp_values", strlen("set_sp_values")) == 0)
{
    /*
        Set SP Values

        Request:
        {
            "request": "set_sp_values",
            "request_parameters": {
                "sp": [
                    {"status": 3, "hi_value": 270.0, "lo_value": 180.0},
                    {"status": 3, "hi_value": 270.0, "lo_value": 180.0},
                    {"status": 3, "hi_value": 270.0, "lo_value": 180.0},
                    {"status": 3, "hi_value": 30.0, "lo_value": 0.0},
                    {"status": 3, "hi_value": 30.0, "lo_value": 0.0},
                    {"status": 3, "hi_value": 30.0, "lo_value": 0.0},
                    {"status": 3, "hi_value": 0.0, "lo_value": 0.8}
                ]
            }
        }

        Response: NA
    */
    const cJSON *request_parameters = NULL;
    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    if (cJSON_IsObject(request_parameters))
    {
        const cJSON *sp_array = NULL;
        sp_array = cJSON_GetObjectItemCaseSensitive(request_parameters, "sp");
        if (cJSON_IsArray(sp_array))
        {
            int array_size = cJSON_GetArraySize(sp_array);
            if (array_size <= 7) // Ensure the array has at most 7 elements
            {
                for (int i = 0; i < array_size; i++)
                {
                    const cJSON *sp_item = cJSON_GetArrayItem(sp_array, i);
                    if (cJSON_IsObject(sp_item))
                    {
                        const cJSON *status = cJSON_GetObjectItemCaseSensitive(sp_item, "status");
                        const cJSON *hi_value = cJSON_GetObjectItemCaseSensitive(sp_item, "hi_value");
                        const cJSON *lo_value = cJSON_GetObjectItemCaseSensitive(sp_item, "lo_value");

                        if (cJSON_IsNumber(status) && cJSON_IsNumber(hi_value) && cJSON_IsNumber(lo_value))
                        {
                            EPROM_General.sp.status[i] = status->valueint;
                            EPROM_General.sp.hi_value[i] = hi_value->valuedouble;
                            EPROM_General.sp.lo_value[i] = lo_value->valuedouble;
                        }
                    }
                }

                // Log the updated values
                for (int i = 0; i < array_size; i++)
                {
                    QL_MQTT_LOG("SP[%d]: status=%d, hi_value=%.2f, lo_value=%.2f",
                                i, EPROM_General.sp.status[i], EPROM_General.sp.hi_value[i], EPROM_General.sp.lo_value[i]);
                }

                // Trigger EPROM save and data publish
                flag_flashUpdateEPROM_General = 1;
                flag_flashUpdateEPROM_General_WaitCounter = 5;
                flagMqttPubLogData = 1;
            }
            else
            {
                QL_MQTT_LOG("Invalid SP array size. Expected at most 7 elements.");
            }
        }
        else
        {
            QL_MQTT_LOG("Invalid SP parameter in request.");
        }
    }
    else
    {
        QL_MQTT_LOG("Invalid request_parameters in request.");
    }
}
	//****
	else if (strncmp(request->valuestring, "set_def_timer", strlen("set_def_timer")) == 0)
{
    /*
        Set Default Timer Values

        Request:
        {
            "request": "set_def_timer",
            "request_parameters": {
                "def_timer": [1, 400, 15, 5, 15, 10, 2, 2, 10]
            }
        }

        Response: NA
    */
    const cJSON *request_parameters = NULL;
    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    if (cJSON_IsObject(request_parameters))
    {
        const cJSON *def_timer_array = NULL;
        def_timer_array = cJSON_GetObjectItemCaseSensitive(request_parameters, "def_timer");
        if (cJSON_IsArray(def_timer_array))
        {
            int array_size = cJSON_GetArraySize(def_timer_array);
            if (array_size == 9) // Ensure the array has exactly 9 elements
            {
                for (int i = 0; i < array_size; i++)
                {
                    const cJSON *timer_value = cJSON_GetArrayItem(def_timer_array, i);
                    if (cJSON_IsNumber(timer_value))
                    {
                        EPROM_General.Def_timer[i] = timer_value->valueint;
                    }
                }

                // Log the updated values
                QL_MQTT_LOG("Def_timer updated: [%d, %d, %d, %d, %d, %d, %d, %d, %d]",
                            EPROM_General.Def_timer[0], EPROM_General.Def_timer[1], EPROM_General.Def_timer[2],
                            EPROM_General.Def_timer[3], EPROM_General.Def_timer[4], EPROM_General.Def_timer[5],
                            EPROM_General.Def_timer[6], EPROM_General.Def_timer[7], EPROM_General.Def_timer[8]);

                // Trigger EPROM save and data publish
                flag_flashUpdateEPROM_General = 1;
                flag_flashUpdateEPROM_General_WaitCounter = 5;
                flagMqttPubLogData = 1;
            }
            else
            {
                QL_MQTT_LOG("Invalid def_timer array size. Expected 9 elements.");
            }
        }
        else
        {
            QL_MQTT_LOG("Invalid def_timer parameter in request.");
        }
    }
    else
    {
        QL_MQTT_LOG("Invalid request_parameters in request.");
    }
}
	else if (strncmp(request->valuestring, "get_modem_imei", strlen("get_modem_imei")) == 0)
	{
		/*
			Request:
			{"request": "get_modem_imei", "request_parameters": {"imei": 1}}

			Response:
			{"imei": "123456789012345"}
		*/
		const cJSON *request_parameters = NULL;
		request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
		if (cJSON_IsObject(request_parameters))
		{
			const cJSON *imei_request = NULL;
			imei_request = cJSON_GetObjectItemCaseSensitive(request_parameters, "imei");
			if (imei_request && imei_request->valueint == 1)
			{
				char imei[64] = {0};
				int ret = ql_dev_get_imei(imei, sizeof(imei), 0);
				if (ret == 0)
				{
					cJSON *response_json = cJSON_CreateObject();
					cJSON_AddStringToObject(response_json, "imei", imei);

					// Serialize and send the response
					char *response_string = cJSON_PrintUnformatted(response_json);
					if (response_string)
					{
						QL_MQTT_LOG("Sending IMEI response: %s", response_string);
						if (com_mode == MQTT)
						{
							flagMqttPubLogData = 1;
							strncpy((char *)MqttPubBuf, response_string, sizeof(MqttPubBuf) - 1);
							MqttPubBuf[sizeof(MqttPubBuf) - 1] = '\0';
						}
						free(response_string);
					}

					if (response_json)
					{
						cJSON_Delete(response_json);
					}
				}
				else
				{
					QL_MQTT_LOG("Failed to retrieve IMEI, error: %d", ret);
				}
			}
		}
	}

	else if (strncmp(request->valuestring, "set_no_of_circuits", strlen("set_no_of_circuits")) == 0)
{
    /*
        Set Number of Circuits

        Request:
        {"request": "set_no_of_circuits", "request_parameters": {"no_of_circuits": 2}}

        Response: NA
    */
    const cJSON *request_parameters = NULL;
    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    if (cJSON_IsObject(request_parameters))
    {
        const cJSON *no_of_circuits_json = NULL;
        no_of_circuits_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "no_of_circuits");
        if (cJSON_IsNumber(no_of_circuits_json))
        {
            // Update the NoofCkt in EPROM
            EPROM_General.NoofCkt = no_of_circuits_json->valueint;

            // Update the global variable
            gFinalAnaValF[NO_OF_CKT_gFinalAnaValF] = EPROM_General.NoofCkt;

            QL_MQTT_LOG("Number of Circuits updated to: %d", EPROM_General.NoofCkt);

            // Trigger EPROM save and data publish
            flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter = 5;
            flagMqttPubLogData = 1;
        }
    }
}
// else if (strncmp(request->valuestring, "set_meter_make", strlen("set_meter_make")) == 0)
// {
//     /*
//         Set Meter Make Request

//         Request:
//         {"request": "set_meter_make", "request_parameters": {"meter_make": "HPL_DLMS"}}
//     */
//     const cJSON *request_parameters = NULL;
//     request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
//     if (cJSON_IsObject(request_parameters))
//     {
//         const cJSON *meter_make_json = NULL;

//         // Extract meter_make parameter
//         meter_make_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "meter_make");
//         if (cJSON_IsString(meter_make_json) && (meter_make_json->valuestring != NULL))
//         {
//             if (strncmp(meter_make_json->valuestring, "HPL_DLMS", strlen("HPL_DLMS")) == 0)
//             {
//                 EPROM_General.MeterMake = METER_HPL_DLMS;
//             }
//             else if (strncmp(meter_make_json->valuestring, "GENERIC", strlen("GENERIC")) == 0)
//             {
//                 EPROM_General.MeterMake = METER_GENERIC;
//             }

//             // Update gFinalAnaValF with the new meter make
//             gFinalAnaValF[METER_MAKE_gFinalAnaValF] = EPROM_General.MeterMake;

//             // Log the change
//             QL_MQTT_LOG("Meter Make updated to: %d", EPROM_General.MeterMake);

//             // Trigger EPROM save and data publish
//             flag_flashUpdateEPROM_General = 1;
//             flag_flashUpdateEPROM_General_WaitCounter = 5;
//             flagMqttPubLogData = 1;
//         }
//         else
//         {
//             QL_MQTT_LOG("Invalid meter_make parameter in request");
//         }
//     }
//     else
//     {
//         QL_MQTT_LOG("Invalid request_parameters in request");
//     }
// }
//********** */

// 	else if (strncmp(request->valuestring, "set_meter_config", strlen("set_meter_config")) == 0)
// {
//     /*
//         Set Meter Type and Number of Circuits

//         Request:
//         {"request": "set_meter_config", "request_parameters": {"meter_type": "SINGLEPHASE", "no_of_circuits": 1}}

//         Response: NA
//     */
//     const cJSON *request_parameters = NULL;
//     request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
//     if (cJSON_IsObject(request_parameters))
//     {
//         const cJSON *meter_type_json = NULL;
//         const cJSON *no_of_circuits_json = NULL;

//         // Extract meter_type parameter
//         meter_type_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "meter_type");
//         if (cJSON_IsString(meter_type_json) && (meter_type_json->valuestring != NULL))
//         {
//             if (strncmp(meter_type_json->valuestring, "SINGLEPHASE", strlen("SINGLEPHASE")) == 0)
//             {
//                 EPROM_General.MeterType = SINGLEPHASE;
//             }
//             else if (strncmp(meter_type_json->valuestring, "THREEPHASE", strlen("THREEPHASE")) == 0)
//             {
//                 EPROM_General.MeterType = THREEPHASE;
//             }
//         }

//         // Extract no_of_circuits parameter
//         no_of_circuits_json = cJSON_GetObjectItemCaseSensitive(request_parameters, "no_of_circuits");
//         if (cJSON_IsNumber(no_of_circuits_json))
//         {
//             EPROM_General.NoofCkt = no_of_circuits_json->valueint;
//         }

//         // Log the changes
//         QL_MQTT_LOG("Meter Type updated to: %d", EPROM_General.MeterType);
//         QL_MQTT_LOG("Number of Circuits updated to: %d", EPROM_General.NoofCkt);

//         // Trigger EPROM save and data publish
//         flag_flashUpdateEPROM_General = 1;
//         flag_flashUpdateEPROM_General_WaitCounter = 5;
//         flagMqttPubLogData = 1;
//     }
// }
		// else if (strncmp(request->valuestring, "get_modem_imei", strlen("get_modem_imei")) == 0)
		// {
		// 	/*
		// 		Request:
		// 		{"request": "get_modem_imei", "request_parameters": {"imei": 1}}

		// 		Response:
		// 		{"imei": "123456789012345"}
		// 	*/
		// 	const cJSON *request_parameters = NULL;
		// 	request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
		// 	if (cJSON_IsObject(request_parameters))
		// 	{
		// 		const cJSON *imei_request = NULL;
		// 		imei_request = cJSON_GetObjectItemCaseSensitive(request_parameters, "imei");
		// 		if (imei_request && imei_request->valueint == 1)
		// 		{
//**********
		// else if (strncmp(request->valuestring, "get_modem_imei", strlen("get_modem_imei")) == 0)
		// {
		// 	/*
		// 		Request:
		// 		{"request": "get_modem_imei", "request_parameters": {"imei": 1}}

		// 		Response:
		// 		{"imei": "123456789012345"}
		// 	*/
		// 	const cJSON *request_parameters = NULL;
		// 	request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
		// 	if (cJSON_IsObject(request_parameters))
		// 	{
		// 		const cJSON *imei_request = NULL;
		// 		imei_request = cJSON_GetObjectItemCaseSensitive(request_parameters, "imei");
		// 		if (imei_request && imei_request->valueint == 1)
		// 		{
		// 			char imei[64] = {0};
		// 			int ret = ql_dev_get_imei(imei, sizeof(imei), 0);
		// 			if (ret == 0)
		// 			{
		// 				cJSON *response_json = cJSON_CreateObject();
		// 				cJSON_AddStringToObject(response_json, "imei", imei);

		// 				// Serialize and send the response
		// 				char *response_string = cJSON_PrintUnformatted(response_json);
		// 				if (response_string)
		// 				{
		// 					QL_MQTT_LOG("Sending IMEI response: %s", response_string);
		// 					// Send the response via MQTT or another communication channel
		// 					// Example: mqtt_publish(response_string);
		// 					if(ql_mqtt_publish(&mqtt_cli, "v1/devices/Response", (char *)&MqttPubBuf, (unsigned short)strlen((char *)MqttPubBuf), 0, 0, mqtt_requst_result_cb, NULL) == MQTTCLIENT_WOUNDBLOCK)
		// 					{
		// 						QL_MQTT_LOG("MQTT modem_MQTT_publish \"v1/devices/Response\" WOUNDBLOCK");
		// 					}
		// 					else if(ql_mqtt_publish(&mqtt_cli, "v1/devices/Response", (char *)&MqttPubBuf, (unsigned short)strlen((char *)MqttPubBuf), 0, 0, mqtt_requst_result_cb, NULL) == MQTTCLIENT_SUCCESS)
		// 					{
		// 						QL_MQTT_LOG("MQTT modem_MQTT_publish \"v1/devices/Response\" SUCCESS");
		// 					}
		// 					else
		// 					{
		// 						QL_MQTT_LOG("MQTT modem_MQTT_publish \"v1/devices/Response\" ERROR");
		// 					}
							
							
		// 					QL_MQTT_LOG("======wait publish result");
		// 					ql_rtos_semaphore_wait(mqtt_semp, QL_WAIT_FOREVER);
		// 				}
		// ql_rtos_task_sleep_ms(100);
		// QL_MQTT_LOG("MQTT modem_MQTT_publish \"v1/devices/Response\"");
							
		// 					free(response_string);
		// 				}

		// 				cJSON_Delete(response_json);
		// 			}
		// 			else
		// 			{
		// 				QL_MQTT_LOG("Failed to retrieve IMEI, error: %d", ret);
		// 			}
		// 		}
		// 	}
		// }
				


		
		// Add IMEI request handling
		// else if(strncmp(request->valuestring,"get_modem_imei",strlen("get_modem_imei"))==0)
		// {
		// 	/*
		// 		Get Modem IMEI

		// 		Request:
		// 		{"request": "get_modem_imei","request_parameters": {"imei": 1}}

		// 		Response: NA
		// 	*/
		// 	const cJSON *request_parameters = NULL;
		// 	request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
		// 	if( cJSON_IsObject(request_parameters))
		// 	{
		// 		const cJSON *imei = NULL;
		// 		imei = cJSON_GetObjectItemCaseSensitive(request_parameters, "imei");
		// 		if(com_mode == MQTT)
		// 		{
		// 			flagMqttPubLogData = 1;
		// 			if(imei->valueint == 1)
		// 			{
		// 				// flagMqttPubLogData = 1 ;
		// 			}
		// 		}
		// 	}
		// }
		// else if(strnmpr(request->valuestring,"get_imei",strlen("get_imei"))==0)
		// {
		// 	/*
		// 		Get IMEI

		// 		Request:
		// 		{"request": "get_imei","request_parameters": {"imei": 1}}

		// 		Response: NA
		// 	*/
		// 	const cJSON *request_parameters = NULL;
		// 	request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
		// 	if( cJSON_IsObject(request_parameters))
		// 	{
		// 		const cJSON *imei = NULL;
		// 		imei = cJSON_GetObjectItemCaseSensitive(request_parameters, "imei");
		// 		if(com_mode == MQTT)
		// 		{
		// 			flagMqttPubLogData = 1;
		// 			if(imei->valueint == 1)
		// 			{
		// 				// flagMqttPubLogData = 1 ;
		// 			}
		// 		}
		// 	}
		// }
	
    	else if(strncmp(request->valuestring,"astro_offset",strlen("astro_offset"))==0)
    	{
    		/*
				Set PCBPLC service mode

				Request:
				{"request": "astro_offset","request_parameters": {"offset_value": -10}}

				Response: NA

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *offset_value = NULL;
        	    offset_value = cJSON_GetObjectItemCaseSensitive(request_parameters, "offset_value");

        	    gFinalAnaValF[ASTRO_OFFSET_gFinalAnaValF] = offset_value->valueint;
				EPROM_General.Cust_Detail.Offset_Value = offset_value->valueint;
				
				Get_Astro_time();

				// Log after calling Get_Astro_time
				QL_MQTT_LOG("After calling Get_Astro_time: Sunrise - %02d:%02d:%02d, Sunset - %02d:%02d:%02d",
					(int)gFinalAnaValF[SUNRISE_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_SEC_gFinalAnaValF],
					(int)gFinalAnaValF[SUNSET_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_SEC_gFinalAnaValF]);

				flag_flashUpdateEPROM_General = 1;
				flag_flashUpdateEPROM_General_WaitCounter=5;
				flagMqttPubLogData = 1;
    	    }
    	}
    	else if(strncmp(request->valuestring,"set_RTC",strlen("set_RTC"))==0)
    	{
    		/*
				Set PCBPLC service mode

				Request:
				{"request": "set_RTC","request_parameters": {"date": "19052022", "time": "112700"}}

				Response: NA

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *date = NULL;
        	    date = cJSON_GetObjectItemCaseSensitive(request_parameters, "date");

				const cJSON *time = NULL;
        	    time = cJSON_GetObjectItemCaseSensitive(request_parameters, "time");

				// // Parse date and time strings
    			// sscanf(date->valuestring, "%hhd%hhd%4d", &update_time.mDate, &update_time.month, &update_time.myear);
    			// sscanf(time->valuestring, "%hhd%hhd%hhd", &update_time.mHour, &update_time.minute, &update_time.mSecond);
				// update_time.myear = update_time.myear-2000;
				
// Log the received date and time strings
QL_MQTT_LOG("Received Date String from server : %s", date->valuestring);
QL_MQTT_LOG("Received Time String from server: %s", time->valuestring);

				// Manually parse date and time strings
				update_time.mDate = (date->valuestring[0] - '0') * 10 + (date->valuestring[1] - '0');
				update_time.month = (date->valuestring[2] - '0') * 10 + (date->valuestring[3] - '0');
				update_time.myear = (date->valuestring[4] - '0') * 1000 + (date->valuestring[5] - '0') * 100 + (date->valuestring[6] - '0') * 10 + (date->valuestring[7] - '0');
				update_time.mHour= (time->valuestring[0] - '0') * 10 + (time->valuestring[1] - '0');
				update_time.minute = (time->valuestring[2] - '0') * 10 + (time->valuestring[3] - '0');
				update_time.mSecond = (time->valuestring[4] - '0') * 10 + (time->valuestring[5] - '0');
				update_time.myear = update_time.myear-2000;

				QL_MQTT_LOG("Date:%2d%2d%2d",update_time.mDate,update_time.month,update_time.myear);
				QL_MQTT_LOG("Time:%2d%2d%2d",update_time.mHour,update_time.minute,update_time.mSecond);
			// Log the parsed date and time values
QL_MQTT_LOG("Parsed Date: %02d/%02d/%04d", update_time.mDate, update_time.month, update_time.myear + 2000);
QL_MQTT_LOG("Parsed Time: %02d:%02d:%02d", update_time.mHour, update_time.minute, update_time.mSecond);

			
				rtc_intialized = 1;

				// Log before calling Get_Astro_time
QL_MQTT_LOG("Before calling Get_Astro_time: Current Time - %02d:%02d:%02d, Date - %02d/%02d/%04d",
	update_time.mHour, update_time.minute, update_time.mSecond,
	update_time.mDate, update_time.month, update_time.myear + 2000);


				Get_Astro_time();

// Log after calling Get_Astro_time
QL_MQTT_LOG("After calling Get_Astro_time: Sunrise - %02d:%02d:%02d, Sunset - %02d:%02d:%02d",
	(int)gFinalAnaValF[SUNRISE_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_SEC_gFinalAnaValF],
	(int)gFinalAnaValF[SUNSET_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_SEC_gFinalAnaValF]);


				flagMqttPubLogData = 1;
    	    }
    	}
		else if(strncmp(request->valuestring,"set_Location",strlen("set_Location"))==0)
    	{
    		/*
				Set PCBPLC service mode

				Request:
				{"request": "set_Location","request_parameters": {"LAT": 23.4469, "LOG":73.4594}}

				Response: NA

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *LAT = NULL;
        	    LAT = cJSON_GetObjectItemCaseSensitive(request_parameters, "LAT");
				EPROM_General.Cust_Detail.Lattitude = LAT->valuedouble;

				const cJSON *LOG = NULL;
        	    LOG = cJSON_GetObjectItemCaseSensitive(request_parameters, "LOG");
				EPROM_General.Cust_Detail.Longitude = LOG->valuedouble;


				QL_MQTT_LOG("Before calling Get_Astro_time: Current Time - %02d:%02d:%02d, Date - %02d/%02d/%04d",
					update_time.mHour, update_time.minute, update_time.mSecond,
					update_time.mDate, update_time.month, update_time.myear + 2000);

				Get_Astro_time();
				
				QL_MQTT_LOG("After calling Get_Astro_time: Sunrise - %02d:%02d:%02d, Sunset - %02d:%02d:%02d",
					(int)gFinalAnaValF[SUNRISE_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNRISE_SEC_gFinalAnaValF],
					(int)gFinalAnaValF[SUNSET_HOUR_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_MIN_gFinalAnaValF], (int)gFinalAnaValF[SUNSET_SEC_gFinalAnaValF]);
				
				flag_flashUpdateEPROM_General = 1;
				flag_flashUpdateEPROM_General_WaitCounter=5;
				flagMqttPubLogData = 1;
    	    }
    	}
    	else if(strncmp(request->valuestring,"get_lograte",strlen("get_lograte"))==0)
    	{
    		/*
				Get PCBPLC service mode

				Request: {"request": "get_lograte","request_parameters": "all"}

				Response: {"client_id": 5, "group_id": 5, "rtu_id": 5, "lograte": 15}

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsString(request_parameters))
    	    {
    	    	if(strncmp(request_parameters->valuestring,"all",strlen("all"))==0)
        	    {
            	    if(com_mode == MQTT)
            	    {
            	    	flagMQTTPubGetLograte = 1;
            	    }
        	    }
    	    }
    	}
		else if(strncmp(request->valuestring,"set_lograte",strlen("set_lograte"))==0)
    	{
    		/*
				Set PCBPLC service mode

				Request: {"request": "set_lograte","request_parameters": {"lograte": 15}}

				Response: NA

    		 */
    	    const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if( cJSON_IsObject(request_parameters))
    	    {
        	    const cJSON *lograte = NULL;
        	    lograte = cJSON_GetObjectItemCaseSensitive(request_parameters, "lograte");
            	EPROM_General.LogRate = lograte->valueint;

                flag_flashUpdateEPROM_General = 1;
                flag_flashUpdateEPROM_General_WaitCounter=5;
				flagMqttPubLogData = 1;
    	    }
    	}
		else if(strncmp(request->valuestring,"ota",strlen("ota"))==0)
		{
			const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if(cJSON_IsObject(request_parameters))
    	    {
				const cJSON *url = NULL;
				url = cJSON_GetObjectItemCaseSensitive(request_parameters, "url");
				strcpy(mBinPath, url->valuestring);
				OTA_flag = 1;
				ql_fota_http_app_init1();

				EPROM_General.flashOTA = 1;
				flag_flashUpdateEPROM_General = 1;
				flag_flashUpdateEPROM_General_WaitCounter = 5;
    	    }
		}
		else if(strncmp(request->valuestring,"reset",strlen("reset"))==0)
		{
			const cJSON *request_parameters = NULL;
    	    request_parameters = cJSON_GetObjectItemCaseSensitive(Received_json, "request_parameters");
    	    if(cJSON_IsObject(request_parameters))
    	    {
				const cJSON *default_data = NULL;
				default_data = cJSON_GetObjectItemCaseSensitive(request_parameters, "default_data");
				
				ExtFlash_Read_RuntimePara(default_data->valueint);
				ExtFlash_Read_EPROM_General(default_data->valueint);
				ExtFlash_Read_EPROM_Schedule(default_data->valueint);
				syncExtFlashVariableWithPCBPLCVariable();
    	    }
		}
    }

	// if((flag_flashUpdateEPROM_Modbus_Quary_Detail == 1)||(flag_flashUpdateEPROM_Schedule == 1) || (flag_flashUpdateEPROM_General == 1))
	// {
	// 	syncExtFlashVariableWithPCBPLCVariable();
	// }
    //end:
	syncExtFlashVariableWithPCBPLCVariable();
    cJSON_Delete(Received_json);
	return ret;
}


/**************************************************************************//**
 * Function name 	: buildProIdFrameJson
 * arguments		: 1)
 * 		 			:
 * return			:
 * Note				: Response:
					  {
							"CMD": 2,
							"CMDState": 1,
							"DeviceID": "abcd1234-12",
							"slotNo": 1,
							"FW_Version": "x.x.x",
							"IMEI": "1234567890"
					  }
 * After Power Cycle
 * 					  {
							"CMD": 2,
							"CMDState": 4,
							"DeviceID": "abcd1234-12",
							"slotNo": 1,
							"FW_Version": "x.x.x",
							"IMEI": "1234567890"
					  }
 *****************************************************************************/

unsigned char buildProIdFrameJson(unsigned char sendPort,unsigned char afterPowerCycle)
{
	unsigned char result=1;//,tempBuff[100];

	cJSON *json_Full_Obejct;
	json_Full_Obejct = cJSON_CreateObject();


	if(afterPowerCycle == 0)
	{
		cJSON_AddNumberToObject(json_Full_Obejct, "CMD",2);
		cJSON_AddNumberToObject(json_Full_Obejct, "CMDState",1);
	}
	else if(afterPowerCycle == 1)
	{
		cJSON_AddNumberToObject(json_Full_Obejct, "CMD",2);
		cJSON_AddNumberToObject(json_Full_Obejct, "CMDState",4);
	}
	else if(afterPowerCycle == 2)
	{
		cJSON_AddNumberToObject(json_Full_Obejct, "CMD",3);
		cJSON_AddNumberToObject(json_Full_Obejct, "CMDState",1);
	}
	cJSON_AddStringToObject(json_Full_Obejct, "DeviceID", (const char *)EPROM_General.DeviceID);
	cJSON_AddNumberToObject(json_Full_Obejct, "slotNo",1);
	cJSON_AddStringToObject(json_Full_Obejct, "FW_Version", DEFAULT_FV_VERSION);//EPROM_General.Rtu_Detail.Hex_Version);

//	memset(tempBuff,0,sizeof(tempBuff));
//	sprintf((char *)tempBuff,"%02X:%02X:%02X:%02X:%02X:%02X",
//			EPROM_General.bleDetails.BLE_MAC_Add[0],EPROM_General.bleDetails.BLE_MAC_Add[1],EPROM_General.bleDetails.BLE_MAC_Add[2],
//			EPROM_General.bleDetails.BLE_MAC_Add[3],EPROM_General.bleDetails.BLE_MAC_Add[4],EPROM_General.bleDetails.BLE_MAC_Add[5]);
//	cJSON_AddStringToObject(json_Full_Obejct, "BleMAC", (const char *)tempBuff);
//
//	memset(tempBuff,0,sizeof(tempBuff));
//	sprintf((char *)tempBuff,"%02X:%02X:%02X:%02X:%02X:%02X",ethernetMac[5],ethernetMac[4],ethernetMac[3],ethernetMac[2],ethernetMac[1],ethernetMac[0]);
//	cJSON_AddStringToObject(json_Full_Obejct, "EthenetMAC", (const char *)tempBuff);
//
//	memset(tempBuff,0,sizeof(tempBuff));
//	sprintf((char *)tempBuff,"%04X%04X%04X%04X%04X%04X",(unsigned int)(stm32deviceID[2]<<16),(unsigned int)stm32deviceID[2],(unsigned int)stm32deviceID[1]<<16,
//														(unsigned int)stm32deviceID[1],(unsigned int)(stm32deviceID[0])<<16,(unsigned int)(stm32deviceID[0]));
//	cJSON_AddStringToObject(json_Full_Obejct, "ST_ID", (const char *)tempBuff);

	cJSON_AddStringToObject(json_Full_Obejct, "IMEI", (const char *)IMEI);


	if(sendPort == 3)//Serial Port)
	{
		memset(SerialTXBuffer, 0x00, sizeof(SerialTXBuffer));
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&SerialTXBuffer,sizeof(SerialTXBuffer),false)))
		{
			result=0;
		}
	}
//	else if(sendPort == 1)//MQTT_PORT)
//	{
//		memset(MqttPubBuf, 0x00, CIM_MAX_SIZE_OF_MQTT_PAYLOAD);
//		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&MqttPubBuf,CIM_MAX_SIZE_OF_MQTT_PAYLOAD,false)))
//		{
//			result=0;
//		}
//	}
//	else if((sendPort == 2))
//	{
//		memset(tcp_ResponseBuffer, 0x00, 1500);
//		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&tcp_ResponseBuffer,1500,false)))
//		{
//			result=0;
//		}
//	}
	else
	{
		result=0;
	}

	cJSON_Delete(json_Full_Obejct);
	return result;

}

/**************************************************************************//**
 * Function name 	: buildTestMethodAckJson
 * arguments		: 1)
 * 		 			:
 * return			:
 * Note				: Response:
 * TestMethod 1
 * 					  {
							"CMD": 2,
							"CMDState": 2,
							"DeviceID": "abcd1234-12",
							"slotNo": 1,
							"ACK":1,
					  }

 * TestMethod 2
 * 					  {
							"CMD": 2,
							"CMDState": 5,
							"rebootCount" : 5,
							"DeviceID": "abcd1234-12",
							"slotNo": 1,
							"ACK":1,
					  }
 *****************************************************************************/

unsigned char buildTestMethodAckJson(unsigned char sendPort,unsigned char TestMethod)
{
	unsigned char result=1;

	cJSON *json_Full_Obejct;
	json_Full_Obejct = cJSON_CreateObject();

	cJSON_AddNumberToObject(json_Full_Obejct, "CMD",2);
	if(TestMethod == 1)
	{
		cJSON_AddNumberToObject(json_Full_Obejct, "CMDState",2);
	}
	else
	{
		cJSON_AddNumberToObject(json_Full_Obejct, "CMDState",5);
		cJSON_AddNumberToObject(json_Full_Obejct, "rebootCount",EPROM_General.rebootCount);
		//cJSON_AddNumberToObject(json_Full_Obejct, "rebootCount",ram_buff[1]); // Todo : change this

	}
	cJSON_AddStringToObject(json_Full_Obejct, "DeviceID", (const char *)EPROM_General.DeviceID);
	cJSON_AddNumberToObject(json_Full_Obejct, "slotNo",1);
	cJSON_AddNumberToObject(json_Full_Obejct, "ACK",1);

	if(sendPort == 3)//Serial Port)
	{
		memset(SerialTXBuffer, 0x00, sizeof(SerialTXBuffer));
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&SerialTXBuffer,sizeof(SerialTXBuffer),false)))
		{
			result=0;
		}
	}
//	if(sendPort == 1)//MQTT_PORT)
//	{
//		memset(MqttPubBuf, 0x00, CIM_MAX_SIZE_OF_MQTT_PAYLOAD);
//		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&MqttPubBuf,CIM_MAX_SIZE_OF_MQTT_PAYLOAD,false)))
//		{
//			result=0;
//		}
//	}
//	else if((sendPort == 2))
//	{
//		memset(tcp_ResponseBuffer, 0x00, 1500);
//		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&tcp_ResponseBuffer,1500,false)))
//		{
//			result=0;
//		}
//	}
	else
	{
		result=0;
	}

	cJSON_Delete(json_Full_Obejct);
	return result;
}

/**************************************************************************//**
 * Function name 	: buildTestMethodResultJson
 * arguments		: 1)
 * 		 			:
 * return			:
 * Note				: Response:
 * TestMethod 1
					{
						"CMD": 2,
						"CMDState": 3,
						"DeviceID": "abcd1234-12",
						"slotNo": 1,
						"DOResult": "101",
						"DIResult": "101101",
						"RS232_COM": 1,
						"4GPhysical_State": 1,
						"4GSIM_State": 1,
						"4GReg_State": 1,
						"4GInternet_State": 1,
						"4GMQTT_State": 1,
						"4GLTE_Version": 1,
						"FW_Version": "x.x.x",
					}

 * TestMethod 2
					{
						"CMD": 2,
						"CMDState": 6,
						"DeviceID": "abcd1234-12",
						"slotNo": 5,
						"DD": 9,
						"MM": 2,
						"YY": 2023,
						"HH": 10,
						"MN": 37,
						"SS": 50,
						"Flash_State":1,
						"rebootCount" : 5
					}
 *****************************************************************************/

unsigned char buildTestMethodResultJson(unsigned char sendPort,unsigned char TestMethod)
{
	unsigned char result=1,tempBuff[100];

	cJSON *json_Full_Obejct;
	json_Full_Obejct = cJSON_CreateObject();

	cJSON_AddNumberToObject(json_Full_Obejct, "CMD",2);

	if(TestMethod == 1)
	{
		cJSON_AddNumberToObject(json_Full_Obejct, "CMDState",3);
		cJSON_AddStringToObject(json_Full_Obejct, "DeviceID", (const char *)EPROM_General.DeviceID);
		cJSON_AddNumberToObject(json_Full_Obejct, "slotNo",1);
		memset(tempBuff,0,sizeof(tempBuff));
		sprintf((char*)tempBuff,"%d%d%d",pro_DO_State[0],pro_DO_State[1],pro_DO_State[2]);
		cJSON_AddStringToObject(json_Full_Obejct, "DOResult", (const char *)tempBuff);

		memset(tempBuff,0,sizeof(tempBuff));
		sprintf((char*)tempBuff,"%d%d%d%d%d%d",pro_DI_State[0],pro_DI_State[1],pro_DI_State[2],pro_DI_State[3],pro_DI_State[4],pro_DI_State[5]);
		cJSON_AddStringToObject(json_Full_Obejct, "DIResult", (const char *)tempBuff);

		//cJSON_AddNumberToObject(json_Full_Obejct, "RS485_COM1",pro_RS485_1_state);
		//cJSON_AddNumberToObject(json_Full_Obejct, "RS485_COM2",pro_RS485_2_state);
		//cJSON_AddNumberToObject(json_Full_Obejct, "RS232_COM1",pro_RS232_1_state);
		cJSON_AddNumberToObject(json_Full_Obejct, "RS232_COM1",1); // Production development on RS232_COM1 so result is always 1
		//cJSON_AddNumberToObject(json_Full_Obejct, "RS232_COM2",pro_RS232_2_state);

		//cJSON_AddNumberToObject(json_Full_Obejct, "latitude",gps.latitude);
		//cJSON_AddNumberToObject(json_Full_Obejct, "longitude",gps.longitude);

		//cJSON_AddNumberToObject(json_Full_Obejct, "BLECon_State",ble.connectionStatus);

//		if(Modem_PHY_Status == lwgsmOK)
//		{
//			cJSON_AddNumberToObject(json_Full_Obejct, "4GPhysical_State",1);
//		}
//		else
//		{
//			cJSON_AddNumberToObject(json_Full_Obejct, "4GPhysical_State",0);
//		}

		if(card_status == QL_SIM_STATUS_READY)
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GPhysical_State",1);
			cJSON_AddNumberToObject(json_Full_Obejct, "4GSIM_State",1);
		}
		else
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GPhysical_State",0);
			cJSON_AddNumberToObject(json_Full_Obejct, "4GSIM_State",0);
		}

		//if((Modem_gsm_network_status == LWGSM_NETWORK_REG_STATUS_CONNECTED)||(Modem_gsm_network_status == LWGSM_NETWORK_REG_STATUS_CONNECTED_ROAMING))
		if(fNetworkRegistered == 1)
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GReg_State",1);
		}
		else
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GReg_State",0);
		}

		if((fInternetEnabled == 1))
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GInternet_State",1);
		}
		else
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GInternet_State",0);
		}

		cJSON_AddNumberToObject(json_Full_Obejct, "4GRSSI",csq_sim);

		if(mqtt_connected == 1)
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GMQTT_State",1);
		}
		else
		{
			cJSON_AddNumberToObject(json_Full_Obejct, "4GMQTT_State",0);
		}

		cJSON_AddStringToObject(json_Full_Obejct, "4GLTE_Version",( const char * )version_buf); // TODO : add Logic to get LTE Version

		cJSON_AddStringToObject(json_Full_Obejct, "FW_Version", ( const char * )DEFAULT_FV_VERSION);//EPROM_General.Rtu_Detail.Hex_Version);

		//cJSON_AddNumberToObject(json_Full_Obejct, "KEY1",pro_key1_status);
		//cJSON_AddNumberToObject(json_Full_Obejct, "KEY2",pro_key2_status);
	}
	else //if(TestMethod == 2)
	{
		cJSON_AddNumberToObject(json_Full_Obejct, "CMDState",6);
		cJSON_AddStringToObject(json_Full_Obejct, "DeviceID", (const char *)EPROM_General.DeviceID);
		cJSON_AddNumberToObject(json_Full_Obejct, "slotNo",1);
		cJSON_AddNumberToObject(json_Full_Obejct, "DD",rtc_time.mDate);
		cJSON_AddNumberToObject(json_Full_Obejct, "MM",rtc_time.month);
		cJSON_AddNumberToObject(json_Full_Obejct, "YY",rtc_time.myear+2000);
		cJSON_AddNumberToObject(json_Full_Obejct, "HH",rtc_time.mHour);
		cJSON_AddNumberToObject(json_Full_Obejct, "MN",rtc_time.minute);
		cJSON_AddNumberToObject(json_Full_Obejct, "SS",rtc_time.mSecond);
//		cJSON_AddNumberToObject(json_Full_Obejct, "Flash_State",pro_Flash_State);
		cJSON_AddNumberToObject(json_Full_Obejct, "Flash_State",1); // Todo : change this
		cJSON_AddNumberToObject(json_Full_Obejct, "rebootCount",EPROM_General.rebootCount);
		//cJSON_AddNumberToObject(json_Full_Obejct, "rebootCount",ram_buff[1]); // Todo : change this
	}
	cJSON_AddNumberToObject(json_Full_Obejct, "LinePower",DI_Final_value[14]);
	cJSON_AddNumberToObject(json_Full_Obejct, "LowBattery",DI_Final_value[15]);

	if(sendPort == 3)//Serial Port)
	{
		memset(SerialTXBuffer, 0x00, sizeof(SerialTXBuffer));
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&SerialTXBuffer,sizeof(SerialTXBuffer),false)))
		{
			result=0;
		}
	}
//	if(sendPort == 1)//MQTT_PORT)
//	{
//		memset(MqttPubBuf, 0x00, CIM_MAX_SIZE_OF_MQTT_PAYLOAD);
//		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&MqttPubBuf,CIM_MAX_SIZE_OF_MQTT_PAYLOAD,false)))
//		{
//			result=0;
//		}
//	}
//	else if((sendPort == 2))
//	{
//		memset(tcp_ResponseBuffer, 0x00, 1500);
//		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&tcp_ResponseBuffer,1500,false)))
//		{
//			result=0;
//		}
//	}
	else
	{
		result=0;
	}
	cJSON_Delete(json_Full_Obejct);
	return result;
}
unsigned char buildLograteDataJson(unsigned char sendPort)
{
	unsigned char result=1;
	unsigned int i_index=0;

	cJSON *json_Full_Obejct;
	json_Full_Obejct = cJSON_CreateObject();
	cJSON *DI_Arrry;
	cJSON *AI_Tag_Arrry;
// Add identifier and client details
	cJSON_AddStringToObject(json_Full_Obejct, "identifier", "1D");
	cJSON_AddNumberToObject(json_Full_Obejct, "client_id", EPROM_General.Cust_Detail.Client_Id);
	cJSON_AddNumberToObject(json_Full_Obejct, "group_id", EPROM_General.Cust_Detail.Reader_Id);
	
	cJSON_AddNumberToObject(json_Full_Obejct, "rtu_id", EPROM_General.Rtu_Detail.RTUId);
// Added IMEI
	cJSON_AddStringToObject(json_Full_Obejct, "imei", (const char *)IMEI);  // added  IMEI by pc
// Add reboot count
	cJSON_AddNumberToObject(json_Full_Obejct, "rebootCount",EPROM_General.rebootCount); // reboot count from flash added by pc 
// Add firmware version
	cJSON_AddStringToObject(json_Full_Obejct, "FW_Ver", DEFAULT_FV_VERSION);
	// add HW_Version
	cJSON_AddStringToObject(json_Full_Obejct, "HW_Ver", DEFAULT_HW_VERSION);
 // Add date and time
	sprintf((char*)rtcdate, "%02d%02d%02d", rtc_time.mDate, rtc_time.month, 2000 + rtc_time.myear);
	sprintf((char*)rtctime, "%02d%02d%02d", rtc_time.mHour, rtc_time.minute, rtc_time.mSecond);

	cJSON_AddStringToObject(json_Full_Obejct, "date", (const char*)&rtcdate);
	cJSON_AddStringToObject(json_Full_Obejct, "time", (const char*)&rtctime);
// Add mode
	cJSON_AddNumberToObject(json_Full_Obejct, "mode", EPROM_General.DoModeDetails.Do_Mode);
// Add digital input (DI) status
	DI_Arrry = cJSON_CreateArray();
	for(i_index=0;i_index<EPROM_General.AI_DI_DO_Detail.Total_Di;i_index++)
	{
	  //cJSON_AddItemToArray(DI_Arrry, cJSON_CreateNumber(dig_bit_array1[i_index]));
	  cJSON_AddItemToArray(DI_Arrry, cJSON_CreateNumber(DI_Final_value[i_index]));
	}
	cJSON_AddItemToObject(json_Full_Obejct, "di_status", DI_Arrry);
// Add analog input (AI) tags
	AI_Tag_Arrry = cJSON_CreateArray();
	for(i_index=0;i_index<EPROM_General.MaxofSMS;i_index++)
	{
	  cJSON_AddItemToArray(AI_Tag_Arrry, cJSON_CreateNumber(gFinalAnaValF[(EPROM_General.ModSMSList[i_index]-1)/2]));
	}
	cJSON_AddItemToObject(json_Full_Obejct, "ai_tag", AI_Tag_Arrry);
// Serialize and send the JSON
	if(sendPort == 1)//MQTT_PORT)
	{
		memset(MqttPubBuf, 0x00, CIM_MAX_SIZE_OF_MQTT_PAYLOAD);
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&MqttPubBuf,CIM_MAX_SIZE_OF_MQTT_PAYLOAD,false)))
		{
			result=0;
		}
	}
	#if 0
	else if((sendPort == 2))
	{
		memset(tcp_ResponseBuffer, 0x00, 1500);
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&tcp_ResponseBuffer,1500,false)))
		{
			result=0;
		}
	}
	#endif
	else
	{
		result=0;
	}

	cJSON_Delete(json_Full_Obejct);

	return result;
}

/**************************************************************************//**
 * Function name 	: buildGetScheduleJson
 * arguments		: 1)
 * 		 			:
 * return			:
 * Note				:
 * 					:	Response:
 *						{"client_id": 5, "group_id": 5, "rtu_id": 5, "schedule_num": 1,
 *						 "schedule":
 *	 						 [
 *								 {"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0},
 *								 {"enable": 1,"start_hour": 1,"start_minute": 59,"stop_hour": 2,"stop_minute": 0}
 *							 ]
 *						 }
 *****************************************************************************/

unsigned char buildGetScheduleJson(unsigned char sendPort,unsigned scheduleBlock)
{
	unsigned char result=1,i_index=0;
	scheduleBlock = 1; // todo remove this
	cJSON *json_Full_Obejct;
	json_Full_Obejct = cJSON_CreateObject();
	cJSON *schedule_Array;
	cJSON *schedule_Array_Element;

	cJSON_AddNumberToObject(json_Full_Obejct, "client_id", EPROM_General.Cust_Detail.Client_Id);
	cJSON_AddNumberToObject(json_Full_Obejct, "group_id", EPROM_General.Cust_Detail.Reader_Id);
	cJSON_AddNumberToObject(json_Full_Obejct, "rtu_id", EPROM_General.Rtu_Detail.RTUId);
	cJSON_AddNumberToObject(json_Full_Obejct, "schedule_num", scheduleBlock);

	schedule_Array = cJSON_CreateArray();

	for(i_index = 0;i_index<4;i_index++)
	{
		schedule_Array_Element = cJSON_CreateObject();
	    if (schedule_Array_Element == NULL)
	    {
	    	result=0;
	        goto end;
	    }
		cJSON_AddNumberToObject(schedule_Array_Element, "enable", EPROM_Schedule.Schedule[((scheduleBlock-1)*1)+i_index].Sch_En_Di);
		cJSON_AddNumberToObject(schedule_Array_Element, "start_hour", EPROM_Schedule.Schedule[((scheduleBlock-1)*1)+i_index].Start_HH);
		cJSON_AddNumberToObject(schedule_Array_Element, "start_minute", EPROM_Schedule.Schedule[((scheduleBlock-1)*1)+i_index].Start_Min);
		cJSON_AddNumberToObject(schedule_Array_Element, "stop_hour", EPROM_Schedule.Schedule[((scheduleBlock-1)*1)+i_index].Stop_HH);
		cJSON_AddNumberToObject(schedule_Array_Element, "stop_minute", EPROM_Schedule.Schedule[((scheduleBlock-1)*1)+i_index].Stop_Min);
		cJSON_AddItemToArray(schedule_Array, schedule_Array_Element);
	}
	cJSON_AddItemToObject(json_Full_Obejct, "schedule", schedule_Array);

	if(sendPort == 1)//MQTT_PORT)
	{
		memset(MqttPubBuf, 0x00, CIM_MAX_SIZE_OF_MQTT_PAYLOAD);
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&MqttPubBuf,CIM_MAX_SIZE_OF_MQTT_PAYLOAD,false)))
		{
			result=0;
		}
	}
	else
	{
		result=0;
	}

	end:

	cJSON_Delete(json_Full_Obejct);

	return result;
}

/**************************************************************************//**
 * Function name 	: buildGetModeResponseJson
 * arguments		: 1)
 * 		 			:
 * return			:
 * Note				: Response:
 * 					  {"client_id": 5, "group_id": 5, "rtu_id": 5, "pcbplc_mode": 0}
 *****************************************************************************/

unsigned char buildGetModeResponseJson(unsigned char sendPort)
{
	unsigned char result=1;

	cJSON *json_Full_Obejct;
	json_Full_Obejct = cJSON_CreateObject();

	cJSON_AddNumberToObject(json_Full_Obejct, "client_id", EPROM_General.Cust_Detail.Client_Id);
	cJSON_AddNumberToObject(json_Full_Obejct, "group_id", EPROM_General.Cust_Detail.Reader_Id);
	cJSON_AddNumberToObject(json_Full_Obejct, "rtu_id", EPROM_General.Rtu_Detail.RTUId);
	cJSON_AddNumberToObject(json_Full_Obejct, "pcbplc_mode", EPROM_General.DoModeDetails.Do_Mode);

	if(sendPort == 1)//MQTT_PORT)
	{
		memset(MqttPubBuf, 0x00, CIM_MAX_SIZE_OF_MQTT_PAYLOAD);
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&MqttPubBuf,CIM_MAX_SIZE_OF_MQTT_PAYLOAD,false)))
		{
			result=0;
		}
	}
	else
	{
		result=0;
	}

	cJSON_Delete(json_Full_Obejct);

	return result;
}

unsigned char buildGetLograteResponseJson(unsigned char sendPort)
{
	unsigned char result=1;

	cJSON *json_Full_Obejct;
	json_Full_Obejct = cJSON_CreateObject();

	cJSON_AddNumberToObject(json_Full_Obejct, "client_id", EPROM_General.Cust_Detail.Client_Id);
	cJSON_AddNumberToObject(json_Full_Obejct, "group_id", EPROM_General.Cust_Detail.Reader_Id);
	cJSON_AddNumberToObject(json_Full_Obejct, "rtu_id", EPROM_General.Rtu_Detail.RTUId);
	cJSON_AddNumberToObject(json_Full_Obejct, "lograte", EPROM_General.LogRate);

	if(sendPort == 1)
	{
		memset(MqttPubBuf, 0x00, CIM_MAX_SIZE_OF_MQTT_PAYLOAD);
		if(!(cJSON_PrintPreallocated(json_Full_Obejct,(char *)&MqttPubBuf,CIM_MAX_SIZE_OF_MQTT_PAYLOAD,false)))
		{
			result=0;
		}
	}
	else
	{
		result=0;
	}

	cJSON_Delete(json_Full_Obejct);

	return result;
}