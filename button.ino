void button_handle()
{
	static bool press = false;
	static bool press_pre = false;
	static unsigned long t_press = 0;
	static unsigned long t_release = 0;
	myBtn.read();
	
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
		else {
			DEBUG.println("Restart");
			ESP.restart();
		}
	}

}
