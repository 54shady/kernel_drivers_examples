# 看门狗(watchdog)

设备节点信息(watchdog和watchdog0)

	crw-------    1 root     root       10, 130 Jan 18 08:52 /dev/watchdog
	crw-------    1 root     root      247,   0 Jan 18 08:52 /dev/watchdog0

这两给设备节点对应的是同一个设备(硬狗或是软狗)

watchdog是为了兼容老的API接口保留的

## 使用方法

使能看门狗

	fd = open("/dev/watchdog", O_WRONLY)

设置超时时间

	ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
	ioctl(fd, WDIOC_GETTIMEOUT, &timeout);

喂狗操作

	ioctl(fd, WDIOC_KEEPALIVE, 0);
	或
	write(fd, "\0", 1);
