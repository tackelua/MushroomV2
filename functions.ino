float temp;
float humi;
int light;

bool stt_pump1 = true;
bool stt_fan = true;
bool stt_light = true;
unsigned long t_pump1_change, t_fan_change, t_light_change;
String CMD_ID = "         ";


#pragma region functions
bool smart_config() {
	bool sttled = false;
	Serial.println(F("SmartConfig started."));
	unsigned long t = millis();
	WiFi.beginSmartConfig();
	while (1) {
		sttled = !sttled;
		hc595_digitalWrite(LED_STATUS, sttled);
		delay(200);
		if (WiFi.smartConfigDone()) {
			Serial.println(F("SmartConfig: Success"));
			Serial.print(F("RSSI: "));
			Serial.println(WiFi.RSSI());
			WiFi.printDiag(Serial);
			WiFi.stopSmartConfig();
			break;
		}
		if ((millis() - t) > (3 * 60000)) {
			Serial.println(F("ESP restart"));
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
		Serial.println(F("connected\n"));
		Serial.print(F("IP: "));
		Serial.println(WiFi.localIP());
		return true;
	}
	else {
		Serial.println(F("SmartConfig Fail\n"));
	}
	return false;
}

void wifi_init() {
	//WiFi.disconnect();
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);

	//Serial.println(F("SmartConfig started."));
	//WiFi.beginSmartConfig();
	//while (1) {
	//	delay(1000);
	//	if (WiFi.smartConfigDone()) {
	//		Serial.println(F("SmartConfig: Success"));
	//		WiFi.printDiag(Serial);
	//		//WiFi.stopSmartConfig();
	//		break;
	//	}
	//}

	WiFi.printDiag(Serial);

	Serial.println(F("\nConnecting..."));

	if (WiFi.waitForConnectResult() == WL_CONNECTED)
	{
		Serial.println(F("connected\n"));
	}
	else
	{
		Serial.println(F("connect again\n"));
		if (WiFi.waitForConnectResult() == WL_CONNECTED)
		{
			Serial.println(F("connected\n"));
			return;
		}

		smart_config();
	}
}

String http_request(String host, uint16_t port = 80, String url = "/") {
	Serial.println("\r\nGET " + host + ":" + String(port) + url);
	WiFiClient client;
	client.setTimeout(100);
	if (!client.connect(host.c_str(), port)) {
		Serial.println("connection failed");
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
	delay(1);
	static unsigned long t_pre_update = 0;
	static bool wasSync = false;
	if (interval == 0) {
		{
			Serial.println(F("Update timestamp"));
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
					Serial.println(F("Time Updated\r\n"));
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
				Serial.println(F("Time Updated\r\n"));
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
	}
}

void hc595_digitalWrite(int pin, bool status) {
	//Serial.print("pin = ");
	//Serial.println(pin);
	//Serial.print("status = ");
	//Serial.println(status);

	static byte output = 0x00;
	static byte pre_output = -1;

	//8bit: 0 - PUMP1 - PUMP2 - WATER_IN - FAN - LIGHT - LED_STATUS - 0
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

void send_status_to_server(bool pump1, bool fan, bool light);
bool control(int pin, bool status, bool update_to_server = false);
bool control(int pin, bool status, bool update_to_server) { //status = true -> ON; false -> OFF
	bool pin_change = false;

	if ((pin == PUMP1) && (stt_pump1 != status)) {
		t_pump1_change = millis();
		stt_pump1 = status;
		pin_change = true;
		DEBUG.print(F("MIST: "));
		DEBUG.println(status ? "ON" : "OFF");
	}
	if ((pin == FAN) && (stt_fan != status)) {
		t_fan_change = millis();
		stt_fan = status;
		pin_change = true;
		DEBUG.print(F("FAN: "));
		DEBUG.println(status ? "ON" : "OFF");
	}
	if ((pin == LIGHT) && (stt_light != status)) {
		t_light_change = millis();
		stt_light = status;
		pin_change = true;
		DEBUG.print(F("LIGHT: "));
		DEBUG.println(status ? "ON" : "OFF");
	}

	if (pin_change) {
		hc595_digitalWrite(pin, status ? ON : OFF);
		if (update_to_server) {
			send_status_to_server(stt_pump1, stt_fan, stt_light);
		}
	}
	return pin_change;
}

void send_status_to_server(bool pump1 = true, bool fan = true, bool light = true) {
	Serial.println(F("send_status_to_server"));
	if (pump1 == false && fan == false && light == false) {
		Serial.println(F("No any thing change"));
		return;
	}
	StaticJsonBuffer<200> jsBuffer;
	JsonObject& jsStatus = jsBuffer.createObject();
	jsStatus["HUB_ID"] = HubID;
	jsStatus["MIST"] = stt_pump1 ? on_ : off_;
	jsStatus["FAN"] = stt_fan ? on_ : off_;
	jsStatus["LIGHT"] = stt_light ? on_ : off_;
	jsStatus["CMD_ID"] = "HW-" + String(now());;

	String jsStatusStr;
	jsStatusStr.reserve(150);
	jsStatus.printTo(jsStatusStr);
	mqtt_publish("Mushroom/Commands/" + HubID, jsStatusStr, true);
}

bool skip_auto_light = false;
bool skip_auto_pump1 = false;
bool skip_auto_fan = false;
void auto_control() {
	//https://docs.google.com/document/d/1wSJvCkT_4DIpudjprdOUVIChQpK3V6eW5AJgY0nGKGc/edit
	//https://prnt.sc/j2oxmu https://snag.gy/6E7xhU.jpg

	//+ PUMP1 tự tắt sau 1.5 phút
	if ((millis() - t_pump1_change) > 3 * 30000) {
		skip_auto_pump1 = false;
		if (stt_pump1) {
			control(PUMP1, false, true);
		}
	}
	//+ FAN tự tắt sau 1 tiếng
	if ((millis() - t_fan_change) > (1 * 1000 * SECS_PER_HOUR)) {
		skip_auto_fan = false;
		if (stt_fan) {
			control(FAN, false, true);
		}
	}
	//+ LIGHT tự tắt sau 3 tiếng
	if ((millis() - t_light_change) > (3 * 1000 * SECS_PER_HOUR)) {
		skip_auto_light = false;
		if (stt_light) {
			control(LIGHT, false, true);
		}
	}
	//==============================================================
	//1/ Bật tắt đèn
	if (!skip_auto_light) {
		if (light < LIGHT_MIN) {
			control(LIGHT, true, true);
		}
		else if (light > LIGHT_MAX) {
			control(LIGHT, false, true);
		}
	}
	//-------------------
	return;
	//2. Bật tắt phun sương
	//a. Phun trực tiếp vào phôi vào lúc 6h và 16h
	if (!skip_auto_pump1 && ((hour() == 6) || (hour() == 16)) && (minute() == 0) && (second() == 0)) {
		skip_auto_pump1 = true;
		control(PUMP1, true, false);
		control(FAN, true, true);
	}

	//b. Phun sương làm mát, duy trì độ ẩm. Thời gian bật: 3 phút, mỗi lần bật cách nhau 1 giờ.
	if (!skip_auto_pump1 && ((int(temp) > TEMP_MAX) || (int(humi) < HUMI_MIN)) && ((millis() - t_pump1_change) > 3600000) && !stt_pump1) {
		control(PUMP1, true, false);
		control(FAN, true, true);
	}
	//-------------------

	//c. Bật tắt quạt
	if (!skip_auto_fan && ((int)humi > HUMI_MAX) || ((int)temp > TEMP_MAX) && ((millis() - t_pump1_change) < 3600000) && !stt_pump1) {
		control(FAN, true, true);
	}

	if (!skip_auto_fan && !(((int)humi > HUMI_MAX) || ((int)temp > TEMP_MAX)) || (((millis() - t_pump1_change) < 3600000) && stt_pump1) && skip_auto_fan) {
		control(FAN, false, true);
	}
	delay(1);
}

void updateFirmware(String url) {
	ESPhttpUpdate.rebootOnUpdate(true);

	t_httpUpdate_return ret = ESPhttpUpdate.update(url);

	switch (ret) {
	case HTTP_UPDATE_FAILED:
		DEBUG.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
		break;

	case HTTP_UPDATE_NO_UPDATES:
		DEBUG.println(F("HTTP_UPDATE_NO_UPDATES"));
		break;

	case HTTP_UPDATE_OK:
		DEBUG.println(F("HTTP_UPDATE_OK"));
		delay(2000);
		ESP.restart();
		break;
	}
}

void serial_command_handle() {
	if (Serial.available()) {
		String Scmd = Serial.readString();
		Scmd.trim();
		DEBUG.println(F("\r\n>>>"));
		DEBUG.println(Scmd);

		control_handle(Scmd);
	}

	delay(1);
}

void control_handle(String cmd) {
	cmd.toUpperCase();
	if (cmd.indexOf("LIGHT ON") > -1) {
		skip_auto_light = true;
		control(LIGHT, true, true);
	}
	if (cmd.indexOf("LIGHT OFF") > -1) {
		skip_auto_light = true;
		control(LIGHT, false, true);
	}

	if (cmd.indexOf("MIST ON") > -1) {
		skip_auto_pump1 = true;
		control(PUMP1, true, true);
	}
	if (cmd.indexOf("MIST OFF") > -1) {
		skip_auto_pump1 = true;
		control(PUMP1, false, true);
	}

	if (cmd.indexOf("FAN ON") > -1) {
		skip_auto_fan = true;
		control(FAN, true, true);
	}
	if (cmd.indexOf("FAN OFF") > -1) {
		skip_auto_fan = true;
		control(FAN, false, true);
	}
	if (cmd.indexOf("RESET WIFI") > -1) {
		DEBUG.println(F("Reset Wifi"));
		WiFi.disconnect();
	}
}

#pragma endregion


