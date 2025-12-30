/*
 * json_parser.h
 *
 *  Created on: Jun 10, 2024
 *      Author: sanket
 */

#ifndef INC_JSON_PARSER_H_
#define INC_JSON_PARSER_H_
/**************************************************************************//**
 * Includes
 *****************************************************************************/
#include "cJSON.h"
/**************************************************************************//**
 * Constant
 *****************************************************************************/


/**************************************************************************//**
 * Ennam / uninon / Structure
 *****************************************************************************/
typedef enum
{
	JSON_SUCCESS = 0,
	FAILED_CREATE_JSON_OBJECT = -1,
	FAILED_ADD_JSON_OBJECT = -2,
	FAILED_TRANSFER_STRING_FROM_JSON_OBJECT = -3,
	FAILED_CREATE_ARRAY_JSON_OBJECT = -4,
}JSON_ERROR_RESPONSE;

typedef enum
{
	MQTT = 0,
	TCP,
	SERIAL,
}COM_TYPE;

typedef enum
{
	CMD_OTA = 0, // OTA
	CMD_CONF = 1,
	CMD_PRODUCTION = 2,
	CMD_AI_CAL = 3,
	CMD_SET_RTC,
}CMD_TYPE;

typedef enum {
    ACK_FAIL = 0,
	ACK_SUCCESS,
} RES_ACK;

/**************************************************************************//**
 * extern
 *****************************************************************************/
extern float gFinalAnaValF[];
extern unsigned char flagMQTTPubSchedule, flagMQTTPubGetMode, flagMqttPubLogData, flagMqttPubLiveData, flagMqttPubAlarmData, flagMQTTPubGetLograte,flagMQTTPubHistoryData, 
		flagMQTT_ID_First, flagMQTT_ID_afterPowerCycle, flagMQTT_TestMethod_1_ACK, flagMQTT_TestMethod_1_Result, flagMQTT_TestMethod_2_ACK, 
 		flagMQTT_TestMethod_2_Result, flagMQTT_ID_AI_CALI_App, lagMQTT_AI_Channel_CaliResponse, flagMQTT_AI_Channel_Test_Result;

extern unsigned char flagSERIAL_ID_First, flagSERIAL_ID_afterPowerCycle, flagSERIAL_TestMethod_1_ACK, flagSERIAL_TestMethod_1_Result,
		flagSERIAL_TestMethod_2_ACK, flagSERIAL_TestMethod_2_Result,	
		flagSERIAL_ID_AI_CALI_App, flagSERIAL_AI_Channel_CaliResponse, flagSERIAL_AI_Channel_Test_Result;

extern unsigned char pubScheduleBlock;
extern char mBinPath[256];
/**************************************************************************//**
 * function
 *****************************************************************************/
unsigned char buildProIdFrameJson(unsigned char sendPort,unsigned char afterPowerCycle);
unsigned char buildTestMethodAckJson(unsigned char sendPort,unsigned char TestMethod);
unsigned char buildTestMethodResultJson(unsigned char sendPort,unsigned char TestMethod);
JSON_ERROR_RESPONSE parse_JSON_frame(COM_TYPE com_mode, char* jsonString, char * ACK_Response);
unsigned char buildLograteDataJson(unsigned char sendPort);
unsigned char buildGetScheduleJson(unsigned char sendPort,unsigned scheduleBlock);
unsigned char buildGetModeResponseJson(unsigned char sendPort);
unsigned char buildGetLograteResponseJson(unsigned char sendPort);

#endif /* INC_JSON_PARSER_H_ */
