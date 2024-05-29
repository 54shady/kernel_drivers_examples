# RM520N-GL

该模块是一个包含了多个usb接口的usb复合设备,如下

- usb串口驱动(option和ACM)
- usb网口驱动(GobiNet, QMI_WWAN, MBIM, NCM, RNDIS, ECM)

加载好驱动使用lsusb看到的信息

	Bus 001 Device 005: ID 2c7c:0801 Quectel Wireless Solutions Co., Ltd. RM520N-GL

将设备以usb模块接入到usb2接口查看设备配置和信息

	cat /sys/kernel/debug/usb/devices

	T:  Bus=01 Lev=02 Prnt=02 Port=02 Cnt=01 Dev#=  5 Spd=480  MxCh= 0
	D:  Ver= 2.10 Cls=00(>ifc ) Sub=00 Prot=00 MxPS=64 #Cfgs=  1
	P:  Vendor=2c7c ProdID=0801 Rev= 5.04
	S:  Manufacturer=Quectel
	S:  Product=RM520N-GL
	S:  SerialNumber=c7b88096
	C:* #Ifs= 5 Cfg#= 1 Atr=a0 MxPwr=500mA
	I:* If#= 0 Alt= 0 #EPs= 2 Cls=ff(vend.) Sub=ff Prot=30 Driver=option
	E:  Ad=01(O) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	E:  Ad=81(I) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	I:* If#= 1 Alt= 0 #EPs= 3 Cls=ff(vend.) Sub=00 Prot=40 Driver=option
	E:  Ad=83(I) Atr=03(Int.) MxPS=  10 Ivl=32ms
	E:  Ad=82(I) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	E:  Ad=02(O) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	I:* If#= 2 Alt= 0 #EPs= 3 Cls=ff(vend.) Sub=00 Prot=00 Driver=option
	E:  Ad=85(I) Atr=03(Int.) MxPS=  10 Ivl=32ms
	E:  Ad=84(I) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	E:  Ad=03(O) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	I:* If#= 3 Alt= 0 #EPs= 3 Cls=ff(vend.) Sub=00 Prot=00 Driver=option
	E:  Ad=87(I) Atr=03(Int.) MxPS=  10 Ivl=32ms
	E:  Ad=86(I) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	E:  Ad=04(O) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	I:* If#= 4 Alt= 0 #EPs= 3 Cls=ff(vend.) Sub=ff Prot=ff Driver=qmi_wwan_q
	E:  Ad=88(I) Atr=03(Int.) MxPS=   8 Ivl=32ms
	E:  Ad=8e(I) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	E:  Ad=0f(O) Atr=02(Bulk) MxPS= 512 Ivl=0ms

## USB 网口驱动(GobiNet)

GobiNet驱动

网络设备(用于数据传输)

	usb0

QMI节点设备(用于QMI消息交互)

	/dev/qcqmi0

查看是否加载驱动(没有看到usb设备对应的信息,没加载该驱动)

	ls -l /sys/bus/usb/drivers/GobiNet/
		bind
		module -> ../../../../module/GobiNet
		new_id
		remove_id
		uevent
		unbind

## USB 网口驱动(QMI_WWAN) RM520-GL默认使用这个

QMI_WWAN驱动

网络设备(用于数据传输)

	wwan0
	wwan0_1

QMI节点设备(用于QMI消息交互)

	/dev/cdc-wdm0

查看设备节点对应的驱动

	/sys/class/usbmisc/cdc-wdm0/device/driver/module -> ../../../../module/qmi_wwan_q

通过下面命令看到usb对应的每一个端口信息都有,说明驱动已经加载

	ls -l /sys/bus/usb/drivers/qmi_wwan_q/
		1-1.3:1.4 -> ../../../../devices/platform/fc800000.usb/usb1/1-1/1-1.3/1-1.3:1.4
		bind
		module -> ../../../../module/qmi_wwan_q
		new_id
		remove_id
		uevent
		unbind

## USB 转串口option驱动(drivers/usb/serial/option.c)

usb转串口的设备节点和驱动

	ls -l /dev/ttyUSB*
		crw-rw---- 1 root dialout 188, 0  5月 28 09:34 /dev/ttyUSB0
		crw-rw---- 1 root dialout 188, 1  5月 28 09:34 /dev/ttyUSB1
		crw-rw---- 1 root dialout 188, 2  5月 28 09:34 /dev/ttyUSB2
		crw-rw---- 1 root dialout 188, 3  5月 28 09:34 /dev/ttyUSB3

通过下面命令看到usb对应的每一个端口信息都有,说明驱动已经加载

	ls -l /sys/bus/usb/drivers/option/
		1-1.3:1.0 -> ../../../../devices/platform/fc800000.usb/usb1/1-1/1-1.3/1-1.3:1.0
		1-1.3:1.1 -> ../../../../devices/platform/fc800000.usb/usb1/1-1/1-1.3/1-1.3:1.1
		1-1.3:1.2 -> ../../../../devices/platform/fc800000.usb/usb1/1-1/1-1.3/1-1.3:1.2
		1-1.3:1.3 -> ../../../../devices/platform/fc800000.usb/usb1/1-1/1-1.3/1-1.3:1.3
		bind
		uevent
		unbind

每个设备对应的功能,参考对应的芯片手册

	ttyUSB0	DIAG
	ttyUSB1	GNSS
	ttyUSB2	AT
	ttyUSB3	Modem

## 测试

测试AT功能

一个窗口查看输出

	cat /dev/ttyUSB2
	+CME ERROR: 10 // 表示没有插sim卡

一个窗口执行AT命令

	echo -e "at+cpin?\r\n" > /dev/ttyUSB2

查看厂商

	echo -e "at+cgmi\r\n" > /dev/ttyUSB2

查看模块型号

	echo -e "at+cgmm\r\n" > /dev/ttyUSB2

重启模块

	echo -e "at+cfun=1,1\r\n" > /dev/ttyUSB2
