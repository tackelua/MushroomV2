// Sensor.h

#ifndef _SENSOR_h
#define _SENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <SHT1x.h>

void sensor_init();
float readTemp();
float readHumi();
int readLight();
#endif

