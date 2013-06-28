/*********************************************************************************************
* File: SAnode.c
* Project: LG Exec Ed Program
* Copyright: Copyright (c) 2013 Anthony J. Lattanze
* Versions:
*	0.0 June 2013.
*     1.0 June 2013. (S.Choi W.Choi) 
* Description: JoyL SANode main
*
* Parameters:
*
* Internal Methods:
*
************************************************************************************************/

#include <SPI.h>
#include <WiFi.h>
#include <WString.h>
#include <dht.h>
#include <SAC.h>
#include "JoyL_custom.h"

/* Global Definition ============================================ */
#define PRIVATE	static 
#define PUBLIC	extern
#define IN
#define	OUT
//#define TRACE_ENABLE_MAIN
#define	SA_FAIL	false
#define	SA_SUCCESS	true
#define	SA_FALSE	false
#define SA_TRUE	true
#define NETWORK_IN_DATA_LEN 100
#define NETWORK_OUT_DATA_LEN  100 
#define dht_dpin 2

/* Main Configuration====*/
#define	TCP_CLIENT		1
#define	TCP_SERVER		0
#define	TCP_PORT		50000
#define	SEND_FRAGMENT	20
#define	UNIT_TEST		0
#define	UNIT_TEST_SA	0

#define	TCP_CONNECTED		1
#define	TCP_DISCONNECTED	0

/* == SA Auth Status=== */
#define	SA_AUTH_NO		0
#define	SA_AUTH_WAITING	1
#define	SA_AUTH_SUCCESS	2
 
#ifdef TRACE_ENABLE_MAIN
#define SERIAL_PRINT(msg) Serial.print(msg)
#define SERIAL_PRINTLN(msg) Serial.println(msg)
#define SERIAL_PRINT2(msg1,msg2) Serial.print(msg1,msg2)
#define SERIAL_PRINTLN2(msg1,msg2) Serial.println(msg1,msg2)
#else
#define SERIAL_PRINT(msg)
#define SERIAL_PRINTLN(msg)
#define SERIAL_PRINT2(msg1,msg2)
#define SERIAL_PRINTLN2(msg1,msg2)
#endif

/* == Global Data Format ===================== */

typedef enum CMD_MODE_enum 
{
  CMD_SENSOR_LIGHT_RED_ON,
  CMD_SENSOR_LIGHT_BLUE_ON,
  CMD_SENSOR_LIGHT_GREEN_ON,
  CMD_SENSOR_LIGHT_OFF,
  CMD_SENSOR_TEMP_READ,
  CMD_UNKNOWN
} CMD_MODE_t;

/* SA Main Service State */
typedef enum SA_SERVICE_MODE_enum 
{
	SA_INIT,
	SA_NETWORK_CONNECTING,
	SA_NETWORK_ONLINE,
	SA_AUTHENTICATION,
	SA_SENSOR_MONITOR,
	SYS_UNKNOWN
}SA_SERVICE_MODE_t;

/* NM State */
typedef enum NM_STATE_enum
{
	NM_UNKNOW_S = -1,
 	NM_NULL_S,
 	NM_CONV_S,
	NM_CLIENT_MODE_S,
	NM_SERVER_MODE_S,
	NM_END
}NM_STATE_t;

/* == JSON Message Contenty Type=== */
typedef enum PT_JSON_CONTENT_enum
{
	PT_JSON_UNKNOW_S = -1,
		
	/* Receiving Type*/
	PT_JSON_SA_AUTH_ACCEPT,
	PT_JSON_ACTUATOR,

	/* Sending Type*/
	PT_JSON_SA_DESCRIPTION,
 	PT_JSON_STATUS_NOTI,
 	PT_JSON_KEEP_ALIVE,

	PT_JSON_END
}PT_JASON_CONTENT_t;

/* == SA TXN Context=== */
typedef struct
{
	char	 s_network_in_data[NETWORK_IN_DATA_LEN];

	boolean	enable_out_data;
	char	 s_network_out_data[NETWORK_OUT_DATA_LEN];
	SA_SERVICE_MODE_t  sa_service_mode;
#if TCP_CLIENT	
	boolean	tcp_status;
#endif
	int	sa_auth_status;
	
}SA_SERVICE_TXN_CONTEXT_s;

/* == Global Variable ============================================ */

 char inChar;                   /* Character read from client */
 IPAddress ip;                  /* The IP address of the shield */
 IPAddress subnet;              /* The IP address of the shield */
 long rssi;                     /* Wifi shield signal strength */
 byte mac[6];                   /* Wifi shield MAC address */

 int status = WL_IDLE_STATUS;   /* Network connection status */

 //char ssid[] = "Shadyside Inn";
 char ssid[] = "CMU";
 byte servername[] = {128,237,120,175};

 //out-message.. "200"
 char resp_ok[] = "200\n\r";
 char resp_fail[] = "400 \"Unknown command!\"\n\r"; /* for unknown command */
 
 /* ========================================================================= 
  * Module Name: SA SERVICE
  * ========================================================================= */

SA_SERVICE_TXN_CONTEXT_s	 sa_txn_context;

PUBLIC	void SA_Service_Init();
PUBLIC	char* SA_Service_Get_TxnOutData(); 
PUBLIC	void SA_Service_Set_TxnOutData(IN char* out_data );
PUBLIC	boolean SA_Service_get_TxnOutDataEnabled();
PUBLIC	void SA_Service_Flush_TxnInData();
PUBLIC	void SA_Service_Flush_TxnOutData();
PUBLIC	void SA_Service_Flush_TxnContext();
PUBLIC	char* SA_Service_Get_TxnInData(); 
PUBLIC	void SA_Service_Set_TxnInData(IN char* in_data );
PUBLIC	SA_SERVICE_MODE_t SA_Service_GetMode(); 
PUBLIC	void SA_Service_SetMode(IN SA_SERVICE_MODE_t sa_mode ); 

#if	TCP_CLIENT
PUBLIC	boolean SA_Service_GetTCPStatus();
PUBLIC	void SA_Service_SetTCPStatus(IN boolean tcp_st );
#endif

PUBLIC	int SA_Service_GetAuthStatus(); 
PUBLIC	void SA_Service_SetAuthStatus(IN int sa_auth); 

PUBLIC	void SAService();
PUBLIC  void SAConfig();

/* ========================================================================= 
* Module Name: NETWORK MANAGER
* ========================================================================= */

WiFiClient client;  

NM_STATE_t nmState;
boolean b_auth_sending_out=SA_FALSE;

static int enKeepAlived = 0;
int getKeepAlive()
{
	return enKeepAlived;
}
void setKeepAlive(int en)
{
	enKeepAlived = en;
}

int isFirst = 0;

PRIVATE	NM_STATE_t NM_GetState(); 
PRIVATE	void NM_SetState(NM_STATE_t nm_state ); 
PRIVATE	int NM_NullState_Process(); 
PRIVATE	int NM_ConvState_Process(); 
PRIVATE void NM_PrintConnectionStatus();
PRIVATE void	NM_SendOutFragData(IN char* out_data);
PRIVATE	int NM_State_Machine(); 
 
 PUBLIC	void NM_Init_NetworkManager();
 PUBLIC	void NM_NetworkManager();

/* ========================================================================= 
* Module Name: IoT Protocol
* ========================================================================= */

/* Calculate Json message length */
unsigned int getMsgLength(char* str);

/* message copy */
int copyMessage(char* srcStr, char* destStr, int leng);

/* string compare - for decoder */
int compareString(char* srcStr, char* destStr, int leng);

PUBLIC	void IoT_ProtocolDecoder();
PUBLIC	char* IoT_ProtocolEncoder(IN PT_JASON_CONTENT_t	enc_c_type	);

 
/* ========================================================================= 
* Module Name: SA SERVICE
* ========================================================================= */
PUBLIC	void SA_Service_Init()
{
	SA_SERVICE_TXN_CONTEXT_s* sa_context = &sa_txn_context;

	memset(sa_context->s_network_in_data,0x00,NETWORK_IN_DATA_LEN);
	sa_context->enable_out_data = SA_FALSE;
	memset(sa_context->s_network_out_data,0x00,NETWORK_OUT_DATA_LEN);	
	sa_context->sa_service_mode = SA_INIT;
#if	TCP_CLIENT
	sa_context->tcp_status = TCP_DISCONNECTED;
#endif
	sa_context->sa_auth_status = SA_AUTH_NO;
	return;
}

PUBLIC	char* SA_Service_Get_TxnOutData()
{
	return sa_txn_context.s_network_out_data;
}

PUBLIC	void SA_Service_Set_TxnOutData(IN char* out_data )
{
	if(out_data == NULL)
	{
		return;
	}
	strcpy(sa_txn_context.s_network_out_data,out_data);
	sa_txn_context.enable_out_data = SA_TRUE;
	return;
}

PUBLIC	boolean SA_Service_get_TxnOutDataEnabled()
{
	return 	sa_txn_context.enable_out_data;
}

PUBLIC	void SA_Service_Flush_TxnInData()
{
	memset(sa_txn_context.s_network_in_data,0x00,NETWORK_IN_DATA_LEN);
	return;
}

PUBLIC	void SA_Service_Flush_TxnOutData()
{
	sa_txn_context.enable_out_data = SA_FALSE;
	memset(sa_txn_context.s_network_out_data,0x00,NETWORK_OUT_DATA_LEN);
	return;
}

PUBLIC	void SA_Service_Flush_TxnContext()
{
	SA_SERVICE_TXN_CONTEXT_s* sa_context = &sa_txn_context;

	memset(sa_context->s_network_in_data,0x00,NETWORK_IN_DATA_LEN);
	memset(sa_context->s_network_out_data,0x00,NETWORK_OUT_DATA_LEN); 
	sa_context->sa_service_mode = SA_INIT;
#if	TCP_CLIENT
	sa_context->tcp_status = TCP_DISCONNECTED;
#endif
	sa_txn_context.sa_auth_status = SA_AUTH_NO;

#if 0
	memset(sa_context->sa_auth_info,0x00,SA_AUTH_INFO_LEN+1);
#endif 
	return;
}

PUBLIC	char* SA_Service_Get_TxnInData()
{
	return	sa_txn_context.s_network_in_data;
}

PUBLIC	void SA_Service_Set_TxnInData(IN char* in_data )
{
	if(in_data == NULL)
	{
		return;
	}

	strcpy(sa_txn_context.s_network_in_data,in_data);
	return;
}

PUBLIC	SA_SERVICE_MODE_t SA_Service_GetMode()
{
	return	sa_txn_context.sa_service_mode;
	
}

PUBLIC	void SA_Service_SetMode(IN SA_SERVICE_MODE_t sa_mode )
{
	sa_txn_context.sa_service_mode = sa_mode;	
	return;
}

#if	TCP_CLIENT
PUBLIC	boolean SA_Service_GetTCPStatus()
{
	return	sa_txn_context.tcp_status;
}

PUBLIC	void SA_Service_SetTCPStatus(IN boolean tcp_st ) 
{
	sa_txn_context.tcp_status = tcp_st;
	return;
}
#endif

PUBLIC	int SA_Service_GetAuthStatus()
{
	return	sa_txn_context.sa_auth_status;
}

PUBLIC	void SA_Service_SetAuthStatus(IN int sa_auth ) 
{
	sa_txn_context.sa_auth_status = sa_auth;
	return;
}

PUBLIC	void SAService()
{
   int	sa_mode = SA_Service_GetMode();
   char* resultMsg = NULL;

   switch (sa_mode)
   {
     case SA_INIT:
		//Serial.println("sas:int");
		sac.init_device();
		sac.write_device("light","red");
		SA_Service_SetMode(SA_NETWORK_CONNECTING);
		break;

     case SA_NETWORK_CONNECTING:
		Serial.println("sas:net connecting");
		NM_NetworkManager();		   
		if(SA_Service_GetAuthStatus()!= SA_AUTH_SUCCESS)
			SA_Service_SetMode(SA_AUTHENTICATION);	   	
		else
			SA_Service_SetMode(SA_NETWORK_ONLINE);
		break;

     case SA_AUTHENTICATION:
        Serial.println("sas:authentication");
	    if(SA_Service_GetAuthStatus()== SA_AUTH_NO)
	   	{
			SA_Service_SetMode(SA_NETWORK_ONLINE);
	   	}
	    else if(SA_Service_GetAuthStatus()== SA_AUTH_WAITING)
	   	{
			//SERIAL_PRINTLN( " Authenticator : Waiting~ " );
			SA_Service_SetMode(SA_NETWORK_ONLINE);
	   	}
	    else if(SA_Service_GetAuthStatus()== SA_AUTH_SUCCESS)
	   	{
			SA_Service_SetMode(SA_NETWORK_ONLINE);
	   	}
	    break;	   

     case SA_NETWORK_ONLINE:
		//Serial.println("SAC:net online");
		NM_NetworkManager();

		if(SA_Service_GetAuthStatus()== SA_AUTH_SUCCESS)
		{
			SA_Service_SetMode(SA_SENSOR_MONITOR);
		}
		break;

	case SA_SENSOR_MONITOR:
       Serial.println("SAC:sa monitor");
	   resultMsg = sac.read_device(0,"monitor");
	   //Serial.println(resultMsg);
       //if ((resultMsg != NULL) || (isFirst == 0))
       //{
          //isFirst = 1;
          Serial.println("##Sending#");
	      NM_SendOutFragData(resultMsg);
       //}
       SA_Service_SetMode(SA_NETWORK_ONLINE);
	   delay(3000);
       break;

     default:
       break;
   }

   return;
} 

 
 /* ========================================================================= 
 * Module Name: SA_Configuration
 * ========================================================================= */
 void SAConfig()
{
	sac.init();
	SA_Registeration();
}

 /* ========================================================================= 
 * Module Name: NETWORK MANAGER
 * ========================================================================= */
PRIVATE	NM_STATE_t NM_GetState()
{
	return nmState;
	
}
PRIVATE	void NM_SetState(NM_STATE_t nm_state )
{
	nmState = nm_state;
	return;
}

PRIVATE	int NM_NullState_Process()
{
	Serial.println("TCP connecting..");
	if(client.connect(servername,TCP_PORT))
	{
		SA_Service_SetTCPStatus(TCP_CONNECTED);
		Serial.println("TCP connected!");
	}
	else 
	{
		SA_Service_SetTCPStatus(TCP_DISCONNECTED);
		SERIAL_PRINTLN("Failed:TCP");
		return	SA_FAIL;
	}

	return SA_SUCCESS;
}

PRIVATE	int NM_ConvState_Process()
{
	char* in_data = SA_Service_Get_TxnInData();
	char* out_data= SA_Service_Get_TxnOutData();

	if(!client.connected()){

		Serial.println("TCP Re-connecting..");

		for(int i=0;i<5;i++)
		{
			if(client.connect(servername,TCP_PORT))
			{
				SA_Service_SetTCPStatus(TCP_CONNECTED);
				Serial.println("TCP Re-connected!");
				break;
			}
		}

		if(SA_Service_GetTCPStatus()== TCP_DISCONNECTED)
		{			
			SA_Service_Flush_TxnContext();		
			SA_Service_SetMode(SA_NETWORK_CONNECTING);
			SA_Service_SetAuthStatus(SA_AUTH_NO);
			NM_SetState(NM_NULL_S);
			Serial.println("TCP Disconnected!");
			return	SA_FAIL;
		}
	}
	else {		
	  /* Read if we have any data to be read */
	  if (client.available())
	  {
		  memset(in_data, 0x00, NETWORK_IN_DATA_LEN);
		  for(int i = 0; i < NETWORK_IN_DATA_LEN; i++)
		  {
			  in_data[i] = client.read();
			  if (in_data[i] == '\n' /*&& char_buf != '\r'*/) break;								  
		  } 
		  SA_Service_Set_TxnInData(in_data);

		  /*Print read buffer*/
		  Serial.println("##RCVED##");
		  Serial.println(in_data);

		  IoT_ProtocolDecoder();
	  }

		/*Write Auth data if any*/
		if(SA_Service_GetAuthStatus()==SA_AUTH_NO)
		{
			//Serial.println("Auth Send");
			
			if(!b_auth_sending_out)
			{
				NM_SendOutFragData(sac.get_deviceInfoList(0));
				setKeepAlive(1);
			}
			b_auth_sending_out=SA_TRUE;
			return SA_SUCCESS;										
	    }

		/* Write if we have any data to send */
		if(SA_Service_get_TxnOutDataEnabled())
		{				 
			Serial.println("##Sending##");
			NM_SendOutFragData(out_data);
			
			SA_Service_Flush_TxnOutData();		  		
		}
	 }

	return SA_SUCCESS;
}

PRIVATE	void NM_PrintConnectionStatus() 
{
	// Print the basic connection and network information: Network, IP, and Subnet mask
	ip = WiFi.localIP();
	Serial.println("WIFI Connected:");
	Serial.println(ssid);
	Serial.println("IP address: ");
	Serial.println(ip);
	subnet = WiFi.subnetMask();
	//SERIAL_PRINTLN("Netmask: ");
	//SERIAL_PRINTLN(subnet);
  
	// Print our MAC address.
	WiFi.macAddress(mac);
#if 0
	Serial.print("WiFi Shield MAC address: ");
	SERIAL_PRINTLN2(mac[5],HEX);
	Serial.print(":");
	SERIAL_PRINTLN2(mac[4],HEX);
	Serial.print(":");
	SERIAL_PRINTLN2(mac[3],HEX);
	Serial.print(":");
	SERIAL_PRINTLN2(mac[2],HEX);
	Serial.print(":");
	SERIAL_PRINTLN2(mac[1],HEX);
	Serial.print(":");
	SERIAL_PRINTLN2(mac[0],HEX);
#endif
	// Print the wireless signal strength:
	rssi = WiFi.RSSI();
	//Serial.print("Signal strength (RSSI): ");
	//Serial.print(rssi);
	//SERIAL_PRINTLN(" dBm");
}

#define TX_UNIT_SIZE 10

PRIVATE void NM_SendOutFragData(IN char* out_data)
{
	int j,k,l,m=0;
	char c_buf[TX_UNIT_SIZE+1]={0,};
	char* c = out_data;
	int s_len=strlen(c);
       
	if(s_len<TX_UNIT_SIZE)
	{
		client.print(c);
		Serial.print(c);
		return;
	}
	else
	{
		l= s_len/TX_UNIT_SIZE;
		m= s_len%TX_UNIT_SIZE;		

		for(j=0;j<l;j++)
		{
			k=TX_UNIT_SIZE*j;
			strncpy(c_buf,c+k,TX_UNIT_SIZE);
			client.print(c_buf);
			Serial.print(c_buf);
			memset(c_buf,NULL,TX_UNIT_SIZE+1);
		}

		strncpy(c_buf,&c[k+TX_UNIT_SIZE],m);
		client.println(c_buf);	
		Serial.println(c_buf);
	}						
}

PRIVATE	int	NM_State_Machine()
{
	boolean b_result = false;
	
	NM_STATE_t	nm_state = NM_UNKNOW_S ;
	nm_state = NM_GetState();
	
	switch (nm_state)
	{
		case NM_NULL_S:
			Serial.println("NM:null");
			if(NM_NullState_Process()==SA_FAIL)
			{
				return b_result = SA_FAIL;
			}
			NM_SetState(NM_CONV_S);
			break;
			
		case NM_CONV_S:
			//Serial.println("NM:conv");
			if(NM_ConvState_Process()==SA_FAIL)
			{
				return b_result = SA_FAIL;
			}	
			break;

		default:
			break;
	}	

	return	SA_SUCCESS;
}

PUBLIC	void NM_Init_NetworkManager()
{	
	NM_SetState(NM_NULL_S);
}

PUBLIC	void NM_NetworkManager()
{
	/* Attempt to connect to Wifi network.*/
	while(status != WL_CONNECTED) 
	{ 
	  Serial.println("W connecting ");
	  // Serial.println(ssid);
	  status = WiFi.begin(ssid);
	  
	  /*Print the basic connection and network information.*/
	  if(status == WL_CONNECTED)
	  {
		  NM_PrintConnectionStatus();
		  b_auth_sending_out=SA_FALSE;
		  SA_Service_Flush_TxnContext();
	  }
	}
 
	if(NM_State_Machine()== SA_FAIL)
	{
		// Serial.println(NM_State_Machine:SA_FAIL);
		return;
	} 
}

/* ======================================================== 
* Module Name: IoT Protocol
* ========================================================= */

/* Calculate Json message length */
unsigned int getMsgLength(char* str)
{
  int cnt = 0;
  char readchar;
  
  if (str != NULL)
  {
    do
    {
		readchar = str[cnt];
		cnt++;
    } while (readchar != '\0') ;
  }
  return cnt;
}

/* message copy */
int copyMessage(char* srcStr, char* destStr, int leng)
{
  int cnt=0;
  if ((srcStr != NULL) && (destStr != NULL))
  {
	for (cnt=0 ; cnt < leng ; cnt++)
	{
	  destStr[cnt] = srcStr[cnt];
	}
  }
  return cnt;  
}

/* string compare - for decoder */
int compareString(char* srcStr, char* destStr, int leng)
{
  int cnt=1;

  if ((srcStr == NULL) || (destStr == NULL))
  {
	return 0;
  }
  
  for (cnt=0 ; cnt < leng ; cnt++)
  {
	if (srcStr[cnt]!= destStr[cnt])
	{
	   return 0;   
	}
  }

  return 1; /* if same, return 1 */
}

#define MAX_CMD_SIZE 20
#define MAX_PARAM_SIZE 20

char acturator_cmd[MAX_CMD_SIZE];
char acturator_param[MAX_PARAM_SIZE];

int Json_ParcingData(char* jsonMsg, char* data, int th)
{
	int cnt = 0;
	int wp = 0;
	char readchar;
	int count = th * 2;

	if (jsonMsg == NULL || data == NULL || count == 0)
		return 0;
	
	do
	{
		readchar = jsonMsg[cnt]; 
		switch (readchar)
		{
			case '{':
			case '}':
			case '[':
			case ']':
			case ':':
			case ',':
			case ' ':
			case '\t': break;
			
			case '"': count--; break; /* wh_debug : check */
			default: if (count > 0 && count < 3) data[wp++] = readchar; break;
		}
		cnt++;
	} while (count > 0);
	data[wp] = '\0';

	return 1;
}

int Json_ParcingSACommand(char* jsonMsg, char* cmd, char* param)
{
	Json_ParcingData(jsonMsg, &cmd[0], 1);
	Json_ParcingData(jsonMsg, &param[0], 2);
	return 1;
}

PUBLIC	void IoT_ProtocolDecoder()
{
	PT_JASON_CONTENT_t c_type = PT_JSON_UNKNOW_S;

	if(compareString(sa_txn_context.s_network_in_data, "/1.5/activation ", 16)) 
	{
		SA_Service_Set_TxnOutData(resp_ok);
		SA_Service_Flush_TxnInData();
		SA_Service_SetMode(SA_AUTHENTICATION);
		SA_Service_SetAuthStatus(SA_AUTH_SUCCESS);
		isFirst = 0;
		sac.write_device("light","off");
	}
	else if(compareString(sa_txn_context.s_network_in_data, "/1.5/actuator ", 13)) 
	{
		// in-message..   /actuator {"light":"pattern"}
		// in-message..   /actuator {"temperature"}
		Json_ParcingSACommand(sa_txn_context.s_network_in_data+13,acturator_cmd,acturator_param);
		SA_Service_Set_TxnOutData(resp_ok);
		
		if (sac.write_device(acturator_cmd,acturator_param) == 0)
		{
			SA_Service_Set_TxnOutData(resp_fail);
			SA_Service_Flush_TxnInData();
		}
	}
	else if(compareString(sa_txn_context.s_network_in_data, "401", 3)) 
	{
		SERIAL_PRINTLN("401 recieved");
		SA_Service_SetMode(SA_AUTHENTICATION);
		SA_Service_SetAuthStatus(SA_AUTH_WAITING);
		sac.write_device("light","green");
	}

	else if(compareString(sa_txn_context.s_network_in_data, "200", 3)) 
	{
		if (isFirst == 0)
		{
			SA_Service_Set_TxnOutData(resp_ok);
			SA_Service_Flush_TxnInData();
			SA_Service_SetMode(SA_AUTHENTICATION);
			SA_Service_SetAuthStatus(SA_AUTH_SUCCESS);
			isFirst = 1;
			sac.write_device("light","off");
		}
	}
	return;
}

char aliveMsg_light[] = {"/1.5/sanode/78C40E019AC8/heartbeat {\"sessionID\":\"joyl-light\"}"};
char aliveMsg_ht[] = {"/1.5/sanode/78C40E0174F9/heartbeat {\"sessionID\":\"joyl-humidity\"}"};
char str_joyl_light[] = {"joyl-light"};

PUBLIC	char* IoT_ProtocolEncoder(IN PT_JASON_CONTENT_t	enc_c_type	)
{
	char*	sa_encoded_data = NULL;
	PT_JASON_CONTENT_t	c_type = enc_c_type;

	switch (c_type)
	{	
		case PT_JSON_SA_DESCRIPTION:
			sa_encoded_data = sac.get_deviceInfoList(0);			
			break;
	
		case PT_JSON_KEEP_ALIVE:
			if (compareString(sac.get_sessionID(0),str_joyl_light,10))
			{
				SA_Service_Set_TxnOutData(aliveMsg_light);
			}
			else
			{
				SA_Service_Set_TxnOutData(aliveMsg_ht);
			}
			break;

		default:
			break;
	}

	return	sa_encoded_data;
}

void callback_keepAlive()
{
	int need_delay = 0;

	if(getKeepAlive() == 1)
	{  
		if ((SA_Service_GetMode() == SA_NETWORK_ONLINE) &&
   		    (SA_Service_GetAuthStatus() == SA_AUTH_NO))
		{
			need_delay = 1;
		}

		if (need_delay == 0)
		{
			Serial.println("keep alive");
			IoT_ProtocolEncoder(PT_JSON_KEEP_ALIVE);
		}
	}
}

/* ========================================================================= 
* Module Name: SETUP
* ========================================================================= */
void setup() 
{
	// Initialize a serial terminal for debug messages.
	Serial.begin(9600); 

	SAConfig();
	SA_Service_Init();
	NM_Init_NetworkManager();
}
 
/* ========================================================================= 
* Module Name: LOOP
* ========================================================================= */

long timerCnt = 0;

void loop()
{
	SAService();

	/* instead of a timer */
	if (timerCnt == 0)
	{
		callback_keepAlive();
	}
	timerCnt++;
	if (timerCnt > 9000000); timerCnt =0;
}
