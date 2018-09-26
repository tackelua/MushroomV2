// 
// 
// 
#include "Sensor.h"
#include <DHT.h>
#include "BH1750.h"
#include "hardware.h"

float readTemp() {
#ifdef DHT
	float t = dht.readTemperature();
	if (isnan(t)) {
		DEBUG.println(("#ERR read Temp fail"));
		return 0;
	}
	return t;
#endif // DHT

#ifdef SHT
	ulong t = millis();
	float temp = sht.readTemperatureC(); //change waitForResultSHT loop from 100 times to 10 times
	DEBUG.print(("Read Temp: "));
	DEBUG.print(String(temp, 2));
	DEBUG.print(" in ");
	DEBUG.print(String(millis() - t));
	DEBUG.println(("ms"));
	return (temp < 0.01f ? -1.0f : temp);
#endif // SHT

}

float readHumi() {
#ifdef DHT
	float h = dht.readHumidity();
	if (isnan(h)) {
		DEBUG.println(("#ERR read Humi fail"));
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