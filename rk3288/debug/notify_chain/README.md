# Notify

[参考文章](http://blog.chinaunix.net/uid-25871104-id-3086446.html)

![call flow](./call_flow.png)

## 测试方法

[参考文章](http://www.linuxidc.com/Linux/2013-07/86999.htm)

demo1 使用方法

	insmod demo1_core.ko
	insmod demo1_register.ko
	insmod demo1_notifier.ko

demo 使用方法

	insmod chain_core.ko
	insmod register.ko
	insmod notifier.ko
