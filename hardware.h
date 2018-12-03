#ifndef _HARDWARE_h
#define _HARDWARE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#include "SoftwareSerial.h"

#define STM32 Serial
//#define DEBUG Serial

#ifndef DEBUG
extern SoftwareSerial DEBUG;
#endif // !DEBUG



//#include "mqtt_helper.h"
extern String HubID;

enum STM32_RELAY {
	R_LIGHT = 1,
	R_PUMP_MIX,
	R_PUMP_FLOOR,
	R_FAN_MIX,
	R_FAN_WIND
};

#define ON	HIGH 
#define OFF	LOW

#endif

