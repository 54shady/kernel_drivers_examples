# Camera in linux

[参考文档 RKISP_Driver_User_Manual](RKISP_Driver_User_Manual)

[参考文档 Camera_Engine_Rkisp_User_Manual](Camera_Engine_Rkisp_User_Manual)

![RKISP1 topography ](./rkisp1.png)

## CIS(cmos image sensor)摄像头驱动

查看驱动版本信息(dmesg | grep rkisp)

	rkisp1 ff910000.rkisp1: rkisp1 driver version: v00.01.02

或查看节点信息

	cat /sys/module/video_rkisp1/parameters/version

### MIPI CIS注册

下面以isp0和ov13850为例进行说明(只做链路联通实例,其余信息省略)

连接情况sensor<-->mipi_dphy<-->isp

	//sensor
	ov13850:ov13850@10 {
		port {
			ucam_out0: endpoint {
			remote-endpoint = <&mipi_in_ucam0>; //mipi dphy端的port名
			data-lanes = <1>; // 1lane
			//data-lanes = <1 2>; // 2lane
			//data-lanes = <1 2 3 4>; // 4lane
		};
	};

	//mipi dphy
	&mipi_dphy_rx0 {
		port {
			port@0 {
				mipi_in_ucam0: endpoint@1 {
					remote-endpoint = <&ucam_out0>; //sensor端的port名
				};
			};
			port@1 {
				dphy_rx0_out: endpoint@0 {
					remote-endpoint = <&isp0_mipi_in>; //isp端的port名
				};
			};
		};
	};

	//isp
	&rkisp1_0 {
		port {
			isp0_mipi_in: endpoint@0 {
				remote-endpoint = <dphy_rx0_out>; //mipi dphy端的port名
			};
		};
	};

	&isp0_mmu {
		status = "okay"; // isp驱动使用了iommu,所以isp iommu也需要打开
	};

### DVP CIS注册

两个DVP连接到一个ISP上(前,后摄像头)

	// dvp sensor1
	gc0312@21 {
		port {
			gc0312_out: endpoint {
				remote-endpoint = <&dvp_in_fcam>;// isp端的port名
			};
		};
	};

	// dvp sensor2
	gc2145@3c {
		port {
			gc2145_out: endpoint {
				remote-endpoint = <&dvp_in_bcam>;// isp 端的 port 名
			};
		};
	};

	// isp
	&rkisp1 {
		ports {
			port@0 {
				dvp_in_fcam: endpoint@0 {
					remote-endpoint = <&gc0312_out>;  // sensor端port名
				};
				dvp_in_bcam: endpoint@1 {
					remote-endpoint = <&gc2145_out>;  // sensor端port名
				};
			};
		};
	};

## 摄像头应用

![camera engine framework](./camera_engine.png)

Buildroot中手动编译camera_engine

	make -C /path/to/buildroot O=/path/to/buildroot/output/rockchip_rk3399 camera_engine_rkisp-rebuild

Buildroot编译中间产物目录/path/to/buildroot/output/rockchip_rk3399/build
Buildroot目标文件系统目录/path/to/buildroot/output/rockchip_rk3399/target

拷贝生成的文件到目标文件系统中

	camera_engine_rkisp-1.0/build/lib/librkisp.so ==> /path/to/target/usr/lib/
	camera_engine_rkisp-1.0/plugins/3a/rkiq/af/lib64/librkisp_af.so ==> usr/lib/rkisp/af/
	camera_engine_rkisp-1.0/plugins/3a/rkiq/aec/lib64/librkisp_aec.so ==> usr/lib/rkisp/ae/
	camera_engine_rkisp-1.0/plugins/3a/rkiq/awb/lib64/librkisp_awb.so ==> usr/lib/rkisp/awb/
	camera_engine_rkisp-1.0/build/lib/libgstrkisp.so ==> usr/lib/gstreamer-1.0/

buildroot SDK中(buildroot/package/rockchip/camera_engine_rkisp/camera_engine_rkisp.mk)已完成上述拷贝

	$(INSTALL) -D -m 644 $(@D)/build/lib/librkisp.so $(TARGET_DIR)/usr/lib/
	$(INSTALL) -D -m 644 $(@D)/plugins/3a/rkiq/af/$(CAMERA_ENGINE_RKISP_LIB)/librkisp_af.so $(RKafDir)/
	$(INSTALL) -D -m 644 $(@D)/plugins/3a/rkiq/aec/$(CAMERA_ENGINE_RKISP_LIB)/librkisp_aec.so $(RKaeDir)/
	$(INSTALL) -D -m 644 $(@D)/plugins/3a/rkiq/awb/$(CAMERA_ENGINE_RKISP_LIB)/librkisp_awb.so $(RKawbDir)/
	$(INSTALL) -D -m 644 $(@D)/build/lib/libgstrkisp.so $(RKgstDir)/

IQ文件会全部拷贝到etc/iqfiles目录下

	$(INSTALL) -D -m 644 $(@D)/iqfiles/*.xml $(TARGET_DIR)/etc/iqfiles/

当系统启动后,如下脚本来匹配当前连接的sensor来设置链接/etc/cam_iq.xml

	/etc/init.d/S50set_pipeline start

### Test mipi camera on Buildroot

Test mipi camera using gstreamer(camera_rkisp.sh)

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/gstreamer-1.0
	gst-launch-1.0 rkisp device=/dev/video0 io-mode=1 analyzer=1 enable-3a=1 path-iqf=/etc/cam_iq.xml ! video/x-raw,format=NV12,width=640,height=480, framerate=30/1 ! videoconvert ! autovideosink


Dump picture to file(using 7yuv to view the file)

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/gstreamer-1.0
	gst-launch-1.0 rkisp device=/dev/video0 io-mode=1 analyzer=1 enable-3a=1 path-iqf=/etc/cam_iq.xml ! video/x-raw,format=NV12,width=640,height=480, framerate=30/1 ! videoconvert ! filesink location=/tmp/streamer.yuv

Dump picture to file(using 7yuv to view the file)

	rkisp_demo --device=/dev/video0 --output=/tmp/streamer.yuv
