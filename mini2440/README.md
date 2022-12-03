# mini2440

## 编译器版本

	gcc version 4.3.2 (Sourcery G++ Lite 2008q3-72)

## uboot

### 代码和补丁

	u-boot-2012.04.01
	patch -p1 < mini2440_uboot20120401.patch

### 编译

	make mini2440_config

### 烧写到norflash(使用norflash启动开发板)

下载u-boot.bin到sdram

	loady 32000000

烧写到norflash

	protect off all
	erase 0 7ffff
	cp.b 32000000 0 80000

### 设置启动参数

	set machid 7cf ;set bootargs console=ttySAC0,115200 root=/dev/nfs nfsroot=192.168.1.100:/export/nfs_rootfs ip=192.168.1.230:192.168.1.100:192.168.1.1:255.255.255.0::eth0:off ;nfs 30000000 192.168.1.100:/export/nfs_rootfs/uImage;bootm 30000000

## kernel

### 代码和补丁

	linux-3.4.2
	patch -p1 < linux-3.4.2_camera_mini2440.patch

### 编译

	mkdir out
	make ARCH=arm CROSS_COMPILE=arm-linux- mini2440_defconfig O=out
	make ARCH=arm CROSS_COMPILE=arm-linux- O=out -j8 uImage
