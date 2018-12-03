// mqtt_helper.h

#ifndef _MQTT_HELPER_h
#define _MQTT_HELPER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
extern int TEMP_MAX, TEMP_MIN, HUMI_MAX, HUMI_MIN, LIGHT_MAX, LIGHT_MIN;
//extern long DATE_HAVERST_PHASE;
extern long SENSOR_UPDATE_INTERVAL_DEFAULT;
extern bool LIBRARY;
extern volatile long SENSOR_UPDATE_INTERVAL;
extern bool skip_update_sensor_after_control;
extern unsigned long t_skip_update_sensor_after_control;

extern String mqtt_Message;
extern const String on_;
extern const String off_;
extern const char* file_libConfigs;

extern String tp_Status;
extern String tp_Library;
extern String tp_Request;
extern String tp_Response;
extern String tp_Debug;
extern String tp_Terminal;
extern String tp_Terminal_Hub;
extern String tp_Terminal_Res;
extern String tp_Interval;
extern String tp_Light;
extern String tp_Pump_Mix;
extern String tp_Pump_Floor;
extern String tp_Fan_Mix;
extern String tp_Fan_Wind;

void mqtt_callback(char* topic, uint8_t* payload, unsigned int length);
void mqtt_reconnect();
void mqtt_init();
void mqtt_loop();
bool mqtt_publish(String topic, String payload, bool retain = false);
void handleTopic__Mushroom_Library_HubID(String mqtt_Message, bool save = true);
void handle_Terminal(String msg);

#endif

