//// 
//// 
//// 
//#include <I2CIO.h>
//#include <LCD.h>
//#include <LiquidCrystal_I2C.h>
//#include <Wire.h> 
//#include "Screen.h"
//
//
//LiquidCrystal_I2C lcd(0x38, 4, 5, 6, 0, 1, 2, 3, 7, NEGATIVE);
//
//void ScreenClass::init() {
//	Wire.begin();
//	lcd.begin(8, 2);
//	lcd.display();
//	lcd.clear();
//	lcd.setCursor(0, 0);
//	lcd.print("TEMP: ");
//	lcd.setCursor(0, 1);
//	lcd.print("HUMI: ");
//}
//
//void ScreenClass::print_Temp(float temp) {
//	lcd.setCursor(0, 5);
//	lcd.print(temp);
//	lcd.print("*C  ");
//}
//
//void ScreenClass::print_Humi(float humi) {
//	lcd.setCursor(1, 5);
//	lcd.print(humi);
//	lcd.print("%  ");
//}
//
//ScreenClass Screen;
//
