// 
// 
// 

#include "mqtt_helper.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "hardware.h"
#include <FS.h>

extern String HubID;
extern String _firmwareVersion;
extern String _hardwareVersion;
extern void updateFirmware(String url);
extern String CMD_ID;
extern bool flag_schedule_pump_floor;

extern bool skip_auto_pump_mix;
extern bool skip_auto_light;
extern bool skip_auto_fan_mix;
extern bool skip_auto_fan_wind;

extern time_t AUTO_OFF_LIGHT;
extern time_t AUTO_OFF_PUMP_MIX;
extern time_t AUTO_OFF_PUMP_FLOOR;
extern time_t AUTO_OFF_FAN_MIX;
extern time_t AUTO_OFF_FAN_WIND;

const char* file_libConfigs = "/configs.json";

const char* mqtt_server = "mic.duytan.edu.vn";
const char* mqtt_user = "Mic@DTU2017";
const char* mqtt_password = "Mic@DTU2017!@#";
const uint16_t mqtt_port = 1883;

String tp_Status;
String tp_Library;
String tp_Request;
String tp_Response;
String tp_Debug;
String tp_Terminal;
String tp_Terminal_Hub;
String tp_Terminal_Res;
String tp_Interval;
String tp_Light;
String tp_Pump_Mix;
String tp_Pump_Floor;
String tp_Fan_Mix;
String tp_Fan_Wind;

const String on_ = "on";
const String off_ = "off";

int TEMP_MAX, TEMP_MIN, HUMI_MAX, HUMI_MIN, LIGHT_MAX, LIGHT_MIN;
//long DATE_HAVERST_PHASE;
long SENSOR_UPDATE_INTERVAL_DEFAULT = 30000;
bool LIBRARY = true;

volatile long SENSOR_UPDATE_INTERVAL = SENSOR_UPDATE_INTERVAL_DEFAULT;
bool skip_update_sensor_after_control = false;
time_t t_skip_update_sensor_after_control;

String mqtt_Message;

WiFiClient mqtt_espClient;
PubSubClient mqtt_client(mqtt_espClient);

extern String timeStr;
extern bool STT_PUMP_MIX, STT_FAN_MIX, STT_LIGHT;
extern bool pin_change;
extern void control(int pin, bool status, bool isCommandFromApp);
extern void stm32_digitalWrite(int pin, bool status);
extern String make_status_string_to_stm32();
extern void send_message_to_stm32(String cmd);
extern void send_control_message_all_to_stm32();
extern void send_status_to_server();
extern void set_hub_id_to_stm32(String id);

extern bool response_version();
extern bool response_library();

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
		control(R_PUMP_MIX, true, isCommandFromApp);

		//control(R_PUMP_FLOOR, true, isCommandFromApp);
		flag_schedule_pump_floor = true;
	}
	else if (pump_mix_stt == off_)
	{
		skip_auto_pump_mix = true;
		control(R_PUMP_MIX, false, isCommandFromApp);
		control(R_PUMP_FLOOR, false, isCommandFromApp);
	}

	String light_stt = commands["LIGHT"].as<String>();
	if (light_stt == on_)
	{
		skip_auto_light = true;
		control(R_LIGHT, true, isCommandFromApp);
	}
	else if (light_stt == off_)
	{
		skip_auto_light = true;
		control(R_LIGHT, false, isCommandFromApp);
	}

	String fan_stt = commands["FAN"].as<String>();
	if (fan_stt == on_)
	{
		skip_auto_fan_mix = true;
		skip_auto_fan_wind = true;
		control(R_FAN_MIX, true, isCommandFromApp);
		control(R_FAN_WIND, true, isCommandFromApp);
	}
	else if (fan_stt == off_)
	{
		skip_auto_fan_mix = true;
		skip_auto_fan_wind = true;
		control(R_FAN_MIX, false, isCommandFromApp);
		control(R_FAN_WIND, false, isCommandFromApp);
	}

	SENSOR_UPDATE_INTERVAL = 2000;
	skip_update_sensor_after_control = true;
	t_skip_update_sensor_after_control = millis();
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
	//DATE_HAVERST_PHASE = lib["DATE_HAVERST_PHASE"].as<long>();

	long interval = lib["SENSOR_UPDATE_INTERVAL"].as<long>();
	if (interval > 0) {
		SENSOR_UPDATE_INTERVAL_DEFAULT = interval;
	}
	
	String library = lib["LIBRARY"].asString();
	if (library == "ENABLE") {
		LIBRARY = true;
	}
	else if (library == "DISABLE") {
		LIBRARY = false;
	}

	response_library();
	//DEBUG.println("TEMP_MAX : " + String(TEMP_MAX));
	//DEBUG.println("TEMP_MIN : " + String(TEMP_MIN));
	//DEBUG.println("HUMI_MAX : " + String(HUMI_MAX));
	//DEBUG.println("HUMI_MIN : " + String(HUMI_MIN));
	//DEBUG.println("LIGHT_MAX : " + String(LIGHT_MAX));
	//DEBUG.println("LIGHT_MIN : " + String(LIGHT_MIN));
	//DEBUG.println("DATE_HAVERST_PHASE : " + String(DATE_HAVERST_PHASE));
	//DEBUG.println("SENSOR_UPDATE_INTERVAL : " + String(SENSOR_UPDATE_INTERVAL_DEFAULT));
	//DEBUG.println("LIBRARY : " + LIBRARY);
	//DEBUG.println("\r\n");
}

void handle_Terminal(String msg) {
	yield();
	if (msg == "/restart") {
		mqtt_publish(tp_Terminal_Res, "Restarting");
		DEBUG.println("\r\nRestart\r\n");
		ESP.restart();
		delay(100);
	}
	if (msg.startsWith("/uf")) {
		String url;
		url = msg.substring(3);
		url.trim();

		if (!url.startsWith("http")) {
			url = "http://gith.cf/files/MushroomV2.bin";
		}
		mqtt_publish(tp_Terminal_Res, "Updating new firmware " + url);
		DEBUG.print(("\nUpdating new firmware: "));
		updateFirmware(url);
		DEBUG.println(("DONE!"));
	}
	if (msg.startsWith("/v") || msg.indexOf("/version") > -1) {
		response_version();
	}
	else if (msg.startsWith("/l") || msg.indexOf("/library") > -1) {
		response_library();
	}
	else if (msg.startsWith("/set id")) {
		String id = msg.substring(7);
		id.trim();
		String _hubID = HubID;
		extern String setID(String);
		setID(id);
		set_hub_id_to_stm32(id);
		mqtt_publish("Mushroom/Terminal/RESPONSE/" + _hubID, "Set ID " + _hubID + " to " + id);
	}
	else if (msg.startsWith("/reset id")) {
		extern String setID();
		String _hubID = HubID;
		String id = setID();
		set_hub_id_to_stm32(id);
		mqtt_publish("Mushroom/Terminal/RESPONSE/" + _hubID, "Reset ID " + id);
	}
	else if (msg.startsWith("/id")) {
		mqtt_publish(tp_Terminal_Res, HubID);
	}
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

String make_json_data__mqtt_publish_when_connected() {
	DynamicJsonBuffer jsBuffer(500);
	JsonObject& jsData = jsBuffer.createObject();
	jsData["HUB_ID"] = HubID;
	jsData["STATUS"] = "ONLINE";
	jsData["FW_VER"] = _firmwareVersion;
	jsData["HW_VER"] = _hardwareVersion;
	jsData["WIFI"] = WiFi.SSID();
	jsData["SIGNAL"] = String(wifi_quality());

	String buf;
	jsData.printTo(buf);
	return buf;
}

bool mqtt_publish(String topic, String payload, bool retain) {
	yield();
	if (!mqtt_client.connected()) {
		return false;
	}
	digitalWrite(LED_BUILTIN, ON);
	DEBUG.print(("MQTT<<<  "));
	DEBUG.println(topic);
	DEBUG.println(payload);

	bool ret = false;
	int retries = 0;
	while (!ret) {
		ret = mqtt_client.publish(topic.c_str(), payload.c_str(), retain);
		yield();
		digitalWrite(LED_BUILTIN, OFF);
		if (ret) {
			DEBUG.println("--\r\nOK");
		}
		else {
			DEBUG.println("--\r\nFAIL");
			delay(200);
			if (retries++ > 5) break;
		}
	}
	DEBUG.println();
	return ret;
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

	//control pump_mix, LIGHT, fan
	if (topicStr == String(tp_Request))
	{
		handleTopic__Mushroom_Commands_HubID(mqtt_Message);
	}

	else if (topicStr == String(tp_Library)) {
		handleTopic__Mushroom_Library_HubID(mqtt_Message);
	}

	else if (topicStr == tp_Terminal_Hub) {
		handle_Terminal(mqtt_Message);


		String mqtt_cmd = mqtt_Message;
		mqtt_cmd.toUpperCase();
		extern void control_handle(String cmd);
		control_handle(mqtt_cmd);
	}

	else if (topicStr == tp_Terminal) {
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
					mqtt_publish(tp_Terminal_Res, "Updating new firmware " + ver);
					DEBUG.print(("\nUpdating new firmware: "));
					DEBUG.println(ver);
					DEBUG.println(url);
					updateFirmware(url);
					DEBUG.println(("DONE!"));
				}
			}
		}
	}

	else if (topicStr == tp_Status) {
		StaticJsonBuffer<500> jsonBuffer;
		JsonObject& status = jsonBuffer.parseObject(mqtt_Message);
		String stt = status["STATUS"].as<String>();
		if (stt == "OFFLINE") {
			String h_online = make_json_data__mqtt_publish_when_connected();
			mqtt_publish((tp_Status).c_str(), h_online.c_str(), true);
		}
	}

	else if (topicStr == tp_Light) {
		AUTO_OFF_LIGHT = mqtt_Message.toInt();
	}
	else if (topicStr == tp_Pump_Mix) {
		AUTO_OFF_PUMP_MIX = mqtt_Message.toInt();
	}
	else if (topicStr == tp_Pump_Floor) {
		AUTO_OFF_PUMP_FLOOR = mqtt_Message.toInt();
	}
	else if (topicStr == tp_Fan_Mix) {
		AUTO_OFF_PUMP_MIX = mqtt_Message.toInt();
	}
	else if (topicStr == tp_Fan_Wind) {
		AUTO_OFF_FAN_WIND = mqtt_Message.toInt();
	}

	//DEBUG.print(("#2 FREE RAM : "));
	//DEBUG.println(ESP.getFreeHeap());
	//t = millis() - t;
	//DEBUG.println("Time: " + String(t));
}


void mqtt_reconnect() {  // Loop until we're reconnected
	if (!mqtt_client.connected()) {
		DEBUG.println(("\r\nAttempting MQTT connection..."));
		//boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
		unsigned long seed = now() + analogRead(A0);
		randomSeed(seed);
		String client_id = "MUSHROOM-" + HubID + "_" + now() + random(0xffffff);
		DEBUG.println((client_id));
		if (mqtt_client.connect(client_id.c_str(), mqtt_user, mqtt_password, (tp_Status).c_str(), 0, true, String("{\"HUB_ID\":\"" + HubID + "\",\"STATUS\":\"OFFLINE\"}").c_str())) {
			DEBUG.println(("connected."));

			String h_online = make_json_data__mqtt_publish_when_connected();
			mqtt_publish((tp_Status).c_str(), h_online.c_str(), true);

			mqtt_client.subscribe((tp_Status).c_str());
			mqtt_client.subscribe((tp_Library).c_str());
			mqtt_client.subscribe((tp_Request).c_str());
			mqtt_client.subscribe((tp_Interval).c_str());
			mqtt_client.subscribe((tp_Terminal_Hub).c_str());
			mqtt_client.subscribe(tp_Terminal.c_str());
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

void mqtt_init_topics() {
	tp_Status = "Mushroom/Status/" + HubID;
	tp_Library = "Mushroom/Library/" + HubID;
	tp_Request = "Mushroom/Commands/REQUEST/" + HubID;
	tp_Response = "Mushroom/Commands/RESPONSE/" + HubID;
	tp_Debug = "Mushroom/Debug/" + HubID;
	tp_Terminal = "Mushroom/Terminal";
	tp_Terminal_Hub = "Mushroom/Terminal/" + HubID;
	tp_Terminal_Res = "Mushroom/Terminal/RESPONSE/" + HubID;
	tp_Interval = "Mushroom/Sensor/INTERVAL/" + HubID;
	tp_Light = "Mushroom/<HubID>/AUTO_OFF/LIGHT";
	tp_Pump_Mix = "Mushroom/<HubID>/AUTO_OFF/PUMP_MIX";
	tp_Pump_Floor = "Mushroom/<HubID>/AUTO_OFF/PUMP_FLOOR";
	tp_Fan_Mix = "Mushroom/<HubID>/AUTO_OFF/FAN_MIX";
	tp_Fan_Wind = "Mushroom/<HubID>/AUTO_OFF/FAN_WIND";
	yield();
}
void mqtt_init() {
	//http.setReuse(true);
	mqtt_init_topics();

	mqtt_Message.reserve(MQTT_MAX_PACKET_SIZE); //tao buffer khoang trong cho mqtt_Message
	mqtt_client.setServer(mqtt_server, mqtt_port);
	mqtt_client.setCallback(mqtt_callback);
	yield();
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