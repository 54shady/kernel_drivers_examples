# 在android上使用gdb调试

## android源码中的gdb和gdbserver工具

gdbserver(GNU gdbserver (GDB) 7.6)

	prebuilts/misc/android-arm/gdbserver/gdbserver

gdb(GNU gdb (GDB) 7.6)

	prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-gdb

## 应用调试(使用ADB转发端口连接)

### 在target上准备被调试程序

在开发板上启动gdbserver监听已经运行的程序的pid(比如911)

	gdbserver :1234 --attach 911

或者是手动执行该被调试程序

	gdbserver :1234 app_need_to_be_debug

### 在host上设置连接方法

启动gdb

	./prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-gdb -tui

host TCP端口4321转发到remote TCP端口1234,也可以在未启动gdb前配置好

	(gdb) shell adb forward tcp:4321 tcp:1234

连接远程target

	(gdb) target remote localhost:4321

库文件绝对路径

	set solib-absolute-prefix /android_src/out/target/product/rk3288/symbols

库文件相对路径

	set solib-search-path /android_src/out/target/product/rk3288/symbols/system/lib

加载调试二进制文件(io)对应源码目录(external/io)

	(gdb) file /android_src/out/target/product/rk3288/symbols/system/xbin/io

### 调试过程中细节问题

执行continue后提示如下时说明程序没有跑起来

	(gdb) c
	The program is not being run.

此时需要在target上将程序跑起来

	gdbserver :1234 app_need_to_be_debug

在host端再次连接target

	(gdb) target remote localhost:4321

打断点时出现如下错误(程序的内存还没映射成功)

	(gdb) b foo
	Cannot access memory at address 0xe8a

此时需要先让程序跑起来一下

	(gdb) c

之后再次执行打断点操作即可

	(gdb) b foo

另外可以将在gdb里这些操作配置写在一个文件中(比如gdb_config),在gdb里直接source即可

	(gdb) source gdb_config

gdb_config内容如下供参考

	layout split
	shell adb forward tcp:4321 tcp:1234
	target remote localhost:4321
	set solib-absolute-prefix /path/to/android_src/out/target/product/rk3288/symbols
	set solib-search-path /path/to/android_src/out/target/product/rk3288/symbols/system/lib
	file /path/to/android_src/out/target/product/rk3288/symbols/system/xbin/gout1

## 通过串口调试内核

### 配置内核

配置如下选项

	CONFIG_KGDB=y
	CONFIG_CONSOLE_POLL=y
	CONFIG_HAVE_ARCH_KGDB=y
	CONFIG_KGDB_SERIAL_CONSOLE=y
	CONFIG_KGDB_KDB=y
	CONFIG_KDB_CONTINUE_CATASTROPHIC=0

### TARGET上打开KGDB调试开关

关闭watchdog

	echo 0 > /proc/sys/kernel/watchdog

设置kgdb over console(其中ttyFIQ0是调试串口)

	echo 'ttyFIQ0,115200' > /sys/module/kgdboc/parameters/kgdboc

开启kgdb

	echo g > /proc/sysrq-trigger

### HOST上调试

在android上使用SDK里的gdb工具

	prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-gdb -tui

通过串口连接到TARGET(由于使用的是调试串口,所以需要关掉调试串口)

	(gdb) set remotebaud 115200
	(gdb) target remote /dev/ttyUSB0

加载未压缩的内核

	file vmlinux

在使用GDB单步调试内核过程中会遇到由于代码经过编译器优化后无法显示变量值问题

	value optimized out

解决方法一般是使用-O0来编译内核

解决方法二是针对某个函数关闭优化

	void __attribute__((optimize("O0"))) foo(unsigned char data)
	{
		// unmodifiable compiler code
	}

解决方法三是针对某个文件(模块XXX)不做优化处理

	CFLAGS_XXX.o = -O0

比如需要调试composite.c和f_acm.c这两个模块
修改(kernel/drivers/usb/gadget/Makefile)

	libcomposite-y += composite.o functions.o configfs.o
	CFLAGS_composite.o = -O0

	usb_f_acm-y := f_acm.o
	CFLAGS_f_acm.o = -O0
