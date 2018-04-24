// 
// 
// 

#include "mqtt_helper.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "hardware.h"

extern String HubID;
extern String _firmwareVersion;
extern void updateFirmware(String url);
extern String CMD_ID;


const char* mqtt_server = "mic.duytan.edu.vn";
const char* mqtt_user = "Mic@DTU2017";
const char* mqtt_password = "Mic@DTU2017!@#";
const uint16_t mqtt_port = 1883;

const String on_ = "on";
const String off_ = "off";

int TEMP_MAX, TEMP_MIN, HUMI_MAX, HUMI_MIN, LIGHT_MAX, LIGHT_MIN;
bool library = false;

extern String timeStr;
extern bool stt_pump1, stt_fan, stt_light;
extern bool control(int pin, bool status, bool update_to_server = true);
extern void send_status_to_server(bool pump1, bool fan, bool light);
extern void hc595_digitalWrite(int pin, bool status);

String mqtt_Message;

WiFiClient mqtt_espClient;
PubSubClient mqtt_client(mqtt_espClient);

#pragma region parseTopic
void handleTopic__Mushroom_Commands_HubID() {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& commands = jsonBuffer.parseObject(mqtt_Message);

	String HUB_ID = commands["HUB_ID"].as<String>();
	bool pump1_change = false;
	bool fan_change = false;
	bool light_change = false;

	CMD_ID = commands["CMD_ID"].as<String>();
	bool isCommandFromApp = false;
	CMD_ID.trim();
	if (CMD_ID.startsWith("HW-")) {
		//DEBUG.print(F("Command ID "));
		//DEBUG.print(CMD_ID);
		//DEBUG.print(F(" was excuted."));
		static bool firsControlFromRetain = true;
		if (!firsControlFromRetain) {
			DEBUG.println(F("Skipped\r\n"));
			return;
		}
		firsControlFromRetain = false;
	}
	else {
		isCommandFromApp = true;
	}
	String pump1_stt = commands["MIST"].as<String>();
	if (pump1_stt == on_)
	{
		pump1_change = control(PUMP1, true, false);
	}
	else if (pump1_stt == off_)
	{
		pump1_change = control(PUMP1, false, false);
	}

	String light_stt = commands["LIGHT"].as<String>();
	extern bool skip_auto_light;
	extern int light;
	if (light_stt == on_)
	{
		light_change = control(LIGHT, true, false);
		if (light < LIGHT_MIN) {
			skip_auto_light = false;
		}
		else if (light > LIGHT_MAX) {
			skip_auto_light = true;
		}
	}
	else if (light_stt == off_)
	{
		light_change = control(LIGHT, false, false);
		if (light > LIGHT_MAX) {
			skip_auto_light = false;
		}
		else if (light < LIGHT_MIN) {
			skip_auto_light = true;
		}
	}

	String fan_stt = commands["FAN"].as<String>();
	if (fan_stt == on_)
	{
		fan_change = control(FAN, true, false);
	}
	else if (fan_stt == off_)
	{
		fan_change = control(FAN, false, false);
	}

	if (isCommandFromApp) {
		Serial.print(F("Send status relay to server "));
		Serial.print(pump1_change);
		Serial.print(fan_change);
		Serial.println(light_change);
		send_status_to_server(pump1_change, fan_change, light_change);
	}
}

void handleTopic__Mushroom_Library_HubID() {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& lib = jsonBuffer.parseObject(mqtt_Message);
	TEMP_MAX = lib["TEMP_MAX"].as<int>();
	TEMP_MIN = lib["TEMP_MIN"].as<int>();
	HUMI_MAX = lib["HUMI_MAX"].as<int>();
	HUMI_MIN = lib["HUMI_MIN"].as<int>();
	LIGHT_MAX = lib["LIGHT_MAX"].as<int>();
	LIGHT_MIN = lib["LIGHT_MIN"].as<int>();
	library = true;
	DEBUG.println("TEMP_MAX = " + String(TEMP_MAX));
	DEBUG.println("TEMP_MIN = " + String(TEMP_MIN));
	DEBUG.println("HUMI_MAX = " + String(HUMI_MAX));
	DEBUG.println("HUMI_MIN = " + String(HUMI_MIN));
	DEBUG.println("LIGHT_MAX = " + String(LIGHT_MAX));
	DEBUG.println("LIGHT_MIN = " + String(LIGHT_MIN));
}
#pragma endregion

int wifi_quality() {
	int dBm = WiFi.RSSI();
	if (dBm <= -100)
		return 0;
	else if (dBm >= -50)
		return 100;
	else
		return int(2 * (dBm + 100));
}

void mqtt_callback(char* topic, uint8_t* payload, unsigned int length) {
	//ulong t = millis();
	//DEBUG.print(F("\r\n#1 FREE RAM : "));
	//DEBUG.println(ESP.getFreeHeap());

	String topicStr = topic;

	DEBUG.println(F("\r\n>>>"));
	//DEBUG.println(topicStr);
	DEBUG.print(F("Message arrived: "));
	DEBUG.print(topicStr);
	DEBUG.print(F("["));
	DEBUG.print(length);
	DEBUG.println(F("]"));

	mqtt_Message = "";
	hc595_digitalWrite(LED_STATUS, ON);
	for (uint i = 0; i < length; i++) {
		//DEBUG.print((char)payload[i]);
		mqtt_Message += (char)payload[i];
	}
	hc595_digitalWrite(LED_STATUS, OFF);

	DEBUG.println(mqtt_Message);

	//control pump1, light, fan
	if (topicStr == String("Mushroom/Commands/" + HubID))
	{
		handleTopic__Mushroom_Commands_HubID();
	}

	else if (topicStr == String("Mushroom/Library/" + HubID)) {
		handleTopic__Mushroom_Library_HubID();
	}

	else if (topicStr == "Mushroom/Terminal") {
		StaticJsonBuffer<250> jsonBuffer;
		JsonObject& terminal = jsonBuffer.parseObject(mqtt_Message);
		/*
		{
		   "Command" : "FOTA",
		   "HUB_ID" : "all",
		   "Version" : "",
		   "Url" : ""
		}
		*/
		DEBUG.println(F("Update firmware function"));
		String command = terminal["Command"].as<String>();
		if (command == "FOTA") {
			String hub = terminal["Hub_ID"].as<String>();
			if ((hub == HubID) || (hub == "all")) {
				String ver = terminal["Version"].as<String>();
				if (ver != _firmwareVersion) {
					String url = terminal["Url"].as<String>();
					mqtt_publish("Mushroom/Terminal/" + HubID, "Updating new firmware");
					DEBUG.print(F("\nUpdating new firmware: "));
					DEBUG.println(ver);
					DEBUG.println(url);
					updateFirmware(url);
					DEBUG.println(F("DONE!"));
				}
			}
		}
	}

	else if (topicStr == "Mushroom/Terminal/" + HubID) {
		if (mqtt_Message.indexOf("/get version") > -1) {
			//StaticJsonBuffer<200> jsBuffer;
			DynamicJsonBuffer jsBuffer(200);
			JsonObject& jsData = jsBuffer.createObject();
			jsData["HUB_ID"] = HubID;
			jsData["FW Version"] = _firmwareVersion;

			String data;
			data.reserve(100);
			jsData.printTo(data);
			mqtt_publish("Mushroom/Terminal/" + HubID, data);
		}

		String mqtt_cmd = mqtt_Message;
		mqtt_cmd.toUpperCase();
		extern void control_handle(String cmd);
		control_handle(mqtt_cmd);
	}

	//DEBUG.print(F("#2 FREE RAM : "));
	//DEBUG.println(ESP.getFreeHeap());
	//t = millis() - t;
	//DEBUG.println("Time: " + String(t));
}

void mqtt_reconnect() {  // Loop until we're reconnected
	while (!mqtt_client.connected()) {
		DEBUG.print(F("Attempting MQTT connection..."));
		//boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
		if (mqtt_client.connect(HubID.c_str(), mqtt_user, mqtt_password, ("Mushroom/Status/" + HubID).c_str(), 0, true, String(HubID + " offline").c_str())) {
			DEBUG.print(F(" Connected."));
			//String h_online = "{\"HUB_ID\":\"" + HubID + "\",\"STATUS\":\"ONLINE\",\"FW_VER\":\"" + _firmwareVersion + "\",\"WIFI\":\"" + WiFi.SSID() + "\",\"SIGNAL\":" + String(wifi_quality()) + "}";
			String h_online = HubID + " online";
			mqtt_client.publish(("Mushroom/Status/" + HubID).c_str(), (HubID + " online").c_str(), true);
			mqtt_client.publish(("Mushroom/Status/" + HubID).c_str(), h_online.c_str(), true);

			mqtt_client.publish(("Mushroom/SetWifi/" + HubID).c_str(), "Success");
			mqtt_client.subscribe(("Mushroom/Library/" + HubID).c_str());
			mqtt_client.subscribe(("Mushroom/Commands/" + HubID).c_str());
			mqtt_client.subscribe("Mushroom/Terminal");
		}
		else {
			DEBUG.print(F("failed, rc="));
			DEBUG.print(mqtt_client.state());
			return;
			//DEBUG.println(F(" try again"));
			//delay(500);
		}
	}
}

void mqtt_init() {
	//http.setReuse(true);
	mqtt_Message.reserve(MQTT_MAX_PACKET_SIZE); //tao buffer khoang trong cho mqtt_Message
	mqtt_client.setServer(mqtt_server, mqtt_port);
	mqtt_client.setCallback(mqtt_callback);
}

void mqtt_loop() {
	if (!WiFi.isConnected()) {
		hc595_digitalWrite(LED_STATUS, ON);
		return;
	}
	if (!mqtt_client.connected()) {
		mqtt_reconnect();
	}
	mqtt_client.loop();
	delay(1);
}

bool mqtt_publish(String topic, String payload, bool retain) {
	if (!mqtt_client.connected()) {
		return false;
	}
	DEBUG.print(F("MQTT publish to topic: "));
	DEBUG.println(topic);
	DEBUG.println(payload);
	DEBUG.println();

	hc595_digitalWrite(LED_STATUS, ON);
	bool ret = mqtt_client.publish(topic.c_str(), payload.c_str(), retain);
	delay(1);
	hc595_digitalWrite(LED_STATUS, OFF);
	return ret;
}