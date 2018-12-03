extern float TEMP;
extern float HUMI;
extern int LIGHT;
extern bool WATER_HIGH;
extern bool WATER_LOW;

extern bool STT_PUMP_MIX;
extern bool STT_PUMP_FLOOR;
extern bool STT_FAN_MIX;
extern bool STT_FAN_WIND;
extern bool STT_LIGHT;

extern unsigned long t_pump_mix_change;
extern unsigned long t_pump_floor_change;
extern unsigned long t_fan_mix_change;
extern unsigned long t_fan_wind_change;
extern unsigned long t_light_change;

extern bool pin_change;
bool pump_floor_on = false;

bool skip_auto_light = false;
bool skip_auto_pump_mix = false;
bool skip_auto_fan_mix = false;
bool skip_auto_fan_wind = false;

time_t AUTO_OFF_LIGHT = 60 * 1000 * SECS_PER_MIN; //60 phút
time_t AUTO_OFF_PUMP_MIX = 6 * 60 * 1000;//180s
time_t AUTO_OFF_PUMP_FLOOR = 90000;//90s
time_t AUTO_OFF_FAN_MIX = AUTO_OFF_PUMP_MIX + 5;
time_t AUTO_OFF_FAN_WIND = 10 * 1000 * SECS_PER_MIN;//10 phút

time_t DELAY_SEND_CMD_STM32 = 1000;

void auto_return_automation() {
	//auto trở lại sau khi điều khiển 1 phút
	unsigned long t_auto_return = 1 * 60000;
	if (skip_auto_light && (millis() - t_light_change) > t_auto_return) {
		skip_auto_light = false;
	}
	if (skip_auto_pump_mix && (millis() - t_pump_mix_change) > t_auto_return) {
		skip_auto_pump_mix = false;
	}
	if (skip_auto_fan_mix && (millis() - t_fan_mix_change) > t_auto_return) {
		skip_auto_fan_mix = false;
	}
	if (skip_auto_fan_wind && (millis() - t_fan_wind_change) > t_auto_return) {
		skip_auto_fan_wind = false;
	}
}

void auto_off_light() {
	if (!skip_auto_light && STT_LIGHT && LIGHT > LIGHT_MAX) {
		static time_t t = millis();
		if (millis() - t < DELAY_SEND_CMD_STM32) {
			return;
		}
		t = millis();
		control(R_LIGHT, OFF, false);
		DEBUG_println("AUTO LIGHT OFF");
	}
}
void auto_off_pump_mix() {
	if (!skip_auto_pump_mix && STT_PUMP_MIX && (HUMI > HUMI_MIN || TEMP < TEMP_MAX)) {
		static time_t t = millis();
		if (millis() - t < DELAY_SEND_CMD_STM32) {
			return;
		}
		t = millis();
		control(R_PUMP_MIX, OFF, false);
		control(R_FAN_MIX, OFF, false);
		DEBUG_println("AUTO PUMP_MIX OFF");
		DEBUG_println("AUTO FAN_MIX OFF WITH PUMP_MIX");

		if (flag_schedule_pump_floor) {
			flag_schedule_pump_floor = false;
			pump_floor_on = true;
		}
	}
}
void auto_off_pump_floor() {
	//chỉ OFF PUMP_FLOOR theo thời gian (90s)
}
void auto_off_fan_mix() {
	//cùng lúc với PUMP_MIX
}
void auto_off_fan_wind() {
	if (!skip_auto_fan_wind && STT_FAN_WIND && (TEMP < TEMP_MAX && HUMI < HUMI_MAX)) {
		static time_t t = millis();
		if (millis() - t < DELAY_SEND_CMD_STM32) {
			return;
		}
		t = millis();
		control(R_FAN_WIND, OFF, false);
		DEBUG_println("3.4 AUTO FAN_WIND OFF");
	}
}
void auto_off_over_time() {
	if (STT_LIGHT && millis() - t_light_change > AUTO_OFF_LIGHT) {
		control(R_LIGHT, OFF, false);
		DEBUG_println("OVERTIME AUTO LIGHT OFF");
	}

	if (STT_PUMP_MIX && millis() - t_pump_mix_change > AUTO_OFF_PUMP_MIX) {
		control(R_PUMP_MIX, OFF, false);
		control(R_FAN_MIX, OFF, false);
		DEBUG_println("OVERTIME AUTO PUMP_MIX OFF");
		DEBUG_println("OVERTIME AUTO FAN_MIX OFF WITH PUMP_MIX");
	}

	if (STT_PUMP_FLOOR && millis() - t_pump_floor_change > AUTO_OFF_PUMP_FLOOR) {
		control(R_PUMP_FLOOR, OFF, false);
		DEBUG_println("OVERTIME AUTO PUMP_FLOOR OFF");
	}

	//if (STT_FAN_MIX && millis() - t_fan_mix_change > AUTO_OFF_FAN_MIX) {
	//	DEBUG_println("OVERTIME AUTO FAN_MIX OFF");
	//	control(R_FAN_MIX, false, false);
	//}

	if (STT_FAN_WIND && millis() - t_fan_wind_change > AUTO_OFF_FAN_WIND) {
		control(R_FAN_WIND, OFF, false);
		DEBUG_println("OVERTIME AUTO FAN_WIND OFF");
	}
}
void auto_off() {
	auto_off_over_time();

	auto_off_light();
	auto_off_pump_mix();
	auto_off_pump_floor();
	auto_off_fan_mix();
	auto_off_fan_wind();
}

void auto_on() {
	if (!LIBRARY) {
		return;
	}
	static time_t t = millis();
	if (millis() - t < DELAY_SEND_CMD_STM32) {
		return;
	}
	t = millis();

	if (TEMP > TEMP_MAX || HUMI < HUMI_MIN) {
		time_t _30mins = 30 * 1000 * SECS_PER_MIN;
		time_t t_delay = millis() - t_pump_mix_change;
		if (t_delay > _30mins) { /*30 phút*/
			control(R_PUMP_MIX, ON, false);
			control(R_FAN_MIX, ON, false);

			if (STT_PUMP_MIX == OFF) {
				DEBUG_println("4.1a AUTO PUMP_MIX ON");
			}
			if (STT_FAN_MIX == OFF) {
				DEBUG_println("4.1a AUTO FAN_MIX ON WITH PUMP_MIX");
			}

			flag_schedule_pump_floor = true;
		}
		else {
			control(R_PUMP_FLOOR, ON, false);

			time_t t_delay_fan_wind = millis() - t_fan_wind_change;
			if (t_delay_fan_wind > _30mins) {
				control(R_FAN_WIND, ON, false);
			}

			if (STT_PUMP_FLOOR == OFF) {
				DEBUG_println("4.1c AUTO PUMP_FLOOR ON");
			}

			if (t_delay_fan_wind > _30mins && STT_FAN_WIND == OFF) {
				DEBUG_println("4.1c AUTO FAN_WIND ON");
			}
		}
	}

	if (pump_floor_on) {
		pump_floor_on = false;
		control(R_PUMP_FLOOR, ON, false);
		DEBUG_println("4.1b (2.2) AUTO PUMP_FLOOR ON");
	}

	if (TEMP > TEMP_MAX && HUMI > HUMI_MAX) {
		time_t t_delay = millis() - t_fan_wind_change;
		if (t_delay > 20 * 1000 * SECS_PER_MIN) {
			control(R_FAN_MIX, ON, false);
			control(R_FAN_WIND, ON, false);

			if (STT_FAN_MIX == OFF) {
				DEBUG_println("4.2a AUTO FAN_MIX ON");
			}
			if (STT_FAN_WIND == OFF) {
				DEBUG_println("4.2a AUTO FAN_WIND ON");
			}
		}
		else {
			control(R_FAN_MIX, ON, false);
			if (STT_FAN_MIX == OFF) {
				DEBUG_println("4.2b AUTO FAN_MIX ON");
			}
		}
	}

	if (LIGHT < LIGHT_MIN && hour() > 6 && hour() < 18) {
		time_t t_delay = millis() - t_light_change;
		if (t_delay > 10 * 1000 * SECS_PER_MIN) {
			control(R_LIGHT, ON, false);
			if (STT_LIGHT == OFF) {
				DEBUG_println("4.3 AUTO LIGHT ON");
			}
		}
	}
}

void auto_control() {
	auto_return_automation();
	auto_off();
	auto_on();
	if (pin_change) {
		send_control_message_all_to_stm32();
	}
	yield();
}


//void auto_control() {
//	//https://docs.google.com/document/d/1wSJvCkT_4DIpudjprdOUVIChQpK3V6eW5AJgY0nGKGc/edit
//	//https://prnt.sc/j2oxmu https://snag.gy/6E7xhU.jpg
//	//==============================================================
//
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
//			}
//		}
//	}
//	else if ((millis() - t_pump_mix_change) > AUTO_OFF_PUMP_MIX) {
//		skip_auto_pump_mix = false;
//		if (STT_PUMP_MIX) {
//			DEBUG.println("AUTO PUMP_MIX OFF");
//			control(R_PUMP_MIX, false, false);
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
