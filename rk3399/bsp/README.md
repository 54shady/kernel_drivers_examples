# BSP for Firefly_RK3399

## 分区表

![默认分区表](default_storage_map.png)

更新分区表方法1(烧写分区表文件)

	rkdeveloptool db rk3399_loader_v1.08.106.bin
	rkdeveloptool gpt parameter_gpt.txt

parameter_gpt.txt内容如下

	CMDLINE:
	mtdparts=rk29xxnand:0x00001f40@0x00000040(loader1),0x00000080@0x00001f80(reserved1),0x00002000@0x00002000(reserved2),0x00002000@0x00004000(loader2),0x00002000@0x00006000(atf),0x00038000@0x00008000(boot:bootable),-@0x0040000(rootfs)

更新分区表方法2(uboot中操作)

	=> env set partitions name=rootfs,size=-,type=system
	=> gpt write mmc 0 $partitions


更新分区表方法3(fastboot烧写,参考官网)

## U-Boot 2017.09

使用ROCKCHIP官方代码

	mkdir rk-linux
	cd rk-linux

	repo init --repo-url=https://github.com/rockchip-linux/repo -u https://github.com/rockchip-linux/manifests -b master
	repo sync

编译Uboot

	build/mk-uboot.sh rk3399-firefly

或者使用54shady的代码

[Firefly_RK3399_UBOOT_Usage](https://github.com/54shady/firefly_rk3399_uboot)

### 启动分析

启动参数涉及下面环境变量

	bootcmd=run distro_bootcmd

	distro_bootcmd=for target in ${boot_targets}; do run bootcmd_${target}; done

	boot_targets=mmc0 mmc1 usb0 pxe dhcp

	bootcmd_mmc0=setenv devnum 0; run mmc_boot

	mmc_boot=if mmc dev ${devnum}; then setenv devtype mmc; run scan_dev_for_boot_part; fi

	scan_dev_for_boot_part=part list ${devtype} ${devnum} -bootable devplist; env exists devpli st || setenv devplist 1; for distro_bootpart in ${devplist}; do if fstype ${devtype} ${devn um}:${distro_bootpart} bootfstype; then run scan_dev_for_boot; fi; done

	scan_dev_for_boot=echo Scanning ${devtype} ${devnum}:${distro_bootpart}...; for prefix in $ {boot_prefixes}; do run scan_dev_for_extlinux; run scan_dev_for_scripts; done;run scan_dev_ for_efi;

	scan_dev_for_extlinux=if test -e ${devtype} ${devnum}:${distro_bootpart} ${prefix}extlinux/ extlinux.conf; then echo Found ${prefix}extlinux/extlinux.conf; run boot_extlinux; echo SCR IPT FAILED: continuing...; fi

	boot_extlinux=sysboot ${devtype} ${devnum}:${distro_bootpart} any ${scriptaddr} ${prefix}extlinux/extlinux.conf

上面环境变量最终结果

	sysboot mmc 0:4 any 0x00500000 /extlinux/extlinux.conf

## Kernel (4.4.16)

获取代码

	git clone https://github.com/54shady/firefly_rk3399_kernel.git

编译出Image和dtb

	./mk-kernel.sh

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
