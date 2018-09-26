#include <DNSServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>
#include <Time.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include "hardware.h"
#include "mqtt_helper.h"


#define __VERSION__	"3.0.1"
String _firmwareVersion = __VERSION__ " " __DATE__ " " __TIME__;

//HardwareSerial STM32_Serial = Serial;
#define STM32 Serial

bool STT_PUMP1 = true;
bool STT_PUMP2 = true;
bool STT_WATER_IN = true;
bool STT_FAN = true;
bool STT_LIGHT = true;
bool STT_LED_STATUS = true;

bool flag_SmartConfig = false;

void updateTimeStamp(unsigned long interval);
bool control(int pin, bool status, bool update_to_server, bool isCommandFromApp);
String getID() {
	byte mac[6];
	WiFi.macAddress(mac);
	String id;
	for (int i = 0; i < 6; i++)
	{
		id += mac[i] < 10 ? "0" + String(mac[i], HEX) : String(mac[i], HEX);
	}
	id.toUpperCase();
	return id.substring(id.length() - 6);
}
String HubID = getID();

void setup()
{
	delay(1000);
	Serial.begin(74880);
	Serial.setTimeout(20);

	//control(PUMP1, false, false, false);
	//control(FAN, false, false, false);
	//control(LIGHT, false, false, false);
	//control(LED_BUILTIN, false, false, false);

	Serial.println("HID=" + HubID);

	Serial.println(("LED_BUILTIN ON"));

	wifi_init();
	Serial.print("RSSI: ");
	Serial.println(WiFi.RSSI());

	digitalWrite(LED_BUILTIN, OFF);
	Serial.println(("LED_BUILTIN OFF"));

	//DEBUG.print(("IP: "));
	//DEBUG.println(WiFi.localIP().toString());
	DEBUG.print(("Firmware Version: "));
	DEBUG.println(_firmwareVersion);

	Serial.print("ID = ");
	Serial.println(HubID);
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
	get_data_sensor(5000);
	//Blynk.run();
}
