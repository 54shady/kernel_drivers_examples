#RK3588 mipi camera

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

### vicap

- rk3588所有camera数据都要通过vicap在连接到isp
- rk3588只有一个vicap硬件
- vicap支持同时输出6路mipi phy和1路dvp,所以可以同时接7路摄像头
- 节点为rkcif_mipi_lvds~rkcif_mipi_lvds5, rkcif_dvp共7个节点

### 直通/回读

- 直通：指数据经过vicap采集,直接发送给isp处理,不存储到ddr
		hdr直通时只有短帧是真正的直通,长帧需要存在ddr中,isp再从ddr读
- 回读：指数据经过vicap采集到ddr,应用获取到数据后,将buffer的地址推送到isp
		isp再从ddr中获取数据

## sensor

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
&rkcif {
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

	i2c4->csi2_dphy0->mipi2_csi2->rkcif_mipi_lvds2->rkcif_mipi_lvds2_sditf->rkisp0_vir0

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

列出指定的设备

	media-ctl -d /dev/media1 -e "rkisp_mainpath"
	/dev/video8

	media-ctl -d /dev/media1 -e "rkisp_selfpath"
	/dev/video9

查询设备支持的格式和参数

	v4l2-ctl --device=/dev/video8 --list-formats-ext
	v4l2-ctl --device=/dev/video8 --list-ctrls

### 使用media-ctl来输出dot文件,后绘图

media0是sensor到vicap的pipeline

	media-ctl -d /dev/media0 --print-dot > media0.dot

media1是vicap到isp的pipeline

	media-ctl -d /dev/media1 --print-dot > media1.dot

将dot绘制成图片(可以修改dot文件中rankdir=LR)

	dot -Tpng media0.dot -o media0.png
	dot -Tpng media1.dot -o media1.png
