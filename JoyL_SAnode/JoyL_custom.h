//#define TRACE_ENABLE_SA

#ifdef TRACE_ENABLE_SA
#define SERIAL_PRINT_SA(msg) Serial.print(msg)
#define SERIAL_PRINTLN_SA(msg) Serial.println(msg)
#else
#define SERIAL_PRINT_SA(msg)
#define SERIAL_PRINTLN_SA(msg)
#endif

/*=================================================
  Common functions
  =================================================*/
extern void SA_Registeration();
static int translate(char* inStr);

#define START_DESC "/1.5/sanode%{"
#define END_DESC "}\n"

/*=================================================
  SAnode : Light Sensor Specifications
  =================================================*/
boolean redState = false;
boolean blueState = false;
boolean greenState = false;
int redPin = 2;
int bluePin = 4;
int greenPin = 3;

static int onLightInit(void);
static char* onLightRead(char* cmd);
static int onLightWrite(char* cmd, char* param);
static void LightController(char command);

#define DEVICE_NAME_LIGHT  "Light"
#define DEVICE_UID_LIGHT   "78C40E0174F9"

/*
{
  "nodeName":"Light",
  "nodeID":"000000000001",
  "activationCode":"joyl-light",
  "actuatorList":
    {"light":["off","red","blue","green","pattern"]}
}	
*/

char device_Light[] = {
 START_DESC
 "[nodeName]      : [Light],        "
 "[nodeID]        : [78C40E019AC8], "
 "[activationCode]: [joyl-light],   "
 "[actuatorList]  : "
 " < "
 "  [light]    : +[off],[red],[blue],[green],[pattern]+ "
 " > "
END_DESC
};

static NODE_FUNC light_func = {
    DEVICE_NAME_LIGHT,
	DEVICE_UID_LIGHT,
    device_Light,
    "joyl-light",
	onLightInit,
    onLightRead,
    onLightWrite,
};

/* basic functions ----------------------------------------*/
static int onLightInit(void)
{
	SERIAL_PRINTLN_SA("onLightInit");

	pinMode(redPin, OUTPUT);
	digitalWrite(redPin, HIGH);
	
	pinMode(bluePin, OUTPUT);
	digitalWrite(bluePin, HIGH);
	
	pinMode(greenPin, OUTPUT);
	digitalWrite(greenPin, HIGH);	 

	translate(device_Light);
	return 0;
}

static char* onLightRead(char* cmd)
{
	SERIAL_PRINTLN_SA("onLightRead");
	return NULL;
}


extern int compareString(char* srcStr, char* destStr, int leng);

static int onLightWrite(char* cmd, char* param)
{
	//Serial.println(cmd);
	//Serial.println(param);

	if (compareString("light",cmd,5)==1)
	{
		if (compareString("off",param,3)==1) 
		{	
			LightController('O');
		}
		else if (compareString("red",param,3)==1) 
		{
			LightController('R');
		}
		else if (compareString("blue",param,4)==1) 
		{
			LightController('B');
		}
		else if (compareString("green",param,5)==1) 
		{
			LightController('G');
		}
		else if (compareString("pattern",param,5)==1) 
		{
			LightController('P');
		}
		else
		{
			return 0;
		}
		return 1;
	}
	return 0;
}

/* private functions ----------------------------------------*/
static void LightRBGPinCtrl(int val_red, int val_blue, int val_green)
{
	digitalWrite(redPin, val_red);
	digitalWrite(bluePin, val_blue);
	digitalWrite(greenPin, val_green);
	
	redState = (val_red == HIGH)? false : true;
	blueState = (val_blue == HIGH)? false : true;
	greenState = (val_green == HIGH)? false : true;
}

static void LightController(char command)
{
     switch (command)
     {
       case 'R':
       case 'r': 
         if (redState)
         {  
           digitalWrite(redPin, HIGH);
           redState = false;
         } else {
           LightRBGPinCtrl(LOW,HIGH,HIGH);
         }
         break;
         
       case 'B':
       case 'b': 
         if (blueState)
         {  
           digitalWrite(bluePin, HIGH);
           blueState = false;
         } else {
		   LightRBGPinCtrl(HIGH,LOW,HIGH);
        }
         break;
         
       case 'G':
       case 'g':
         if (greenState)
         {  
           digitalWrite(greenPin, HIGH);
           greenState = false;
         } else {
		   LightRBGPinCtrl(HIGH,HIGH,LOW);
         }
         break;

       case 'P':
       case 'p':
		   LightRBGPinCtrl(LOW,HIGH,HIGH);delay(500);
		   LightRBGPinCtrl(HIGH,LOW,HIGH);delay(500);
		   LightRBGPinCtrl(HIGH,HIGH,LOW);delay(500);
		   LightRBGPinCtrl(LOW,HIGH,HIGH);delay(500);
		   LightRBGPinCtrl(HIGH,LOW,HIGH);delay(500);
		   LightRBGPinCtrl(HIGH,HIGH,LOW);delay(500);
		   LightRBGPinCtrl(HIGH,HIGH,HIGH);delay(500);
		   break;

       case 'O': /* Turn everything off */
       case 'o':
	   	  LightRBGPinCtrl(HIGH,HIGH,HIGH);
          break;

       default:
		   break;
     }
}

 /*=================================================
   SAnode : Humidity & Temperature Sensor Specifications
   =================================================*/
 dht DHT;

 #define dht_dpin 2

 static int onHTInit(void);
 static char* onHTRead(char* cmd);
 static int onHTWrite(char* cmd, char* param);

 /*
 {
   "nodeName":"HTSensor",
   "nodeID":"78C40E0174F9",
   "activationCode":"joyl-humidity",
   "sensorList":
	 {"temperature":{"unit":"celsius"},
	  "humidity": {"unit":"percentage"}}
 }
 */
 char device_HTSensor[] = {
 START_DESC  
 "[nodeName]      : [HTSensor],      "
 "[nodeID]        : [78C40E0174F9],  "
 "[activationCode]: [joyl-humidity], "
 "[sensorList]    : "
 " < "
 "  [temperature] : < [unit]:[celsius]    >, "
 "  [humidity]    : < [unit]:[percentage] >  "
 " > "
 END_DESC
 };

#define DEVICE_NAME_HT  "HTSensor"
#define DEVICE_UID_HT   "78C40E0174F9"

 static NODE_FUNC HT_func = {
	 DEVICE_NAME_HT,
	 DEVICE_UID_HT,
	 device_HTSensor,
	 "joyl-humidity",
	 onHTInit,
	 onHTRead,
	 onHTWrite,
 };
 
 /* basic functions ----------------------------------------*/
 static int onHTInit(void)
 {
	 SERIAL_PRINTLN_SA("onHTInit");

	 translate(device_HTSensor);

	 return 0;
 }
 
 char report_msg[] = ("/1.5/sanode/78C40E0174F9 {\"sessionID\":\"joyl-humidity\",\"sensorList\":{\"temperature\":000,\"humidity\":111}}");

 static char* report_msg_HT(char* val_temperature, char* val_humidity)
 {
 	//out-message.. /sanode/78C40E0174F9 {"sessionID":"joyl-humidity","sensorList":{"temperature":27,"humidity":70}}

	int cnt = 0;

	if ((val_temperature) == NULL || (val_humidity == NULL))
	{
		return NULL;
	}
	
	for (cnt=0 ; cnt < 3 ; cnt++)
	{
		report_msg[82+cnt] = (val_temperature[cnt] != '\0') ? val_temperature[cnt] : ' ';
		report_msg[97+cnt] = (val_humidity[cnt] != '\0') ? val_humidity[cnt] : ' ';
	}
	return report_msg;
 }

 char val_result1[5];
 char val_result2[5];
#if 0 /* to report when different value */
 double temperature_old;
 double humidity_old;
#endif
 static char* onHTRead(char* cmd)
 {
	int isMonitor;
		
	SERIAL_PRINTLN_SA("onHTRead");

	isMonitor = compareString("monitor",cmd,7);

	if ((compareString("temperature",cmd,11)==1) ||
 	  (compareString("humidity",cmd,8)==1) ||
 	  (isMonitor==1))
	{
		DHT.read11(dht_dpin);
		delay(3000);
		
		itoa(DHT.temperature, &val_result1[0], 10);
		itoa(DHT.humidity, &val_result2[0], 10);

#if 0 /* to report when different value */
		if (((isMonitor==1)) && 
			(temperature_old == DHT.temperature) &&
			(humidity_old == DHT.humidity))
		{
			return NULL;
		}
		else
		{
			temperature_old = DHT.temperature;
			humidity_old = DHT.humidity;
		}
#endif
		return report_msg_HT(val_result1, val_result2);
	}
	return NULL;
 }
 
 static int onHTWrite(char* cmd, char* param)
 {
	 SERIAL_PRINTLN_SA("onHTWrite");
	 return 0;
 }

 /* private functions ----------------------------------------*/
#if 0
 static void HTTController()
  {
	 // Now we read the data from the sensor
	 DHT.read11(dht_dpin);
 
	 SERIAL_PRINT_SA("Current humidity = ");
	 SERIAL_PRINT_SA(DHT.humidity);
	 SERIAL_PRINT_SA("%  ");
	 SERIAL_PRINT_SA("temp = ");
	 SERIAL_PRINT_SA(((DHT.temperature*1.8)+32));
	 SERIAL_PRINT_SA("F  ");
	 SERIAL_PRINT_SA("temp = ");
	 SERIAL_PRINT_SA(DHT.temperature);
	 SERIAL_PRINTLN_SA("C	");

	 // The following delay sets our sample rate
	 delay(3000);
  }
#endif

 /*=================================================
   Common function Specifications
   =================================================*/

 extern void SA_Registeration()
 {
 	DHT.read11(dht_dpin);

	/* SAnode detection - temparary */
 	if ((DHT.temperature == 0) && (DHT.humidity == 0))
 	{
		sac.add_device(&light_func);
 	}
	else
	{
		sac.add_device(&HT_func);
	}
 }

 static int translate(char* inStr)
 {
	 int cnt = 0;
	 int wp = 0;
	 int opened = 0;
	 char readChar;
 
	 //SERIAL_PRINTLN_SA(inStr);
 
	 do {
		 readChar = inStr[cnt];
 
		 switch (readChar)
		 {
			 case '[': inStr[wp++] = '"'; break;
			 case ']': inStr[wp++] = '"'; break;
			 case '<': inStr[wp++] = '{'; break;
			 case '>': inStr[wp++] = '}'; break;
			 case '%': inStr[wp++] = ' '; break;
			 case '+':
				 if (opened == 0)
				 {
					 inStr[wp++] = '[';
					 opened = 1;
				 }
				 else
				 {
					 inStr[wp++] = ']';
					 opened = 0;
				 }
				 break;
			 case ' ': break;
			 case '\t': break;
			 default: inStr[wp++] = readChar; break;
		 }
		 cnt++;
	 }while ((readChar != '\0'));
	 SERIAL_PRINTLN_SA(inStr);
	 return cnt;
 }
 /*=================================================
   End
   =================================================*/




