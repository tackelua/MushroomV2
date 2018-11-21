#define IGNORE_THIS_FILE
#ifndef IGNORE_THIS_FILE

VERSION 0.1.3

CONTROL
APP -> HUB
Topic : "Mushroom/Commands/REQUEST/<HubID>"
	{
		"MES_ID" : "",			//SW-xxx (optional)
		//"HUB_ID" : "",		 
		//"SOURCE" : "",		//"APP" (APP Client) or "HUB"
		//"DEST" : "",			//"APP" (APP Client) or "HUB"
		//"TIMESTAMP" : "",		//option
		"MIST" : "",			//"on" or "off"
		"LIGHT" : "",			//"on" or "off"
		"FAN" : "",				//"on" or "off"
		//"VALIDATE" : ""		//(optional) validate the command, cần bảo mật thì tính sau
	}
HUB -> APP
Topic : "Mushroom/Commands/RESPONSE/<HubID>"
	{
		"MES_ID" : "",			//HW-xxx
		"HUB_ID" : "",
		//"SOURCE" : "",		
		//"DEST" : "",		
		"TIMESTAMP" : long,
		"TYPE" : "",			//"RESPONSE" nghĩa là APP điều khiển, "NOTIFY" : HUB tự điều khiển theo chế độ tự động
		"MIST" : "",		
		"LIGHT" : "",		
		"FAN" : "",		
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
		"SENSOR_UPDATE_INTERVAL_DEFAULT" : long
		"LIBRARY": ""						//"ENABLE" for AUTOMATIC or "DISABLE"
	}

AUTO OFF
Topic :
	"Mushroom/<HubID>/AUTO_OFF/LIGHT"
	"Mushroom/<HubID>/AUTO_OFF/PUMP_MIX"
	"Mushroom/<HubID>/AUTO_OFF/PUMP_FLOOR"
	"Mushroom/<HubID>/AUTO_OFF/FAN_MIX"
	"Mushroom/<HubID>/AUTO_OFF/FAN_WIND"

Value: <int> milliseconds


SENSOR
HUB -> APP
Topic: "Mushroom/Sensor/<HubID>"
	{  
		"MES_ID":"",
		"HUB_ID":"",
		"TEMP":int,
		"HUMI":int,
		"LIGHT":int,
		"WATER_EMPTY":"",			// "YES" or "NO"
		"RSSI":int,
		"TIMESTAMP": long
	}


STATUS
Topic: "Mushroom/Status/<HubID>"
	{
		"HUB_ID" : "",
		"STATUS" : "",				//"ONLINE" (HUB send) or "OFFLINE" (BROKER send LWT)
		"FW_VER" : "",				//only available when online
		"HW_VER" : "",				//only available when online
		"WIFI" : "",				//only available when online
		"SIGNAL" : int				//only available when online
	}


LOGS
Topic: "Mushroom/Logs/<HubID>"
	{
		"HUB_ID" : "",
		"Content" : "",				//"Light" or "Pump_Mix" or "Pump_Floor" or "Fan_Mix" or "Fan_Wind" + " on" or " off"
		"From" : ""					//"APP" or "HUB"
		"Timestamp" : "",			
	}

//=======================================

TERMINAL
Topic: tp_Terminal
	{
		"Command" : "FOTA",
		"Hub_ID" : "all",
		"Version" : "",
		"Url" : ""
	}

Topic: "Mushroom/Terminal/<HubID>"
	"/restart"
	"/uf"
	"/version" or "/v"
	"/library" or "/l"


#endif