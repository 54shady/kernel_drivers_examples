# RK3588 Basic info

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

## Display info

接上hdmi前

	cat /sys/class/drm/card0-DSI-1/status
	connected

	cat /sys/class/drm/card0-HDMI-A-1/status
	disconnected

接上hdmi后

	cat /sys/class/drm/card0-HDMI-A-1/status
	connected

	cat /sys/class/drm/card0-DSI-1/status
	connected
