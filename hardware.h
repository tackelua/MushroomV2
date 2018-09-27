#ifndef _HARDWARE_h
#define _HARDWARE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#define STM32 Serial
#define DEBUG Serial1

//#include "mqtt_helper.h"
extern String HubID;

enum STM32_RELAY {
	PUMP1 = 1,
	PUMP2,
	FAN,
	LIGHT,
	WATER_IN
};

#define ON	LOW 
#define OFF	HIGH

#endif

