# Display info

## 系统中的信息

系统启动日志有如下两条,分别对应card0和card1

	[drm] Initialized rockchip 3.0.0 20140818 for display-subsystem on minor 0
	[drm] Initialized rknpu 0.7.2 20220428 for fdab0000.npu on minor 1

接上hdmi前(vp2, connector mipi dis)

	cat /sys/class/drm/card0-DSI-1/status
	connected

	cat /sys/class/drm/card0-HDMI-A-1/status
	disconnected

	cat /sys/class/drm/card0-HDMI-A-1/enabled
	disabled

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


安装相应的软件查看connector信息(需要停止xorg或wayland)

	apt install -y libdrm-tests

	pkill Xwayland
	modetest -M rockchip -c

	Connectors:
	id      encoder status          name            size (mm)       modes   encoders
	185     0       disconnected    HDMI-A-1        0x0             0       184
	195     194     connected       DSI-1           0x0             1       194
	...

可以看到和当前connector(185)关联的encoder是184

查看当前的encoder

	modetest -M rockchip -e

	Encoders:
	id      crtc    type    possible crtcs  possible clones
	182     0       Virtual 0x0000000f      0x00000001
	184     0       TMDS    0x00000001      0x00000002
	194     102     DSI     0x00000004      0x00000004

可以看到有3个encoder

	182(Virtual) 和 184(TMDS) 连接的crtc是 0
	194(DSI) 连接的 crtc 是 194

查看plane情况(可以看到有4个crtc分别对应vp0~vp3)

	modetest -M rockchip -p | grep id -A2

	CRTCs:
	id      fb      pos     size
	68      0       (0,0)   (1920x1080)
	85      0       (0,0)   (0x0)
	102     199     (0,0)   (1080x1920)
	119     0       (0,0)   (0x0)
	--
	Planes: //比较多,不全部列出
	id      crtc    fb      CRTC x,y        x,y     gamma size      possible crtcs
	54      0       0       0,0             0,0     0               0x0000000f

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

再次查看crtc信息

	modetest -M rockchip -p | grep id -A2

	modetest -M rockchip -p | grep id -A2
	id      fb      pos     size
	68      199     (0,0)   (1920x1080)
	  #0 1920x1080 60.00 1920 2008 2052 2200 1080 1084 1089 1125 148500 flags: phsync, pvsync; type: preferred, driver
	--
	id      crtc    fb      CRTC x,y        x,y     gamma size      possible crtcs
	54      68      199     0,0             0,0     0               0x0000000f
	  formats: XR24 AR24 XB24 AB24 RG24 BG24 RG16 BG16 NV12 NV21 NV16 NV61 NV24 NV42 NV15 NV20 NV30 YVYU VYUY YUYV UYVY

设置分辨率(显示屏上显示彩条纹)

	modetest -M rockchip -s 185@68:1920x1080-60

- 185是上面查询到的hdmi connector的id
- 68是上面查询到的crtc的id

再查看connector的情况(和未接显示器比已经看到连接了encoder 184)

	modetest -M rockchip -c

	Connectors:
	id      encoder status          name            size (mm)       modes   encoders
	185     184     connected       HDMI-A-1        530x300         29      184

查看当前的encoder

	modetest -M rockchip -e

	Encoders:
	id      crtc    type    possible crtcs  possible clones
	182     0       Virtual 0x0000000f      0x00000001
	184     68      TMDS    0x00000001      0x00000002
	194     102     DSI     0x00000004      0x00000004

和上面对比,这里184已经和68连接上了

### EDID

	apt install edid-decode
	cat /sys/class/drm/card0-HDMI-A-1/edid > /root/hdmi-edid.bin
	edid-decode /root/hdmi-edid.bin

## DRM Card

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


drm encoder 的种类(drivers/gpu/drm/drm_encoder.c)

	static const struct drm_prop_enum_list drm_encoder_enum_list[] = {
		{ DRM_MODE_ENCODER_NONE, "None" },
		{ DRM_MODE_ENCODER_DAC, "DAC" },
		{ DRM_MODE_ENCODER_TMDS, "TMDS" },
		{ DRM_MODE_ENCODER_LVDS, "LVDS" },
		{ DRM_MODE_ENCODER_TVDAC, "TV" },
		{ DRM_MODE_ENCODER_VIRTUAL, "Virtual" },
		{ DRM_MODE_ENCODER_DSI, "DSI" },
		{ DRM_MODE_ENCODER_DPMST, "DP MST" },
		{ DRM_MODE_ENCODER_DPI, "DPI" },
	};

在rockchip virtual vop base on vkms (drivers/gpu/drm/rockchip/rockchip_drm_vvop.c) 驱动中注册了这个虚拟的encoder

	ret = drm_encoder_init(drm_dev, encoder, &vvop_encoder_funcs,
			       DRM_MODE_ENCODER_VIRTUAL, NULL);

通过配置选项CONFIG_DRM_ROCKCHIP_VVOP来控制编译,但是代码中没有编译

代码drivers/gpu/drm/drm_writeback.c 编译了

	ret = drm_encoder_init(dev, &wb_connector->encoder,
			       &drm_writeback_encoder_funcs,
			       DRM_MODE_ENCODER_VIRTUAL, NULL);

可以从系统中看到对应的writeback信息

	/sys/devices/platform/display-subsystem/drm/card0/card0-Writeback-1
	/sys/class/drm/card0-Writeback-1 -> ../../devices/platform/display-subsystem/drm/card0/card0-Writeback-1

## 代码中的信息

在arch/arm64/boot/dts/rockchip/rk3588s.dtsi中vop配置如下(省去部分)

	vop: vop@fdd90000 {
		compatible = "rockchip,rk3588-vop";
		...
		vop_out: ports {
			#address-cells = <1>;
			#size-cells = <0>;

			vp0: port@0 {
				//配置video port 0的连接信息
				//这里可以连接到dp0, (edp0, hdmi0)上,其中edp0和hdmi0二选一
				vp0_out_dp0: endpoint@0 {
					reg = <0>;
					remote-endpoint = <&dp0_in_vp0>;
				};

				vp0_out_edp0: endpoint@1 {
					reg = <1>;
					remote-endpoint = <&edp0_in_vp0>;
				};

				vp0_out_hdmi0: endpoint@2 {
					reg = <2>;
					remote-endpoint = <&hdmi0_in_vp0>;
				};
			};

			vp1: port@1 {
				//配置video port 1的连接信息
				//这里可以连接到dp0, (edp0, hdmi0)上,其中edp0和hdmi0二选一
				vp1_out_dp0: endpoint@0 {
					reg = <0>;
					remote-endpoint = <&dp0_in_vp1>;
				};

				vp1_out_edp0: endpoint@1 {
					reg = <1>;
					remote-endpoint = <&edp0_in_vp1>;
				};

				vp1_out_hdmi0: endpoint@2 {
					reg = <2>;
					remote-endpoint = <&hdmi0_in_vp1>;
				};
			};

			vp2: port@2 {
				//配置video port 2的连接信息
				//这里可以连接到dp0, (edp0, hdmi0), dsi0, dsi1上,其中edp0和hdmi0二选一
				vp2_out_dp0: endpoint@0 {
					reg = <0>;
					remote-endpoint = <&dp0_in_vp2>;
				};

				vp2_out_edp0: endpoint@1 {
					reg = <1>;
					remote-endpoint = <&edp0_in_vp2>;
				};

				vp2_out_hdmi0: endpoint@2 {
					reg = <2>;
					remote-endpoint = <&hdmi0_in_vp2>;
				};

				vp2_out_dsi0: endpoint@3 {
					reg = <3>;
					remote-endpoint = <&dsi0_in_vp2>;
				};

				vp2_out_dsi1: endpoint@4 {
					reg = <4>;
					remote-endpoint = <&dsi1_in_vp2>;
				};
			};

			vp3: port@3 {
				//配置video port 3的连接信息
				//这里可以连接到dsi0, dsi1, rgb上
				vp3_out_dsi0: endpoint@0 {
					reg = <0>;
					remote-endpoint = <&dsi0_in_vp3>;
				};

				vp3_out_dsi1: endpoint@1 {
					reg = <1>;
					remote-endpoint = <&dsi1_in_vp3>;
				};

				vp3_out_rgb: endpoint@2 {
					reg = <2>;
					remote-endpoint = <&rgb_in_vp3>;

上面的代码信息对应如下![vop图](./vop.png)

## MIPI-DSI

以单通道mipi dsi(vp3->dsi0->panel)配置连接

	framebuff==>  vop[vp3_out_dsi0]--route_dsi0-->[dsi0_in_vp3]mipi-dsi[dis_out_panel]->[panel_in_dsi]panel

### dts配置

配置vp3到dis0

	&route_dsi0 {
		connect = <&vp3_out_dsi0>;
	};

vop中vp3连接dsi0_in_vp3

	vop: vop@fdd90000 {
		vop_out: ports {
			vp3: port@3 {
				vp3_out_dsi0: endpoint@0 {
					remote-endpoint = <&dsi0_in_vp3>;

dsi0中的连接信息

	dsi0: dsi@fde20000 {
		phys = <&mipi_dcphy0>; //使用的dcphy0
		ports {
			dsi0_in: port@0 { // dsi in 连vop
				dsi0_in_vp3: endpoint@1 {
					remote-endpoint = <&vp3_out_dsi0>;
		    dsi0_out: port@1 { // dsi out 连 panel
						dsi_out_panel: endpoint {
							remote-endpoint = <&panel_in_dsi>;

### basic

上面的连接在系统中如下

	cat /sys/kernel/debug/dri/0/summary

	Video Port0: DISABLED
	Video Port1: DISABLED
	Video Port2: DISABLED
	Video Port3: ACTIVE
		Connector: DSI-1
			bus_format[100a]: RGB888_1X24
			overlay_mode[0] output_mode[0] color_space[0], eotf:0
		Display mode: 1024x600p71
			clk[60000] real_clk[60000] type[48] flag[a]
			H: 1024 1160 1164 1324
			V: 600 616 618 634
		Esmart3-win0: ACTIVE
			win_id: 11
			format: XR24 little-endian (0x34325258) SDR[0] color_space[0] glb_alpha[0xff]
			rotate: xmirror: 0 ymirror: 0 rotate_90: 0 rotate_270: 0
			csc: y2r[0] r2y[0] csc mode[0]
			zpos: 3
			src: pos[0, 0] rect[1024 x 600]
			dst: pos[0, 0] rect[1024 x 600]
			buf[0]: addr: 0x00000000ee29a000 pitch: 4096 offset: 0

查看vop, dsi, mipi clock信息

	cat /sys/kernel/debug/clk/clk_summary | grep vop
	cat /sys/kernel/debug/clk/clk_summary | grep dsi
	cat /sys/kernel/debug/clk/clk_summary | grep mipi

查看drm buffer使用情况

	cat /sys/kernel/debug/dri/0/mm_dump

查看详细信息

	cat /sys/kernel/debug/dri/0/state

	...
	crtc[119]: video_port3
	...
			mode: "1024x600": 71 60000 1024 1160 1164 1324 600 616 618 634 0x48 0xa
	connector[195]: DSI-1
			crtc=video_port3

设置分辨率(在adb shell中输入下面命令显示屏上显示彩条纹)

	pkill Xwayland
	modetest -M rockchip -s 195@119:1024x600
	//根据上面命令得到需要设置71.48Hz
	modetest -M rockchip -s 195@119:1024x600-71.48

查看当前显示分辨率

	cat /sys/class/drm/card0-DSI-1/modes

强行开关显示设备

	echo on > /sys/class/drm/card0-DSI-1/status
	echo off > /sys/class/drm/card0-DSI-1/status

打开drm调试信息

	echo 0xff > /sys/module/drm/parameters/debug

## 带宽

### 图像带宽计算

1080p ARGB图像带宽计算

- ARGB格式一个像素点占用内存带宽是4Byte
- 1080p占用内存 = (1920 x 1080 x 4) / 1024 = 8100 KB
- 刷新频率是60fps时占用带宽 = 8100 x 60 = 474.6 MB/s

如果数据格式换成YUV420SP(NV12)

- NV12格式一个像素点占用内存带宽是1.5Byte (12bit)
- 1080p占用内存 = (1920 x 1080 x 1.5) / 1024 = 3037.5 KB
- 刷新频率是60fps时占用带宽 = 3037.5 x 60 = 178 MB/s

### 显示接口带宽计算

假设有如下显示时序

		disp_timings0: display-timings {
			native-mode = <&dsi0_timing0>;
			dsi0_timing0: timing0 {
                clock-frequency = <60000000>;
                hactive = <1024>;
                vactive = <600>;
                hback-porch = <160>;
                hfront-porch = <136>;
                vback-porch = <16>;
                vfront-porch = <16>;
                hsync-len = <4>;
                vsync-len = <2>;
				hsync-active = <0>;
				vsync-active = <0>;
				de-active = <0>;
				pixelclk-active = <1>;
			};
		};

	htotal = hfp + hsync + hbp + hactive = 136 + 4 + 160 + 1024 = 1324
	vtotal = vfp + vsync + vbp + vactive = 16 + 2 + 16 + 600 = 634
	按60fps算dclk(每个像素点时钟) = htotal x vtotal x fps = 1324 x 634 x 60 = 50364960 Hz
	dts里实际按取整配置60000000 (60Mhz)

对于一个4lane的mipi接口上传输的频率是

	60 (Mhz) * 3 (RGB) * 8 (bpc) / 4 (lane) / 0.9 (mipi传输效率) = 400 Mbps

	一个像素由rgb 3个分量,每个分量是8 bit,一共 4lane mipi
