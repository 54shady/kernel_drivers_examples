# OpenCL on RK3588

## basic

computer vision (CNN) and generative AI (LLM)

[Install OpenCL on RK3588](https://www.roselladb.com/install-opencl-orangepi5-debian-ubuntu.htm)

[setting up opencl on rk3588](https://clehaxze.tw/gemlog/2023/06-17-setting-up-opencl-on-rk3588-using-libmali.gmi)

下载userspace-driver和gpu firmware到对应的目录,或者从[gitee-skrk下载](https://gitee.com/zeroway/skrk)

	cd /usr/lib
	sudo wget https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/libmali-valhall-g610-g6p0-x11-wayland-gbm.so

	cd /lib/firmware
	sudo wget https://github.com/JeffyCN/mirrors/raw/libmali/firmware/g610/mali_csffw.bin

安装对应的工具

	apt install -y mesa-opencl-icd clinfo
	apt install -y libxcb-dri2-0 libxcb-dri3-0 libwayland-client0 libwayland-server0 libx11-xcb1

下面这个是安装mesa-opencl-icd包时安装的(dpkg -L mesa-opencl-icd)

	cat /etc/OpenCL/vendors/mesa.icd
	libMesaOpenCL.so.1

平台名为Clover

	root@3588:~# clinfo -l
	Platform #0: Clover

执行clinfo查询(Number of devices 是 0)

	root@3588:~# clinfo

	Number of platforms                               1
	  Platform Name                                   Clover
	  Platform Vendor                                 Mesa
	  Platform Version                                OpenCL 1.1 Mesa 20.3.5
	  Platform Profile                                FULL_PROFILE
	  Platform Extensions                             cl_khr_icd
	  Platform Extensions function suffix             MESA

	  Platform Name                                   Clover
	Number of devices                                 0

	NULL platform behavior
	  clGetPlatformInfo(NULL, CL_PLATFORM_NAME, ...)  Clover
	  clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, ...)   No devices found in platform [Clover?]
	  clCreateContext(NULL, ...) [default]            No devices found in platform
	  clCreateContextFromType(NULL, CL_DEVICE_TYPE_DEFAULT)  No devices found in platform
	  clCreateContextFromType(NULL, CL_DEVICE_TYPE_CPU)  No devices found in platform
	  clCreateContextFromType(NULL, CL_DEVICE_TYPE_GPU)  No devices found in platform
	  clCreateContextFromType(NULL, CL_DEVICE_TYPE_ACCELERATOR)  No devices found in platform
	  clCreateContextFromType(NULL, CL_DEVICE_TYPE_CUSTOM)  No devices found in platform
	  clCreateContextFromType(NULL, CL_DEVICE_TYPE_ALL)  No devices found in platform

	ICD loader properties
	  ICD loader Name                                 OpenCL ICD Loader
	  ICD loader Vendor                               OCL Icd free software
	  ICD loader Version                              2.2.11
	  ICD loader Profile                              OpenCL 2.1

创建mali的icd文件

	mkdir -p /etc/OpenCL/vendors
	echo "/usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so" | sudo tee /etc/OpenCL/vendors/mali.icd

多了一个平台名为 ARM Platform的(该platorm有device)

	root@3588:~# clinfo -l
	Platform #0: ARM Platform
	arm_release_ver of this libmali is 'g6p0-01eac0', rk_so_ver is '7'.
	 `-- Device #0: Mali-LODX r0p0
	Platform #1: Clover

查询指定平台的设备信息

	root@3588:~# clinfo -d 0:0
	  Platform Name                                   ARM Platform
	arm_release_ver of this libmali is 'g6p0-01eac0', rk_so_ver is '7'.
	  Device Name                                     Mali-LODX r0p0
	  Device Vendor                                   ARM
	  Device Vendor ID                                0xa8670000
	  Device Version                                  OpenCL 2.1 v1.g6p0-01eac0.2819f9d4dbe0b5a2f89c835d8484f9cd
	  Device UUID                                     000067a8-0100-0000-0000-000000000000
	  Driver UUID                                     1e0cb80a-4d25-a21f-2c18-f7de010f1315

如果没有查询到新平台,需要检查下依赖

	ldd /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so

使用[clpeak测试选platform要选1, 或者删掉/etc/OpenCL/vendors/mesa.icd ](https://github.com/krrishnarraj/clpeak)

	./clpeak -p 1

## program

[RK3588 - OpenCL环境搭建](https://blog.csdn.net/Graceful_scenery/article/details/135783830)

download opencl(v2 and v3) header files

	wget https://www.roselladb.com/download/CLv2.zip
	wget https://www.roselladb.com/download/CLv3.zip

unzip the v2 to rk3588

	unzip CLv2.zip
	adb push CL /usr/include

compile [test program on rk3588](./test.c)

	gcc test.c -otest -lmali
	ldd test
		libmali.so.1 => /lib/aarch64-linux-gnu/libmali.so.1 (0x0000007fac1e3000)
	./test

未更新mali库和固件前默认情况如下,编译程序模式使用这个路径的库

	ls -l /usr/lib/aarch64-linux-gnu/libmali.so
		-> libmali.so.1.0.0 -> libmali-valhall-g610-g6p0-wayland.so
	ls -l /usr/lib/aarch64-linux-gnu/libOpenGL.so
		-> libOpenGL.so.0.0.0

更新库后需要修改一下连接

	rm /usr/lib/aarch64-linux-gnu/libmali.so.1.0.0
	ln -s /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/aarch64-linux-gnu/libmali.so.1.0.0
	ldconfig -v
