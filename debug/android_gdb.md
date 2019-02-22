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

### 实例1(android上动态库bluetooth.default.so调试)

错误日志如下(size > GKI_MAX_BUF_SIZE)

	--------- beginning of crash
	01-14 04:19:10.896  1404  1471 F libc    : system/bt/hci/src/buffer_allocator.c:26: buffer_alloc: assertion "size <= GKI_MAX_BUF_SIZE" failed
	01-14 04:19:10.897  1404  1471 F libc    : Fatal signal 6 (SIGABRT), code -6 in tid 1471 (bluedroid wake/)
	01-14 04:19:10.898   234   234 F DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
	01-14 04:19:10.899   234   234 F DEBUG   : Revision: '0'
	01-14 04:19:10.899   234   234 F DEBUG   : ABI: 'arm'
	01-14 04:19:10.899   234   234 F DEBUG   : pid: 1404, tid: 1471, name: bluedroid wake/  >>> com.android.bluetooth <<<
	01-14 04:19:10.899   234   234 F DEBUG   : signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
	01-14 04:19:10.915   234   234 F DEBUG   : Abort message: 'system/bt/hci/src/buffer_allocator.c:26: buffer_alloc: assertion "size <= GKI_MAX_BUF_SIZE" failed'
	01-14 04:19:10.915   234   234 F DEBUG   :     r0 00000000  r1 000005bf  r2 00000006  r3 a0c16978
	01-14 04:19:10.915   234   234 F DEBUG   :     r4 a0c16980  r5 a0c16930  r6 00000000  r7 0000010c
	01-14 04:19:10.915   234   234 F DEBUG   :     r8 0000930f  r9 0000930f  sl a15e8f48  fp a15e8ecc
	01-14 04:19:10.915   234   234 F DEBUG   :     ip 00000006  sp a0c16390  lr b6ca7ec9  pc b6caa2b8  cpsr 400f0010
	01-14 04:19:10.926   234   234 F DEBUG   :
	01-14 04:19:10.926   234   234 F DEBUG   : backtrace:
	01-14 04:19:10.926   234   234 F DEBUG   :     #00 pc 000442b8  /system/lib/libc.so (tgkill+12)
	01-14 04:19:10.926   234   234 F DEBUG   :     #01 pc 00041ec5  /system/lib/libc.so (pthread_kill+32)
	01-14 04:19:10.926   234   234 F DEBUG   :     #02 pc 0001badf  /system/lib/libc.so (raise+10)
	01-14 04:19:10.927   234   234 F DEBUG   :     #03 pc 00018c91  /system/lib/libc.so (__libc_android_abort+34)
	01-14 04:19:10.927   234   234 F DEBUG   :     #04 pc 00016784  /system/lib/libc.so (abort+4)
	01-14 04:19:10.927   234   234 F DEBUG   :     #05 pc 0001a6f3  /system/lib/libc.so (__libc_fatal+16)
	01-14 04:19:10.927   234   234 F DEBUG   :     #06 pc 00018d19  /system/lib/libc.so (__assert2+20)
	01-14 04:19:10.927   234   234 F DEBUG   :     #07 pc 000edabd  /system/lib/hw/bluetooth.default.so
	01-14 04:19:10.927   234   234 F DEBUG   :     #08 pc 000f05ed  /system/lib/hw/bluetooth.default.so
	01-14 04:19:10.927   234   234 F DEBUG   :     #09 pc 000ef03f  /system/lib/hw/bluetooth.default.so
	01-14 04:19:10.927   234   234 F DEBUG   :     #10 pc 000ee02f  /system/lib/hw/bluetooth.default.so
	01-14 04:19:10.927   234   234 F DEBUG   :     #11 pc 000fad73  /system/lib/hw/bluetooth.default.so
	01-14 04:19:10.927   234   234 F DEBUG   :     #12 pc 000fbccf  /system/lib/hw/bluetooth.default.so
	01-14 04:19:10.927   234   234 F DEBUG   :     #13 pc 000417c7  /system/lib/libc.so (_ZL15__pthread_startPv+30)
	01-14 04:19:10.927   234   234 F DEBUG   :     #14 pc 00019313  /system/lib/libc.so (__start_thread+6)
	01-14 04:19:11.343   234   234 F DEBUG   :
	01-14 04:19:11.343   234   234 F DEBUG   : Tombstone written to: /data/tombstones/tombstone_01

欲调试的库bluetooth.default.so被进程com.android.bluetooth使用

所以可以将gdbserver附加到该进程即可

在android设备上执行(gdb attach到blue的进程上)

	gdbserver :1234 --attach `ps | grep bluetooth | busybox1.11 awk '{print $2}'`

gdb配置文件内容如下(ble_cfg)

	layout split
	shell adb forward tcp:4321 tcp:1234
	target remote localhost:4321
	set solib-absolute-prefix /home/zeroway/android6.0/out/target/product/rk3288/symbols
	set solib-search-path /home/zeroway/android6.0/out/target/product/rk3288/symbols/system/lib

在主机上启动gdb(进入后source ble_cfg)
并在system/bt/hci/src/buffer_allocator.c中函数(buffer_alloc)打断点

	./prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-gdb
	(gdb) source ble_cfg
	(gdb) b buffer_alloc
	(gdb) continue

设置条件断点

	(gdb) b buffer_alloc if size > 4096

在android设备上操作蓝牙(比如刷新操作)之后就能停在buffer_alloc函数

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

```c
void __attribute__((optimize("O0"))) foo(unsigned char data)
{
	// unmodifiable compiler code
}
```

解决方法三是针对某个文件(模块XXX)不做优化处理

	CFLAGS_XXX.o = -O0

比如需要调试composite.c和f_acm.c这两个模块
修改(kernel/drivers/usb/gadget/Makefile)

```Makefile
libcomposite-y += composite.o functions.o configfs.o
CFLAGS_composite.o = -O0

usb_f_acm-y := f_acm.o
CFLAGS_f_acm.o = -O0
```

## 使用GDB的一些便利之处

在看代码过程中经常遇到下面这样的代码

```c
value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);

static inline int usb_ep_queue(struct usb_ep *ep,
				   struct usb_request *req, gfp_t gfp_flags)
{
	return ep->ops->queue(ep, req, gfp_flags);
}
```

需要找到这里调用的queue是哪里的代码,往往需要花上一段时间

这里可以利用gdb来查看(dwc_otg_pcd_ep_ops)

	(gdb) p *cdev->gadget->ep0
	$8 = {
	  driver_data = 0xc48c6e80,
	  name = 0xc0c53644 "ep0",
	  ops = 0xc1a40d24 <dwc_otg_pcd_ep_ops>,
	  ep_list = {
		next = 0xdce41128,
		prev = 0xdce41128
	  },
	  maxpacket = 64,
	  max_streams = 0,
	  mult = 0,
	  maxburst = 0,
	  address = 0 '\000',
	  desc = 0x0 <__vectors_start>,
	  comp_desc = 0x0 <__vectors_start>
	}
