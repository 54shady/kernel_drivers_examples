# usb摄像头名和端口号绑定

先查询设备信息

	udevadm info -a -p $(udevadm info -q path -n /dev/videoX)

将插入到usb5-1.1端口..., 1-1.4端口的摄像头添加一个链接(/etc/udev/rules.d/10-usb-camera.rules)

	KERNEL=="video[0-9]", KERNELS=="5-1.1", SUBSYSTEMS=="usb", ATTRS{devpath}=="1.1", SYMLINK+="usb-511-video%n"
	KERNEL=="video[0-9]", KERNELS=="5-1.2", SUBSYSTEMS=="usb", ATTRS{devpath}=="1.2", SYMLINK+="usb-512-video%n"
	KERNEL=="video[0-9]", KERNELS=="5-1.3", SUBSYSTEMS=="usb", ATTRS{devpath}=="1.3", SYMLINK+="usb-513-video%n"
	KERNEL=="video[0-9]", KERNELS=="5-1.4", SUBSYSTEMS=="usb", ATTRS{devpath}=="1.4", SYMLINK+="usb-514-video%n"
	KERNEL=="video[0-9]", KERNELS=="1-1.4", SUBSYSTEMS=="usb", ATTRS{devpath}=="1.4", SYMLINK+="usb-114-video%n"

其中%n表示取KERNEL的device name index

	The %n in the udev rule's SYMLINK field is a substitution operator that corresponds to the kernel device name.

重新触发udev规则

	udevadm control --reload-rules && udevadm trigger
