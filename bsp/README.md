# BSP for Firefly_RK3399

## U-Boot 2017.09

获取ROCKCHIP官方代码

	mkdir rk-linux
	cd rk-linux

	repo init --repo-url=https://github.com/rockchip-linux/repo -u https://github.com/rockchip-linux/manifests -b master
	repo sync

编译Uboot

	build/mk-uboot.sh rk3399-firefly

## Kernel (4.4.16)

获取代码,这里使用Firefly官网提供的android 6.0里的内核代码

编译出Image和dtb

拷贝到rk-linux/out/kernel目录下

再打包生成boot.img

	build/mk-image.sh -c rk3399 -t boot

## Rootfs

	ROCKCHIP官方提供的linaro-rootfs.img即可,或是自己制作

## Flash Image

使用rkdeveloptool烧写所有镜像

	rkdeveloptool db rk3399_loader_v1.08.106.bin
	rkdeveloptool wl 0x40 idbloader.img
	rkdeveloptool wl 0x4000 uboot.img
	rkdeveloptool wl 0x6000 trust.img
	rkdeveloptool wl 0x8000 boot.img
	rkdeveloptool wl 0x40000 linaro-rootfs.img
	rkdeveloptool rd

进入uboot后,写入gpt

	gpt write mmc 0 $partitions

文件build/extlinux/rk3399.conf内容如下

	label kernel-4.4
		kernel /Image
		fdt /rk3399-firefly-linux.dtb
		append  earlyprintk console=ttyFIQ0,115200 rw root=PARTUUID=b921b045-1d rootfstype=ext4 init=/sbin/init rootwait
