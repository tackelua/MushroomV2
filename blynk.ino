#define BL_LIGHT  V1
#define BL_PUMP1  V2
#define BL_FAN	  V3
#define BL_TEMP	  V4
#define BL_HUMI	  V5
#define BL_LIGHT_SS	 V6


void blynk_init() {
	Blynk.config("e7979867f95e41d0aa4f8d28d904b398", "gith.cf", 8442);
	Blynk.connect();
}
BLYNK_WRITE(BL_LIGHT) {
	int status = param.asInt();
	DEBUG.printf("BLYNK LIGHT %d\r\n", status);
	control(LIGHT, (bool)status, true, true);
}
BLYNK_WRITE(BL_PUMP1) {
	int status = param.asInt();
	DEBUG.printf("BLYNK PUMP %d\r\n", status);
	control(PUMP1, (bool)status, true, true);
}
BLYNK_WRITE(BL_FAN) {
	int status = param.asInt();
	DEBUG.printf("BLYNK FAN %d\r\n", status);
	control(FAN, (bool)status, true, true);
}