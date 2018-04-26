
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
#include <ArduinoJson.h>

//#define USE_OLED
#ifdef USE_OLED
#include <SSD1306.h>
#include <qrcode.h>
#endif // USE_OLED

#define __VERSION__	"2.1.5"

String _firmwareVersion = __VERSION__ " " __DATE__ " " __TIME__;

Button myBtn(PININ_BUTTON, true, true, 20);


bool STT_PUMP1 = false;
bool STT_PUMP2 = false;
bool STT_WATER_IN = false;
bool STT_FAN = false;
bool STT_LIGHT = false;
bool STT_LED_STATUS = false;

extern void updateTimeStamp(unsigned long interval);
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
String HubID = getID();

#ifdef USE_OLED
SSD1306  display(0x3c, SDA, SCL);
QRcode qrcode(&display);
#endif // USE_OLED

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

	//Serial.println(F("SmartConfig started."));
	//WiFi.beginSmartConfig();
	//while (1) {
	//	delay(100);
	//	Serial.print(F("."));
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

	//Serial.println(F("SmartConfig: Success"));
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

	control(PUMP1, false, false);
	control(FAN, false, false);
	control(LIGHT, false, false);
	control(LED_STATUS, false, false);

	Serial.println("HID=" + HubID);


	Serial.println(F("LED_STATUS ON"));

	sensor_init();

	wifi_init();
	Serial.print("RSSI: ");
	Serial.println(WiFi.RSSI());

	hc595_digitalWrite(LED_STATUS, OFF);
	Serial.println(F("LED_STATUS OFF"));

#ifdef USE_OLED
	display.clear();
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(ArialMT_Plain_16);
	display.drawString(0, 10, "Connected");
	display.display();
#endif // USE_OLED

	DEBUG.print(F("IP: "));
	DEBUG.println(WiFi.localIP());
	DEBUG.print(F("Firmware Version: "));
	DEBUG.println(_firmwareVersion);

	Serial.print("ID = ");
	Serial.println(HubID);
	updateTimeStamp(0);
	mqtt_init(); 
}

void loop()
{
	/* add main program code here */
	updateTimeStamp(3600000);
	mqtt_loop();
	serial_command_handle();
	button_handle();
	update_sensor(10000);
	auto_control();
}
