#ifndef __DEFINE__H_
#define __DEFINE__H_

#define	DI_UPDATE  1
#define	DO_UPDATE  2
#define AI_UPDATE  3
#define AO_UPDATE  4
#define KB_AI_T_UPDATE	6
#define KB_AI_SH_UPDATE	 7
#define	KB_AI_SL_UPDATE	 8
#define	KB_AI_CS_UPDATE	 9
#define	KB_AI_CZ_UPDATE	 10
#define GENERAL_V 11
#define PLC_VAR 12
#define PLC_STACK 13

#define AI_TOTAL1 14
#define FEILD_AI 15
#define AI_TOTAL2 23

#define PLC_CLOCKS_A  16
#define PLC_CLOCKS_ST 17
#define PLC_CLOCKS_TM 18
#define PLC_CLOCKS_ACC 19

#define DO_UPDATE_STATUS 20
#define QUERY_UPDATE 21
#define PARA_UPDATE 22
#define KBM_UPDATE_SETUP 25
#define KBM_UPDATE_METER 26
#define RUNTIME_UPDATE 27
#define REC_VAR 28
#define RTC_CH  29
#define SDL_UPDATE 30
#define AI_TOT_FLG 31

#define MASTER 1
#define SLAVE  2

#define	MAX_CHNL_CONFIG			8
#define	MAX_ANA_CHNS			128
#define	MAX_DIG_CHNS			960
#define MAX_SCHEDULE   		    10

#define MAX_SECS_BET_ALARMS     400

#define MaxValidParamaters 544

#define MAX_DI			(960)
#define MAXDO			(960)
#define MAX_VAO			(100)
#define MAX_AO			(32)

#define MAX_PHYSICAL_DO			8
#define MAX_PHYSICAL_DI			16
#define MAX_PHYSICAL_AI			6

//*******New iFPC CIM ********/

#define MAXMTR					12 // 8 cim
#define MAXANA_VAR				12 

#define MAX_DO_CHANNEL    		3
#define MAX_DI_CHANNEL    		30

typedef enum
{
	STANDARD_EMT				=	1,
	CUSTOM_MODE 				=	2,
 	MODBUS_MASTER_MODE_TYPE_MAX = 	3
}ModBusMasterModeType_e;

#define MAXSMSMODPARA	128

#define MAX_EMT_SIZE 200
#define GPRS_COMM	


#define RAED_LAST_BLOCK			111
#define gcDay_PTR		0xD002

#define NEWEMT_MAP 305

#define MAX_GENERAL_AI  300

#define MAXSLAVE 10 

#define MODMAX_PARA	(3200 + 500)


#define RED_BUFFER 3300

#define SLAVE_SIZE	64
#define REG_SIZE	128


/*Energy meter settings*/
#define NIPPEN				1
#define HORIZON				2
#define TRINITY				3
#define CIMCON				4
#define LnT					5

#define ENABLED				1
#define DISABLED			0

#define YES					1
#define NO					2

#define TRUE				1
#define FALSE				0


#define MODDIV 6

typedef enum
{
	COIL_STATUS					=	1,
	INPUT_STATUS				=	2,
	HOLDING_REGISTER			=	3,
	INPUT_REGISTER				=	4,
	SINGLE_COIL					=	5,
	SINGLE_REGISTER				=	6,
	MODBUS_QUERY_FC_TYPE_MAX	=	7
}ModbusQueryFcType_e;

/******************************************         added by samir   ************************/
#define 		MAXTRANSBLOCK		64     // maximum transmission limit
#define         BSIZE           	1500 	// 2000-was orgCirc and Ext cir buf size (.PLC is coming initially here from PC in Circ buf)
#define			BUFFER_SIZE			60000 //58000	/*47600 10000- 4800  Buffer size where .PLC is loaded after decoding*/
#define			MAX_BUFSIZE			500
#define         BSIZE_E         	BSIZE    // Enlarged buffer size as 8400
//#define			STARTBUFFER		BSIZE
#define 		GSIZE_E				1500
#define 		DCU_SERIALBUFFER 	1024
#define WAIT_FOR_FRAME          	0
#define WAIT_FOR_DATA           	1
#define FRAMESIZE					sizeof(struct sSendFrame)
#define MAX_INTEGER_ARR_SIZE		15
# define FIRST_CALL					0
# define SECOND_CALL				1
//# define MAX_CLOCKS  	  			300    // original 100
# define FIRST_CALL         		0
//# define PI						3.14
#define	MANUAL_MODE					1
//#define OPERATION_BOX				0xffff
//#define CONDITION_BOX				0
#define CONNECTOR					1
#define SUPER_BLOCK					2
#define CIRCLE						3
#define PID_NOT_ACTIVE          	0
#define PID_PROF_ACTIVE				1
#define PID_PROF_HOLD				2
#define DIG_VAL_POS					0x01  /* 0000 0001 */
#define ACS_VAL_POS					0x02  /* 0000 0010 */
#define FEED_VAL_POS				0x04  /* 0000 0100 */
#define CON_VAL_POS					0x08  /* 0000 1000 */
#define MOD_VAL_POS					0x10  /* 0001 0000 */
#define SEQ							11
#define EMR							22
// #define INT							33
#define GREATER_EQUAL_TO_CODE		((unsigned char) 128)  /* � */
#define LESS_EQUAL_TO_CODE			((unsigned char) 129)  /* � */
#define EQUAL_TO_CODE				((unsigned char) 130)  /* � */
#define NOT_EQUAL_TO_CODE			((unsigned char) 131)  /* � */
#define GET_DIG_CODE 				((unsigned char) 132)  /* � */
#define PUT_IN_DIG_CODE 			((unsigned char) 133)  /* � */
#define GET_ANA_CODE				((unsigned char) 134)  /* � */
#define PUT_IN_ANA_CODE				((unsigned char) 135)  /* � */
#define GET_VAR_CODE				((unsigned char) 136)  /* � */
#define PUT_IN_VAR_CODE				((unsigned char) 137)  /* � */
#define GET_STACK_VAR_CODE			((unsigned char) 138)  /* � */
#define PUT_IN_STACK_CODE			((unsigned char) 139)  /* � */
#define CALL_CODE					((unsigned char) 140)  /* � */
#define PUSH_2						((unsigned char) 141)  /* � */
/*#define PUT_IN_STRING_CODE		((unsigned char) 142) */ /* � */	  // added by samir for dual string function
#define PUT_IN_STRING_CODE			((unsigned char) 144)  /* � */	  // added by samir for dual string function
#define GET_STRING_CODE				((unsigned char) 143)  /*   */	  // added by samir for
#define STRING_START_CODE			((unsigned char) 2)    /*  */
#define PLC_FUNCTION_CODE			((unsigned char) 151)  /* � */
#define MAX_CODE					((unsigned char) 254)  /* � */
#define ESCAP						((unsigned char) 248)				// 248
#define PROC_DIGITAL_VAR    		1
#define PROC_ANALOG_VAR				2
#define PROC_PLC_VAR 				3
#define PROC_CONSTANT_VAR			4
#define PROC_STACK_VAR				5
#define PROC_FUNCTION_VAR			6
#define PLC_HEADER_LEN				80
//#define MAX_PLCVAR				500
#define	MAX_PLCSTRING       		100
#define MAX_PID						1
//#define	MAX_ANA_CH16        	(300) //350
//#define CLK_TCK 					1
#define  io_adc		    			0x00009800
#define  Aout						0x00009004
#define  Aout1						0x00009005
#define  Aout2						0x00009006

//#define CLK_TCK 1     /* TO EXCLUDE TIME.H */
#define ACK                     	06                             //  /* Acknowledge */
#define NAK                     	21
#define TIME_OUT                	1000         //approx 20sec, time out of serial communication
#define MAXHEADERSIZE 				10
#define ENQ 						5
#define EOT 						4
#define SEND_ENQ					1
#define WAIT_FOR_EOT				2
#define HDDBINFO					10
#define PLCFILE                 	11
#define RCPFILE                 	12
#define PROFILE						13
#define LIMITS                  	14
#define OVERRIDE                	15
#define DIGOUT                  	16
#define SETPLCVAR               	17
#define ANAOUT                  	18
#define ANASCANSKIP					19
#define PLCLOGIC					20
#define CHANGEVAL					21
#define DATETIME                	22
#define NOT_FOR_THIS_RTU			50
#define PLCLOGICXFER				51
#define PLCENABLE					52
#define	PLCTEST						53
#define SMSRECEIVE					55
#define RESET_XFER					57
#define CIRCBUFF_SIZE       		sizeof(CIRCBUFF)
//#define MAX_PLCVAR				300
//#define	MAX_ANA_CH16        	72
#define SMSDATA						1000 /* 160 */
#define MAX_PID						1
#define MAX_PROFILE					1
#define MAX_INTEGER_ARR_SIZE		15

#define PCB_PLC_FILE_START  		0x3000
#define REC_FILE_START  			0x0
									//0xC000
#define SMS_INDEX					0xE000

#define MAXSTRLEN					15		/* Maximum string length*/

#define schflag_PTR       			0
#define schstarthour_PTR  			4
#define schstartmin_PTR	  			8
#define schstophour_PTR   			12
#define schstopmin_PTR    			16
/******************************************         added by samir   ************************/

//************************ gFinalAnaValF Location Def

#define PHYSICAL_DO_gFinalAnaValF						33
#define PHYSICAL_DI_gFinalAnaValF						0
#define PHYSICAL_AI_gFinalAnaValF						45

#define RTC_TIME_DATE_gFinalAnaValF						51

#define MODEM_PHY_STATUS_gFinalAnaValF					600
#define MODEM_GSM_SIM_STATUS_gFinalAnaValF				601
#define MODEM_GSM_NETWORK_STATUS_gFinalAnaValF			602
#define MODEM_GSM_GACT_STATUS_gFinalAnaValF				603
#define MODEM_GSM_RSSI_gFinalAnaValF					615
#define MODEM_MQTT_CONNECTION_STATUS_gFinalAnaValF		604
#define MODEM_APN_NAME_gFinalAnaValF					614

#define RTU_ID_gFinalAnaValF							605
#define LOG_RATE_gFinalAnaValF							606
#define CLIENT_ID_gFinalAnaValF							616
#define READER_ID_gFinalAnaValF							617

#define MAX_DO_gFinalAnaValF							607
#define MAX_DI_gFinalAnaValF							608
#define MAX_AI_gFinalAnaValF							609

// #define MODBUS_MAX_PARAMETER_gFinalAnaValF			610
#define NO_OF_CKT_gFinalAnaValF							610
// #define MODBUS_MAX_QUERY_gFinalAnaValF				611
#define METER_MAKE_gFinalAnaValF						611
#define METER_TYPE_gFinalAnaValF						612
// #define MODBUS_MAX_NO_OF_SMSTAG_gFinalAnaValF		613
#define MAX_NO_OF_SMSTAG_gFinalAnaValF					613


#define MODE_AUTO_MANUAL_gFinalAnaValF					618

#define MODEM_MQTT_BROKER_IP_0_gFinalAnaValF			619
#define MODEM_MQTT_BROKER_IP_1_gFinalAnaValF			620
#define MODEM_MQTT_BROKER_IP_2_gFinalAnaValF			621
#define MODEM_MQTT_BROKER_IP_3_gFinalAnaValF			622
#define MODEM_MQTT_BROKER_PORT_gFinalAnaValF			623
#define ETHERNET_IP_0_gFinalAnaValF  					624
#define ETHERNET_IP_1_gFinalAnaValF  					625
#define ETHERNET_IP_2_gFinalAnaValF  					626
#define ETHERNET_IP_3_gFinalAnaValF  					627
#define ETHERNET_SUBNET_0_gFinalAnaValF  				628
#define ETHERNET_SUBNET_1_gFinalAnaValF  				629
#define ETHERNET_SUBNET_2_gFinalAnaValF  				630
#define ETHERNET_SUBNET_3_gFinalAnaValF  				631
#define ETHERNET_GATEWAY_0_gFinalAnaValF  				632
#define ETHERNET_GATEWAY_1_gFinalAnaValF  				633
#define ETHERNET_GATEWAY_2_gFinalAnaValF  				634
#define ETHERNET_GATEWAY_3_gFinalAnaValF  				635
#define RS485_1_BAUDRATE_gFinalAnaValF  				636
#define RS485_2_BAUDRATE_gFinalAnaValF  				637
#define RS232_1_BAUDRATE_gFinalAnaValF  				638
#define RS232_2_BAUDRATE_gFinalAnaValF  				639
#define RS485_1_MASTER_SLAVE_DEBUG_gFinalAnaValF  		640
#define RS485_2_MASTER_SLAVE_DEBUG_gFinalAnaValF  		641
#define RS232_1_MASTER_SLAVE_DEBUG_gFinalAnaValF  		642
#define RS232_2_MASTER_SLAVE_DEBUG_gFinalAnaValF  		643
#define RS485_1_MODBUS_RTU_ASCII_gFinalAnaValF  		644
#define RS485_2_MODBUS_RTU_ASCII_gFinalAnaValF  		645
#define RS232_1_MODBUS_RTU_ASCII_gFinalAnaValF  		646
#define RS232_2_MODBUS_RTU_ASCII_gFinalAnaValF  		647
#define RS485_1_MODBUS_POLL_FRQ_gFinalAnaValF  			648
#define RS485_2_MODBUS_POLL_FRQ_gFinalAnaValF  			649
#define RS232_1_MODBUS_POLL_FRQ_gFinalAnaValF  			650
#define RS232_2_MODBUS_POLL_FRQ_gFinalAnaValF  			651
#define MODEM_MQTT_LIVE_FRQ_gFinalAnaValF				652
#define BLE_CONEECTION_STATE_gFinalAnaValF				653
#define BLE_MAC_0_gFinalAnaValF					  		654
#define BLE_MAC_1_gFinalAnaValF							655
#define BLE_MAC_2_gFinalAnaValF							656
#define BLE_MAC_3_gFinalAnaValF							657
#define BLE_MAC_4_gFinalAnaValF							658
#define BLE_MAC_5_gFinalAnaValF							659
#define GPS_LAT_gFinalAnaValF							660
#define GPS_Log_gFinalAnaValF							661
#define GPS_NO_OF_SATALITE_gFinalAnaValF				662
#define GPS_DATE_gFinalAnaValF							663
#define GPS_MONTH_gFinalAnaValF							664
#define GPS_YEAR_gFinalAnaValF							665
#define GPS_HOUR_gFinalAnaValF							666
#define GPS_MIN_gFinalAnaValF							667
#define GPS_SEC_gFinalAnaValF							668
#define GPS_ALTITUDE_gFinalAnaValF						669
#define GPS_3DFIX_gFinalAnaValF							670
#define ETHERNET_MAC_0_gFinalAnaValF					671
#define ETHERNET_MAC_1_gFinalAnaValF					672
#define ETHERNET_MAC_2_gFinalAnaValF					673
#define ETHERNET_MAC_3_gFinalAnaValF					674
#define ETHERNET_MAC_4_gFinalAnaValF					675
#define ETHERNET_MAC_5_gFinalAnaValF					676
#define REBOOT_COUNT_gFinalAnaValF						677

#define SUNRISE_HOUR_gFinalAnaValF						678
#define SUNRISE_MIN_gFinalAnaValF						679
#define SUNRISE_SEC_gFinalAnaValF						680
#define SUNSET_HOUR_gFinalAnaValF						681
#define SUNSET_MIN_gFinalAnaValF						682
#define SUNSET_SEC_gFinalAnaValF						683
#define COMM_MODE_ETHER_GPRS_gFinalAnaValF				684
#define ETHER_MQTT_CONNECTION_STATUS_gFinalAnaValF		685

#define TIMEZONE_SIGN_gFinalAnaValF						686
#define TIMEZONE_HOUR_gFinalAnaValF						687
#define TIMEZONE_MIN_gFinalAnaValF						688
#define ASTRO_OFFSET_gFinalAnaValF						689
#define DEVICE_REBOOT_gFinalAnaValF						690
#define DEVICE_REBOOT_TIME_DAY_NIGHT_gFinalAnaValF		691


#define GENERAL_PURPOSE_AI_gFinalAnaValF				700

#define SCHEDULE_gFinalAnaValF							1485
#define MODBUS_READ_QUERY_gFinalAnaValF					1945
#define MODBUS_WRITE_QUERY_gFinalAnaValF				2245
#define MODBUS_WRITE_QUERY_ACK_gFinalAnaValF			2253
#define SCALING_PARA_AI_gFinalAnaValF					2285
//*****************************************************

#define COM_RS232					0
#define COM_RS485					1
#define COM1PORT   					0
#define COM3PORT   					3

#define SW_LGA_LOGIC
#define SW_CCA_SCADA
#define SW_CCA_LOGIC
#define STREET_LIGHT_EQ

#define STREET_LIGHT_EQ

#ifdef STREET_LIGHT_EQ

#define SUNRISE 		0
#define SUNSET			1
#define PI				3.14159
#define	zenith 			90500

#endif

#define LGG_WEBSCANET				1
#define WEB_WEBSCANET				2

#define MAXSAVE						2
#define MAXBYTESAVE 				1536

#define NOOFAISMD 					64
#define NOOFDISMD 					64
#define B_SIZE_RF 					512
//***********check
// configuration.h
/*
// Declare the enum for gFinalAnaValF indices
typedef enum {
	MODEM_MQTT_BROKER_IP_0_gFinalAnaValF = ip_t.ip[0],
	MODEM_MQTT_BROKER_IP_1_gFinalAnaValF = ip_t.ip[1],
	MODEM_MQTT_BROKER_IP_2_gFinalAnaValF = ip_t.ip[2],
	MODEM_MQTT_BROKER_IP_3_gFinalAnaValF = ip_t.ip[3],
	MODEM_MQTT_BROKER_PORT_gFinalAnaValF = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port

	// gFinalAnaValF[MODEM_MQTT_BROKER_IP_0_gFinalAnaValF] = ip_t.ip[0];
    // gFinalAnaValF[MODEM_MQTT_BROKER_IP_1_gFinalAnaValF] = ip_t.ip[1];
    // gFinalAnaValF[MODEM_MQTT_BROKER_IP_2_gFinalAnaValF] = ip_t.ip[2];
    // gFinalAnaValF[MODEM_MQTT_BROKER_IP_3_gFinalAnaValF] = ip_t.ip[3];
    // gFinalAnaValF[MODEM_MQTT_BROKER_PORT_gFinalAnaValF] = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port;
	// ... other enum entries
  } FinalAnaValF_Indices;
  */
  

typedef enum
{
	LG_WEBSCANET 	=	6,			  // LG to Webscanet 2.0.0.
	LGO_SCADA		=	5,
	WEB_SCADA		=	4,			  // WEbscanet Old 
	MCS_DATA		=	3,
	LG_GSM_DATA 	=	2,
	CCA_SCADA		=	1,
	LG_SCADA		=	0   // LG Old 
}ScadaType_e;

#define SMS_COMM


#define DCU_LAN					3
#define BOTH					2
#define DCU_SERIAL				1
#define GPRS_OVER_DCU			0


#define SMS1					1
#define SMS2					2
#define SMS3					3
#define SMS4					4
#define SMS5					5
#define SMS6					6
#define SMS7					7
#define SMS8					8
#define SMS9					9

#define NOBOOT  				0
#define SDBOOT  				1
#define LANBOOT 				2
#define FLASHBOOT 				3
 

#define SW_EPROM_BOOT_LOGIC

#ifdef SW_EPROM_BOOT_LOGIC

	#define REC_SIZE 			20
	#define TRANS_SIZE 			500
	
	#define PLCGPRS_PLC			1
	#define PLCGPRS_REC			2
	#define HEXGPRS_FILE		3
	#define HEXPLBPLC
	#define BOOTPCB_PLC_FILE_START  0x8000	   // Save in to BANK4 where only 7 Day Stoare and rest of space for Save PCBPLC which are load from gprs 
	#define BOOTREC_FILE_START  	0xF400 
	#define BOOTHEX_FILE_START  	0x0000
	#define FRAMESIZE_BOOT			512

#endif

#define MODBUD_BYTE				600 // ebcause of PLC rec from GPRS

#endif