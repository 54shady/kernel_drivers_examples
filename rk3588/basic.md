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

接上hdmi前(vp2, connector mipi dis)

	cat /sys/class/drm/card0-DSI-1/status
	connected

	cat /sys/class/drm/card0-HDMI-A-1/status
	disconnected

	cat /sys/kernel/debug/dri/0/summary
	Video Port0: DISABLED
	Video Port1: DISABLED
	Video Port2: ACTIVE
		Connector: DSI-1
			bus_format[100a]: RGB888_1X24
			overlay_mode[0] output_mode[0] color_space[0], eotf:0
		Display mode: 1080x1920p60
			clk[132000] real_clk[132000] type[48] flag[a]
			H: 1080 1095 1099 1129
			V: 1920 1935 1937 1952
		Esmart2-win0: ACTIVE
			win_id: 9
			format: XR24 little-endian (0x34325258) SDR[0] color_space[0] glb_alpha[0xff]
			rotate: xmirror: 0 ymirror: 0 rotate_90: 0 rotate_270: 0
			csc: y2r[0] r2y[0] csc mode[0]
			zpos: 2
			src: pos[0, 0] rect[1080 x 1920]
			dst: pos[0, 0] rect[1080 x 1920]
			buf[0]: addr: 0x0000000000000000 pitch: 4320 offset: 0
	Video Port3: DISABLED

接上hdmi后(vp0, hdmi)

	cat /sys/class/drm/card0-HDMI-A-1/status
	connected

	cat /sys/class/drm/card0-DSI-1/status
	connected

	cat /sys/kernel/debug/dri/0/summary
	Video Port0: ACTIVE
    Connector: HDMI-A-1
        bus_format[100a]: RGB888_1X24
        overlay_mode[0] output_mode[f] color_space[0], eotf:0
    Display mode: 1920x1080p60
        clk[148500] real_clk[148500] type[48] flag[5]
        H: 1920 2008 2052 2200
        V: 1080 1084 1089 1125
    Esmart0-win0: ACTIVE
        win_id: 8
        format: XR24 little-endian (0x34325258) SDR[0] color_space[0] glb_alpha[0xff]
        rotate: xmirror: 0 ymirror: 0 rotate_90: 0 rotate_270: 0
        csc: y2r[0] r2y[0] csc mode[0]
        zpos: 0
        src: pos[0, 0] rect[1920 x 1080]
        dst: pos[0, 0] rect[1920 x 1080]
        buf[0]: addr: 0x0000000000e10000 pitch: 7680 offset: 0
	Video Port1: DISABLED
	Video Port2: DISABLED
	Video Port3: DISABLED

### DRM Card

SOC上的DSS(card0, card1)

	cat /sys/kernel/debug/dri/0/name
	rockchip dev=display-subsystem master=display-subsystem unique=display-subsystem

	/sys/class/drm/card0 -> ../../devices/platform/display-subsystem/drm/card0

对应的render信息

	ls -l /sys/class/drm/renderD128/device/
		driver -> ../../../bus/platform/drivers/rockchip-drm
		of_node -> ../../../firmware/devicetree/base/display-subsystem
		supplier:platform:fdd90000.vop -> ../../virtual/devlink/platform:fdd90000.vop--platform:display-subsystem
		supplier:platform:fde20000.dsi -> ../../virtual/devlink/platform:fde20000.dsi--platform:display-subsystem
		supplier:platform:fde80000.hdmi -> ../../virtual/devlink/platform:fde80000.hdmi--platform:display-subsystem

card0下的四个video port(hardware)

	video_port0
	video_port1
	video_port2
	video_port3

- crtc : 显示控制器

	card0下的四个显示控制器crtc(老版本的说法叫lcdc),是video port的软件抽象

		/sys/kernel/debug/dri/0/crtc-0
		/sys/kernel/debug/dri/0/crtc-1
		/sys/kernel/debug/dri/0/crtc-2
		/sys/kernel/debug/dri/0/crtc-3

- encoder: 指rgb, lvds, dsi, edp, hdmi, vga等显示接口
- connector : encoder和panel之间的接口部分
- panel：泛指屏幕,各种lcd显示设备的软件抽象
- plane : 在rk平台指soc内部vop(lcdc)模块win图层的抽象
- gem : drm下buffer管理和分配,类似ion, dma buffer

![drm1](./drm1.png)

![drm0](./drm0.png)

NPU Card1

	cat /sys/kernel/debug/dri/1/name
	rknpu dev=fdab0000.npu unique=fdab0000.npu

	/sys/class/drm/card1 -> ../../devices/platform/fdab0000.npu/drm/card1

NPU对应的render信息

	ls -l /sys/class/drm/renderD129/device/

	driver -> ../../../bus/platform/drivers/RKNPU
	iommu -> ../fdab9000.iommu/iommu/fdab9000.iommu
	iommu_group -> ../../../kernel/iommu_groups/0
	of_node -> ../../../firmware/devicetree/base/npu@fdab0000
	subsystem -> ../../../bus/platform
	supplier:i2c:2-0042 -> ../../virtual/devlink/i2c:2-0042--platform:fdab0000.npu
	supplier:platform:fd8d8000.power-management:power-controller -> ../../virtual/devlink/platform:fd8d8000.power-management:power-controller--platform:fdab0000.npu
	supplier:platform:fdab9000.iommu -> ../../virtual/devlink/platform:fdab9000.iommu--platform:fdab0000.npu
	supplier:platform:firmware:scmi -> ../../virtual/devlink/platform:firmware:scmi--platform:fdab0000.npu
	supplier:regulator:regulator.34 -> ../../virtual/devlink/regulator:regulator.34--platform:fdab0000.npu
