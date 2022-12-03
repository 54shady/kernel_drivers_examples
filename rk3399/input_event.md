# User Buttons

使用evtest来查看输入设备和输入事件

For example, on the sapphire-excavator-board(buildroot)

	[root@rk3399:/]# evtest
	No device specified, trying to scan all of /dev/input/event*
	Available devices:
	/dev/input/event0:      gsl3673
	/dev/input/event1:      adc-keys
	/dev/input/event2:      gpio-keys
	Select the device event number [0-2]: 1
	Input driver version is 1.0.1
	Input device ID: bus 0x19 vendor 0x1 product 0x1 version 0x100
	Input device name: "adc-keys"
	Supported events:
	  Event type 0 (EV_SYN)
	  Event type 1 (EV_KEY)
		Event code 1 (KEY_ESC)
		Event code 114 (KEY_VOLUMEDOWN)
		Event code 115 (KEY_VOLUMEUP)
		Event code 139 (KEY_MENU)
		Event code 158 (KEY_BACK)
	Properties:
	Testing ... (interrupt to exit)
	Event: time 1358512388.027673, type 1 (EV_KEY), code 115 (KEY_VOLUMEUP), value 1
	Event: time 1358512388.027673, -------------- SYN_REPORT ------------
	Event: time 1358512388.127788, type 1 (EV_KEY), code 115 (KEY_VOLUMEUP), value 0
	Event: time 1358512388.127788, -------------- SYN_REPORT ------------
