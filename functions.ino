﻿float temp;
float humi;
int light;
bool water_high;
bool water_low;

unsigned long t_pump1_change, t_pump2_change, t_fan_change, t_light_change;
String CMD_ID = "         ";

#pragma region functions
bool ledStt = false;
bool smart_config() {
	DEBUG.println(("SmartConfig started."));
	unsigned long t = millis();
	WiFi.beginSmartConfig();
	while (1) {
		ledStt = !ledStt;
		digitalWrite(LED_BUILTIN, ledStt);
		delay(200);
		if (WiFi.smartConfigDone()) {
			DEBUG.println(("SmartConfig: Success"));
			DEBUG.print(("RSSI: "));
			DEBUG.println(WiFi.RSSI());
			WiFi.printDiag(Serial);
			WiFi.stopSmartConfig();
			break;
		}
		if ((millis() - t) > (5 * 60000)) {
			DEBUG.println(("ESP restart"));
			ESP.restart();
			return false;
		}
	}

	WiFi.reconnect();
	if (WiFi.waitForConnectResult() == WL_CONNECTED)
	{
		DEBUG.println(("connected\n"));
		DEBUG.print(("IP: "));
		DEBUG.println(WiFi.localIP());
		return true;
	}
	else {
		DEBUG.println(("SmartConfig Fail\n"));
	}
	return false;
}

void wifi_init() {
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);
	WiFi.begin("MIC", "");
	DEBUG.println();
	WiFi.printDiag(Serial);
	DEBUG.println();
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
}

extern PubSubClient mqtt_client;

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

		StaticJsonBuffer<200> jsBuffer;
		JsonObject& jsData = jsBuffer.createObject();
		jsData["HUB_ID"] = HubID;
		jsData["TEMP"] = (abs(temp) > 200.0 ? -1 : (int)temp);
		jsData["HUMI"] = (abs(humi) > 200.0 ? -1 : (int)humi);
		jsData["LIGHT"] = light == 1024 ? -1 : light;

		bool waterEmpty = water_low;
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

		String data;
		data.reserve(120);
		jsData.printTo(data);
		mqtt_publish(("Mushroom/Sensor/" + HubID), data, true);

		Blynk.virtualWrite(BL_TEMP, (abs(temp) > 200.0 ? -1 : (int)temp));
		Blynk.virtualWrite(BL_HUMI, (abs(humi) > 200.0 ? -1 : (int)humi));
		Blynk.virtualWrite(BL_LIGHT_SS, light == 1024 ? -1 : light);
	}
}

String make_status_string_to_stm32() {
	//PUMP1 - PUMP2 - FAN - LIGHT - WATER_IN
	String s = String(STT_PUMP1) + String(STT_PUMP2) + String(STT_FAN) + String(STT_LIGHT);
	return s;
}
void stm32_digitalWrite(int pin, bool status) {
	//DEBUG.print("pin = ");
	//DEBUG.println(pin);
	//DEBUG.print("status = ");
	//DEBUG.println(status);
	String c = "cmd:set-relay-status-all|HUB|" + make_status_string_to_stm32();
	send_message_to_stm32(c);
}

bool create_logs(String relayName, bool status, bool isCommandFromApp) {
	StaticJsonBuffer<200> jsLogBuffer;
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
	String strTime = (F("["));
	strTime += (hour() < 10 ? String("0") + hour() : hour());
	strTime += (F(":"));
	strTime += (minute() < 10 ? String("0") + minute() : minute());
	strTime += (F(":"));
	strTime += (second() < 10 ? String("0") + second() : second());
	strTime += (F("] "));
	Blynk.virtualWrite(V0, strTime + content + " (" + String(isCommandFromApp ? "APP" : "HUB") + ")\r\n");
	Blynk.notify("HUB " + HubID + " " + content + " (" + String(isCommandFromApp ? "APP" : "HUB") + ")");
	return res;
}

void send_status_to_server();
bool control(int pin, bool status, bool update_to_server, bool isCommandFromApp);
bool control(int pin, bool status, bool update_to_server, bool isCommandFromApp) { //status = true -> ON; false -> OFF
	bool pin_change = false;

	if ((pin == PUMP2) && (STT_PUMP2 != status)) {
		t_pump2_change = millis();
		STT_PUMP2 = status;
		create_logs("Pump2", status, isCommandFromApp);
		DEBUG.print(("PUMP2: "));
		DEBUG.println(status ? "ON" : "OFF");
		stm32_digitalWrite(pin, status ? ON : OFF);
		return true;
	}
	if ((pin == PUMP1) && (STT_PUMP1 != status)) {
		t_pump1_change = millis();
		STT_PUMP1 = status;
		pin_change = true;
		create_logs("Mist", status, isCommandFromApp);
		DEBUG.print(("PUMP: "));
		DEBUG.println(status ? "ON" : "OFF");
	}
	if ((pin == FAN) && (STT_FAN != status)) {
		t_fan_change = millis();
		STT_FAN = status;
		pin_change = true;
		create_logs("Fan", status, isCommandFromApp);
		DEBUG.print(("FAN: "));
		DEBUG.println(status ? "ON" : "OFF");
	}
	if ((pin == LIGHT) && (STT_LIGHT != status)) {
		t_light_change = millis();
		STT_LIGHT = status;
		pin_change = true;
		create_logs("Light", status, isCommandFromApp);
		DEBUG.print(("LIGHT: "));
		DEBUG.println(status ? "ON" : "OFF");
	}

	if (pin_change) {
		stm32_digitalWrite(pin, status ? ON : OFF);
		if (update_to_server) {
			send_status_to_server();
		}
	}
	return pin_change;
}

void send_status_to_server() {
	DEBUG.println(("send_status_to_server"));

	StaticJsonBuffer<200> jsBuffer;
	JsonObject& jsStatus = jsBuffer.createObject();
	jsStatus["HUB_ID"] = HubID;
	jsStatus["MIST"] = STT_PUMP1 ? on_ : off_;
	jsStatus["FAN"] = STT_FAN ? on_ : off_;
	jsStatus["LIGHT"] = STT_LIGHT ? on_ : off_;
	jsStatus["CMD_ID"] = "HW-" + String(now());;

	String jsStatusStr;
	jsStatusStr.reserve(150);
	jsStatus.printTo(jsStatusStr);
	mqtt_publish("Mushroom/Commands/" + HubID, jsStatusStr, true);

	Blynk.virtualWrite(BL_PUMP1, STT_PUMP1);
	Blynk.virtualWrite(BL_FAN, STT_FAN);
	Blynk.virtualWrite(BL_LIGHT, STT_LIGHT);
}

void send_message_to_stm32(String cmd) {
	delay(10);
	digitalWrite(LED_BUILTIN, ON);
	DEBUG.println("\r\nSTM32<<<");
	STM32.println(cmd);
	delay(50);
	//DEBUG.println();
	digitalWrite(LED_BUILTIN, OFF);
}

void send_time_to_stm32() {
	if (timeStatus() == timeSet) {
		String t = "cmd:set-time|" + String(hour()) + "|" + String(minute()) + "|" + String(second());
		send_message_to_stm32(t);
	}
}

bool skip_auto_light = false;
bool skip_auto_pump1 = false;
bool skip_auto_fan = false;
void auto_control() {
	//https://docs.google.com/document/d/1wSJvCkT_4DIpudjprdOUVIChQpK3V6eW5AJgY0nGKGc/edit
	//https://prnt.sc/j2oxmu https://snag.gy/6E7xhU.jpg

	//+ PUMP1 tự tắt sau 1.5 phút
	if ((millis() - t_pump1_change) > (20000)) {
		skip_auto_pump1 = false;
		if (STT_PUMP1) {
			DEBUG.println("AUTO PUMP1 OFF");
			control(PUMP1, false, true, false);
		}
	}
	if ((millis() - t_pump2_change) > 20000) {
		if (STT_PUMP2) {
			DEBUG.println("AUTO PUMP2 OFF");
			control(PUMP2, false, true, false);
		}
	}
	//+ FAN tự tắt sau 20 phút
	if ((millis() - t_fan_change) > (20 * SECS_PER_MIN)) {
		skip_auto_fan = false;
		if (STT_FAN) {
			DEBUG.println("AUTO FAN OFF");
			control(FAN, false, true, false);
		}
	}
	//+ LIGHT tự tắt sau 3 tiếng
	if ((millis() - t_light_change) > (1 * 1000 * SECS_PER_HOUR)) {
		skip_auto_light = false;
		if (STT_LIGHT) {
			DEBUG.println("AUTO LIGHT OFF");
			control(LIGHT, false, true, false);
		}
	}
	//==============================================================
	//1/ Bật tắt đèn
	if (!skip_auto_light) {
		if (library && (light < LIGHT_MIN) && (hour() >= 6) && (hour() <= 18)) {
			DEBUG.println("AUTO LIGHT ON");
			control(LIGHT, true, true, false);
		}
		else if (library && (light > LIGHT_MAX)) {
			DEBUG.println("AUTO LIGHT OFF");
			control(LIGHT, false, true, false);
		}
	}
	//-------------------

	//2. Bật tắt phun sương
	//a. Phun trực tiếp vào phôi vào lúc 6h và 16h
	if (((hour() == 6) || (hour() == 16)) && (minute() == 0) && (second() == 0)) {
		skip_auto_pump1 = true;
		skip_auto_fan = true;
		DEBUG.println("AUTO PUMP1 ON");
		DEBUG.println("AUTO PUMP2 ON");
		control(PUMP2, true, true, false);
		control(PUMP1, true, true, false);
		DEBUG.println("AUTO FAN ON");
		control(FAN, true, true, false);
	}

	//b. Phun sương làm mát, duy trì độ ẩm. Thời gian bật: 3 phút, mỗi lần bật cách nhau 1 giờ.
	if (!skip_auto_pump1 && library && ((int(temp) > TEMP_MAX) || (int(humi) < HUMI_MIN)) && ((millis() - t_pump1_change) > 3600000) && !STT_PUMP1) {
		DEBUG.println("AUTO PUMP1 ON");
		control(PUMP1, true, true, false);
		DEBUG.println("AUTO FAN ON");
		control(FAN, true, true, false);
	}
	//-------------------

	//c. Bật tắt quạt
	if (!skip_auto_fan && library && (((int)humi > HUMI_MAX) || ((int)temp > TEMP_MAX)) && ((millis() - t_fan_change) > 3600000) && !STT_FAN) {
		DEBUG.println("AUTO FAN ON");
		control(FAN, true, true, false);
	}
	yield();
}

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
			control(LIGHT, true, true, true);
		}
		if (cmd.indexOf("LIGHT OFF") > -1) {
			skip_auto_light = true;
			control(LIGHT, false, true, true);
		}

		if (cmd.indexOf("MIST ON") > -1) {
			skip_auto_pump1 = true;
			control(PUMP1, true, true, true);
		}
		if (cmd.indexOf("MIST OFF") > -1) {
			skip_auto_pump1 = true;
			control(PUMP1, false, true, true);
		}

		if (cmd.indexOf("FAN ON") > -1) {
			skip_auto_fan = true;
			control(FAN, true, true, true);
		}
		if (cmd.indexOf("FAN OFF") > -1) {
			skip_auto_fan = true;
			control(FAN, false, true, true);
		}
		if (cmd.indexOf("RESET WIFI") > -1) {
			DEBUG.println(("Reset Wifi"));
			WiFi.disconnect();
		}
	}
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
	msg.trim();
	if (msg.startsWith("res:relay-status-all|HUB|")) {
		String STT = msg.substring(String("res:relay-status-all|HUB|").length());
		//PUMP1 - PUMP2 - FAN - LIGHT - WATER_IN
		if (STT[STM32_RELAY::PUMP1] == '1') STT_PUMP1 = true;
		else if (STT[STM32_RELAY::PUMP1] == '0') STT_PUMP1 = false;

		if (STT[STM32_RELAY::PUMP2] == '1') STT_PUMP2 = true;
		else if (STT[STM32_RELAY::PUMP2] == '0') STT_PUMP2 = false;

		if (STT[STM32_RELAY::FAN] == '1') STT_FAN = true;
		else if (STT[STM32_RELAY::FAN] == '0') STT_FAN = false;

		if (STT[STM32_RELAY::LIGHT] == '1') STT_LIGHT = true;
		else if (STT[STM32_RELAY::LIGHT] == '0') STT_LIGHT = false;
	}
	else if (msg.startsWith("res:relay-status|HUB|")) {
		String RL = msg.substring(String("res:relay-status|HUB|").length());
		//PUMP1 - PUMP2 - FAN - LIGHT - WATER_IN
		int relay = RL.toInt();
		String STT = RL.substring(RL.indexOf('|') + 1);
		char stt = STT[0];
		switch (relay)
		{
		case PUMP1:
			STT_PUMP1 = stt;
			break;
		case PUMP2:
			STT_PUMP2 = stt;
			break;
		case FAN:
			STT_FAN = stt;
			break;
		case LIGHT:
			STT_LIGHT = stt;
			break;
		default:
			break;
		}
	}
	else if (msg.startsWith("res:sensor-all|HUB|")) {
		//res:sensor-all|HUB|t0=30.02|h0=80.89|l0=6244|w1=0|w0=1\r\n
		float t0 = stm32_msg_get_params(msg, "t0");
		if (t0 != 0) {
			temp = t0;
		}

		float h0 = stm32_msg_get_params(msg, "h0");
		if (h0 != 0) {
			humi = h0;
		}

		int l0 = stm32_msg_get_params(msg, "l0");
		if (l0 != 0) {
			light = l0;
		}

		bool w0 = int(stm32_msg_get_params(msg, "w0"));
		if (w0 != 0) {
			water_low = w0;
		}

		bool w1 = int(stm32_msg_get_params(msg, "w1"));
		if (w1 != 0) {
			water_high = w1;
		}

		update_sensor(0);
	}
	else if (msg.startsWith("cmd:get-time")) {
		//cmd:get-time\r\n
		send_time_to_stm32();
	}
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
		STM32.println(("\r\nSTM32>>>"));
		STM32.println(Scmd);
		//DEBUG.println();
		digitalWrite(LED_BUILTIN, OFF);

		control_stm32_message(Scmd);
		control_handle(Scmd);
	}
	yield();
}

void get_data_sensor(unsigned long interval) {
	static unsigned long t = -1;
	if (millis() - t > interval) {
		t = millis();
		send_message_to_stm32("cmd:get-sensor-all|HUB");
	}

}
void set_hub_id_to_stm32(String id) {
	send_message_to_stm32("cmd:set-hub-id|" + id);
}
#pragma endregion


