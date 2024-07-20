#RK3588 mipi camera

[isp30 development](Rockchip_Development_Guide_ISP30_CN_v1.2.3.pdf)

[video input guide](Rockchip_Driver_Guide_VI_CN_v1.1.1.pdf)

[camera trouble shooting](Rockchip_Trouble_Shooting_Linux4.4_Camera_CN.pdf)

[camera development](Rockchip_Developer_Guide_Linux4.4_Camera_CN.pdf)

## terminology

- 3A: AF自动对焦,AE自动曝光,AWB自动白平衡
- bayer raw(或raw bayer): 指sensort或isp输出的rggb, bggr, gbrg, grbg等帧格式
- iq(Image Quality): 指为bayer raw camera调试iq xml 用于3A tuning
	linux sdk:/etc/iqfiles/imx415_CMK-OT2022-PX1_IR0147-50IRC-8M-F20.json
- FCC: FourCC(Four Character Codes): 用四个字符来命名图像格式,存储在内存中的格式
	命令 v4l2-ctl --device=/dev/video8 --list-formats-ext 列出的就是FCC
- mbus-code (Media Bus Pixel Codes): 在物理总线上传输的格式,区别于FCC
	通过命令 media-ctl -d /dev/media0 -p 查看到
	entity 48: m00_b_imx415 4-001a (1 pad, 1 link)
		pad0: Source
			[fmt:SGBRG10_1X10/3864x2192@10000/300000 //这里就是mbus-code简写

	命令media-ctl --known-mbus-fmts 可以列出所有支持的mbus-code,

- 各种vdd
	dvdd : Digital core power
	dovdd : Digital I/O power
	avdd : Analog power

## basic(下列节点分别在rk3588s.dtsi和rk3588.dtsi)

### rk3588的mipi phy 有 dcphy 和 dphy

#### dcphy

rk3588支持两个dcphy,节点为csi2_dcphy0/csi2_dcphy1
每个dcphy支持rx/tx同时使用(对于camera输入使用的是rx)
支持dphy/cphy协议复用(同一个dcphy的rx/tx同一时刻只能是dphy协议或是cphy协议)

#### dphy

rk3588支持两个dphy,节点为csi2_dphy0_hw/csi2_dphy1_hw

### csi2

每一个mipi phy都需要一个csi2模块来解析mipi协议
节点为mipi0_csi2~mipi5_csi2共6个

### vicap(VI, VIP: Video Input Processor)

- rk3588所有camera数据都要通过vicap在连接到isp
- rk3588只有一个vicap硬件
- vicap支持同时输出6路mipi phy和1路dvp,所以可以同时接7路摄像头
- 节点为rkcif_mipi_lvds~rkcif_mipi_lvds5, rkcif_dvp共7个节点

### 直通/回读

- 直通：指数据经过vicap采集,直接发送给isp处理,不存储到ddr
		hdr直通时只有短帧是真正的直通,长帧需要存在ddr中,isp再从ddr读
- 回读：指数据经过vicap采集到ddr,应用获取到数据后,将buffer的地址推送到isp
		isp再从ddr中获取数据

## sensor(drivers/media/i2c/imx415.c)

&i2c4 {
	status = "okay";
	imx415: imx415@1a {
		compatible = "sony,imx415";
		port {
			ucam_out0: endpoint {
				remote-endpoint = <&mipi_in_ucam0>;// dphy
				data-lanes = <1 2 3 4>;
			};
		};
	};
};

通过 media-ctl 获取拓扑如果看到sensor有注册entity说明注册成功

## dphy

&csi2_dphy0 {
	status = "okay";
	ports {
		port@0 {
			mipi_in_ucam0: endpoint@1 {
				reg = <1>;
				remote-endpoint = <&ucam_out0>; // sensor
				data-lanes = <1 2 3 4>;
			};
		};
		port@1 {
			csidphy0_out: endpoint@0 {
				reg = <0>;
				remote-endpoint = <&mipi2_csi2_input>;// csi host
			};
		};
	};
};

使能物理dphy0

	csi2_dphy0: csi2-dphy0 { //csi2_dphy0是逻辑节点,依赖下面的物理节点
		compatible = "rockchip,rk3568-csi2-dphy";
		rockchip,hw = <&csi2_dphy0_hw>; //依赖的物理节点,所以下面也要开
		status = "disabled";
	};
	&csi2_dphy0_hw {
		status = "okay";
	};

## csi host

&mipi2_csi2 {
	status = "okay";
	ports {
		port@0 {
			mipi2_csi2_input: endpoint@1 {
				reg = <1>;
				remote-endpoint = <&csidphy0_out>; //dphy
			};
		};
		port@1 {
			mipi2_csi2_output: endpoint@0 {
				reg = <0>;
				remote-endpoint = <&cif_mipi_in2>; //cif
			};
		};
	};
};

## vicap

&rkcif_mipi_lvds2 { //是vicap的一个逻辑节点,物理rkcif和iommu也要配置
	status = "okay";
	port {
		cif_mipi_in2: endpoint {
			remote-endpoint = <&mipi2_csi2_output>;
		};
	};
};
&rkcif { //将sensor数据保存到ddr中,仅转存数据
	status = "okay";
};
&rkcif_mmu {
	status = "okay";
};

&rkcif_mipi_lvds2_sditf { //是rkcif_mipi_lvds2的虚拟节点,用来连接isp
	status = "okay";
	port {
		mipi_lvds_sditf: endpoint {
			remote-endpoint = <&isp0_vir0>;
		};
	};
};

&rkisp0_vir0 {
	status = "okay";
	port {
		#address-cells = <1>;
		#size-cells = <0>;
		isp0_vir0: endpoint@0 {
			reg = <0>;
			remote-endpoint = <&mipi_lvds_sditf>;
		};
	};
};

## 最终连接情况

	i2c4->csi2_dphy0->mipi2_csi2->rkcif_mipi_lvds2->rkcif_mipi_lvds2_sditf->rkisp0_vir2
	i2c5->csi2_dcphy0->mipi0_csi2->rkcif_mipi_lvds->rkcif_mipi_lvds_sditf->rkisp0_vir0

## debug(使用工具查看连接情况)

### 判断rkisp驱动加载状态

	rkisp驱动加载成功会有videoX, mediaX在/dev/下

### 判断sensor是否加载成功

	dmesg | grep Async
	[    2.306725] rockchip-mipi-csi2: Async registered subdev
	[    2.475497] rkcif-mipi-lvds2: Async subdev notifier completed
	[    2.587220] rkisp0-vir0: Async subdev notifier completed

查看所有摄像头设备,及其作用

	v4l2-ctl --list-devices

	rkisp-statistics (platform: rkisp):
			/dev/video15
			/dev/video16

	rkcif-mipi-lvds2 (platform:rkcif):
			/dev/media0

	rkcif (platform:rkcif-mipi-lvds2):
			/dev/video0
			/dev/video1
			/dev/video2
			/dev/video3
			/dev/video4
			/dev/video5
			/dev/video6
			/dev/video7

	rkisp_mainpath (platform:rkisp0-vir0):
			/dev/video8
			/dev/video9
			/dev/video10
			/dev/video11
			/dev/video12
			/dev/video13
			/dev/video14
			/dev/media1

列出拍照设备MP/SP(rkisp有两个视频输出设备mainpath,selfpath都能输出图像)

	media-ctl -d /dev/media1 -e "rkisp_mainpath"
	/dev/video8

	media-ctl -d /dev/media1 -e "rkisp_selfpath"
	/dev/video9

查询设备支持的格式和参数

	v4l2-ctl --device=/dev/video8 --list-formats-ext
	v4l2-ctl --device=/dev/video8 --list-ctrls

通过命令 media-ctl -d /dev/media1 -p 查询到sensor对应的设备(/dev/v4l-subdev2)

	entity 48: m00_b_imx415 4-001a (1 pad, 1 link)
				 type V4L2 subdev subtype Sensor flags 0
				 device node name /dev/v4l-subdev2

查看该设备的参数如下

	v4l2-ctl -d /dev/v4l-subdev2 -l

	User Controls

						   exposure 0x00980911 (int)    : min=4 max=2242 step=1 default=2242 value=203
					horizontal_flip 0x00980914 (bool)   : default=0 value=0
					  vertical_flip 0x00980915 (bool)   : default=0 value=0

	Image Source Controls

				  vertical_blanking 0x009e0901 (int)    : min=58 max=30575 step=1 default=58 value=58
				horizontal_blanking 0x009e0902 (int)    : min=4936 max=4936 step=1 default=4936 value=4936 flags=read-only
					  analogue_gain 0x009e0903 (int)    : min=0 max=240 step=1 default=0 value=0

	Image Processing Controls

					 link_frequency 0x009f0901 (intmenu): min=0 max=3 default=0 value=1
						 pixel_rate 0x009f0902 (int64)  : min=0 max=712800000 step=1 default=356800000 value=356800000 flags=read-only

获取并设置参数

	v4l2-ctl -d /dev/v4l-subdev2 --get-ctrl exposure
	v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl exposure=2000

### 使用media-ctl来输出dot文件,后绘图

media0是sensor到vicap的pipeline

	media-ctl -d /dev/media0 --print-dot > media0.dot

media1是vicap到isp的pipeline

	media-ctl -d /dev/media1 --print-dot > media1.dot

将dot绘制成图片(可以修改dot文件中rankdir=LR)

	dot -Tpng media0.dot -o media0.png
	dot -Tpng media1.dot -o media1.png

### 拍照测试

在板上拍照

	v4l2-ctl --verbose -d /dev/video8 \
		--set-fmt-video=width=1920,height=1080,pixelformat='NV12' \
		--stream-mmap=4 \
		--set-selection=target=crop,flags=0,top=0,left=0,width=1920,height=1080 \
		--stream-count=10 \
		--stream-to=/data/out.yuv

在x86host上显示

	ffplay -f rawvideo -video_size 1920x1080 -pix_fmt nv12 out.yuv
	W=1920;H=1080; mplayer out.yuv -loop 0 -demuxer rawvideo -fps 30 -rawvideo w=${W}:h=${H}:size=$((${W}*${H}*2)):format=NV12
