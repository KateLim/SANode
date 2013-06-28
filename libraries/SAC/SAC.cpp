//
//    FILE: dht22.cpp
// VERSION: 0.1.00
// PURPOSE: DHT22 Temperature & Humidity Sensor library for Arduino
//
// DATASHEET: 
//
// HISTORY:
// 0.1.0 by Rob Tillaart (01/04/2011)
// inspired by DHT11 library
//

#include "SAC.h"

//#define TRACE_ENABLE

#ifdef TRACE_ENABLE
#define SERIAL_PRINT(msg) Serial.print(msg)
#define SERIAL_PRINTLN(msg) Serial.println(msg)
#else
#define SERIAL_PRINT(msg)
#define SERIAL_PRINTLN(msg)
#endif

#define TIMEOUT 10000

/////////////////////////////////////////////////////
//
// PUBLIC
//

NODE_FUNC devicefuncs[MAX_NUM_DEVICES];;

SAC sac;

int SAC::init()
{
	SERIAL_PRINTLN("[sac] init");

	int cnt = 0;
	p_devicefuncs = devicefuncs;
	num_of_devices = 0;
	
	for (cnt=0 ; cnt < MAX_NUM_DEVICES ;cnt++)
	{
		p_devicefuncs[cnt].name = NULL;
		p_devicefuncs[cnt].uid = NULL;
		p_devicefuncs[cnt].pDesc = NULL;
		p_devicefuncs[cnt].sessionID = NULL;
		p_devicefuncs[cnt].onInit = NULL;
		p_devicefuncs[cnt].onRead = NULL;
		p_devicefuncs[cnt].onWrite = NULL;
	}
}

int SAC::init_device()
{
	SERIAL_PRINTLN("[sac] init_device");

	int cnt = 0;
	for (cnt=0 ; cnt < num_of_devices ;cnt++)
	{
		p_devicefuncs[cnt].onInit();
	}

	return num_of_devices;	
}

int SAC::add_device(const NODE_FUNC *pNodeFunc)
{
	SERIAL_PRINTLN("[sac] add_device");

	p_devicefuncs[num_of_devices].name = pNodeFunc->name;
	p_devicefuncs[num_of_devices].uid = pNodeFunc->uid;
	p_devicefuncs[num_of_devices].pDesc = pNodeFunc->pDesc;
	p_devicefuncs[num_of_devices].sessionID = pNodeFunc->sessionID;
	p_devicefuncs[num_of_devices].onInit = pNodeFunc->onInit;
	p_devicefuncs[num_of_devices].onRead = pNodeFunc->onRead;
	p_devicefuncs[num_of_devices].onWrite = pNodeFunc->onWrite;

	num_of_devices++;

	return num_of_devices;
}

int SAC::remove_device(String strName, String strUID)
{
#if 0
	int cnt = 0;
	for (cnt=0 ; cnt < num_of_devices ;cnt++)
	{
		if (strName.equals(&devicefuncs[num_of_devices].name) &&
			strUID.equals(&devicefuncs[num_of_devices].uid))
		{
			p_devicefuncs[cnt].name = NULL;
			p_devicefuncs[cnt].uid = NULL;
			p_devicefuncs[cnt].pDesc = NULL;			
			p_devicefuncs[cnt].sessionID = NULL;			
			p_devicefuncs[cnt].onInit = NULL;
			p_devicefuncs[cnt].onRead = NULL;
			p_devicefuncs[cnt].onWrite = NULL;
			num_of_devices--;
		}
	}
#endif

	return num_of_devices;
}

int SAC::read_device(char* cmd)
{
	SERIAL_PRINTLN("[sac] read_device");

	int cnt = 0;
	for (cnt=0 ; cnt < num_of_devices ;cnt++)
	{
	   p_devicefuncs[cnt].onRead(cmd);
	}

	return 0;
}

char* SAC::read_device(int n, char* cmd)
{
	SERIAL_PRINTLN("[sac] read_device");

	if (n < MAX_NUM_DEVICES)
	{
	   	return p_devicefuncs[n].onRead(cmd);
	}

	return NULL;
}

int SAC::write_device(char* cmd, char* param)
{
	SERIAL_PRINTLN("[sac] write_device");

	int cnt = 0;
	int result = 0;
	
	for (cnt=0 ; cnt < num_of_devices ;cnt++)
	{
	   result += p_devicefuncs[cnt].onWrite(cmd, param);
	}

	return result;
}

int SAC::get_num_of_devices()
{
	return num_of_devices;
}

char* SAC::get_deviceInfoList(int n)
{
	return p_devicefuncs[n].pDesc;
}

const char* SAC::get_uID(int n)
{
	return p_devicefuncs[n].uid;
}

char* SAC::get_sessionID(int n)
{
	return p_devicefuncs[n].sessionID;
}

int SAC::set_sessionID(int n, char* ssid)
{
	p_devicefuncs[n].sessionID = ssid;
	return 1;
}

int SAC::translate(char* inStr)
{
	return 0;
}

// END OF FILE

