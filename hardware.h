#ifndef _HARDWARE_h
#define _HARDWARE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//#include "mqtt_helper.h"
extern String HubID;

#define DEBUG Serial

enum STM32_RELAY {
	PUMP1 = 1,
	PUMP2,
	FAN,
	LIGHT,
	WATER_IN
};

#define ON	HIGH 
#define OFF	LOW

#endif

