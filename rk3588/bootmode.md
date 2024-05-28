# rk3588 (download) boot mode

## check mmc index

在uboot中查看mmc的index

	=> mmc list
	mmc@fe2c0000: 1
	mmc@fe2e0000: 0 (eMMC)
	mmc@fe2c0000: 2
	mmc@fe2e0000: 3

## enter each mode from uboot

### loader mode

通过uboot命令行进入到rkusb loader(same as in kernel shell `reboot loader`)

	rockusb 0 mmc 0

或者通过在kernel shell命令输入如下进入

	reboot loader

在主机上看到的是usb download gadget设备

	lsusb
	Bus 003 Device 076: ID 2207:350b Fuzhou Rockchip Electronics Company USB download gadget

### usb mass storage mode

[u-boot-ums](https://docs.vicharak.in/vaaman-linux/linux-usage-guide/u-boot-ums/)

通过uboot命令行进入到usb mass storage设备(此时开发板的mmc以u盘的形式挂载在主机)

	ums 0 mmc 0

或者通过在kernel shell命令输入如下进入

	reboot ums

在主机上看到的是u盘(可以直接挂在对应的分区到本地)

	lsusb
	Bus 003 Device 085: ID 2207:0010 Fuzhou Rockchip Electronics Company GoClever Tab R83

	sda           8:0    1  29.1G  0 disk
	├─sda1        8:1    1     8M  0 part
	├─sda2        8:2    1     4M  0 part
	├─sda3        8:3    1    64M  0 part
	├─sda4        8:4    1   128M  0 part
	├─sda5        8:5    1    32M  0 part
	├─sda6        8:6    1    14G  0 part
	├─sda7        8:7    1   128M  0 part
	└─sda8        8:8    1  14.8G  0 part

### maskrom mode

通过uboot命令行进入到rockchip maskrom mode

	=> rbrom

	using rk3588 edge sdk tool to check
	./edge flash -q
	maskrom

在主机上看到的是usb设备

	lsusb
	Bus 003 Device 086: ID 2207:350b Fuzhou Rockchip Electronics Company
