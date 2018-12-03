#define IGNORE_AUTOMATION
#ifndef IGNORE_AUTOMATION

//YYMMDD
VERSION = 181203-1

https://docs.google.com/document/d/1svQLMZP1yxSOVfA69VlzfAsRBd0GcIfdrHD7v6-GutA/edit


1/ INPUT:

1.1/ Config lấy từ Mushroom/Library/<HubID>
	CONFIGS :
		{
			"TEMP_MAX": int,
			"TEMP_MIN": int,
			"HUMI_MAX": int,
			"HUMI_MIN": int,
			"LIGHT_MAX": int,
			"LIGHT_MIN": int,
			"DATE_HAVERST_PHASE": long,
			"SENSOR_UPDATE_INTERVAL_DEFAULT" : long
			"LIBRARY": ""	 //"ENABLE" for AUTOMATIC or "DISABLE"
		}

1.2/ Thời gian tự động tắt
	AUTO OFF
	Topic :
		"Mushroom/<HubID>/AUTO_OFF/LIGHT"
		"Mushroom/<HubID>/AUTO_OFF/PUMP_MIX"
		"Mushroom/<HubID>/AUTO_OFF/PUMP_FLOOR"
		"Mushroom/<HubID>/AUTO_OFF/FAN_MIX"
		"Mushroom/<HubID>/AUTO_OFF/FAN_WIND"
	
	Value: <int> milliseconds

1.3/ AUTO_OFF Default.
	AUTO_OFF_LIGHT = 60 * 1000 * SECS_PER_MIN; //60 phút
	AUTO_OFF_PUMP_MIX = 6 * 60 * 1000;//180s
	AUTO_OFF_PUMP_FLOOR = 90000;//90s
	AUTO_OFF_FAN_MIX = AUTO_OFF_PUMP_MIX + 5;
	AUTO_OFF_FAN_WIND = 10 * 1000 * SECS_PER_MIN;//10 phút


2/ KHOẢNG THỜI GIAN
Trong chế độ AUTO

2.1/ Bơm phun sương (PUMP_MIX):
	Thời gian bơm tối đa động theo Config hoặc khi thỏa mãn điều kiện HUMI > HUMI_MIN và TEMP < TEMP_MAX
	~
	Thời gian bơm tối đa động = AUTO_OFF_PUMP_MIX
	=>
	PUMP_MIX OFF ngay lập tức

2.2/ Bơm tưới nền (PUMP_FLOOR)
	Thời gian bơm : 3 phút, ngay sau bơm phun sương.
	=>
	Nếu PUMP_MIX OFF thì 
	PUMP_FLOOR ON
	
	Sau 3 phút 
	PUMP_FLOOR OFF

3.3/ Quạt đối lưu (FAN_MIX)
3.3.1/ Thời gian hoạt động : theo PUMP_MIX
	=>
	FAN_MIX ON/OFF cùng lúc với PUMP_MIX
	
3.3.2/ Khoảng thời gian 2 lần hoạt động tự động : 15 phút.
	=>
	Nếu thỏa thời gian hiện tại lớn hơn 15 phút so với lần thay đổi cuối.
	~
	TIME_NOW - t_fan_mix_change > 15 phút
	->
	FAN_MIX ON 

3.4/ Quạt thông gió(FAN_WIND)
	Thời gian hoạt động : 10 phút hoặc khi thỏa mãn điều kiện TEMP < TEMP_MAX và HUMI < HUMI_MAX
	=>
	FAN_WIND OFF

3.5/ Đèn chiếu sáng(LIGHT)
	Thời gian hoạt động : 30 phút hoặc thỏa mãn điều kiện LIGHT > LIGHT_MAX
	=>
	LIGHT OFF


4/ ĐIỀU KIỆN
4.1/ Nếu(TEMP > TEMP_MAX) hoặc (HUMI < HUMI_MIN)
	=>
	a/ Nếu thời gian delay PUMP_MIX tối thiểu > 30 phút
		=>
		PUMP_MIX ON và FAN_MIX ON
	
	b/ --PUMP_FLOOR ON và FAN_WIND ON sau khi PUMP_MIX OFF-- (remove vì trùng mục 2.2)
	
	c/ Nếu thời gian delay PUMP_MIX tối thiểu < 30 phút
		=>
		
		PUMP_FLOOR ON IF PUMP_MIX OFF
		và
		Nếu thời gian delay FAN_WIND > 30 phút
		=>
		FAN_WIND ON

4.2/ Nếu(TEMP > TEMP_MAX) và (HUMI > HUMI_MAX)
	~
	Thời gian delay tối thiểu của FAN_WIND = thời gian hiện tại - lần cuối FAN_WIND thay đổi
	=>
	a/ Nếu thời gian delay tối thiểu của FAN_WIND > 20 phút
		=>
		FAN_MIX ON và FAN_WIND ON
	
	b/ Nếu thời gian delay tối thiểu của FAN_WIND < 20 phút
		=>
		FAN_MIX ON

4.3/ Nếu(LIGHT < light_min) và thời gian delay tối thiểu > 10 phút và trong khoảng thời gian 18h đến 6h trong ngày
	~
	thời gian delay tối thiểu = thời gian hiện tại - lần cuối LIGHT thay đổi
	=>
	LIGHT ON

==
5/ THÊM ĐIỀU KIỆN
5.1/ Các ngõ điều khiển tự động hoạt động theo chế độ AUTO trở lại sau 1 phút kể từ lúc người dùng điều khiển.



#endif