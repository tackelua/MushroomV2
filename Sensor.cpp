// 
// 
// 
#include "Sensor.h"
#include <DHT.h>
#include "BH1750.h"
#include "hardware.h"

#ifdef DHT
DHT dht(DHTPIN, DHTTYPE);
#endif // DHT

#ifdef SHT
SHT1x sht(SHT_dataPin, SHT_clockPin);
#endif // SHT

void sensor_init() {
#ifdef DHT
	dht.begin();
#endif // DHT

#ifdef BH1750
	BH1750.INIT(BH1750_ADDRESS);
#endif // BH1750

#ifdef USED_LIGHT_SENSOR_ANALOG
	pinMode(PININ_LIGHT_SENSOR, INPUT);
#endif // USED_LIGHT_SENSOR_ANALOG
}

float readTemp() {
#ifdef DHT
	float t = dht.readTemperature();
	if (isnan(t)) {
		DEBUG.println(F("#ERR read Temp fail"));
		return 0;
	}
	return t;
#endif // DHT

#ifdef SHT
	ulong t = millis();
	float temp = sht.readTemperatureC(); //change waitForResultSHT loop from 100 times to 10 times
	DEBUG.print(F("Read Temp: "));
	DEBUG.print(temp);
	DEBUG.print(" in ");
	DEBUG.print(millis() - t);
	DEBUG.println(F("ms"));
	return (temp < 0.01f ? -1.0f : temp);
#endif // SHT

}

float readHumi() {
#ifdef DHT
	float h = dht.readHumidity();
	if (isnan(h)) {
		DEBUG.println(F("#ERR read Humi fail"));
		return 0;
	}
	return h;
#endif // DHT

#ifdef SHT
	float h = sht.readHumidity();
	return (h < 0.01f ? -1.0f : h);
#endif // SHT

}

int readLight() {
#ifdef BH1750
	return BH1750.BH1750_Read(BH1750_ADDRESS);
#endif // BH1750

#ifdef USED_LIGHT_SENSOR_ANALOG
	int l = analogRead(PININ_LIGHT_SENSOR);
	return (l == 1024 ? -1.0f : l);
#endif // USED_LIGHT_SENSOR_ANALOG

}