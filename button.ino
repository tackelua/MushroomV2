void button_handle()
{
	myBtn.read();
	if (myBtn.pressedFor(1000)) {
		smart_config();
	}
}
