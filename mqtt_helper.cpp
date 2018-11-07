// 
// 
// 

#include "mqtt_helper.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "hardware.h"
#include <FS.h>

extern String HubID;
extern String _firmwareVersion;
extern void updateFirmware(String url);
extern String CMD_ID;
extern bool flag_schedule_pump_floor;

extern bool skip_auto_pump_mix;
extern bool skip_auto_light;
extern bool skip_auto_fan_mix; 
extern bool skip_auto_fan_wind;

const char* file_libConfigs = "/configs.json";
const char* mqtt_server = "mic.duytan.edu.vn";
const char* mqtt_user = "Mic@DTU2017";
const char* mqtt_password = "Mic@DTU2017!@#";
const uint16_t mqtt_port = 1883;

const String on_ = "on";
const String off_ = "off";

int TEMP_MAX, TEMP_MIN, HUMI_MAX, HUMI_MIN, LIGHT_MAX, LIGHT_MIN;
long DATE_HAVERST_PHASE;
long SENSOR_UPDATE_INTERVAL = 30000;
bool library = false;

extern String timeStr;
extern bool STT_PUMP_MIX, STT_FAN_MIX, STT_LIGHT;
extern bool pin_change;
extern void control(int pin, bool status, bool isCommandFromApp);
extern void stm32_digitalWrite(int pin, bool status);
extern String make_status_string_to_stm32();
extern void send_message_to_stm32(String cmd);
extern void send_status_to_server();

String mqtt_Message;

WiFiClient mqtt_espClient;
PubSubClient mqtt_client(mqtt_espClient);

void send_control_message_all_to_stm32();

#pragma region parseTopic
void handleTopic__Mushroom_Commands_HubID(String mqtt_Message) {
	StaticJsonBuffer<500> jsonBuffer;
	JsonObject& commands = jsonBuffer.parseObject(mqtt_Message);

	//String HUB_ID = commands["HUB_ID"].as<String>();

	//CMD_ID = commands["CMD_ID"].as<String>();

	//CMD_ID.trim();
	//if (CMD_ID.startsWith("HW-")) {
	//	static bool firsControlFromRetain = true;
	//	if (!firsControlFromRetain) {
	//		DEBUG.println(("Skipped\r\n"));
	//		return;
	//	}
	//	firsControlFromRetain = false;
	//}
	//else {
	//	isCommandFromApp = true;
	//}

	bool isCommandFromApp = true;
	String pump_mix_stt = commands["MIST"].as<String>();
	if (pump_mix_stt == on_)
	{
		skip_auto_pump_mix = true;
		control(PUMP_MIX, true, isCommandFromApp);
		
		//control(PUMP_FLOOR, true, isCommandFromApp);
		flag_schedule_pump_floor = true;
	}
	else if (pump_mix_stt == off_)
	{
		skip_auto_pump_mix = true;
		control(PUMP_MIX, false, isCommandFromApp);
		control(PUMP_FLOOR, false, isCommandFromApp);
	}

	String light_stt = commands["LIGHT"].as<String>();
	if (light_stt == on_)
	{
		skip_auto_light = true;
		control(LIGHT, true, isCommandFromApp);
	}
	else if (light_stt == off_)
	{
		skip_auto_light = true;
		control(LIGHT, false, isCommandFromApp);
	}

	String fan_stt = commands["FAN"].as<String>();
	if (fan_stt == on_)
	{
		skip_auto_fan_mix = true;
		skip_auto_fan_wind = true;
		control(FAN_MIX, true, isCommandFromApp);
		control(FAN_WIND, true, isCommandFromApp);
	}
	else if (fan_stt == off_)
	{
		skip_auto_fan_mix = true;
		skip_auto_fan_wind = true;
		control(FAN_MIX, false, isCommandFromApp);
		control(FAN_WIND, false, isCommandFromApp);
	}

	if (pin_change) {
		send_control_message_all_to_stm32();
	}
}

void handleTopic__Mushroom_Library_HubID(String mqtt_Message, bool save) {
	const size_t bufferSize = JSON_OBJECT_SIZE(9) + 300;
	DynamicJsonBuffer jsonBuffer(500);
	JsonObject& lib = jsonBuffer.parseObject(mqtt_Message);
	if (!lib.success())
	{
		DEBUG.println(F("\r\nParse Libs failed"));
		//DEBUG.println(mqtt_Message);
		return;
	}

	if (save) {
		File libConfigs;
		libConfigs = SPIFFS.open(file_libConfigs, "w");
		if (libConfigs) {
			//lib.prettyPrintTo(DEBUG);
			lib.printTo(libConfigs);
			libConfigs.close();
		}
		else {
			DEBUG.println(F("failed to open configs file for writing"));
		}
	}

	TEMP_MAX = lib["TEMP_MAX"].as<int>();
	TEMP_MIN = lib["TEMP_MIN"].as<int>();
	HUMI_MAX = lib["HUMI_MAX"].as<int>();
	HUMI_MIN = lib["HUMI_MIN"].as<int>();
	LIGHT_MAX = lib["LIGHT_MAX"].as<int>();
	LIGHT_MIN = lib["LIGHT_MIN"].as<int>();
	DATE_HAVERST_PHASE = lib["DATE_HAVERST_PHASE"].as<long>();

	long interval = lib["SENSOR_UPDATE_INTERVAL"].as<long>();
	if (interval > 0) {
		SENSOR_UPDATE_INTERVAL = interval;
	}

	String LIBRARY = lib["LIBRARY"].asString();
	if (LIBRARY == "ENABLE") {
		library = true;
	}
	else if (LIBRARY == "DISABLE") {
		library = false;
	}
	
	DEBUG.println("TEMP_MAX : " + String(TEMP_MAX));
	DEBUG.println("TEMP_MIN : " + String(TEMP_MIN));
	DEBUG.println("HUMI_MAX : " + String(HUMI_MAX));
	DEBUG.println("HUMI_MIN : " + String(HUMI_MIN));
	DEBUG.println("LIGHT_MAX : " + String(LIGHT_MAX));
	DEBUG.println("LIGHT_MIN : " + String(LIGHT_MIN));
	DEBUG.println("DATE_HAVERST_PHASE : " + String(DATE_HAVERST_PHASE));
	DEBUG.println("SENSOR_UPDATE_INTERVAL : " + String(SENSOR_UPDATE_INTERVAL));
	DEBUG.println("LIBRARY : " + LIBRARY);
	DEBUG.println("\r\n");
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
	digitalWrite(LED_BUILTIN, ON);
	//ulong t = millis();
	//DEBUG.print(("\r\n#1 FREE RAM : "));
	//DEBUG.println(ESP.getFreeHeap());

	String topicStr = topic;

	DEBUG.println(("\r\nMQTT>>>"));
	//DEBUG.println(topicStr);
	DEBUG.print(("Message arrived: "));
	DEBUG.print(topicStr);
	DEBUG.print(("["));
	DEBUG.print(String(length));
	DEBUG.println(("]"));

	mqtt_Message = "";
	for (uint i = 0; i < length; i++) {
		//DEBUG.print((char)payload[i]);
		mqtt_Message += (char)payload[i];
	}

	DEBUG.println(mqtt_Message);

	digitalWrite(LED_BUILTIN, OFF);

	//control pump_mix, light, fan
	if (topicStr == String("Mushroom/Commands/REQUEST/" + HubID))
	{
		handleTopic__Mushroom_Commands_HubID(mqtt_Message);
	}

	else if (topicStr == String("Mushroom/Library/" + HubID)) {
		handleTopic__Mushroom_Library_HubID(mqtt_Message);
	}

	else if (topicStr == "Mushroom/Terminal/" + HubID) {
		if (mqtt_Message == "/restart") {
			mqtt_publish("Mushroom/Terminal/RESPONSE/" + HubID, "Restarting");
			DEBUG.println("\r\nRestart\r\n");
			ESP.restart();
			delay(100);
		}
		if (mqtt_Message.startsWith("/uf")) {
			String url;
			url = mqtt_Message.substring(3);
			url.trim();

			if (!url.startsWith("http")) {
				url = "http://gith.cf/files/MushroomV2.bin";
			}
			mqtt_publish("Mushroom/Terminal/RESPONSE/" + HubID, "Updating new firmware " + url);
			DEBUG.print(("\nUpdating new firmware: "));
			updateFirmware(url);
			DEBUG.println(("DONE!"));
		}
		if (mqtt_Message.startsWith("/v") || mqtt_Message.indexOf("/version") > -1) {
			//StaticJsonBuffer<500> jsBuffer;
			DynamicJsonBuffer jsBuffer(500);
			JsonObject& jsData = jsBuffer.createObject();
			jsData["HUB_ID"] = HubID;
			jsData["FW Version"] = _firmwareVersion;

			String data;
			data.reserve(100);
			jsData.printTo(data);
			mqtt_publish("Mushroom/Terminal/RESPONSE/" + HubID, data);
		}
		else if (mqtt_Message.startsWith("/l") || mqtt_Message.indexOf("/library") > -1) {
			//StaticJsonBuffer<500> jsBuffer;
			DynamicJsonBuffer jsBuffer(500);
			JsonObject& jsLib = jsBuffer.createObject();
			jsLib["TEMP_MAX"] = TEMP_MAX;
			jsLib["TEMP_MIN"] = TEMP_MIN;
			jsLib["HUMI_MAX"] = HUMI_MAX;
			jsLib["HUMI_MIN"] = HUMI_MIN;
			jsLib["LIGHT_MAX"] = LIGHT_MAX;
			jsLib["LIGHT_MIN"] = LIGHT_MIN;

			String libs;
			libs.reserve(100);
			jsLib.printTo(libs);
			mqtt_publish("Mushroom/Terminal/RESPONSE/" + HubID, libs);
		}

		String mqtt_cmd = mqtt_Message;
		mqtt_cmd.toUpperCase();
		extern void control_handle(String cmd);
		control_handle(mqtt_cmd);
	}

	else if (topicStr == "Mushroom/Terminal") {
		StaticJsonBuffer<500> jsonBuffer;
		JsonObject& terminal = jsonBuffer.parseObject(mqtt_Message);
		/*
		Mushroom/Terminal
		{
		   "Command" : "FOTA",
		   "Hub_ID" : "all",
		   "Version" : "",
		   "Url" : ""
		}
		*/
		DEBUG.println(("Update firmware function"));
		String command = terminal["Command"].as<String>();
		if (command == "FOTA") {
			String hub = terminal["Hub_ID"].as<String>();
			if ((hub == HubID) || (hub == "all")) {
				String ver = terminal["Version"].as<String>();
				if (ver != _firmwareVersion) {
					String url = terminal["Url"].as<String>();
					mqtt_publish("Mushroom/Terminal/RESPONSE/" + HubID, "Updating new firmware " + ver);
					DEBUG.print(("\nUpdating new firmware: "));
					DEBUG.println(ver);
					DEBUG.println(url);
					updateFirmware(url);
					DEBUG.println(("DONE!"));
				}
			}
		}
	}

	//DEBUG.print(("#2 FREE RAM : "));
	//DEBUG.println(ESP.getFreeHeap());
	//t = millis() - t;
	//DEBUG.println("Time: " + String(t));
}

void mqtt_reconnect() {  // Loop until we're reconnected
	if (!mqtt_client.connected()) {
		DEBUG.print(("\r\nAttempting MQTT connection..."));
		//boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
		if (mqtt_client.connect(HubID.c_str(), mqtt_user, mqtt_password, ("Mushroom/Status/" + HubID).c_str(), 0, true, String("{\"HUB_ID\":\"" + HubID + "\",\"STATUS\":\"OFFLINE\"}").c_str())) {
			DEBUG.print((" Connected."));
			String h_online = "{\"HUB_ID\":\"" + HubID + "\",\"STATUS\":\"ONLINE\",\"FW_VER\":\"" + _firmwareVersion + "\",\"WIFI\":\"" + WiFi.SSID() + "\",\"SIGNAL\":" + String(wifi_quality()) + "}";
			mqtt_client.publish(("Mushroom/Status/" + HubID).c_str(), h_online.c_str(), true);

			mqtt_client.subscribe(("Mushroom/Library/" + HubID).c_str());
			mqtt_client.subscribe(("Mushroom/Commands/REQUEST/" + HubID).c_str());
			mqtt_client.subscribe(("Mushroom/Sensor/INTERVAL/" + HubID).c_str());
			mqtt_client.subscribe(("Mushroom/Terminal/" + HubID).c_str());
			mqtt_client.subscribe("Mushroom/Terminal");
		}
		else {
			DEBUG.print(("failed, rc="));
			DEBUG.println(String(mqtt_client.state()));
			return;
			//DEBUG.println((" try again"));
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
		return;
	}
	if (!mqtt_client.connected()) {
		mqtt_reconnect();
	}
	mqtt_client.loop();
	yield();
}

bool mqtt_publish(String topic, String payload, bool retain) {
	if (!mqtt_client.connected()) {
		return false;
	}
	digitalWrite(LED_BUILTIN, ON);
	DEBUG.print(("MQTT<<<  "));
	DEBUG.println(topic);
	DEBUG.println(payload);
	DEBUG.println();

	bool ret = mqtt_client.publish(topic.c_str(), payload.c_str(), retain);
	yield();
	digitalWrite(LED_BUILTIN, OFF);
	return ret;
}