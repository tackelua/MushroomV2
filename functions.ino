﻿float temp;
float humi;
int light;

unsigned long t_pump1_change, t_pump2_change, t_fan_change, t_light_change;
String CMD_ID = "         ";


#pragma region functions
bool ledStt = false;
bool smart_config() {
	Serial.println(("SmartConfig started."));
	unsigned long t = millis();
	WiFi.beginSmartConfig();
	while (1) {
		ledStt = !ledStt;
		hc595_digitalWrite(LED_STATUS, ledStt);
		delay(200);
		if (WiFi.smartConfigDone()) {
			Serial.println(("SmartConfig: Success"));
			Serial.print(("RSSI: "));
			Serial.println(WiFi.RSSI());
			WiFi.printDiag(Serial);
			WiFi.stopSmartConfig();
			break;
		}
		if ((millis() - t) > (5 * 60000)) {
			Serial.println(("ESP restart"));
			ESP.restart();
			return false;
		}
		myBtn.read();
		if (myBtn.wasPressed()) {
			Serial.println("Log out Smart Config");
			WiFi.stopSmartConfig();
			break;
			//WiFi.waitForConnectResult();
			//return false;
		}
	}

	WiFi.reconnect();
	if (WiFi.waitForConnectResult() == WL_CONNECTED)
	{
		Serial.println(("connected\n"));
		Serial.print(("IP: "));
		Serial.println(WiFi.localIP());
		return true;
	}
	else {
		Serial.println(("SmartConfig Fail\n"));
	}
	return false;
}

void wifi_init() {
	//WiFi.disconnect();
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);
	//WiFi.begin("MIC");

	//Serial.println(("SmartConfig started."));
	//WiFi.beginSmartConfig();
	//while (1) {
	//	delay(1000);
	//	if (WiFi.smartConfigDone()) {
	//		Serial.println(("SmartConfig: Success"));
	//		WiFi.printDiag(Serial);
	//		//WiFi.stopSmartConfig();
	//		break;
	//	}
	//}
	DEBUG.println();
	WiFi.printDiag(Serial);
	DEBUG.println();

	//Serial.println(("\nConnecting..."));
	//
	//if (WiFi.waitForConnectResult() == WL_CONNECTED)
	//{
	//	Serial.println(("connected\n"));
	//}
	//else
	//{
	//	Serial.println(("connect again\n"));
	//	if (WiFi.waitForConnectResult() == WL_CONNECTED)
	//	{
	//		Serial.println(("connected\n"));
	//		return;
	//	}
	//
	//	smart_config();
	//}
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
void led_loop() {
	if (flag_SmartConfig) {
		static unsigned long t = millis();
		if ((millis() - t) > 100) {
			t = millis();
			ledStt = !ledStt;
			hc595_digitalWrite(LED_STATUS, ledStt);
		}
	}
	else if (!mqtt_client.connected()) { //loi ket noi den server
		static unsigned long t = millis();
		if ((millis() - t) > 1000) {
			t = millis();
			ledStt = !ledStt;
			hc595_digitalWrite(LED_STATUS, ledStt);
		}
	}
	else if (digitalRead(PININ_WATER_L)) { //water empty 
		static unsigned long t = millis();
		if (ledStt) {
			if ((millis() - t) > 1000) {
				t = millis();
				ledStt = !ledStt;
				hc595_digitalWrite(LED_STATUS, ledStt);
			}
		}
		else {
			if ((millis() - t) > 3000) {
				t = millis();
				ledStt = !ledStt;
				hc595_digitalWrite(LED_STATUS, ledStt);
			}
		}
	}
	else { //normal
		if (!ledStt) {
			hc595_digitalWrite(LED_STATUS, ledStt);
		}
	}
}

String http_request(String host, uint16_t port = 80, String url = "/") {
	Serial.println("\r\nGET " + host + ":" + String(port) + url);
	WiFiClient client;
	client.setTimeout(100);
	if (!client.connect(host.c_str(), port)) {
		Serial.println("connection failed");
		delay(1000);
		return "";
	}
	client.print(String("GET ") + url + " HTTP/1.1\r\n" +
		"Host: " + host + "\r\n" +
		"Connection: close\r\n\r\n");
	unsigned long timeout = millis();
	while (client.available() == 0) {
		if (millis() - timeout > 2000) {
			Serial.println(">>> Client Timeout !");
			client.stop();
			return "";
		}
		delay(1);
	}

	// Read all the lines of the reply from server and print them to Serial
	//while (client.available()) {
	//	String line = client.readStringUntil('\r');
	//	Serial.print(line);
	//}
	//Serial.println();
	//Serial.println();
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
	delay(1);
	static unsigned long t_pre_update = 0;
	static bool wasSync = false;
	if (interval == 0) {
		{
			Serial.println(("Update timestamp"));
			String strTimeStamp = http_request("date.jsontest.com");
			Serial.println(strTimeStamp);
			DynamicJsonBuffer timestamp(500);
			JsonObject& jsTimeStamp = timestamp.parseObject(strTimeStamp);
			if (jsTimeStamp.success()) {
				String tt = jsTimeStamp["milliseconds_since_epoch"].asString();
				tt = tt.substring(0, tt.length() - 3);
				long ts = tt.toInt();
				if (ts > 1000000000) {
					t_pre_update = millis();
					wasSync = true;
					setTime(ts);
					adjustTime(7 * SECS_PER_HOUR);
					Serial.println(("Time Updated\r\n"));
					return;
				}
			}
		}

		String strTimeStamp = http_request("mic.duytan.edu.vn", 88, "/api/GetUnixTime");
		Serial.println(strTimeStamp);
		DynamicJsonBuffer timestamp(500);
		JsonObject& jsTimeStamp = timestamp.parseObject(strTimeStamp);
		if (jsTimeStamp.success()) {
			time_t ts = String(jsTimeStamp["UNIX_TIME"].asString()).toInt();
			if (ts > 1000000000) {
				t_pre_update = millis();
				wasSync = true;
				setTime(ts);
				adjustTime(7 * SECS_PER_HOUR);
				Serial.println(("Time Updated\r\n"));
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
	delay(1);
}
void update_sensor(unsigned long period) {
	//check waterEmpty nếu thay đổi thì update ngay lập tức
	static bool waterEmpty_old = false;
	static bool waterEmpty_new;
	waterEmpty_new = !digitalRead(PININ_WATER_L);
	if (waterEmpty_new != waterEmpty_old) {
		waterEmpty_old = waterEmpty_new;
		update_sensor(0);
	}

	//update sensors data to server every period milli seconds
	static unsigned long preMillis = millis();
	if ((millis() - preMillis) > period) {
		preMillis = millis();
		temp = readTemp();
		//
		//mqtt_loop();
		//serial_command_handle();
		//
		humi = readHumi();
		//
		//mqtt_loop();
		//serial_command_handle();
		//
		light = readLight();
		StaticJsonBuffer<200> jsBuffer;
		JsonObject& jsData = jsBuffer.createObject();
		jsData["HUB_ID"] = HubID;
		jsData["TEMP"] = (abs(temp) > 200.0 ? -1 : (int)temp);
		jsData["HUMI"] = (abs(humi) > 200.0 ? -1 : (int)humi);
		jsData["LIGHT"] = light == 1024 ? -1 : light;

		bool waterEmpty = digitalRead(PININ_WATER_L);
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

void hc595_digitalWrite(int pin, bool status) {
	//Serial.print("pin = ");
	//Serial.println(pin);
	//Serial.print("status = ");
	//Serial.println(status);

	static byte output = 0x00;
	static byte pre_output = -1;

	//8bit: const 0 - PUMP1 - PUMP2 - WATER_IN - FAN - LIGHT - LED_STATUS - const 0
	byte out;
	switch (pin)
	{
	case PUMP1:
	case PUMP2:
	case WATER_IN:
	case FAN:
	case LIGHT:
		out = 0x01 << pin;
		if (status) {
			output |= out;
		}
		else {
			output &= ~out;
		}
		break;
	case LED_STATUS:
		//if (status) {
//  output = output | 0b01000000;
//}
//else {
//  output = output & 0b10111111;
//}
		ledStt = status;
		out = 0x01 << pin;
		if (!status) { //LED hiện tại alway off, on when signal. -> alway on, off when signal
			output |= out;
		}
		else {
			output &= ~out;
		}
		break;
	default:
		break;
	}

	if (output != pre_output) {
		pre_output = output;
		digitalWrite(HC595_STCP, LOW);
		shiftOut(HC595_DATA, HC595_SHCP, MSBFIRST, output);
		//Note 3: MSBFIRST có thể đổi thành LSBFIRST và ngược lại.
		digitalWrite(HC595_STCP, HIGH);
	}
	//Serial.print("OUT = ");
	//Serial.println(output, BIN);
	//Serial.println();
}

//String make_status_string_to_stm32() {
//	//PUMP1 - PUMP2 - FAN - LIGHT - WATER_IN
//	String s = String(STT_PUMP1) + String(STT_PUMP2) + String(STT_FAN) + String(STT_LIGHT) + String(STT_WATER_IN);
//	return s;
//}
//void stm32_digitalWrite(int pin, bool status) {
//	//Serial.print("pin = ");
//	//Serial.println(pin);
//	//Serial.print("status = ");
//	//Serial.println(status);
//	String c = "set-relay-status-all|HUB|" + make_status_string_to_stm32();
//	STM32.println(c);
//}

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
		hc595_digitalWrite(pin, status ? ON : OFF);
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
		hc595_digitalWrite(pin, status ? ON : OFF);
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
	delay(1);
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

void control_handle(String cmd) {
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
//void control_stm32_message(String msg) {
//	msg.trim();
//	msg.toLowerCase();
//	if (msg.startsWith("relay-status-all|HUB|")) {
//		String STT = msg.substring(String("relay-status-all|HUB|").length());
//		//PUMP1 - PUMP2 - FAN - LIGHT - WATER_IN
//		STT_PUMP1 = (STT[STM32_RELAY::PUMP1] == '1' ? true : false);
//		STT_PUMP2 = (STT[STM32_RELAY::PUMP2] == '1' ? true : false);
//		STT_FAN = (STT[STM32_RELAY::FAN] == '1' ? true : false);
//		STT_LIGHT = (STT[STM32_RELAY::LIGHT] == '1' ? true : false);
//		STT_WATER_IN = (STT[STM32_RELAY::WATER_IN] == '1' ? true : false);
//	}
//	else if (msg.startsWith("relay-status|HUB|")) {
//		String RL = msg.substring(String("relay-status|HUB|").length());
//		//PUMP1 - PUMP2 - FAN - LIGHT - WATER_IN
//		int relay = RL.toInt();
//		String STT = RL.substring(RL.indexOf('|') + 1);
//		char stt = STT[0];
//		switch (relay)
//		{
//		case PUMP1:
//			STT_PUMP1 = stt;
//		case PUMP2:
//			STT_PUMP2 = stt;
//		case FAN:
//			STT_FAN = stt;
//		case LIGHT:
//			STT_LIGHT = stt;
//		case WATER_IN:
//			STT_WATER_IN = stt;
//		default:
//			break;
//		}
//
//		
//	}
//}

void serial_command_handle() {
	if (Serial.available()) {
		String Scmd = Serial.readString();
		Scmd.trim();
		DEBUG.println(("\r\n>>>"));
		DEBUG.println(Scmd);

		control_handle(Scmd);
	}

	delay(1);
}

//void stm32_command_handle() {
//	if (STM32.available()) {
//		String Scmd = Serial.readString();
//		Scmd.trim();
//		DEBUG.println(("\r\n>>>"));
//		DEBUG.println(Scmd);
//
//		control_stm32_message(Scmd);
//	}
//
//	delay(1);
//}

#pragma endregion


