/*================================================================
  Copyright (c) 2020 Quectel Wireless Solution, Co., Ltd.  All Rights Reserved.
  Quectel Wireless Solution Proprietary and Confidential.
=================================================================*/
/*=================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

WHEN              WHO         WHAT, WHERE, WHY
------------     -------     -------------------------------------------------------------------------------

=================================================================*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ql_api_osi.h"
#include "ql_api_nw.h"
#include "gpio_DIDO.h"

#include "ql_log.h"
#include "ql_api_datacall.h"
#include "ql_mqttclient.h"
#include "ql_api_sim.h"
#include "ql_api_dev.h"
#include "ql_ssl.h"
#include "mqtt_demo.h"
#include "json_parser_sp.h"
#include "rs232_UART.h"
#include "configuration.h"
#include "I2C_RTC_LCD.h"
#include "spi_flash_at25ff.h"
#include "ql_power.h"

#define QL_MQTT_LOG_LEVEL	            QL_LOG_LEVEL_INFO
#define QL_MQTT_LOG(msg, ...)			QL_LOG(QL_MQTT_LOG_LEVEL, "ql_MQTT", msg, ##__VA_ARGS__)
#define QL_MQTT_LOG_PUSH(msg, ...)	    QL_LOG_PUSH("ql_MQTT", msg, ##__VA_ARGS__)
static ql_task_t mqtt_task = NULL;

#define MQTT_CLIENT_IDENTITY        "quectel_01"
#define MQTT_CLIENT_USER            ""
#define MQTT_CLIENT_PASS            ""

#define MQTT_CLIENT_ONENET_PRODUCTID             "417661"
#define MQTT_CLIENT_ONENET_ACCESSKEY             "aggblpDBWoKg2CGdbxp/CVKt9xXWcV162hJ1+Bhzvw4="
#define MQTT_CLIENT_ONENET_DEVICENAME            "test_led"
#define MQTT_CLIENT_ONENET_VERSION               "2018-10-31"

#define USE_CRT_BUFFER 0

#define MQTT_CLIENT_QUECTEL_URL                  "mqtt://203.88.135.129:1884"//""mqtt://220.180.239.212:8306"
#define MQTT_CLIENT_ONENET_URL                   "mqtt://203.88.135.129:1884"//"mqtt://mqtts.heclouds.com:1883" //onenet 的ip地址

#if USE_CRT_BUFFER
#define MQTT_CLIENT_QUECTEL_SSL_URL              "mqtt://203.88.135.129:1884"//"mqtts://112.31.84.164:8308"
#else
#define MQTT_CLIENT_QUECTEL_SSL_URL              "mqtt://203.88.135.129:1884"//"mqtts://220.180.239.212:8307"
#endif
#define MQTT_CLIENT_ONENET_SSL_URL               "mqtt://203.88.135.129:1884"//"mqtts://mqttstls.heclouds.com:8883"//onenet 的ip地址

// publist 的内容
#define MQTT_PUB_MSG0 "{\"id\": 000000,\"dp\": {\"temperatrue\": [{\"v\": 0.001,}],\"power\": [{\"v\": 0.001,}]}}"
#define MQTT_PUB_MSG1 "{\"id\": 111111,\"dp\": {\"temperatrue\": [{\"v\": 1.000,}],\"power\": [{\"v\": 1.001,}]}}"
#define MQTT_PUB_MSG2 "{\"id\": 222222,\"dp\": {\"temperatrue\": [{\"v\": 2.000,}],\"power\": [{\"v\": 2.002,}]}}"

uint8_t MqttPubBuf[CIM_MAX_SIZE_OF_MQTT_PAYLOAD];
uint8_t *ACK_string;
struct mqtt_connect_client_info_t  client_info = {0};
char mqtt_recv_buf[1024];
mqtt_client_t  mqtt_cli;

int MQTT_send_OTA_status = 0;
unsigned char gStopHistoricalDataStore = 0;
//unsigned char flagMQTTPubSchedule=0,flagMQTTPubGetMode = 0,flagMqttPubLogData = 0,flagMqttPubLiveData=0;
//unsigned char pubScheduleBlock=0;

static ql_sem_t  mqtt_semp;
int  mqtt_connected = 0;
unsigned char flag_modem_MQTT_Reconnect=0;
uint32_t fMQTTpublishCount,fMQTTreceivedCount;
uint8_t fNetworkRegistered = 0,fInternetEnabled = 0,fSubscribe=0,fMQTTClintInt=0;
ql_sim_status_e card_status = QL_SIM_STATUS_UNKNOW;
bool b_mqtt_message_received = 0;
char IMEI[64] = {0};
char version_buf[64] = {0};
char MQTT_Client_URL[200];
uint8_t MqttSubTopic[100] = "v1/devices/1/1/2";
static unsigned long lastWriteTime = 0; // Tracks last write time in ms

mqtt_client_t  mqtt_cli;

// static int networkDownStartDay = -1;       // Day of month when it went down
// static int networkDownStartHour = -1;      // Hour of day
// static bool networkWasEverDown = false;    // Flag to track if the issue started


// static int networkDownStartDay = -1;        // Day of month when network went down
// static int networkDownStartHour = -1;       // Hour when network went down
// static int networkDownStartMinute = -1;     // Minute when network went down
// static bool networkWasEverDown = false;     // Flag to track if the issue started

static int networkDownStartMonth = -1;
static int networkDownStartDay = -1;
static int networkDownStartHour = -1;
static bool networkWasEverDown = false;

#if USE_CRT_BUFFER
char *root_ca_crt_buffer= "-----BEGIN CERTIFICATE-----\r\n\
MIIEhDCCAuwCCQDuE1BpeAeMwzANBgkqhkiG9w0BAQsFADCBgjELMAkGA1UEBhMC\r\n\
Q04xCzAJBgNVBAgMAkFIMQswCQYDVQQHDAJIRjEQMA4GA1UECgwHUVVFQ1RFTDEL\r\n\
MAkGA1UECwwCU1QxFjAUBgNVBAMMDTExMi4zMS44NC4xNjQxIjAgBgkqhkiG9w0B\r\n\
CQEWE2VkZGllLnpoYW5nQHF1ZWN0ZWwwIBcNMjIwMTI1MDcyMzI3WhgPMjEyMjAx\r\n\
MDEwNzIzMjdaMIGCMQswCQYDVQQGEwJDTjELMAkGA1UECAwCQUgxCzAJBgNVBAcM\r\n\
AkhGMRAwDgYDVQQKDAdRVUVDVEVMMQswCQYDVQQLDAJTVDEWMBQGA1UEAwwNMTEy\r\n\
LjMxLjg0LjE2NDEiMCAGCSqGSIb3DQEJARYTZWRkaWUuemhhbmdAcXVlY3RlbDCC\r\n\
AaIwDQYJKoZIhvcNAQEBBQADggGPADCCAYoCggGBAMt3cjY0eLEDqv8Y7FomA+7N\r\n\
G5ztAbR7+P/WxjPlodqRDZ5HQORkfAr44gAZcWsKoo4DHTInwr9JBbBnETBMnL8+\r\n\
13h1PRp5CfwXKFvjppWYvBZfeTwhWQYbSMKINoS+d1Zl11jg/+ZbSd7Fi0bYq8ip\r\n\
Hbt30H+NANQZP1XQdsCf5/kvn+vXiP4EgJc56JQ9L6ALIF2Q6F3G/PTaYItg463N\r\n\
lv/S+eRi1VMDSs8Qc+DTlVwlgZZJdSlC8Yjr5pVqoyXm8ENKfSTrdhrLiKSWJTz9\r\n\
JUr04E7SJ+CoBAnLYNPHR2y0CFS/15aCa1JbK27ZJ/0cvBvzpWdkcgrDtKIcxNYM\r\n\
9QFPpehb1N4pgqi0NPhCkc/BasfmXUaTwM4ghhi4tQRptKMdTN/kdyC+V5a8Hyhb\r\n\
Nvw5qeJlLJKpgZ9X3HQzuKstKMkxLNuDIzK9TvO7zLowr+0BetUdllq+fDjXQM0M\r\n\
+9P3Xv2VmDwGRkmZ0IjYpDjm+qqGTFVLzzVwEqVD6wIDAQABMA0GCSqGSIb3DQEB\r\n\
CwUAA4IBgQAuNVwkBhd5nyWMmV/ESNxy59Sz+5FcesGclKjs4YocgcKbLD2bS+LN\r\n\
lKk6zenES7Cq6+l3NMAxxh/QhgHUCThAfREzfPXbmiicrUfaudN4YFivpoFwKIAs\r\n\
NczsL9S3FPbzAB4nLDATacc2BK0//aKMOU2t3KLNNomKbzlR+EW3wd0F1GoZ9SY6\r\n\
sCQeLa8Wp1KarOmbvgoFL/DAiTSqjjsU/Lq24dOCCctmG+qXRZxQa4npHD4xJwQJ\r\n\
qzA0JLu4n+DgoJftm1KpvB0wuzTn6M9+wnk5rv/fGc2t4Zra8B4prEReZZVfy65d\r\n\
cb8pBdb20Yrmznj+6DR50X/o/8Qzoyj9XpxtjwF23ql0XPYCI7kB03Ms9euP0btc\r\n\
HFacHapm0qBKx+vWy0V2Qf482OWSbewqaRbud44sErNoKqpqm02yN8PpsCywpFUj\r\n\
UC5G5DzxzYspMzQv/yidti0scMSKFObseZmNGlRYymCWhXnxmoCFjLpw5RnJSB2+\r\n\
cZ/1KFFHHZI=\r\n\
-----END CERTIFICATE-----";

char *client_crt_buffer= "-----BEGIN CERTIFICATE-----\r\n\
MIIEhDCCAuwCCQDuE1BpeAeMwzANBgkqhkiG9w0BAQsFADCBgjELMAkGA1UEBhMC\r\n\
Q04xCzAJBgNVBAgMAkFIMQswCQYDVQQHDAJIRjEQMA4GA1UECgwHUVVFQ1RFTDEL\r\n\
MAkGA1UECwwCU1QxFjAUBgNVBAMMDTExMi4zMS44NC4xNjQxIjAgBgkqhkiG9w0B\r\n\
CQEWE2VkZGllLnpoYW5nQHF1ZWN0ZWwwIBcNMjIwMTI1MDcyMzI3WhgPMjEyMjAx\r\n\
MDEwNzIzMjdaMIGCMQswCQYDVQQGEwJDTjELMAkGA1UECAwCQUgxCzAJBgNVBAcM\r\n\
AkhGMRAwDgYDVQQKDAdRVUVDVEVMMQswCQYDVQQLDAJTVDEWMBQGA1UEAwwNMTEy\r\n\
LjMxLjg0LjE2NDEiMCAGCSqGSIb3DQEJARYTZWRkaWUuemhhbmdAcXVlY3RlbDCC\r\n\
AaIwDQYJKoZIhvcNAQEBBQADggGPADCCAYoCggGBAMt3cjY0eLEDqv8Y7FomA+7N\r\n\
G5ztAbR7+P/WxjPlodqRDZ5HQORkfAr44gAZcWsKoo4DHTInwr9JBbBnETBMnL8+\r\n\
13h1PRp5CfwXKFvjppWYvBZfeTwhWQYbSMKINoS+d1Zl11jg/+ZbSd7Fi0bYq8ip\r\n\
Hbt30H+NANQZP1XQdsCf5/kvn+vXiP4EgJc56JQ9L6ALIF2Q6F3G/PTaYItg463N\r\n\
lv/S+eRi1VMDSs8Qc+DTlVwlgZZJdSlC8Yjr5pVqoyXm8ENKfSTrdhrLiKSWJTz9\r\n\
JUr04E7SJ+CoBAnLYNPHR2y0CFS/15aCa1JbK27ZJ/0cvBvzpWdkcgrDtKIcxNYM\r\n\
9QFPpehb1N4pgqi0NPhCkc/BasfmXUaTwM4ghhi4tQRptKMdTN/kdyC+V5a8Hyhb\r\n\
Nvw5qeJlLJKpgZ9X3HQzuKstKMkxLNuDIzK9TvO7zLowr+0BetUdllq+fDjXQM0M\r\n\
+9P3Xv2VmDwGRkmZ0IjYpDjm+qqGTFVLzzVwEqVD6wIDAQABMA0GCSqGSIb3DQEB\r\n\
CwUAA4IBgQAuNVwkBhd5nyWMmV/ESNxy59Sz+5FcesGclKjs4YocgcKbLD2bS+LN\r\n\
lKk6zenES7Cq6+l3NMAxxh/QhgHUCThAfREzfPXbmiicrUfaudN4YFivpoFwKIAs\r\n\
NczsL9S3FPbzAB4nLDATacc2BK0//aKMOU2t3KLNNomKbzlR+EW3wd0F1GoZ9SY6\r\n\
sCQeLa8Wp1KarOmbvgoFL/DAiTSqjjsU/Lq24dOCCctmG+qXRZxQa4npHD4xJwQJ\r\n\
qzA0JLu4n+DgoJftm1KpvB0wuzTn6M9+wnk5rv/fGc2t4Zra8B4prEReZZVfy65d\r\n\
cb8pBdb20Yrmznj+6DR50X/o/8Qzoyj9XpxtjwF23ql0XPYCI7kB03Ms9euP0btc\r\n\
HFacHapm0qBKx+vWy0V2Qf482OWSbewqaRbud44sErNoKqpqm02yN8PpsCywpFUj\r\n\
UC5G5DzxzYspMzQv/yidti0scMSKFObseZmNGlRYymCWhXnxmoCFjLpw5RnJSB2+\r\n\
cZ/1KFFHHZI=\r\n\
-----END CERTIFICATE-----";

char *client_key_buffer= "-----BEGIN RSA PRIVATE KEY-----\r\n\
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\r\n\
-----END RSA PRIVATE KEY-----";
#endif
// declare the function
void CheckNetworkAndRebootIfNeeded(void);
static void mqtt_state_exception_cb(mqtt_client_t *client)
{
	QL_MQTT_LOG("mqtt session abnormal disconnect");
	mqtt_connected = 0;
	fMQTTClintInt = 1;
}

static void mqtt_connect_result_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_e status)
{
	QL_MQTT_LOG("status: %d", status);
	if(status == 0){
		mqtt_connected = 1;
		fSubscribe = 0;
	}
	else
	{
		mqtt_connected = 0;
		//fNetworkRegistered = 0;
	}
	ql_rtos_semaphore_release(mqtt_semp);
}

static void mqtt_requst_result_cb(mqtt_client_t *client, void *arg,int err)
{
	QL_MQTT_LOG("err: %d", err);
	fMQTTpublishCount++;
	
	ql_rtos_semaphore_release(mqtt_semp);
}

static void mqtt_received_result_cb(mqtt_client_t *client, void *arg,int err)
{
	QL_MQTT_LOG("err: %d", err);

	ql_rtos_semaphore_release(mqtt_semp);
}

static void mqtt_inpub_data_cb(mqtt_client_t *client, void *arg, int pkt_id, const char *topic, const unsigned char *payload, unsigned short payload_len)
{
	QL_MQTT_LOG("topic: %s", topic); 		//ql_MQTT:mqtt_inpub_data_cb 209 topic: v1/devices/me/attributes
	QL_MQTT_LOG("payload: %s", payload); 	//ql_MQTT:mqtt_inpub_data_cb 210 payload: {"confirmed":true,"data":"040300A1002AA295","devEUI":"ac1f09fffe0e3c9c","deviceName":"dev_ac1f09fffe0d64dc","fPort":130,"timestamp":1700559196}	
	b_mqtt_message_received = 1;
	//lwgsmi_parse_string_using_length((const char**)&payload,(char*)mqtt_recv_buf,payload_len);
	memcpy(mqtt_recv_buf, payload, payload_len);
	fMQTTreceivedCount++;
}

static void mqtt_disconnect_result_cb(mqtt_client_t *client, void *arg,int err){
	QL_MQTT_LOG("err: %d", err);
	QL_MQTT_LOG("mqtt_disconnect_result_cb client: %s", client);
	ql_rtos_semaphore_release(mqtt_semp);
	mqtt_connected = 0;
	fMQTTClintInt = 1;
}

static void fillMQttClient(void)
{
	client_info.keep_alive = 60;
	client_info.pkt_timeout = 5;
	client_info.retry_times = 3;
	client_info.clean_session = 1;
	client_info.will_qos = 0;
	client_info.will_retain = 0;
	client_info.will_topic = NULL;
	client_info.will_msg = NULL;
	if(ProductionMode == 1)
	{
		client_info.client_id = &IMEI[0];
		client_info.client_user = MQTT_CLIENT_USER;
		client_info.client_pass = MQTT_CLIENT_PASS;
		//strcpy((char *)client_info.client_id, (const char *)pro_MQTT_Client_ID);//MQTT_CLIENT_IDENTITY;
		//client_info.client_id = pro_MQTT_Client_ID;//MQTT_CLIENT_IDENTITY;
		//client_info.client_user = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Name;
		//client_info.client_pass = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Pass;
	}
	else
	{
		client_info.client_id = &IMEI[0];
		client_info.client_user = MQTT_CLIENT_USER;
		client_info.client_pass = MQTT_CLIENT_PASS;

		//strcpy((char *)client_info.client_id, (const char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Cli_Id);
		//strcpy((char *)client_info.client_user, (const char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Name);
		//strcpy((char *)client_info.client_pass, (const char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Pass);

	}

	char mqttPort[6] = {0};
	memset(MQTT_Client_URL, 0, sizeof(MQTT_Client_URL));
	strcpy(MQTT_Client_URL, "mqtt://");
	if(ProductionMode == 1)
	{
		strcat(MQTT_Client_URL, (const char *)pro_MQTT_Broker_IP);
		strcat(MQTT_Client_URL, ":");
		itoa(pro_MQTT_Broker_Port, mqttPort, 10);
		strcat(MQTT_Client_URL, mqttPort);
	}
	else
	{
		correctionIP((char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP);
		strcat(MQTT_Client_URL, (const char *)EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP);
		// strcat(MQTT_Client_URL, (const char *)"14.102.161.101");
		strcat(MQTT_Client_URL, ":");
		itoa(EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port, mqttPort, 10);
		// itoa(1883, mqttPort, 10);
		strcat(MQTT_Client_URL, (const char *)mqttPort);
	}

	QL_MQTT_LOG("MQTT_Client_URL=%s", MQTT_Client_URL);
	QL_MQTT_LOG("clientid_str=%s", client_info.client_id);
	QL_MQTT_LOG("username_str=%s", client_info.client_user);
	QL_MQTT_LOG("password_str=%s", client_info.client_pass);
}





// test 

static void mqtt_app_thread(void * arg)
{
	int ret = 0;
	int i = 0;
	int profile_idx = 1;
    ql_data_call_info_s info;
	char ip4_addr_str[16] = {0};
	
	uint8_t nSim = 0;
	uint16_t sim_cid;
	JSON_ERROR_RESPONSE JSON_ret;
	ql_nw_reg_status_info_s nw_info;
    //char *token = NULL;
    //int is_user_onenet = 1;
	//int case_id = 0;

	ql_rtos_semaphore_create(&mqtt_semp, 0);
	ql_rtos_task_sleep_s(15);
    
	QL_MQTT_LOG("========== mqtt demo start ==========");
	
	ACK_string = &MqttPubBuf[0];

	ret = ql_dev_get_imei(IMEI, 64, 0);
	QL_MQTT_LOG("ret:0x%x, IMEI: %s", ret, IMEI);

	ql_dev_get_firmware_version(version_buf, sizeof(version_buf));

	QL_MQTT_LOG("Quectel modem version:  %s", version_buf);

	fillMQttClient();
	flagMqttPubLogData = 1;
	
	while(1)
	{	        
		CheckNetworkAndRebootIfNeeded();
		if((flag_modem_MQTT_Reconnect == 1)||(fMQTTClintInt == 1))
		{
			fMQTTClintInt = 0;
			flag_modem_MQTT_Reconnect = 0;
			ql_mqtt_disconnect(&mqtt_cli, mqtt_disconnect_result_cb, NULL);
			ql_rtos_task_sleep_s(2);
			ql_mqtt_client_deinit(&mqtt_cli);
			ql_rtos_task_sleep_s(2);
			ql_stop_data_call(nSim, profile_idx);
			fNetworkRegistered = 0;
			mqtt_connected = 0;
			fInternetEnabled = 0;
			ql_rtos_task_sleep_s(2);
			fillMQttClient();
			fMQTTClintInt = 0;
		}

		if(fNetworkRegistered == 0 || fInternetEnabled == 0)
		{
			ret = ql_sim_get_card_status(0, &card_status);
			QL_MQTT_LOG("ql_sim_get_card_status ret:0x%x, card_status: %d", ret, card_status);

			if(card_status == QL_SIM_STATUS_READY)
			{
				nSim = 0;
			}
			else
			{
				nSim = 1;
			}

			ret = ql_nw_get_reg_status(nSim, &nw_info);
			QL_MQTT_LOG("ql_nw_get_reg_status ret:0x%x, nw_info: %d", ret, nw_info);

            if((QL_NW_REG_STATE_HOME_NETWORK != nw_info.data_reg.state) && (QL_NW_REG_STATE_ROAMING != nw_info.data_reg.state))
            {
            	ql_rtos_task_sleep_s(2);
				QL_MQTT_LOG("====network register failure!!!!!====");
				fNetworkRegistered = 0;
				fInternetEnabled = 0;
				mqtt_connected = 0;
				
			// 	 store data to flash  check
			 	gStopHistoricalDataStore = 0;       // Enable historical data storage
				 QL_MQTT_LOG("No network detected. Enabling historical data storage.");
					buildLograteDataJson(1);
				//	QL_MQTT_LOG("Publishing Lograte Data: %s", MqttPubBuf);
					ExtFlash_WriteHistoricalData();	
					//gStopHistoricalDataStore = 1;	
            	continue;
            	//ql_rtos_semaphore_wait(ql_data_reg_sem[sim_id], QL_WAIT_FOREVER);
            }
            else
            {
            	fNetworkRegistered = 1;//nw_info.data_reg.state;
				QL_MQTT_LOG("====network registered!!!!====");
				 // Disable historical data storage
					// gStopHistoricalDataStore = 1;
					// QL_MQTT_LOG("Network registered. Disabling historical data storage.");
            }

			QL_MQTT_LOG("wait for network register done");
			i = 0;
			while((ret = ql_network_register_wait(nSim, 120)) != 0 && i < 10)
			{
				i++;
				ql_rtos_task_sleep_s(1);
			}

			if(ret == 0)
			{
				i = 0;
				QL_MQTT_LOG("====network registered!!!!====");
				fNetworkRegistered = 1;

				ql_set_data_call_asyn_mode(nSim, profile_idx, 0);

				QL_MQTT_LOG("===start data call====");
				if(ProductionMode == 1)
				{
					ret=ql_start_data_call(nSim, profile_idx, QL_PDP_TYPE_IP, (char *)pro_APN, NULL, NULL, 0);
				}
				else
				{
					// ret=ql_start_data_call(nSim, profile_idx, QL_PDP_TYPE_IP, "www", NULL, NULL, 0); 
					ret=ql_start_data_call(nSim, profile_idx, QL_PDP_TYPE_IP, EPROM_General.Mo_Comm.Mo_APN, NULL, NULL, 0); 
				}

				if(ret != 0){
					QL_MQTT_LOG("====data call failure:%d!!!!=====",ret);
					fInternetEnabled = 0;
				}
				else
				{
					QL_MQTT_LOG("====data call success:%d!!!!=====",ret);
					fInternetEnabled = 1;
				}

				memset(&info, 0x00, sizeof(ql_data_call_info_s));

				ret = ql_get_data_call_info(nSim, profile_idx, &info);
				if(ret != 0)
				{
					QL_MQTT_LOG("ql_get_data_call_info ret: %d", ret);
					ql_stop_data_call(nSim, profile_idx);
					fNetworkRegistered = 0;
					fInternetEnabled = 0;
					mqtt_connected = 0;
				}
				QL_MQTT_LOG("info->profile_idx: %d", info.profile_idx);
				QL_MQTT_LOG("info->ip_version: %d", info.ip_version);

				QL_MQTT_LOG("info->v4.state: %d", info.v4.state);
				inet_ntop(AF_INET, &info.v4.addr.ip, ip4_addr_str, sizeof(ip4_addr_str));
				QL_MQTT_LOG("info.v4.addr.ip: %s\r\n", ip4_addr_str);

				inet_ntop(AF_INET, &info.v4.addr.pri_dns, ip4_addr_str, sizeof(ip4_addr_str));
				QL_MQTT_LOG("info.v4.addr.pri_dns: %s\r\n", ip4_addr_str);

				inet_ntop(AF_INET, &info.v4.addr.sec_dns, ip4_addr_str, sizeof(ip4_addr_str));
				QL_MQTT_LOG("info.v4.addr.sec_dns: %s\r\n", ip4_addr_str);

				if(QL_DATACALL_SUCCESS != ql_bind_sim_and_profile(nSim, profile_idx, &sim_cid))
				{
					QL_MQTT_LOG("nSim or profile_idx is invalid!!!!");
				}

				if(ql_mqtt_client_init(&mqtt_cli, sim_cid) != MQTTCLIENT_SUCCESS)
				{
					QL_MQTT_LOG("mqtt client init failed!!!!");
					ql_rtos_task_sleep_s(5);
				}
				else
				{
					QL_MQTT_LOG("mqtt_cli:%d", mqtt_cli); 
				}

			}
			else
			{
				QL_MQTT_LOG("====network register failure!!!!!====");
				fNetworkRegistered = 0;
				fInternetEnabled = 0;
				mqtt_connected = 0;
				
			}
		}

		int ret = MQTTCLIENT_SUCCESS;
		
		QL_MQTT_LOG("==============mqtt_client_test[]================%d,%d,%d\n",fNetworkRegistered, fInternetEnabled, mqtt_connected);


		if((fNetworkRegistered == 1) && (fInternetEnabled == 1))
		{
			if(ql_mqtt_client_is_connected(&mqtt_cli) == 0)
			{
				ret = ql_mqtt_connect(&mqtt_cli, MQTT_Client_URL, mqtt_connect_result_cb, NULL, (const struct mqtt_connect_client_info_t *)&client_info, mqtt_state_exception_cb);
				if(ret  == MQTTCLIENT_WOUNDBLOCK)
				{
					QL_MQTT_LOG("====wait connect result");
					ql_rtos_semaphore_wait(mqtt_semp, QL_WAIT_FOREVER);
				}
				else
				{
					QL_MQTT_LOG("===mqtt connect failed ,ret = %d",ret);
					// gStopHistoricalDataStore = 0;
					// buildLograteDataJson(1);
					// QL_MQTT_LOG("Publishing mqttfailed data Data: %s", MqttPubBuf);
					// ExtFlash_WriteHistoricalData();	
					//break;
				}
				ql_rtos_task_sleep_s(5);
				ql_mqtt_set_inpub_callback(&mqtt_cli, mqtt_inpub_data_cb, NULL);
			}
		}

        if((fNetworkRegistered == 1) && (mqtt_connected == 1))
        {
			if(fSubscribe == 0)
			{
				sprintf((char *)MqttSubTopic, "v1/devices/%d/%d/%d", EPROM_General.Cust_Detail.Client_Id, EPROM_General.Cust_Detail.Reader_Id, EPROM_General.Rtu_Detail.RTUId);
				if(ql_mqtt_sub_unsub((mqtt_client_t *)&mqtt_cli, (char *)MqttSubTopic, 1, mqtt_received_result_cb, NULL, 1) == MQTTCLIENT_WOUNDBLOCK)
				{
					QL_MQTT_LOG("======wait subscrible result");
					ql_rtos_semaphore_wait(mqtt_semp, QL_WAIT_FOREVER);
					fSubscribe = 1;
				}
			}

			if(1 == b_mqtt_message_received)
			{
				b_mqtt_message_received = 0;
				JSON_ret = parse_JSON_frame(MQTT,(char *)mqtt_recv_buf,(char *)ACK_string);
				if(JSON_ret!=JSON_SUCCESS)
				{
					QL_MQTT_LOG("parse_JSON_frame error");
				}
			}			
			if (IsLogRateMatched())
            {
				QL_MQTT_LOG("Lograte matched, publishing lograte data...");
				
            	// Build_Data_for_server();
				//if(0.2>(IR || IY || IB) )
				//{hi.sp}
            	flagMqttPubLogData = 1;
            }
			Ethernet_MQTT_PUB_Routine();
			
				
			// add log to check if the data is stored in flash
			QL_MQTT_LOG("g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter = %d", g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);
			SyncHistoricalDataToCloud();
			// if(g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter > 0)
			// {
			// 	//ExtFlash_ReadHistoricalDataLogFromFlash(g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);
			// 	Ethernet_MQTT_PUB_Routine();
			// 	g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter--;	

			// }
			gStopHistoricalDataStore = 1;
        }
		else
			{
				flag_modem_MQTT_Reconnect = 1;  // Reconnect to MQTT server
				QL_MQTT_LOG("MQTT not connected, storing to history data");
					gStopHistoricalDataStore = 0;
					buildLograteDataJson(1);
					QL_MQTT_LOG("Publishing Lograte Data: %s", MqttPubBuf);
					ExtFlash_WriteHistoricalData();	
					//gStopHistoricalDataStore = 1;		
			}
		// if (gStopHistoricalDataStore == 0)
		// {
		// 	//flag_modem_MQTT_Reconnect = 1;  // Reconnect to MQTT server
		// 		gStopHistoricalDataStore = 0;

		// 	// time to write to flash
		
		// 		//static unsigned long lastWriteTime = 0; // Tracks last write time in ms

		// 		// Validate RTC time
		// 		if (rtc_time.mHour > 23 || rtc_time.minute > 59 || rtc_time.mSecond > 59)
		// 		{
		// 			QL_MQTT_LOG("Invalid RTC time values");
		// 			return;
		// 		}

		// 		// Calculate current time in milliseconds
		// 		unsigned long currentTime = 
		// 			((unsigned long)rtc_time.mHour * 3600000UL) + 
		// 			((unsigned long)rtc_time.minute * 60000UL) + 
		// 			((unsigned long)rtc_time.mSecond * 1000UL);

		// 		QL_MQTT_LOG("Current time: %lu", currentTime);

		// 		// Check for exact 5-minute boundary and second == 0
		// 		if ((rtc_time.minute % 5 == 0) && (rtc_time.mSecond >= 0))
		// 		// Check for exact 1-hour boundary (minute == 0 and second == 0)
		// 		//if ((rtc_time.minute == 0) && (rtc_time.mSecond >= 0))
		// 		{
		// 			if ((currentTime - lastWriteTime) >= 300000UL) // 5 minutes = 300,000 ms //1 hr = 3600000
		// 			{
		// 				lastWriteTime = currentTime;

		// 				QL_MQTT_LOG("Storing history log at: %02d:%02d:%02d", 
		// 							rtc_time.mHour, rtc_time.minute, rtc_time.mSecond);

		// 				buildLograteDataJson(1);
		// 				QL_MQTT_LOG("Publishing Lograte Data: %s", MqttPubBuf);
		// 				ExtFlash_WriteHistoricalData();
		// 			}
		// 			else
		// 			{
		// 				QL_MQTT_LOG("Duplicate call ignored at %02d:%02d:%02d", 
		// 							rtc_time.mHour, rtc_time.minute, rtc_time.mSecond);
		// 			}
		// 		}
	
		// }
		
		
		ql_rtos_task_sleep_ms(1000);
		//ql_rtos_task_sleep_ms(500);
	}
    ql_rtos_semaphore_delete(mqtt_semp);
    ql_rtos_task_delete(mqtt_task);	

   return;	
}


int ql_mqtt_app_init(void)
{
	QlOSStatus err = QL_OSI_SUCCESS;
	
    err = ql_rtos_task_create(&mqtt_task, 25*1024, APP_PRIORITY_ABOVE_NORMAL, "QmqttApp", mqtt_app_thread, NULL, 5);
	if(err != QL_OSI_SUCCESS)
    {
		QL_MQTT_LOG("mqtt_app init failed");
	}

	return err;
}

void Ethernet_MQTT_PUB_Routine(void)
{
	unsigned char pubTopic = 0; // 0 = "v1/devices/Response" 1 = "v1/devices/me/log"

	if(MQTT_send_OTA_status)				// OTA status
	{
		MQTT_send_OTA_status = 0;
		pubTopic = 3;
	}
	else if(flagMQTTPubSchedule)				// Schedule
	{
		flagMQTTPubSchedule = 0;
		buildGetScheduleJson(1,pubScheduleBlock);
		QL_MQTT_LOG("Publishing flagMQTTPubSchedule Data: %s", MqttPubBuf); // Log the history data string
		pubTopic = 0;
	}
	else if(flagMQTTPubGetMode)				// Mode
	{
		flagMQTTPubGetMode = 0;
		buildGetModeResponseJson(1);
		QL_MQTT_LOG("Publishing MQTTPubGetMode Data: %s", MqttPubBuf); // Log the history data string
		pubTopic = 0;
	}
	else if(flagMqttPubLogData)				// Log data
	{
		flagMqttPubLogData = 0;
		buildLograteDataJson(1);
		QL_MQTT_LOG("Publishing mqttpublogdata Data: %s", MqttPubBuf); 
		pubTopic = 1;
	}
	else if(flagMQTTPubGetLograte)			// Log rate
	{
		flagMQTTPubGetLograte = 0;
		buildGetLograteResponseJson(1);
		QL_MQTT_LOG("Publishing mqtt get logData: %s", MqttPubBuf); // 
		pubTopic = 0;
	}
	else if(flagMQTTPubHistoryData)			// History data
	{
		flagMQTTPubHistoryData = 0;
		// buildGetLograteResponseJson(1);
		QL_MQTT_LOG("Publishing History Data: %s", MqttPubBuf); // Log the history data string
		pubTopic = 1;
	}
	else									// Default return
	{
		return;
	}

	//  // Check network and MQTT connection
    // if (fNetworkRegistered == 0 || mqtt_connected == 0)
    // {
	// 	buildLograteDataJson(1);
	// //	QL_MQTT_LOG("Publishing Lograte Data: %s", MqttPubBuf);
	// 	ExtFlash_WriteHistoricalData();	
	// 	//gStopHistoricalDataStore = 1;	
    //     QL_MQTT_LOG("No network or MQTT connection. Storing event in history.");
       
		
    //     return;
    // }

	if(pubTopic == 0)						// Response
	{
		if(ql_mqtt_publish(&mqtt_cli, "v1/devices/Response",(char *)&MqttPubBuf, (unsigned short)strlen((char *)MqttPubBuf), 0, 0, mqtt_requst_result_cb,NULL) == MQTTCLIENT_WOUNDBLOCK){
			QL_MQTT_LOG("======wait publish result");
			ql_rtos_semaphore_wait(mqtt_semp, QL_WAIT_FOREVER);
		}
		ql_rtos_task_sleep_ms(100);
		QL_MQTT_LOG("MQTT modem_MQTT_publish \"v1/devices/Response\"");
	}
	else if(pubTopic == 1)					// Log rate
	{
		QL_MQTT_LOG("MqttPubBuf:%s", MqttPubBuf);
		if(ql_mqtt_publish(&mqtt_cli, "v1/devices/me/log",(char *)&MqttPubBuf, (unsigned short)strlen((char *)MqttPubBuf), 0, 0, mqtt_requst_result_cb,NULL) == MQTTCLIENT_WOUNDBLOCK){
			QL_MQTT_LOG("======wait publish result");
			ql_rtos_semaphore_wait(mqtt_semp, QL_WAIT_FOREVER);
		}
		ql_rtos_task_sleep_ms(100);
		QL_MQTT_LOG("MQTT modem_MQTT_publish \"v1/devices/me/log\"");
	}
}

int IsLogRateMatched(void)
{
	int tIsMatched = 0;
	static unsigned char tsOld_minute = 0;  // Initialize to 0 for first comparison
	static unsigned char tsIsFirstTime = 1; // Flag to indicate the first run
	static unsigned char tsOld_hour = 0;    // Initialize to 0 for first comparison

	// Ensure rtc_time and EPROM_General are properly initialized before use
	if (rtc_time.mHour < 0 || rtc_time.mHour > 23 || rtc_time.minute < 0 || rtc_time.minute > 59 || rtc_time.mSecond < 0 || rtc_time.mSecond > 59)
	{
		QL_MQTT_LOG("Invalid RTC time values");
		return 0;
	}

	if (EPROM_General.LogRate <= 0)
	{
		QL_MQTT_LOG("Invalid LogRate value");
		return 0;
	}

	int nowInSeconds = (rtc_time.mHour * 3600) + (rtc_time.minute * 60) + rtc_time.mSecond;
	int logRateInSeconds = EPROM_General.LogRate * 60;

	if ((nowInSeconds % logRateInSeconds) < 2)  // 2-second window
	{
		if ((tsIsFirstTime == 1) || (tsOld_minute != rtc_time.minute))
		{
			tsIsFirstTime = 0;
			tsOld_minute = rtc_time.minute;
			tsOld_hour = rtc_time.mHour;
			tIsMatched = 1;

			QL_MQTT_LOG("Log rate matched at hour: %d, minute: %d", tsOld_hour, tsOld_minute);
		}
		else if ((EPROM_General.LogRate % 60) == 0 && tsOld_hour != rtc_time.mHour)
		{
			tsOld_hour = rtc_time.mHour;
			tsOld_minute = rtc_time.minute;
			tIsMatched = 1;

			QL_MQTT_LOG("Hourly log rate matched at hour: %d, minute: %d", tsOld_hour, tsOld_minute);
		}
	}

	return tIsMatched;
}


// int IsLogRateMatched(void)
// {
//     int tIsMatched = 0;
//     static unsigned char tsOld_minute;
//     static unsigned char tsIsFirstTime = 1;
//     static unsigned char tsOld_hour;

//     if ((((rtc_time.mHour * 3600) + (rtc_time.minute * 60) + rtc_time.mSecond) % (EPROM_General.LogRate * 60)) < 2)  // for log rate data structure match to use in IsLogRateMatched() func || ticket : https://cimcondigital.atlassian.net/browse/IRTU6000PP-22
//     {
//         if((1 == tsIsFirstTime) || (tsOld_minute != rtc_time.minute))
//         {
//             tsIsFirstTime = 0;
//             tsOld_minute = rtc_time.minute;
//             tsOld_hour = rtc_time.mHour;
//             tIsMatched = 1;
			

//             QL_MQTT_LOG("Log rate matched at hour: %d, minute: %d", tsOld_hour, tsOld_minute);

//         }
//         else if ( ((EPROM_General.LogRate % 60) == 0) && (tsOld_hour != rtc_time.mHour))  // for log rate data structure match to use in IsLogRateMatched() func || ticket : https://cimcondigital.atlassian.net/browse/IRTU6000PP-22
//         {
//             tsOld_hour = rtc_time.mHour;
//             tIsMatched = 1;
// 			QL_MQTT_LOG("Log rate matched at hour: %d, minute: %d", tsOld_hour, tsOld_minute);
//         }
//     }
//     return (tIsMatched);
// }

// void ExtFlash_WriteHistoricalData(void)
// {
// 	unsigned int WriteAddress;

// 	if(gStopHistoricalDataStore == 0)
// 	{
// 		if(g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter<MAX_HISTORY_DATA_PACKETS)
// 		{
// 			WriteAddress = g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter*1024+HISTORY_DATA_FILE_START_ADDRESS;
// 			QL_MQTT_LOG("Writing history data to flash at address: 0x%X, Page Counter: %d", WriteAddress, g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);
// 			at25dfx_write((uint8_t *)&MqttPubBuf, 1024, WriteAddress);
// 		}
// 		else
// 		{
// 			QL_MQTT_LOG("History data storage full. Overwriting from the beginning.");
// 			g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter = 0;
// 			g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten = 1;
// 			WriteAddress = g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter*1024+HISTORY_DATA_FILE_START_ADDRESS;
// 			QL_MQTT_LOG("Overwriting history data to flash at address: 0x%X", WriteAddress);
// 			at25dfx_write((uint8_t *)&MqttPubBuf, 1024, WriteAddress);
// 		}
// 		g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter++;

// 		QL_MQTT_LOG("Incremented Page Counter: %d", g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);

//         // Log the data being written
//         QL_MQTT_LOG("Data being written to flash: %s", MqttPubBuf);

// 		at25dfx_write((uint8_t *)&MqttPubBuf, 1024, WriteAddress);
// 	}
// 	else
// 	{
// 		QL_MQTT_LOG("Historical data storage disabled.");
// 	}
// }

void ExtFlash_WriteHistoricalData(void)

	{
			
				gStopHistoricalDataStore = 0;

			
				//static unsigned long lastWriteTime = 0; // Tracks last write time in ms

				// Validate RTC time
				if (rtc_time.mHour > 23 || rtc_time.minute > 59 || rtc_time.mSecond > 59)
				{
					QL_MQTT_LOG("Invalid RTC time values");
					return;
				}

				// Calculate current time in milliseconds
				unsigned long currentTime = 
					((unsigned long)rtc_time.mHour * 3600000UL) + 
					((unsigned long)rtc_time.minute * 60000UL) + 
					((unsigned long)rtc_time.mSecond * 1000UL);

				QL_MQTT_LOG("Current time: %lu", currentTime);

				// Check for exact 5-minute boundary and second == 0
				 if (((rtc_time.minute % 2 == 0) && (rtc_time.mSecond <= 2) )|| (flagMqttPubLogData ==1))     // 2 min 
			
				// Check for exact 1-hour boundary (minute == 0 and second == 0)

				//if (((rtc_time.minute == 0) && (rtc_time.mSecond <= 2)) || (flagMqttPubLogData ==1))         // hourly check 
				
				//if ((rtc_time.minute == 0) && (rtc_time.mSecond <= 2)) //|| (flagMqttPubLogData ==1))
				// {
					//if ((currentTime - lastWriteTime) >= 3600000UL) // 5 minutes = 300,000 ms //1 hr = 3600000
					// if ((currentTime - lastWriteTime) >= 120000UL) 
					 {
						flagMqttPubLogData = 0;
						lastWriteTime = currentTime;

						QL_MQTT_LOG("Storing history log at: %02d:%02d:%02d", 
									rtc_time.mHour, rtc_time.minute, rtc_time.mSecond);

						buildLograteDataJson(1);
						QL_MQTT_LOG("Publishing Lograte Data: %s", MqttPubBuf);
													
						{
							unsigned int WriteAddress;

							if (gStopHistoricalDataStore == 0)
							{
								// Parse the JSON string in MqttPubBuf
								cJSON *root = cJSON_Parse((char *)MqttPubBuf);
								if (root == NULL)
								{
									QL_MQTT_LOG("Failed to parse JSON from MqttPubBuf");
									return;
								}

								// Get the di_status array
								cJSON *di_status = cJSON_GetObjectItemCaseSensitive(root, "di_status");
								if (di_status != NULL && cJSON_IsArray(di_status))
								{
									// Modify di_status[7] to 0
									cJSON_ReplaceItemInArray(di_status, 7, cJSON_CreateNumber(0));
								}
								else
								{
									QL_MQTT_LOG("di_status array not found or invalid");
									cJSON_Delete(root);
									return;
								}

								// Serialize the modified JSON back to MqttPubBuf
								char *modifiedString = cJSON_PrintUnformatted(root);
								if (modifiedString == NULL)
								{
									QL_MQTT_LOG("Failed to serialize modified JSON");
									cJSON_Delete(root);
									return;
								}
								strncpy((char *)MqttPubBuf, modifiedString, sizeof(MqttPubBuf) - 1);
								MqttPubBuf[sizeof(MqttPubBuf) - 1] = '\0'; // Ensure null termination
								free(modifiedString);
								cJSON_Delete(root);



								// Determine the write address
								if (g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter < MAX_HISTORY_DATA_PACKETS)
								{
									WriteAddress = g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter * 1024 + HISTORY_DATA_FILE_START_ADDRESS;
									QL_MQTT_LOG("Writing history data to flash at address: 0x%X, Page Counter: %d", WriteAddress, g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);
								}
								else
								{
									QL_MQTT_LOG("History data storage full. Overwriting from the beginning.");
									g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter = 0;
									g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten = 1;
									WriteAddress = g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter * 1024 + HISTORY_DATA_FILE_START_ADDRESS;
									QL_MQTT_LOG("Overwriting history data to flash at address: 0x%X", WriteAddress);
								}

								// Write the modified data to flash
								at25dfx_write((uint8_t *)&MqttPubBuf, 1024, WriteAddress);

								// Increment the page counter
								g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter++;
								ExtFlash_Write_HistoryPara();
								QL_MQTT_LOG("Incremented Page Counter: %d", g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);

								// Log the modified data being written
								QL_MQTT_LOG("Data being written to flash: %s", MqttPubBuf);
							}
							else
							{
								QL_MQTT_LOG("Historical data storage disabled.");
							}
					
						}
						
					 }
					else
					{
						QL_MQTT_LOG("Duplicate call ignored at %02d:%02d:%02d", 
									rtc_time.mHour, rtc_time.minute, rtc_time.mSecond);
					}
				// }


	}

// {
//     unsigned int WriteAddress;

//     if (gStopHistoricalDataStore == 0)
//     {
//         // Parse the JSON string in MqttPubBuf
//         cJSON *root = cJSON_Parse((char *)MqttPubBuf);
//         if (root == NULL)
//         {
//             QL_MQTT_LOG("Failed to parse JSON from MqttPubBuf");
//             return;
//         }

//         // Get the di_status array
//         cJSON *di_status = cJSON_GetObjectItemCaseSensitive(root, "di_status");
//         if (di_status != NULL && cJSON_IsArray(di_status))
//         {
//             // Modify di_status[7] to 0
//             cJSON_ReplaceItemInArray(di_status, 7, cJSON_CreateNumber(0));
//         }
//         else
//         {
//             QL_MQTT_LOG("di_status array not found or invalid");
//             cJSON_Delete(root);
//             return;
//         }

//         // Serialize the modified JSON back to MqttPubBuf
//         char *modifiedString = cJSON_PrintUnformatted(root);
//         if (modifiedString == NULL)
//         {
//             QL_MQTT_LOG("Failed to serialize modified JSON");
//             cJSON_Delete(root);
//             return;
//         }
//         strncpy((char *)MqttPubBuf, modifiedString, sizeof(MqttPubBuf) - 1);
//         MqttPubBuf[sizeof(MqttPubBuf) - 1] = '\0'; // Ensure null termination
//         free(modifiedString);
//         cJSON_Delete(root);



//         // Determine the write address
//         if (g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter < MAX_HISTORY_DATA_PACKETS)
//         {
//             WriteAddress = g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter * 1024 + HISTORY_DATA_FILE_START_ADDRESS;
//             QL_MQTT_LOG("Writing history data to flash at address: 0x%X, Page Counter: %d", WriteAddress, g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);
//         }
//         else
//         {
//             QL_MQTT_LOG("History data storage full. Overwriting from the beginning.");
//             g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter = 0;
//             g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten = 1;
//             WriteAddress = g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter * 1024 + HISTORY_DATA_FILE_START_ADDRESS;
//             QL_MQTT_LOG("Overwriting history data to flash at address: 0x%X", WriteAddress);
//         }

//         // Write the modified data to flash
//         at25dfx_write((uint8_t *)&MqttPubBuf, 1024, WriteAddress);

//         // Increment the page counter
//         g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter++;
//         QL_MQTT_LOG("Incremented Page Counter: %d", g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter);

//         // Log the modified data being written
//         QL_MQTT_LOG("Data being written to flash: %s", MqttPubBuf);
//     }
//     else
//     {
//         QL_MQTT_LOG("Historical data storage disabled.");
//     }
// // 	}
	
// // }
// }


void SyncHistoricalDataToCloud(void)
{
	QL_MQTT_LOG("SyncHistoricalDataToCloud: Start syncing historical data. SendPointer=%d, PageCounter=%d, Overwritten=%d",
		g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer,
		g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter,
		g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten);
   
    if (fNetworkRegistered && fInternetEnabled && mqtt_connected)
    {
		
        unsigned int sendPtr = g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer;
        unsigned int writePtr = g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter;

        unsigned int pending;
        if (writePtr >= sendPtr) {
            pending = writePtr - sendPtr;
        } else {
            pending = (MAX_HISTORY_DATA_PACKETS - sendPtr) + writePtr;
        }

        QL_MQTT_LOG("SyncHistoricalDataToCloud: Start syncing. SendPointer=%u, PageCounter=%u, Pending=%u, Overwritten=%u",
            sendPtr, writePtr, pending, g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten);

        for (unsigned int i = 0; i < pending; i++) {
            unsigned int page = (sendPtr + i) % MAX_HISTORY_DATA_PACKETS;
            unsigned int addr = page * 1024 + HISTORY_DATA_FILE_START_ADDRESS;

            at25dfx_read((uint8_t *)&MqttPubBuf, 1024, addr);

			QL_MQTT_LOG("SyncHistoricalDataToCloud: Data read from flash: %s", MqttPubBuf);

            // Extract time for debug
            char* time_ptr = strstr((char*)MqttPubBuf, "\"time\":\"");
            char time_val[16] = {0};
            if (time_ptr) {
                strncpy(time_val, time_ptr + 8, 6);
                time_val[6] = '\0';
            }


			QL_MQTT_LOG("SyncHistoricalDataToCloud: About to send log data to cloud. Page: %u, Addr: 0x%X, Time: %s, Data: %s",
                page, addr, time_val, MqttPubBuf);
            QL_MQTT_LOG("SyncHistory: Sending page %u at addr 0x%X, time: %s", page, addr, time_val);

            if (ql_mqtt_publish(&mqtt_cli, "v1/devices/me/log", (char *)&MqttPubBuf, (unsigned short)strlen((char *)MqttPubBuf), 0, 0, mqtt_requst_result_cb, NULL) == MQTTCLIENT_WOUNDBLOCK) {
                QL_MQTT_LOG("======wait publish result");
                ql_rtos_semaphore_wait(mqtt_semp, QL_WAIT_FOREVER);
            }
            ql_rtos_task_sleep_ms(200);

            // Move the send pointer forward and persist it
            g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer = (page + 1) % MAX_HISTORY_DATA_PACKETS;
            ExtFlash_Write_HistoryPara();
        }
    }
}

// void SyncHistoricalDataToCloud(void)
// {
//     // Only sync if network and MQTT are up
//     if (fNetworkRegistered && fInternetEnabled && mqtt_connected)
//     {
//         QL_MQTT_LOG("SyncHistoricalDataToCloud: Start syncing historical data. SendPointer=%d, PageCounter=%d, Overwritten=%d",
//             g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer,
//             g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter,
//             g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten);
//         while (g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer < g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter)
//         {
//             QL_MQTT_LOG("SyncHistoricalDataToCloud: Reading page %u from flash for sync", g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer);
//             unsigned int page = g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer;
//             unsigned int addr = page * 1024 + HISTORY_DATA_FILE_START_ADDRESS;

//             // Read record from flash into MqttPubBuf
//             at25dfx_read((uint8_t *)&MqttPubBuf, 1024, addr);
// 			// Log the data read from flash before publishing
// 			QL_MQTT_LOG("SyncHistoricalDataToCloud: Data read from flash: %s", MqttPubBuf);
// 			//ExtFlash_ReadHistoricalDataLogFromFlash(g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer);

					
// 			// 		MqttPubBuf[1023] = '\0';   // Or at least at buffer_size-1
// 			// // Check if the data is valid (not erased/empty)
// 			// if (MqttPubBuf[0] != 0xFF)
// 			// {
// 			// 	flagMQTTPubHistoryData = 1;
// 			// 	if (g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten == 1)
// 			// 	{
// 			// 		QL_MQTT_LOG("History Data (Overwritten): %s", MqttPubBuf); // Log the history data string
// 			// 	}
// 			// 	else
// 			// 	{
// 			// 		QL_MQTT_LOG("History Data: %s", MqttPubBuf); // Log the history data string
// 			// 	}
// 			// }





//             // Extract time for debug
//             char* time_ptr = strstr((char*)MqttPubBuf, "\"time\":\"");
//             char time_val[16] = {0};
//             if (time_ptr) {
//                 strncpy(time_val, time_ptr + 8, 6);
//                 time_val[6] = '\0';
//             }

//             QL_MQTT_LOG("SyncHistoricalDataToCloud: About to send log data to cloud. Page: %u, Addr: 0x%X, Time: %s, Data: %s",
//                 page, addr, time_val, MqttPubBuf);
//             QL_MQTT_LOG("SyncHistory: Sending page %u at addr 0x%X, time: %s", page, addr, time_val);

//             // Send to cloud (adjust as needed for your publish function)
//             if (ql_mqtt_publish(&mqtt_cli, "v1/devices/me/log", (char *)&MqttPubBuf, (unsigned short)strlen((char *)MqttPubBuf), 0, 0, mqtt_requst_result_cb, NULL) == MQTTCLIENT_WOUNDBLOCK) {
//                 QL_MQTT_LOG("======wait publish result");
//                 ql_rtos_semaphore_wait(mqtt_semp, QL_WAIT_FOREVER);
//             }
//             ql_rtos_task_sleep_ms(200);

//             // On success, move the send pointer forward and persist it
//             g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer++;
//             ExtFlash_Write_HistoryPara();
//         }
//     }
// }


void ExtFlash_Read_HistoryPara(unsigned char makeDefault)
{
    at25dfx_read((uint8_t*) &g_flashhistoryParaSturct, sizeof(g_flashhistoryParaSturct), HISTORY_DATA_RUN_TIME_PARA_START_ADDRESS);


	if ((g_flashhistoryParaSturct.s_ExtDataFlash_CheckByte != 0xAB) || (makeDefault == 1) ||
        (g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer == 0xFFFFFFFF) ||   
        (g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter == 0xFFFFFFFF))    
   // if ((g_flashhistoryParaSturct.s_ExtDataFlash_CheckByte != 0xAB) || (makeDefault == 1))
	 {
        // Set default values if not initialized
        g_flashhistoryParaSturct.s_ExtDataFlash_CheckByte = 0xAB;
        g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten = 0;
        g_flashhistoryParaSturct.s_ExtDataFlash_PageCounter = 0;
		g_flashhistoryParaSturct.s_ExtDataFlash_SendPointer = 0; // Initialize send pointer
        g_flashhistoryParaSturct.s_temp_Counter = 0;
        memset(g_flashhistoryParaSturct.unused, 0, sizeof(g_flashhistoryParaSturct.unused));
        // Write default struct to flash
        at25dfx_write((uint8_t *)&g_flashhistoryParaSturct, sizeof(g_flashhistoryParaSturct), HISTORY_DATA_RUN_TIME_PARA_START_ADDRESS);
    }
}
void ExtFlash_Write_HistoryPara(void)
{
    at25dfx_write((uint8_t *)&g_flashhistoryParaSturct, sizeof(g_flashhistoryParaSturct), HISTORY_DATA_RUN_TIME_PARA_START_ADDRESS);
}

void ExtFlash_Read_RuntimePara(unsigned char makeDefault)
{
	at25dfx_read((uint8_t*) &g_flashRunTimeParaSturct, sizeof(g_flashRunTimeParaSturct), HISTORY_DATA_RUN_TIME_PARA_START_ADDRESS);

	if((g_flashRunTimeParaSturct.s_ExtDataFlash_CheckByte != 0xAB)||(makeDefault == 1))
	{
		g_flashRunTimeParaSturct.s_ExtDataFlash_CheckByte = 0xAB;
		g_flashRunTimeParaSturct.TotalMinuteckt1 = 0;
		g_flashRunTimeParaSturct.TotalMinuteckt2 = 0;
		QL_MQTT_LOG("RESET");
		at25dfx_write((uint8_t *)&g_flashRunTimeParaSturct, sizeof(g_flashRunTimeParaSturct), HISTORY_DATA_RUN_TIME_PARA_START_ADDRESS);
	}
	EPROM4HISTORY.TotalMinuteckt1 = g_flashRunTimeParaSturct.TotalMinuteckt1;
	EPROM4HISTORY.TotalMinuteckt2 = g_flashRunTimeParaSturct.TotalMinuteckt2;
	EPROM4HISTORY.Runptr = g_flashRunTimeParaSturct.Runptr;
	QL_MQTT_LOG("%d %d",EPROM4HISTORY.TotalMinuteckt1 ,EPROM4HISTORY.TotalMinuteckt2);
}

void ExtFlash_write_RuntimePara(void)
{
	at25_status_t ret;	
	g_flashRunTimeParaSturct.s_ExtDataFlash_CheckByte = 0xAB;
	g_flashRunTimeParaSturct.TotalMinuteckt1 = EPROM4HISTORY.TotalMinuteckt1;
	g_flashRunTimeParaSturct.TotalMinuteckt2 = EPROM4HISTORY.TotalMinuteckt2;
	g_flashRunTimeParaSturct.Runptr = EPROM4HISTORY.Runptr;
	if((ret = at25dfx_write((uint8_t *)&g_flashRunTimeParaSturct, sizeof(g_flashRunTimeParaSturct), HISTORY_DATA_RUN_TIME_PARA_START_ADDRESS)))
		QL_MQTT_LOG("%d",ret);
	QL_MQTT_LOG("%d %d",EPROM4HISTORY.TotalMinuteckt1 ,EPROM4HISTORY.TotalMinuteckt2);
}
/*
void ExtFlash_ReadHistoricalDataLogFromFlash(unsigned int pageCounter)
{
	unsigned int HistoryDataAddressPointer = pageCounter*1024+HISTORY_DATA_FILE_START_ADDRESS;

	if(g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten == 1)
	{
		QL_MQTT_LOG("History Data (Overwritten): %s", MqttPubBuf);
		at25dfx_read((uint8_t*) &MqttPubBuf, 1024, HistoryDataAddressPointer);
		if(MqttPubBuf[0] != 0xFF)
		{
			flagMQTTPubHistoryData = 1;
			QL_MQTT_LOG("History Data (Overwritten): %s", MqttPubBuf); // Log the history data string
		}
	}
	else
	{
		at25dfx_read((uint8_t*) &MqttPubBuf, 1024, HistoryDataAddressPointer);
		if(MqttPubBuf[0] != 0xFF)
		{
			flagMQTTPubHistoryData = 1;
			QL_MQTT_LOG("History Data: %s", MqttPubBuf); //log the history data string
		}
	}
}

*/
void ExtFlash_ReadHistoricalDataLogFromFlash(unsigned int pageCounter)
{
    unsigned int HistoryDataAddressPointer = pageCounter * 1024 + HISTORY_DATA_FILE_START_ADDRESS;

    // Read the data from flash into MqttPubBuf
    at25dfx_read((uint8_t *)&MqttPubBuf, 1024, HistoryDataAddressPointer);
	// Ensure null-termination
	MqttPubBuf[1023] = '\0';   // Or at least at buffer_size-1
    // Check if the data is valid (not erased/empty)
    if (MqttPubBuf[0] != 0xFF)
    {
        flagMQTTPubHistoryData = 1;
        if (g_flashhistoryParaSturct.s_ExtDataFlash_IsDataLogOverwritten == 1)
        {
            QL_MQTT_LOG("History Data (Overwritten): %s", MqttPubBuf); // Log the history data string
        }
        else
        {
            QL_MQTT_LOG("History Data: %s", MqttPubBuf); // Log the history data string
        }
    }
}


// IP change 203.088.128.139 -> 203.88.128.139 (Remove Zero)
void correctionIP(char *newIP)
{
	int index=0,NoofZero=0; //sanket  for correction IP v4.9.6
	if(newIP[0] == '0')
	{
		memcpy(newIP+index, newIP+index+1, strlen(newIP+index+1));
		NoofZero++;
		if(newIP[0] == '0')
		{
		    memcpy(newIP+index, newIP+index+1, strlen(newIP+index+1));
		    NoofZero++;
		}
	}
	for( index=0;index<strlen(newIP);index++)
	{
		if(newIP[index] == '.')
		{
			if(newIP[index+1] == '0')
			{
				index++;
				NoofZero++;
				memcpy(newIP+index, newIP+index+1, strlen(newIP+index+1));
				if(newIP[index] == '0')
				{
					memcpy(newIP+index, newIP+index+1, strlen(newIP+index+1));
					NoofZero++;
				}
			}
		}
	}
	newIP[index-NoofZero] = '\0';
}

void CheckNetworkAndRebootIfNeeded()
{
    // 1. Validate RTC time
    if (rtc_time.mHour > 23 || rtc_time.minute > 59 || rtc_time.mSecond > 59 ||
        rtc_time.mDate < 1 || rtc_time.mDate > 31 || rtc_time.month < 1 || rtc_time.month > 12)
    {
        QL_MQTT_LOG("Invalid RTC time");
        return;
    }

    // 2. Check for SIM/network status
    bool isNetworkOK = (fNetworkRegistered == 1 && fInternetEnabled == 1);
    if (isNetworkOK)
    {
   	 // Reset tracking if the network is OK
        networkWasEverDown = false;
        networkDownStartDay = -1;
        networkDownStartHour = -1;
        networkDownStartMonth = -1;
        return;
    }

    // 3. First time network went down — record timestamp
    if (!networkWasEverDown)
    {
        networkWasEverDown = true;
        networkDownStartDay = rtc_time.mDate;
        networkDownStartHour = rtc_time.mHour;
        networkDownStartMonth = rtc_time.month;
        QL_MQTT_LOG("Network down since: %02d/%02d at %02d:00", networkDownStartDay, networkDownStartMonth, networkDownStartHour);
        return;
    }

    // 4. Calculate elapsed time in hours
    int startTotalHours = ((networkDownStartDay - 1) * 24) + networkDownStartHour;
    int currentTotalHours = ((rtc_time.mDate - 1) * 24) + rtc_time.mHour;

    bool monthRolledOver = (rtc_time.month != networkDownStartMonth);

    if (monthRolledOver)
    {
        // Very basic: assume 31 days last month (safe for most practical cases)
        startTotalHours -= (31 * 24);
    }

    int hoursDown = currentTotalHours - startTotalHours;

    // 5. Trigger reboot if it's been 24 hours and time is exactly 13:00:00
    if (rtc_time.mHour == 13 && rtc_time.minute == 0 && rtc_time.mSecond == 0)
    {
        if (hoursDown >= 24)
        {
            QL_MQTT_LOG("Network down for %d hours. Rebooting at 13:00", hoursDown);

            flag_flashUpdateEPROM_General = 1;
            flag_flashUpdateEPROM_General_WaitCounter = 5;
            flagMqttPubLogData = 1;

            QL_MQTT_LOG("Reboot triggered at = Date=%02d/%02d/20%02d, Time=%02d:%02d:%02d", 
                        rtc_time.mDate, rtc_time.month, rtc_time.myear,
                        rtc_time.mHour, rtc_time.minute, rtc_time.mSecond);

            ql_power_reset(RESET_NORMAL);
        }
    }
}
