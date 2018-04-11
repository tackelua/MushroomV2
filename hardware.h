#ifndef _HARDWARE_h
#define _HARDWARE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


#define DEBUG Serial

#define PININ_WATER_H	D0
#define PININ_WATER_L	D5
#define PININ_WATER_OH	D6
#define PININ_BUTTON	D3

#define HC595_DATA		D4
#define HC595_SHCP		D7
#define HC595_STCP		D8

/*
 * Which sensor is used?
 */
 //#define DHT
#define SHT //USE SHT1x

//#define BH1750
#define USED_LIGHT_SENSOR_ANALOG

//========= TEMP - HUMI =========
#ifdef DHT
const int DHTPIN = D2;// 16;  //??c d? li?u t? DHT11 ? chân 2 trên m?ch Arduino
#define DHTTYPE	DHT11
#endif // DHT

#ifdef SHT
#define SHT_dataPin  D1
#define SHT_clockPin D2
#endif // SHT
//===============================

#ifdef BH1750
#define BH1750_ADDRESS	0x23
#endif // BH1750
#ifdef USED_LIGHT_SENSOR_ANALOG
#define PININ_LIGHT_SENSOR	A0
#endif // USED_LIGHT_SENSOR_ANALOG

#define ON	HIGH 
#define OFF	LOW
//#define ON	HIGH
//#define OFF	LOW

#endif

