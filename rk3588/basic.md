# RK3588 Basic info

## Basic

### VPU

[HWA Tutorial On Rockchip VPU](https://jellyfin.org/docs/general/administration/hardware-acceleration/rockchip/)

VPU : is responsible for decoding and encoding video
GPU : is only responsible for graphics and computes, used for OpenCL-based HDR tone-mapping.
RGA : is Rockchip's 2D post-processing unit, used for video scaling, pixel format conversion, subtitle burn-in, etc.

### DeviceTree

device tree on bard

	ls /sys/firmware/devicetree/base/
	ls /proc/device-tree/

convert on board device tree

	dtc -I fs -O dts /proc/device-tree/ -o a.dts
	dtc -I fs -O dts /sys/firmware/devicetree/base -o b.dts

## CPU info

### 基本信息

在rk3588中,cpu如下分簇

	CPU 0 to 3 are Cortex-A55 cores
	CPU 4-5 are two Cortex-A76 cores (cluster 1)
	CPU 6-7 are two Cortex-A76 cores (cluster2)

### 安装软件

安装测试软件

	apt install -y cpufrequtils p7zip

### 信息查询

查看各cpu频率

	cpufreq-info

如下展示cpu4的信息,408MHz-2.30Ghz

	...
	analyzing CPU 4:
	driver: cpufreq-dt
	CPUs which run at the same hardware frequency: 4 5
	CPUs which need to have their frequency coordinated by software: 4 5
	maximum transition latency: 324 us.
	hardware limits: 408 MHz - 2.30 GHz
	available frequency steps: 408 MHz, 600 MHz, 816 MHz, 1.01 GHz, 1.20 GHz, 1.42 GHz, 1.61 GHz, 1.80 GHz, 2.02 GHz, 2.21 GHz, 2.30 GHz
	available cpufreq governors: conservative, ondemand, userspace, powersave, performance, schedutil
	current policy: frequency should be within 408 MHz and 2.30 GHz.
				  The governor "ondemand" may decide which speed to use
				  within this range.
	current CPU frequency is 408 MHz (asserted by call to hardware).
	...

使用7z跑一下压力测试

	7zr b

查看cpu的温度和cpu7的频率

	watch -n 0.1 cat /sys/class/thermal/thermal_zone0/temp
	watch -n 0.1 cpufreq-info -f -c 7

查看cpu电源情况[pvtm](https://www.cnx-software.com/2022/07/17/what-is-pvtm-or-why-your-rockchip-rk3588-cpu-may-not-reach-2-4-ghz/)

	dmesg | grep -E 'pvtm|dmc' | grep -E 'pvtm=|sel='

	grep -E 'pvtm|dmc' | grep -E 'pvtm=|sel=' /var/log/kern.log

会有看到以下几种情况的信息

	//没有风扇的机器B
	cpu cpu0: pvtm=1492
	cpu cpu0: pvtm-volt-sel=4
	cpu cpu4: pvtm=1725
	cpu cpu4: pvtm-volt-sel=5
	cpu cpu6: pvtm=1743
	cpu cpu6: pvtm-volt-sel=5
	mali fb000000.gpu: pvtm=902
	mali fb000000.gpu: pvtm-volt-sel=4
	RKNPU fdab0000.npu: pvtm=905
	RKNPU fdab0000.npu: pvtm-volt-sel=4

	//没有风扇的机器B
	cpu cpu0: pvtm=1490
	cpu cpu0: pvtm-volt-sel=4
	cpu cpu4: pvtm=1727
	cpu cpu4: pvtm-volt-sel=5
	cpu cpu6: pvtm=1744
	cpu cpu6: pvtm-volt-sel=6
	mali fb000000.gpu: pvtm=902
	mali fb000000.gpu: pvtm-volt-sel=4
	RKNPU fdab0000.npu: pvtm=904
	RKNPU fdab0000.npu: pvtm-volt-sel=4

	//带有风扇的机器A
	cpu cpu0: pvtm=1528
	cpu cpu0: pvtm-volt-sel=5
	cpu cpu4: pvtm=1748
	cpu cpu4: pvtm-volt-sel=6
	cpu cpu6: pvtm=1783
	cpu cpu6: pvtm-volt-sel=7
	mali fb000000.gpu: pvtm=914
	mali fb000000.gpu: pvtm-volt-sel=5
	RKNPU fdab0000.npu: pvtm=915
	RKNPU fdab0000.npu: pvtm-volt-sel=5

参考代码arch/arm64/boot/dts/rockchip/rk3588s.dtsi中如下表格

	    cluster2_opp_table: cluster2-opp-table {
			rockchip,pvtm-voltage-sel = <
				/* pvtm value start, pvtm value end, pvtm sel */
				0   1595    0 // 2256Mhz
				1596    1615    1
				1616    1640    2
				1641    1675    3
				1676    1710    4
				1711    1743    5
				1744    1776    6 //2352Mhz
				1777    9999    7 //2400Mhz
			>;
		}

OPP[rockchip cpufreq](https://github.com/54shady/qop_kernel/blob/master/Documentation/devicetree/bindings/cpufreq/cpufreq-rockchip.txt)

## RKMPI

[IPC（网络摄像机）应用框架](https://wiki.luckfox.com/zh/Luckfox-Pico/RKMPI-example)

![ipc](./ipc.png)

- VI 模块捕获视频图像，做剪切、缩放等处理，可以输出多路不同分辩率的图像数据。
- VPSS(视频处理子系统) 模块接收 VI 模块传输的图像
	可对图像进行裁剪、缩放、旋转、像素格式转换等处理。
- VENC 模块可以直接接收 VI 模块捕获的图像或 VPSS 处理后输出的图像数据,
	可叠加用户通过 RGN 模块设置的 OSD 图像，然后按不同协议进行编码并输出相应码流。
- 各个模块通道之间进行绑定，处理后的数据流会直接传输到绑定的下一个多媒体模块。

## MISC

查看脏页达到系统内存多少比例时才刷入磁盘

	cat /proc/sys/vm/dirty_background_bytes
	10

对于系统内存是4G的话

	4 * 10% = 400MB 当脏页达到400MB时才会将缓存刷入存储介质

	比如在使用dd 测试是可以添加conv=fsync来将缓存刷入存储介质

所以测试磁盘读写速度是需要测试的文件比较大,否则测试的速度只是写入到缓存
