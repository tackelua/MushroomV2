#define IGNORE_THIS_FILE
#ifndef IGNORE_THIS_FILE

VERSION 0.1.2

CONTROL
APP -> HUB
Topic : "Mushroom/Commands/REQUEST/<HubID>"
	{
		"MES_ID" : "",			//SW-xxx (optional)
		//"HUB_ID" : "",		 
		//"SOURCE" : "",		//"APP" (APP Client) or "HUB"
		//"DEST" : "",			//"APP" (APP Client) or "HUB"
		//"TIMESTAMP" : "",		//option
		"MIST" : "OFF",			//"ON" or "OFF"
		"LIGHT" : "ON",			//"ON" or "OFF"
		"FAN" : "ON",			//"ON" or "OFF"
		//"VALIDATE" : ""		//validate the command (optional), cần bảo mật thì tính sau
	}
HUB -> APP
Topic : "Mushroom/Commands/RESPONSE/<HubID>"
	{
		"MES_ID" : "",			//HW-xxx
		"HUB_ID" : "",
		//"SOURCE" : "",		
		//"DEST" : "",		
		"TIMESTAMP" : long,
		"TYPE" : "",			//"RESPONSE" nghĩa là app điều khiển, "NOTIFY" : HUB tự điều khiển theo chế độ tự động
		"MIST" : "OFF",		
		"LIGHT" : "ON",		
		"FAN" : "ON",		
		//"VALIDATE" : ""			
	}


LIBRARY
APP->HUB
Topic : "Mushroom/Library/<HubID>"
	{
		"TEMP_MAX": int,
		"TEMP_MIN": int,
		"HUMI_MAX": int,
		"HUMI_MIN": int,
		"LIGHT_MAX": int,
		"LIGHT_MIN": int,
		"DATE_HAVERST_PHASE": long,
		"SENSOR_UPDATE_INTERVAL" : long
		"LIBRARY": ""						//"ENABLE" for AUTOMATIC or "DISABLE"
	}


SENSOR
HUB -> APP
Topic: "Mushroom/Sensor/<HubID>"
	{  
		"HUB_ID":"",
		"TEMP":int,
		"HUMI":int,
		"LIGHT":int,
		"WATER_EMPTY":"",			// "YES" or "NO"
		"RSSI":int
	}


STATUS
Topic: "Mushroom/Status/<HubID>"
	{
		"HUB_ID" : "",
		"STATUS" : "",				//"ONLINE" (HUB send) or "OFFLINE" (BROKER send LWT)
		"FW_VER" : ""				//only available when online
		"WIFI" : "",				//only available when online
		"SIGNAL" : int				//only available when online
	}


//=======================================

TERMINAL
Topic: "Mushroom/Terminal"
	{
		"Command" : "FOTA",
		"Hub_ID" : "all",
		"Version" : "",
		"Url" : ""
	}

Topic: "Mushroom/Terminal/<HubID>"
	"/restart"
	"/uf"
	"/get version"
	"/get library"


#endif