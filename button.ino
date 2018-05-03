extern bool ledStt;
void button_handle()
{
	static bool press = false;
	static bool press_pre = false;
	static unsigned long t_press = 0;
	static unsigned long t_release = 0;
	myBtn.read();
	if (myBtn.pressedFor(10000)) {
		DEBUG.println("Restart");
		ESP.restart();
	}

	press_pre = press;
	press = myBtn.isPressed();
	if (!press_pre && press) { //press
		t_press = millis();
	}
	if (press_pre && !press) { //release
		t_release = millis();

		if ((t_release - t_press) <= 5000) {
			if (flag_SmartConfig) {
				flag_SmartConfig = false;
				DEBUG.println("Stop smart config");
				WiFi.stopSmartConfig();
			}
			else {
				flag_SmartConfig = true;
				DEBUG.println("Begin smart config");
				WiFi.beginSmartConfig();
			}
		}
		else if ((t_release - t_press) > 5000) {
			WiFi.stopSmartConfig();
			DEBUG.println("WiFi Manager Started");
			for (int i = 0; i < 10; i++) {
				hc595_digitalWrite(LED_STATUS, !ledStt);
				delay(50);
			}
			hc595_digitalWrite(LED_STATUS, false);

			WiFiManager wifiManager;
			//wifiManager.resetSettings();
			wifiManager.setTimeout(5 * 60);
			wifiManager.setAPCallback(configModeCallback);
			//wifiManager.startConfigPortal(String("Mushroom" + HubID.substring(HubID.length() - 4)).c_str());
			String apName = "Mushroom_" + HubID.substring(HubID.length() - 4);
			if (!wifiManager.startConfigPortal(apName.c_str())) {
				Serial.println("failed to connect and hit timeout");
			}
		}
		//else {
		//	DEBUG.println("Restart");
		//	ESP.restart();
		//}
	}
	if (press && (millis() - t_press > 5000)) {
		DEBUG.println("Button press over 5s");
		hc595_digitalWrite(LED_STATUS, false);
		delay(20);
		hc595_digitalWrite(LED_STATUS, true);
		delay(20);
	}
	delay(1);
}
