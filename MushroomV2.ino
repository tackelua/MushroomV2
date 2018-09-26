#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <DHT.h>
#include <DNSServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include <Button.h>
#include <TimeLib.h>
#include <Time.h>
#include <SHT1x.h>
#include <ESP8266httpUpdate.h>
#include "hardware.h"
//#include <DHT.h>
#include "Sensor.h"
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include "mqtt_helper.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>

//#define USE_OLED
#ifdef USE_OLED
#include <SSD1306.h>
#include <qrcode.h>
#endif // USE_OLED

#define __VERSION__	"2.1.18"

String _firmwareVersion = __VERSION__ " " __DATE__ " " __TIME__;

//HardwareSerial STM32_Serial = Serial;
//#define STM32 STM32_Serial

//WiFiManager wifiManager;
Button myBtn(PININ_BUTTON, true, true, 20);


bool STT_PUMP1 = true;
bool STT_PUMP2 = true;
bool STT_WATER_IN = true;
bool STT_FAN = true;
bool STT_LIGHT = true;
bool STT_LED_STATUS = true;


//void DEBUG.print(String x, bool isSendToMQTT = false);
//void DEBUG.print(String x, bool isSendToMQTT) {
//	DEBUG.print(x);
//	if (isSendToMQTT) {
//		mqtt_publish("Mushroom/Debug/" + HubID, x, false);
//	}
//}
//void DEBUG.println(String x = "", bool isSendToMQTT = false);
//void DEBUG.println(String x, bool isSendToMQTT) {
//	DEBUG.println(x);
//	if (x = "") return;
//	if (isSendToMQTT) {
//		mqtt_publish("Mushroom/Debug/" + HubID, x, false);
//		mqtt_publish("Mushroom/Debug/" + HubID, "\r\n", false);
//	}
//}
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
	return id;
}
String HubID = "600194092C1D";// getID();

#ifdef USE_OLED
SSD1306  display(0x3c, SDA, SCL);
QRcode qrcode(&display);
#endif // USE_OLED

bool flag_SmartConfig = false;
bool flag_error_wifi = false;
bool flag_water_empty = false;


void setup()
{
#ifdef USE_OLED
	display.init();
	display.clear();
	display.flipScreenVertically();
	display.setContrast(255);

	qrcode.init();
	qrcode.create("HID=" + HubID);
#endif // USE_OLED
	delay(50);
	Serial.begin(74880);
	Serial.setTimeout(20);

	////====	
	//display.clear();
	//display.setTextAlignment(TEXT_ALIGN_LEFT);
	//display.setFont(ArialMT_Plain_16);
	//display.drawString(0, 10, "SmartConfig");
	//display.display();

	//Serial.println(("SmartConfig started."));
	//WiFi.beginSmartConfig();
	//while (1) {
	//	delay(100);
	//	Serial.print(("."));
	//	if (WiFi.smartConfigDone()) {
	//		WiFi.stopSmartConfig();
	//		break;
	//	}
	//}

	//display.clear();
	//display.setTextAlignment(TEXT_ALIGN_LEFT);
	//display.setFont(ArialMT_Plain_16);
	//display.drawString(0, 10, "Success");
	//display.display();

	//Serial.println(("SmartConfig: Success"));
	//WiFi.printDiag(Serial);

	//wifi_init();
	//display.clear();
	//display.setTextAlignment(TEXT_ALIGN_LEFT);
	//display.setFont(ArialMT_Plain_16);
	//display.drawString(0, 10, WiFi.localIP().toString());
	//display.display();
	////====


	//pinMode(LED_STATUS, OUTPUT);
	//pinMode(PUMP1, OUTPUT);
	//pinMode(FAN, OUTPUT);
	//pinMode(LIGHT, OUTPUT);
	pinMode(HC595_DATA, OUTPUT);
	pinMode(HC595_SHCP, OUTPUT);
	pinMode(HC595_STCP, OUTPUT); digitalWrite(HC595_STCP, HIGH);
	pinMode(PININ_WATER_L, INPUT);

	control(PUMP1, false, false, false);
	control(FAN, false, false, false);
	control(LIGHT, false, false, false);
	control(LED_STATUS, false, false, false);

	Serial.println("HID=" + HubID);


	Serial.println(("LED_STATUS ON"));

	sensor_init();

	wifi_init();
	Serial.print("RSSI: ");
	Serial.println(WiFi.RSSI());

	hc595_digitalWrite(LED_STATUS, OFF);
	Serial.println(("LED_STATUS OFF"));

#ifdef USE_OLED
	display.clear();
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(ArialMT_Plain_16);
	display.drawString(0, 10, "Connected");
	display.display();
#endif // USE_OLED

	//DEBUG.print(("IP: "));
	//DEBUG.println(WiFi.localIP().toString());
	DEBUG.print(("Firmware Version: "));
	DEBUG.println(_firmwareVersion);

	Serial.print("ID = ");
	Serial.println(HubID);
	//updateTimeStamp(0);
	mqtt_init(); 
	blynk_init();
}

void loop()
{
	/* add main program code here */
	wifi_loop();
	led_loop();
	updateTimeStamp(3600000);
	mqtt_loop();
	serial_command_handle();
	button_handle();
	update_sensor(10000);
	auto_control();
	Blynk.run();
}
