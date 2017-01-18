# kernel_drivers_examples

Codes test on the kernel version below

	4.4.0

# tiny4412_devicetree

tiny4412内核移植(支持device tree)

[参考文章](http://www.cnblogs.com/pengdonglin137/tag/TINY4412/)

内核版本：Linux-4.4.0

u-boot版本：友善之臂自带的 U-Boot 2010.12

busybox版本：busybox 1.25

交叉编译工具链arm-none-linux-gnueabi-gcc:

gcc version 4.8.3 20140320 (prerelease) (Sourcery CodeBench Lite 2014.05-29)

## 编译内核

配置

	make O=out ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- exynos_defconfig

编译内核

	make O=out ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- uImage -j8 LOADADDR=0x40008000

编译模块

	make O=out ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- modules -j8

编译设备树

	make O=out ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- dtbs -j8

## ramdisk制作

使用脚本make_ramdiskimage.sh或手工执行

```shell
rm -rvf ramdisk*
sudo dd if=/dev/zero of=ramdisk bs=1k count=8192
sudo mkfs.ext4 -F ramdisk
sudo mkdir -p ./initrd
sudo mount -t ext4 ramdisk ./initrd
sudo cp rootfs/* ./initrd -ravf
sudo mknod initrd/dev/console c 5 1
sudo mknod initrd/dev/null c 1 3
sudo umount ./initrd
sudo gzip --best -c ramdisk > ramdisk.gz
sudo mkimage -n "ramdisk" -A arm -O linux -T ramdisk -C gzip -d ramdisk.gz ramdisk.img
```

## 说明

4412>	表示在开发板上执行

PC>		表示在开发主机上执行

## 使用DNW烧写

block size = 512 byte(0x200)

### 下载内核(uImage)

下载内核到DDR里

4412>

	dnw 0x40600000

PC>

	dnw uImage

将DDR里的内核写入EMMC

写起始地址 0x421

写大小0x3000

0x3000 * 0x200 / 0x400 / 0x400 = 6M(大于内核大小即可)

4412>

	mmc write 0 0x40600000 0x421 0x3000

将写入EMMC的文件读出来对比(测试烧写是否正确)

	mmc read 0 0x41600000 0x421 0x3000
	cmp.b 0x41600000 0x40600000 0x3000

### 下载文件系统(ramdisk.img)

下载文件统到DDR里

写起始地址 0x3421

写大小0x2800

0x2800 * 0x200 / 0x400 / 0x400 = 5M(大于文件系统大小即可)

4412>

	dnw 0x41000000

PC>

	dnw ramdisk.img

将DDR里的文件系统写入EMMC

4412>

	mmc write 0 0x41000000 0x3421 0x2800

将写入EMMC的文件读出来对比(测试烧写是否正确)


	mmc read 0 0x41600000 0x3421 0x2800
	cmp.b 0x41600000 0x41000000 0x2800

### 下载设备树(exynos4412-tiny4412.dtb)

下载设备树到DDR里

写起始地址 0x5c21

写大小0x500

0x800 * 0x200 / 0x400 / 0x400 = 1M(大于设备树大小即可)

4412>

	dnw 0x42000000

PC>

	dnw exynos4412-tiny4412.dtb

将DDR里的设备树写入EMMC

4412>

	mmc write 0 0x42000000 0x5c21 0x800

将写入EMMC的文件读出来对比(测试烧写是否正确)


	mmc read 0 0x40600000 0x7530 0x800
	cmp.b 0x40600000 0x42000000 0x800

## 启动

bootm + uImage + ramdisk + dtb

	bootm 0x40600000 0x41000000 0x42000000

## U-Boot环境变量

EMMC或SD卡里文件系统启动

	set bootcmd "movi read kernel 0 0x40600000; mmc read 0 0x41000000 0x3421 0x2800; mmc read 0 0x42000000 0x5c21 0x800; bootm 0x40600000 0x41000000 0x42000000"

	setenv bootargs 'root=/dev/ram0 rw rootfstype=ext4 console=ttySAC0,115200 ethmac=1C:6F:65:34:51:7E init=/linuxrc'

nfs网络文件系统启动

	set bootcmd "movi read kernel 0 0x40600000; mmc read 0 0x42000000 0x5c21 0x800; bootm 0x40600000 - 0x42000000"

	setenv bootargs 'root=/dev/nfs rw nfsroot=192.168.1.100:/home/zeroway/android/src/rootfs_for_tiny4412/rootfs ethmac=1C:6F:65:34:51:7E ip=192.168.1.230:192.168.1.100:192.168.1.1:255.255.255.0::eth0:off console=ttySAC0,115200 init=/linuxrc'

## 使用fastboot烧写镜像

### 烧写内核

4412>

	fastboot

PC>

	fastboot flash kernel uImage

### 烧写文件系统

4412>

	fastboot
PC>

	fastboot flash ramdisk ramdisk.img

## USB

### 网卡配置(dm9621)

BUG
需要冷启动才能正常使用,估计是复位异常,待排查

busybox ifconfig eth0 192.168.1.200 up

### 测试U盘挂载

	mount -o loop /dev/sda4 /mnt/

## 挂载网络文件系统

	mount -t nfs -o nolock,vers=2 192.168.1.100:/home/zeroway/android/src/rootfs_for_tiny4412/rootfs /mnt/

## 驱动实例

下载代码

	git clone https://github.com/54shady/kernel_drivers_examples.git -b tiny4412 --depth=1

[中断方式按键驱动](./debug/buttons/README.md)

[LED驱动](./debug/leds/README.md)
