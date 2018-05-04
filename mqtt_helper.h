// mqtt_helper.h

#ifndef _MQTT_HELPER_h
#define _MQTT_HELPER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
extern int TEMP_MAX, TEMP_MIN, HUMI_MAX, HUMI_MIN, LIGHT_MAX, LIGHT_MIN;
extern bool library;
extern String mqtt_Message;
extern const String on_;
extern const String off_;

extern void oled_analogClock(int _hour, int _min, int _sec, int x, int y);

void mqtt_callback(char* topic, uint8_t* payload, unsigned int length);
void mqtt_reconnect();
void mqtt_init();
void mqtt_loop();
bool mqtt_publish(String topic, String payload, bool retain = false);

#endif

