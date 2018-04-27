#ifdef USE_OLED
void oled_analogClock(int _hour, int _min, int _sec, int x = 0, int y = 0) {
	int screenW = 128;
	int screenH = 64;
	int clockCenterX = screenW / 2;
	int clockCenterY = ((screenH - 16) / 2) + 16;   // top yellow part is 16 px height
	int clockRadius = 23;
	display.clear();

	display.drawCircle(clockCenterX + x, clockCenterY + y, 2);
	//
	//hour ticks
	for (int z = 0; z < 360; z = z + 30) {
		//Begin at 0° and stop at 360°
		float angle = z;
		angle = (angle / 57.29577951); //Convert degrees to radians
		int x2 = (clockCenterX + (sin(angle) * clockRadius));
		int y2 = (clockCenterY - (cos(angle) * clockRadius));
		int x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 8))));
		int y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 8))));
		display.drawLine(x2 + x, y2 + y, x3 + x, y3 + y);
	}

	// display second hand

	/*_sec = ++_sec % 60;
	if (_sec == 0) {
		_min = ++_min % 60;
	}
	if (_min == 0) {
		_hour = ++_hour % 24;
	}*/

	String _time = String(_hour) + ":" + String(_min) + ":" + String(_sec);
	DEBUG.print("OLED Time: ");
	DEBUG.printf(_time);

	float angle = (_sec) * 6;
	angle = (angle / 57.29577951); //Convert degrees to radians
	int x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 5))));
	int y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 5))));
	display.drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);
	//
	// display minute hand
	angle = _min * 6;
	angle = (angle / 57.29577951); //Convert degrees to radians
	x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 4))));
	y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 4))));
	display.drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);
	//
	// display hour hand
	angle = _hour * 30 + int((_min / 12) * 6);
	angle = (angle / 57.29577951); //Convert degrees to radians
	x3 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 2))));
	y3 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 2))));
	display.drawLine(clockCenterX + x, clockCenterY + y, x3 + x, y3 + y);

	display.display();
}

#endif // USE_OLED
