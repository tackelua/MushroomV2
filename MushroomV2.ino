#define MQTT_MAX_PACKET_SIZE 1024

#include <SoftwareSerial.h>
#include <DNSServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>
#include <Time.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
//#define BLYNK_PRINT Serial
//#include <BlynkSimpleEsp8266.h>
#include <FS.h>
#include <EEPROM.h>
#include "hardware.h"
#include "mqtt_helper.h"

#define __VERSION__	"3.1.7d8-beta"
String _firmwareVersion = __VERSION__ " " __DATE__ " " __TIME__;
String _hardwareVersion;


bool flag_schedule_pump_floor = false;
bool flag_isCommandFromApp;

bool flag_SmartConfig = false;

void updateTimeStamp(unsigned long interval);
void control(int pin, bool status, bool isCommandFromApp);

const int id_eeprom_addr = 0;
String getID() {
	EEPROM.begin(50);
	String _id; 
	for (byte i = 0; i < 50; i++) {
		char c = EEPROM.read(id_eeprom_addr + i);
		_id += c; 
		if (c == '.') {
			break;
		}
	}
	_id.trim();

	if (_id.charAt(_id.length() - 1) != '.') {
		_id = setID();
	}
	else {
		_id = _id.substring(0, _id.length() - 1);
	} 
	return _id;
}
String setID() { 
	String _id;
	byte mac[6];
	WiFi.macAddress(mac);
	for (int i = 0; i < 6; i++)
	{
		_id += mac[i] < 10 ? "0" + String(mac[i], HEX) : String(mac[i], HEX);
	}
	_id = _id.substring(_id.length() - 6);
	_id.trim();
	_id.toUpperCase();
	return setID(_id);
}
String setID(String id) { 
	String _id = id;
	_id.trim();
	_id += ".";
	_id.toUpperCase();  
	for (byte i = 0; i < _id.length(); i++) {
		EEPROM.write(id_eeprom_addr + i, _id[i]); 
	}
	EEPROM.commit();
	return _id.substring(0, _id.length() - 1);
}
String HubID;// = getID();

void setup()
{
	pinMode(D3, OUTPUT);
	digitalWrite(D3, HIGH);

	STM32.begin(74880);
	STM32.setTimeout(20);
	DEBUG.begin(74880);
	DEBUG.setTimeout(20);

	DEBUG.print(("\r\nFirmware Version: "));
	DEBUG.println(_firmwareVersion);

	HubID = getID();
	DEBUG.print(("\r\nHubID: "));
	DEBUG.println(HubID);
	DEBUG.println();

	libraries_init();
	wifi_init();
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, OFF);

	updateTimeStamp(0);
	mqtt_init();

	//blynk_init();
	set_hub_id_to_stm32(HubID);
}

void loop()
{
	wifi_loop();
	updateTimeStamp(3600000);
	mqtt_loop();
	stm32_command_handle();
	auto_control();
	get_data_sensor();
	//Blynk.run();
}
