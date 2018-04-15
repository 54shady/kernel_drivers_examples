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

	(gdb) target remote /dev/ttyUSB0

加载未压缩的内核

	file vmlinux
