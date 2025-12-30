/**  
  @file
 mqtt_demo.h

  @brief
  This file provides the definitions for datacall demo, and declares the 
  API functions.

*/
/*============================================================================
  Copyright (c) 2020 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
 =============================================================================*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.


WHEN        WHO            WHAT, WHERE, WHY
----------  ------------   ----------------------------------------------------

=============================================================================*/

#ifndef MQTT_DEMO_H
#define MQTT_DEMO_H
#include "ql_api_sim.h"
		
#ifdef __cplusplus
		extern "C" {
#endif

#define CIM_MAX_SIZE_OF_MQTT_PAYLOAD 	2024

struct Configuration
{
	char  CMD_State;
	char  CMD_Type;
	char  Res_Ack;
	char  RTC_D_T[30];
};

extern uint8_t MqttPubBuf[CIM_MAX_SIZE_OF_MQTT_PAYLOAD];
extern uint8_t *ACK_string;
extern struct mqtt_connect_client_info_t  client_info;
extern char mqtt_recv_buf[1024];
//extern static ql_sem_t  mqtt_semp;
extern int  mqtt_connected;
extern unsigned char flag_modem_MQTT_Reconnect;
extern uint32_t fMQTTpublishCount,fMQTTreceivedCount;
extern uint8_t fNetworkRegistered,fInternetEnabled,fSubscribe,fMQTTClintInt;
extern ql_sim_status_e card_status;
//extern bool b_mqtt_message_received;
extern char IMEI[64];
extern char version_buf[64];
extern unsigned char gStopHistoricalDataStore;
		/*========================================================================
		 *	function Definition
		 *========================================================================*/
int ql_mqtt_app_init(void);
void Ethernet_MQTT_PUB_Routine(void);
int IsLogRateMatched(void);
void ExtFlash_WriteHistoricalData(void);
void ExtFlash_Read_RuntimePara(unsigned char makeDefault);
void ExtFlash_ReadHistoricalDataLogFromFlash(unsigned int pageCounter);
void correctionIP(char *newIP);
void SyncHistoricalDataToCloud(void);
		
#ifdef __cplusplus
		}/*"C" */
#endif
		
#endif   /*DATACALL_DEMO_H*/

