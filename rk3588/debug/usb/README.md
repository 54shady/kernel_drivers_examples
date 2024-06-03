# USB FunctionFS

## USB FFS test demo

### 编译Host App

编译host app在主机上运行

	cd ffs-aio-example/host_app/
	make

### 编译Device App

使用edge SDK自带的交叉编译工具链

	export ARCH=arm64
	export CROSS_COMPILE=~/src/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

交叉编译libaio(commit b8eadc9)和device app

下载libaio代码

	git clone https://pagure.io/libaio.git

修改src/Makefile添加如下(使用交叉编译工具链)

	CC = $(CROSS_COMPILE)gcc
	AR = $(CROSS_COMPILE)ar

	make

编译生成动态和静态库

	src/libaio.a
	src/libaio.so.1.0.2

将静态库和头文件拷贝到指定目录

	cp src/libaio.a ffs-aio-example/device_app/
	cp src/libaio.h ffs-aio-example/device_app/

编译device app

	cd ffs-aio-example/device_app/
	make

### 测试

进入到设备端运行usb.sh进行配置

	usb.sh

在设备端运行device app

	./aio_multibuff /dev/usb-ffs/mydemo

	echo fc000000.usb > /config/usb_gadget/g1/UDC
	echo device > /sys/kernel/debug/usb/fc000000.usb/mode

其中fc000000对应的是dwc3(arch/arm64/boot/dts/rockchip/rk3588s.dtsi)

	usbdrd_dwc3_0: usb@fc000000 {
		compatible = "snps,dwc3";

此时能在主机lsusb上看到如下设备

	Bus 003 Device 087: ID 1d6b:0105 Linux Foundation FunctionFS Gadget

查看设备信息

	lsusb -v -d 1d6b:0105

	cat /sys/kernel/debug/usb/devices
	...
	T:  Bus=03 Lev=01 Prnt=01 Port=02 Cnt=02 Dev#= 87 Spd=480  MxCh= 0
	D:  Ver= 2.10 Cls=00(>ifc ) Sub=00 Prot=00 MxPS=64 #Cfgs=  1
	P:  Vendor=1d6b ProdID=0105 Rev= 3.10
	S:  Manufacturer=myusb
	S:  Product=myusbtest
	S:  SerialNumber=0123456789
	C:* #Ifs= 1 Cfg#= 1 Atr=80 MxPwr=500mA
	I:* If#= 0 Alt= 0 #EPs= 2 Cls=ff(vend.) Sub=00 Prot=00 Driver=(none)
	E:  Ad=81(I) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	E:  Ad=01(O) Atr=02(Bulk) MxPS= 512 Ivl=0ms
	...

在主机上运行host app

	sudo ffs-aio-example/host_app/multibuff_test

## ADB

从设备端分析adb功能

系统默认的配置

	/sys/kernel/config/usb_gadget/rockchip

在设备端挂在的对应的目录是(根据toybrick_adbd的pid查看打开的文件)

	# mount -o uid=2000,gid=2000 -t functionfs adb /dev/usb-ffs/adb
	adb on /dev/usb-ffs/adb type functionfs (rw,relatime)
	ls -l /dev/usb-ffs/adb/

因为/sys/kernel/config/usb_gadget/rockchip/functions/ffs.xxoo和/dev/usb-ffs/xxoo 对应

	/sys/kernel/config/usb_gadget/rockchip/functions/ffs.adb (对应/dev/usb-ffs/adb)
	#ls -l /sys/kernel/config/usb_gadget/rockchip/configs/b.1/ffs.adb

在主机上枚举的设备(lsusb -v -d 2207:0018)

	Bus 003 Device 091: ID 2207:0018 Fuzhou Rockchip Electronics Company TB-RK3588C0-A
