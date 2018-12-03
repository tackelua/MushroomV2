﻿float TEMP;
float HUMI;
int LIGHT;
bool WATER_HIGH;
bool WATER_LOW;

bool STT_PUMP_MIX = OFF;
bool STT_PUMP_FLOOR = OFF;
bool STT_FAN_MIX = OFF;
bool STT_FAN_WIND = OFF;
bool STT_LIGHT = OFF;

unsigned long t_pump_mix_change = -1;
unsigned long t_pump_floor_change = -1;
unsigned long t_fan_mix_change = -1;
unsigned long t_fan_wind_change = -1;
unsigned long t_light_change = -1;

bool pin_change = false;
String CMD_ID = "";

void send_message_to_stm32(String cmd);
void stm32_digitalWrite(int pin, bool status);
String make_status_string_to_stm32();
void send_status_to_server();

#pragma region functions
void wifi_init() {
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);
	DEBUG.println();
	WiFi.printDiag(Serial);
	DEBUG.println();

	WiFiManager wifiManager;
	wifiManager.autoConnect(String("MUSHROOM-" + HubID).c_str());
	//WiFi.begin("DTU", "");

	WiFi.waitForConnectResult();
	if (WiFi.localIP() == INADDR_NONE) {
		DEBUG.println("Can not get IP. Restart");
		ESP.restart();
		delay(1000);
	}
	yield();
}

void wifi_loop() {
	if (WiFi.isConnected() == false) {
		static unsigned long t_pre_check_wifi_connected = -5000 - 1;
		if ((t_pre_check_wifi_connected == 0) || ((millis() - t_pre_check_wifi_connected) > 5000)) {
			t_pre_check_wifi_connected = millis();
			DEBUG.println("Connecting wifi...");
			if (WiFi.waitForConnectResult() == WL_CONNECTED)
			{
				DEBUG.println(("connected\n"));
				DEBUG.println(WiFi.localIP());
			}
		}
	}

	if (flag_SmartConfig) {
		if (WiFi.smartConfigDone()) {
			DEBUG.println(("SmartConfig: Success"));
			DEBUG.print(("RSSI: "));
			DEBUG.println(WiFi.RSSI());
			WiFi.printDiag(DEBUG);
			WiFi.stopSmartConfig();
		}
	}
	yield();
}

void libraries_init() {
	SPIFFS.begin();
	File libConfigs;
	libConfigs = SPIFFS.open(file_libConfigs, "r");
	if (libConfigs) {
		String lib;
		while (libConfigs.available()) {
			lib += (char)libConfigs.read();
		}
		handleTopic__Mushroom_Library_HubID(lib, false);
	}
	else {
		DEBUG.println("\r\nLibraries not available");
	}
	libConfigs.close();
}

//extern PubSubClient mqtt_client;
void DEBUG_println(String x) {
	mqtt_publish(tp_Debug, x, false);
}


String http_request(String host, uint16_t port = 80, String url = "/") {
	DEBUG.println("\r\nGET " + host + ":" + String(port) + url);
	WiFiClient client;
	client.setTimeout(100);
	if (!client.connect(host.c_str(), port)) {
		DEBUG.println("connection failed");
		delay(1000);
		return "";
	}
	client.print(String("GET ") + url + " HTTP/1.1\r\n" +
		"Host: " + host + "\r\n" +
		"Connection: close\r\n\r\n");
	unsigned long timeout = millis();
	while (client.available() == 0) {
		if (millis() - timeout > 2000) {
			DEBUG.println(">>> Client Timeout !");
			client.stop();
			return "";
		}
		yield();
	}

	// Read all the lines of the reply from server and print them to Serial
	//while (client.available()) {
	//	String line = client.readStringUntil('\r');
	//	DEBUG.print(line);
	//}
	//DEBUG.println();
	//DEBUG.println();
	String body;
	if (client.available()) {
		body = client.readString();
		int pos_body_begin = body.indexOf("\r\n\r\n");
		if (pos_body_begin > 0) {
			body = body.substring(pos_body_begin + 4);
		}
	}
	client.stop();
	body.trim();
	return body;
}

void updateTimeStamp(unsigned long interval = 0) {
	if (!WiFi.isConnected()) {
		return;
	}
	yield();
	static unsigned long t_pre_update = 0;
	static bool wasSync = false;
	if (interval == 0) {
		{
			DEBUG.println(("Update timestamp"));
			String strTimeStamp;
			strTimeStamp = http_request("gith.cf", 80, "/timestamp");
			int ln = strTimeStamp.indexOf("\r\n");
			if (ln > -1) {
				strTimeStamp = strTimeStamp.substring(ln + 2);
			}
			DEBUG.println(strTimeStamp);
			strTimeStamp.trim();
			long ts = strTimeStamp.toInt();
			if (ts > 1000000000) {
				t_pre_update = millis();
				wasSync = true;
				setTime(ts);
				adjustTime(7 * SECS_PER_HOUR);
				DEBUG.println(("Time Updated\r\n"));
				send_time_to_stm32();
				return;
			}
		}

		String strTimeStamp = http_request("mic.duytan.edu.vn", 88, "/api/GetUnixTime");
		DEBUG.println(strTimeStamp);
		DynamicJsonBuffer timestamp(500);
		JsonObject& jsTimeStamp = timestamp.parseObject(strTimeStamp);
		if (jsTimeStamp.success()) {
			time_t ts = String(jsTimeStamp["UNIX_TIME"].asString()).toInt();
			if (ts > 1000000000) {
				t_pre_update = millis();
				wasSync = true;
				setTime(ts);
				adjustTime(7 * SECS_PER_HOUR);
				DEBUG.println(("Time Updated\r\n"));
				send_time_to_stm32();
				return;
			}
		}
	}
	else {
		if ((millis() - t_pre_update) > interval) {
			updateTimeStamp();
		}
	}
	if (!wasSync) {
		updateTimeStamp();
	}
	yield();
}

void update_sensor(unsigned long interval) {
	static unsigned long t = -1;
	if (interval == 0 || millis() - t >= interval) {
		t = millis();
		
		static unsigned long mes_id = 1;
		StaticJsonBuffer<500> jsBuffer;
		JsonObject& jsData = jsBuffer.createObject();

		jsData["MES_ID"] = "SS-" + String(mes_id++);
		jsData["HUB_ID"] = HubID;
		jsData["TEMP"] = (abs(TEMP) > 200.0 ? -1 : (int)TEMP);
		jsData["HUMI"] = (abs(HUMI) > 200.0 ? -1 : (int)HUMI);
		jsData["LIGHT"] = LIGHT == 1024 ? -1 : LIGHT;

		bool waterEmpty = WATER_LOW;
		jsData["WATER_EMPTY"] = waterEmpty ? "YES" : "NO";

		int dBm = WiFi.RSSI();
		int quality;
		if (dBm <= -100)
			quality = 0;
		else if (dBm >= -50)
			quality = 100;
		else
			quality = 2 * (dBm + 100);
		jsData["RSSI"] = quality;

		jsData["TIMESTAMP"] = now();

		String data;
		data.reserve(120);
		jsData.printTo(data);
		mqtt_publish(("Mushroom/Sensor/" + HubID), data, true);

		//Blynk.virtualWrite(BL_TEMP, (abs(TEMP) > 200.0 ? -1 : (int)TEMP));
		//Blynk.virtualWrite(BL_HUMI, (abs(HUMI) > 200.0 ? -1 : (int)HUMI));
		//Blynk.virtualWrite(BL_LIGHT_SS, LIGHT == 1024 ? -1 : LIGHT);
	}
}


bool create_logs(String relayName, bool status, bool isCommandFromApp) {
	StaticJsonBuffer<500> jsLogBuffer;
	JsonObject& jsLog = jsLogBuffer.createObject();
	jsLog["HUB_ID"] = HubID;
	String content = relayName;
	content += status ? " on" : " off";
	jsLog["Content"] = content;
	jsLog["From"] = isCommandFromApp ? "APP" : "HUB";
	jsLog["Timestamp"] = String(now());

	String jsStrLog;
	jsStrLog.reserve(150);
	jsLog.printTo(jsStrLog);
	bool res = mqtt_publish("Mushroom/Logs/" + HubID, jsStrLog);
	//String strTime = (F("["));
	//strTime += (hour() < 10 ? String("0") + hour() : hour());
	//strTime += (F(":"));
	//strTime += (minute() < 10 ? String("0") + minute() : minute());
	//strTime += (F(":"));
	//strTime += (second() < 10 ? String("0") + second() : second());
	//strTime += (F("] "));
	//Blynk.virtualWrite(V0, strTime + content + " (" + String(isCommandFromApp ? "APP" : "HUB") + ")\r\n");
	//Blynk.notify("HUB " + HubID + " " + content + " (" + String(isCommandFromApp ? "APP" : "HUB") + ")");
	return res;
}

void control(int pin, bool status, bool isCommandFromApp) { //status = true -> ON; false -> OFF
	yield();
	if ((pin == R_PUMP_FLOOR) && (STT_PUMP_FLOOR != status)) {
		t_pump_floor_change = millis();
		STT_PUMP_FLOOR = status;
		pin_change = true;
		DEBUG.print(("PUMP_FLOOR: "));
		DEBUG.println(status ? "ON" : "OFF");
	}
	if ((pin == R_PUMP_MIX) && (STT_PUMP_MIX != status)) {
		t_pump_mix_change = millis();
		STT_PUMP_MIX = status;
		pin_change = true;
		DEBUG.print(("PUMP: "));
		DEBUG.println(status ? "ON" : "OFF");
		if (flag_schedule_pump_floor) {
			pump_floor_on = true;
		}
	}
	if ((pin == R_FAN_MIX) && (STT_FAN_MIX != status)) {
		t_fan_mix_change = millis();
		STT_FAN_MIX = status;
		pin_change = true;
		DEBUG.print(("FAN_MIX: "));
		DEBUG.println(status ? "ON" : "OFF");
	}
	if ((pin == R_FAN_WIND) && (STT_FAN_WIND != status)) {
		t_fan_wind_change = millis();
		STT_FAN_WIND = status;
		pin_change = true;
		DEBUG.print(("FAN_WIND: "));
		DEBUG.println(status ? "ON" : "OFF");
	}
	if ((pin == R_LIGHT) && (STT_LIGHT != status)) {
		t_light_change = millis();
		STT_LIGHT = status;
		pin_change = true;
		DEBUG.print(("LIGHT: "));
		DEBUG.println(status ? "ON" : "OFF");
	}

	flag_isCommandFromApp = isCommandFromApp;
}

void send_status_to_server() {
	yield();
	DEBUG.println(("send_status_to_server"));
	static long mes_id = 0;
	String commandType;
	if (flag_isCommandFromApp) {
		commandType = "RESPONSE";
	}
	else {
		commandType = "NOTIFY";
	}
	StaticJsonBuffer<500> jsBuffer;
	JsonObject& jsStatus = jsBuffer.createObject();
	jsStatus["MES_ID"] = "HW-" + String(++mes_id);
	jsStatus["HUB_ID"] = HubID;
	jsStatus["TIMESTAMP"] = now();
	jsStatus["TYPE"] = commandType;
	jsStatus["MIST"] = STT_PUMP_MIX ? on_ : off_;
	jsStatus["FAN"] = STT_FAN_WIND ? on_ : off_;
	jsStatus["LIGHT"] = STT_LIGHT ? on_ : off_;

	String jsStatusStr;
	jsStatusStr.reserve(150);
	jsStatus.printTo(jsStatusStr);
	mqtt_publish(tp_Response, jsStatusStr, true);

	//Blynk.virtualWrite(BL_PUMP_MIX, STT_PUMP_MIX);
	//Blynk.virtualWrite(BL_FAN, STT_FAN_MIX);
	//Blynk.virtualWrite(BL_LIGHT, STT_LIGHT);
}

String make_status_string_to_stm32() {
	//PUMP1 - PUMP2 - FAN_MIX - LIGHT - WATER_IN
	String s = String(STT_LIGHT) + String(STT_PUMP_MIX) + String(STT_PUMP_FLOOR) + String(STT_FAN_MIX) + String(STT_FAN_WIND);
	return s;
}

void send_message_to_stm32(String cmd) {
	yield();
	//delay(10);
	digitalWrite(LED_BUILTIN, ON);
	STM32.println(cmd);
	DEBUG.println("\r\nSTM32<<<");
	DEBUG.println(cmd);
	//delay(50);
	//DEBUG.println();
	digitalWrite(LED_BUILTIN, OFF);
}

void stm32_digitalWrite(int pin, bool status) {
	//cmd:set-relay-status|ABC123|5|1\r\n
	String s = status ? "0" : "1";
	String c = "cmd:set-relay-status|HUB|" + String(pin) + "|" + s;
	send_message_to_stm32(c);
}

void send_control_message_all_to_stm32() {
	String cmd_set_relays = "cmd:set-relay-status-all|HUB|" + make_status_string_to_stm32();
	send_message_to_stm32(cmd_set_relays);
	pin_change = false;
	mqtt_publish(tp_Debug, cmd_set_relays);
}

void send_time_to_stm32() {
	if (timeStatus() == timeSet) {
		String t = "cmd:set-time|" + String(hour()) + "|" + String(minute()) + "|" + String(second());
		send_message_to_stm32(t);
	}
}


//
//bool skip_auto_light = false;
//bool skip_auto_pump_mix = false;
//bool skip_auto_fan_mix = false;
//bool skip_auto_fan_wind = false;
//
//time_t AUTO_OFF_LIGHT = 60 * 1000 * SECS_PER_MIN; //60 phút
//time_t AUTO_OFF_PUMP_MIX = 6 * 60 * 1000;//180s
//time_t AUTO_OFF_PUMP_FLOOR = 90000;//90s
//time_t AUTO_OFF_FAN_MIX = AUTO_OFF_PUMP_MIX + 5;
//time_t AUTO_OFF_FAN_WIND = 10 * 1000 * SECS_PER_MIN;//10 phút
//
//
//void auto_control() {
//	//https://docs.google.com/document/d/1wSJvCkT_4DIpudjprdOUVIChQpK3V6eW5AJgY0nGKGc/edit
//	//https://prnt.sc/j2oxmu https://snag.gy/6E7xhU.jpg
//
//	//auto trở lại sau khi điều khiển 1 phút
//	unsigned long t_auto_return = 1 * 60000;
//	if (skip_auto_light && (millis() - t_light_change) > t_auto_return) {
//		skip_auto_light = false;
//	}
//	if (skip_auto_pump_mix && (millis() - t_pump_mix_change) > t_auto_return) {
//		skip_auto_pump_mix = false;
//	}
//	if (skip_auto_fan_mix && (millis() - t_fan_mix_change) > t_auto_return) {
//		skip_auto_fan_mix = false;
//	}
//	if (skip_auto_fan_wind && (millis() - t_fan_wind_change) > t_auto_return) {
//		skip_auto_fan_wind = false;
//	}
//	//==============================================================
//
//	//+ LIGHT tự tắt sau 60 phút
//	if ((millis() - t_light_change) > AUTO_OFF_LIGHT) {
//		skip_auto_light = false;
//		if (STT_LIGHT) {
//			DEBUG.println("AUTO LIGHT OFF");
//			control(R_LIGHT, false, false);
//		}
//	}
//
//	bool pump_floor_on = false;
//	//+ PUMP_MIX tự tắt sau 180s sau đó bật PUMP_FLOOR
//	if (hour() >= 11 && hour() < 15) {
//		if ((millis() - t_pump_mix_change) > AUTO_OFF_PUMP_MIX) {
//			skip_auto_pump_mix = false;
//			if (STT_PUMP_MIX) {
//				DEBUG.println("AUTO PUMP_MIX OFF");
//				control(R_PUMP_MIX, false, false);
//
//				if (flag_schedule_pump_floor) {
//					pump_floor_on = true;
//				}
//			}
//		}
//	}
//	else if ((millis() - t_pump_mix_change) > AUTO_OFF_PUMP_MIX) {
//		skip_auto_pump_mix = false;
//		if (STT_PUMP_MIX) {
//			DEBUG.println("AUTO PUMP_MIX OFF");
//			control(R_PUMP_MIX, false, false);
//
//			if (flag_schedule_pump_floor) {
//				pump_floor_on = true;
//			}
//		}
//	}
//
//	if (pump_floor_on) {
//		DEBUG.println("AUTO PUMP_FLOOR OFF");
//		control(R_PUMP_FLOOR, true, false);
//	}
//	//+ PUMP_FLOOR tự tắt sau 90s
//	if ((millis() - t_pump_floor_change) > AUTO_OFF_PUMP_FLOOR) {
//		if (STT_PUMP_FLOOR) {
//			DEBUG.println("AUTO PUMP_FLOOR OFF");
//			control(R_PUMP_FLOOR, false, false);
//		}
//	}
//	//+ FAN_MIX tự tắt sau 185s
//	if (hour() >= 11 && hour() < 15) {
//		if ((millis() - t_fan_mix_change) > AUTO_OFF_FAN_MIX) {
//			skip_auto_fan_mix = false;
//			if (STT_FAN_MIX) {
//				DEBUG.println("AUTO FAN_MIX OFF");
//				control(R_FAN_MIX, false, false);
//			}
//		}
//	}
//	else if ((millis() - t_fan_mix_change) > AUTO_OFF_FAN_MIX) {
//		skip_auto_fan_mix = false;
//		if (STT_FAN_MIX) {
//			DEBUG.println("AUTO FAN_MIX OFF");
//			control(R_FAN_MIX, false, false);
//		}
//	}
//	//+ FAN_WIND tự tắt sau 10 phút
//	if ((millis() - t_fan_wind_change) > AUTO_OFF_FAN_WIND) {
//		skip_auto_fan_wind = false;
//		if (STT_FAN_WIND) {
//			DEBUG.println("AUTO FAN_WIND OFF");
//			control(R_FAN_WIND, false, false);
//		}
//	}
//
//	////+ LCD BACKLIGHT tự tắt sau 2 phút
//	//if (stt_lcd_backlight && (millis() - t_lcd_backlight_change) > (2 * 1000 * SECS_PER_MIN)) {
//	//	DEBUG.println("AUTO LCD BACKLIGHT OFF");
//	//	lcd.noBacklight();
//	//}
//	//==============================================================
//	//1/ Bật tắt đèn
//	if (!skip_auto_light) {
//		if (library && (!STT_LIGHT) && (LIGHT != -1 && LIGHT < LIGHT_MIN) && (hour() >= 6) && (hour() <= 18)) {
//			DEBUG.println("AUTO LIGHT ON");
//			control(R_LIGHT, true, false);
//		}
//		else if (library && (LIGHT != -1 && LIGHT > LIGHT_MAX) && STT_LIGHT) {
//			DEBUG.println("AUTO LIGHT OFF");
//			control(R_LIGHT, false, false);
//		}
//	}
//	//-------------------
//
//	//2. Bật tắt phun sương
//	//a. Phun trực tiếp vào phôi vào lúc 6h và 16h
//	if (((hour() == 6) || (hour() == 16)) && (minute() == 0) && (second() == 0 || second() == 1)) {
//		skip_auto_pump_mix = true;
//		skip_auto_fan_mix = true;
//		DEBUG.println("AUTO FAN_MIX ON");
//		control(R_FAN_MIX, true, false);
//		DEBUG.println("AUTO PUMP_MIX ON");
//		control(R_PUMP_MIX, true, false);
//
//
//		//DEBUG.println("AUTO PUMP_FLOOR ON");
//		//control(R_PUMP_FLOOR, true, true, false);
//		//DEBUG.println("AUTO FAN_WIND ON");
//		//control(R_FAN_WIND, true, true, false);
//	}
//
//	//b. Phun sương làm mát, duy trì độ ẩm. Thời gian bật: 90s, mỗi lần check điều kiện cách nhau 30 phút.
//	if (!skip_auto_pump_mix && library && ((TEMP != -1 && TEMP > TEMP_MAX) && (HUMI != -1 && HUMI < HUMI_MIN)) && ((millis() - t_pump_mix_change) > (30 * 1000 * SECS_PER_MIN))/* && !STT_PUMP_MIX*/) {
//		if (now() > DATE_HAVERST_PHASE) {
//			DEBUG.println("AUTO PUMP_MIX ON");
//			control(R_PUMP_MIX, true, false);
//			DEBUG.println("AUTO FAN_MIX ON");
//			control(R_FAN_MIX, true, false);
//
//			flag_schedule_pump_floor = true;
//		}
//		else {
//			DEBUG.println("AUTO PUMP_FLOOR ON");
//			control(R_PUMP_FLOOR, true, false);
//			DEBUG.println("AUTO FAN_MIX ON");
//			control(R_FAN_MIX, true, false);
//		}
//	}
//	//-------------------
//
//	//c. Bật tắt quạt
//	//Bật quạt FAN_MIX mỗi 15 phút
//	if (!skip_auto_fan_mix && ((millis() - t_fan_mix_change) > (15 * 1000 * SECS_PER_MIN)) && !STT_FAN_MIX) {
//		DEBUG.println("AUTO FAN_MIX ON");
//		control(R_FAN_MIX, true, false);
//	}
//
//	//FAN_WIND bật nếu thỏa điều kiện, mỗi lần check cách nhau 30 phút
//	if (!skip_auto_fan_wind && library && ((HUMI != -1 && HUMI > HUMI_MAX) || (TEMP != -1 && TEMP > TEMP_MAX)) && ((millis() - t_fan_wind_change) > (30 * 1000 * SECS_PER_MIN)) && !STT_FAN_WIND) {
//		DEBUG.println("AUTO FAN_WIND ON");
//		control(R_FAN_WIND, true, false);
//	}
//
//	//TESTCASE 4, mỗi lần check cách nhau 30 phút
//	if (library && ((HUMI != -1 && HUMI < HUMI_MIN) && (TEMP != -1 && TEMP < TEMP_MIN)) && ((millis() - t_pump_mix_change) > (30 * 1000 * SECS_PER_MIN))) {
//		DEBUG.println("AUTO FAN_WIND OFF");
//		control(R_FAN_WIND, false, false);
//
//		DEBUG.println("AUTO PUMP_MIX ON");
//		control(R_PUMP_MIX, true, false);
//
//		DEBUG.println("AUTO FAN_MIX ON");
//		control(R_FAN_MIX, true, false);
//	}
//
//	if (pin_change) {
//		send_control_message_all_to_stm32();
//	}
//	delay(1);
//}


void updateFirmware(String url) {
	ESPhttpUpdate.rebootOnUpdate(true);

	t_httpUpdate_return ret = ESPhttpUpdate.update(url);

	switch (ret) {
	case HTTP_UPDATE_FAILED:
		DEBUG.printf("HTTP_UPDATE_FAILD Error (%d): %s\r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
		break;

	case HTTP_UPDATE_NO_UPDATES:
		DEBUG.println(("HTTP_UPDATE_NO_UPDATES"));
		break;

	case HTTP_UPDATE_OK:
		DEBUG.println(("HTTP_UPDATE_OK"));
		delay(2000);
		ESP.restart();
		break;
	}
}

void control_handle(String _cmd) {
	if (_cmd.startsWith("/")) {
		String cmd = _cmd;
		cmd.toUpperCase();
		if (cmd.indexOf("LIGHT ON") > -1) {
			skip_auto_light = true;
			control(R_LIGHT, true, true);
		}
		if (cmd.indexOf("LIGHT OFF") > -1) {
			skip_auto_light = true;
			control(R_LIGHT, false, true);
		}

		if (cmd.indexOf("MIST ON") > -1) {
			skip_auto_pump_mix = true;
			control(R_PUMP_MIX, true, true);
		}
		if (cmd.indexOf("MIST OFF") > -1) {
			skip_auto_pump_mix = true;
			control(R_PUMP_MIX, false, true);
		}

		if (cmd.indexOf("FAN_MIX ON") > -1) {
			skip_auto_fan_mix = true;
			control(R_FAN_MIX, true, true);
		}
		if (cmd.indexOf("FAN_MIX OFF") > -1) {
			skip_auto_fan_mix = true;
			control(R_FAN_MIX, false, true);
		}
		if (cmd.indexOf("RESET WIFI") > -1) {
			DEBUG.println(("Reset Wifi"));
			WiFi.disconnect();
		}
		handle_Terminal(_cmd);
	}
	yield();
}

float stm32_msg_get_params(String msg, String params) {
	//res:sensor-all|ABC123|t0=30.02|h0=80.89|l0=6244|w1=0|w0=1\r\n
	msg.trim();
	int idx = msg.indexOf(params + "=");
	String data;
	if (idx > 0) {
		data = msg.substring(idx + params.length() + 1); //+1 cho dấu bằng
	}
	return data.toFloat();
}
void control_stm32_message(String msg) {
	yield();
	static bool STM32_PRE_STT_LIGHT = false;
	static bool STM32_PRE_STT_PUMP_MIX = false;
	static bool STM32_PRE_STT_PUMP_FLOOR = false;
	static bool STM32_PRE_STT_FAN_MIX = false;
	static bool STM32_PRE_STT_FAN_WIND = false;
	msg.trim();
	if (msg.startsWith("res:relay-status-all|HUB|")) {
		String STT = msg.substring(String("res:relay-status-all|HUB|").length() - 1); //tính thứ tự relay từ 1
		//LIGHT - PUMP_MIX - PUMP_FLOOR - FAN_MIX - FAN_WIND

		mqtt_publish(tp_Debug, msg);

		if (STT[R_LIGHT] == '1') {
			STT_LIGHT = true;
		}
		else if (STT[R_LIGHT] == '0') {
			STT_LIGHT = false;
		}

		if (STT[R_PUMP_MIX] == '1') {
			STT_PUMP_MIX = true;
		}
		else if (STT[R_PUMP_MIX] == '0') {
			STT_PUMP_MIX = false;
		}

		if (STT[R_PUMP_FLOOR] == '1') {
			STT_PUMP_FLOOR = true;
		}
		else if (STT[R_PUMP_FLOOR] == '0') {
			STT_PUMP_FLOOR = false;
		}

		if (STT[R_FAN_MIX] == '1') {
			STT_FAN_MIX = true;
		}
		else if (STT[R_FAN_MIX] == '0') {
			STT_FAN_MIX = false;
		}

		if (STT[R_FAN_WIND] == '1') {
			STT_FAN_WIND = true;
		}
		else if (STT[R_FAN_WIND] == '0') {
			STT_FAN_WIND = false;
		}
		send_status_to_server();
	}
	else if (msg.startsWith("res:relay-status|HUB|")) {
		String RL = msg.substring(String("res:relay-status|HUB|").length());
		//PUMP_MIX - PUMP_FLOOR - FAN_MIX - LIGHT - WATER_IN
		int relay = RL.toInt();
		String STT = RL.substring(RL.indexOf('|') + 1);
		bool stt = STT[0];
		switch (relay)
		{
		case R_LIGHT:
			STT_LIGHT = stt;
			break;
		case R_PUMP_MIX:
			STT_PUMP_MIX = stt;
			break;
		case R_PUMP_FLOOR:
			STT_PUMP_FLOOR = stt;
			break;
		case R_FAN_MIX:
			STT_FAN_MIX = stt;
			break;
		case R_FAN_WIND:
			STT_FAN_WIND = stt;
			break;
		default:
			break;
		}
		send_status_to_server();
	}
	else if (msg.startsWith("res:sensor-all|HUB|")) {
		//res:sensor-all|HUB|t0=30.02|h0=80.89|l0=6244|w1=0|w0=1\r\n
		float t0 = stm32_msg_get_params(msg, "t0");
		if (t0 != 0) {
			TEMP = t0;
		}

		float h0 = stm32_msg_get_params(msg, "h0");
		if (h0 != 0) {
			HUMI = h0;
		}

		int l0 = stm32_msg_get_params(msg, "l0");
		if (l0 != 0) {
			LIGHT = l0;
		}

		WATER_LOW = int(stm32_msg_get_params(msg, "w0"));

		WATER_HIGH = int(stm32_msg_get_params(msg, "w1"));

		update_sensor(0);
	}
	else if (msg.startsWith("cmd:get-time")) {
		//cmd:get-time\r\n
		send_time_to_stm32();
	}

	if (STT_LIGHT != STM32_PRE_STT_LIGHT) {
		create_logs("Light", STT_LIGHT, flag_isCommandFromApp);
	}
	if (STT_PUMP_MIX != STM32_PRE_STT_PUMP_MIX) {
		create_logs("Pump_Mix", STT_PUMP_MIX, flag_isCommandFromApp);
	}
	if (STT_PUMP_FLOOR != STM32_PRE_STT_PUMP_FLOOR) {
		create_logs("Pump_Floor", STT_PUMP_FLOOR, flag_isCommandFromApp);
	}
	if (STT_FAN_MIX != STM32_PRE_STT_FAN_MIX) {
		create_logs("Fan_Mix", STT_FAN_MIX, flag_isCommandFromApp);
	}
	if (STT_FAN_WIND != STM32_PRE_STT_FAN_WIND) {
		create_logs("Fan_Wind", STT_FAN_WIND, flag_isCommandFromApp);
	}

	STM32_PRE_STT_LIGHT = STT_LIGHT;
	STM32_PRE_STT_PUMP_MIX = STT_PUMP_MIX;
	STM32_PRE_STT_PUMP_FLOOR = STT_PUMP_FLOOR;
	STM32_PRE_STT_FAN_MIX = STT_FAN_MIX;
	STM32_PRE_STT_FAN_WIND = STT_FAN_WIND;
	yield();
}

//void serial_command_handle() {
//	if (DEBUG.available()) {
//		String Scmd = DEBUG.readString();
//		Scmd.trim();
//		DEBUG.println(("\r\n>>>"));
//		DEBUG.println(Scmd);
//		DEBUG.println();
//
//		control_handle(Scmd);
//	}
//	yield();
//}

void stm32_command_handle() {
	if (STM32.available()) {
		digitalWrite(LED_BUILTIN, ON);
		String Scmd = STM32.readString();
		Scmd.trim();
		DEBUG.println(("\r\nSTM32>>>"));
		DEBUG.println(Scmd);
		//DEBUG.println();
		digitalWrite(LED_BUILTIN, OFF);

		control_stm32_message(Scmd);
		control_handle(Scmd);
	}
	yield();
}

void get_data_sensor() {
	yield();
	static unsigned long t = -1;
	if (skip_update_sensor_after_control && millis() - t_skip_update_sensor_after_control < 2000) {
		return;
	}
	else {
		skip_update_sensor_after_control = false;
	}

	if (millis() - t > SENSOR_UPDATE_INTERVAL) {
		t = millis();
		send_message_to_stm32("cmd:get-sensor-all|HUB");

		if (SENSOR_UPDATE_INTERVAL != SENSOR_UPDATE_INTERVAL_DEFAULT) {
			SENSOR_UPDATE_INTERVAL += 1000;
			if (SENSOR_UPDATE_INTERVAL > SENSOR_UPDATE_INTERVAL_DEFAULT) {
				SENSOR_UPDATE_INTERVAL = SENSOR_UPDATE_INTERVAL_DEFAULT;
			}
		}
	}
}
void set_hub_id_to_stm32(String id) {
	send_message_to_stm32("cmd:set-hub-id|" + id);
}

//RESPONSE MQTT
bool response_version() {
	//StaticJsonBuffer<500> jsBuffer;
	DynamicJsonBuffer jsBuffer(500);
	JsonObject& jsData = jsBuffer.createObject();
	jsData["HUB_ID"] = HubID;
	jsData["FW Version"] = _firmwareVersion;

	String data;
	data.reserve(100);
	jsData.printTo(data);
	return mqtt_publish(tp_Terminal_Res, data);
}

bool response_library() {
	//StaticJsonBuffer<500> jsBuffer;
	DynamicJsonBuffer jsBuffer(500);
	JsonObject& jsLib = jsBuffer.createObject();
	jsLib["TEMP_MAX"] = TEMP_MAX;
	jsLib["TEMP_MIN"] = TEMP_MIN;
	jsLib["HUMI_MAX"] = HUMI_MAX;
	jsLib["HUMI_MIN"] = HUMI_MIN;
	jsLib["LIGHT_MAX"] = LIGHT_MAX;
	jsLib["LIGHT_MIN"] = LIGHT_MIN;
	String library = LIBRARY ? "ENABLE" : "DISABLE";;
	//jsLib["DATE_HAVERST_PHASE"] = DATE_HAVERST_PHASE;
	jsLib["SENSOR_UPDATE_INTERVAL"] = SENSOR_UPDATE_INTERVAL;
	jsLib["SENSOR_UPDATE_INTERVAL_DEFAULT"] = SENSOR_UPDATE_INTERVAL_DEFAULT;
	jsLib["LIBRARY"] = library;

	String libs;
	libs.reserve(500);
	jsLib.printTo(libs);
	return mqtt_publish(tp_Terminal_Res, libs);
}
#pragma endregion


