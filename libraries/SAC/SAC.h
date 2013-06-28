// 
// FILE: SAC.h
// VERSION: 0.1.00
// PURPOSE: SAC
//
// HISTORY:
// 

#ifndef sac_h
#define sac_h

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define SAC_LIB_VERSION "0.1.00"

#include <WString.h>
#include <Arduino.h>

#define MAX_NUM_DEVICES 5

typedef int (*NODE_INIT) (void);
typedef char* (*NODE_READ) (char* cmd);
typedef int (*NODE_WRITE) (char* cmd, char* param);

typedef struct {
	const char* name;
	const char* uid;
	char* pDesc;
	char* sessionID;
    NODE_INIT onInit;
	NODE_READ onRead;
	NODE_WRITE onWrite;
} NODE_FUNC;

class SAC
{
public:
	int init();
	int init_device();
	int add_device(const NODE_FUNC *pNodeFunc);
	int remove_device(String strName, String strUID);
	int read_device(char* cmd);
	char* read_device(int n, char* cmd);
	int write_device(char* cmd, char* param);
	int get_num_of_devices();	
	char* get_deviceInfoList(int n);
	int translate(char* inStr);
	const char* get_uID(int n);
	//int set_uID(int n, char* ssid);	
	char* get_sessionID(int n);
	int set_sessionID(int n, char* ssid);

private:
    NODE_FUNC* p_devicefuncs;
	int num_of_devices;
};

extern SAC sac;

#endif

// END OF FILE
