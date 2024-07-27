# GPIO

## sysfs gpio

以gpio1c6为例(计算该gpio的pin number)

	#define RK_PC6		22
	pinnum = 1 * 32 + 22 = 54

导出该gpio,会生成gpio54的目录

	echo 54 > /sys/class/gpio/export

导出该gpio后发现会被gpio sysfs驱动自动命名(drivers/gpio/gpiolib-sysfs.c)

	grep gpio-54 /sys/kernel/debug/gpio
		 gpio-54  (                    |sysfs            ) out lo

驱动代码中如下

	static ssize_t export_store(struct class *class,
		status = gpiod_request(desc, "sysfs");

查看gpio方向并设置gpio为输出管脚

	cat /sys/class/gpio/gpio54/direction
	echo out > /sys/class/gpio/gpio54/direction

拉高gpio

	echo 1 > /sys/class/gpio/gpio54/value
	cat /sys/class/gpio/gpio54/value

拉低gpio

	echo 0 > /sys/class/gpio/gpio54/value
	cat /sys/class/gpio/gpio54/value

取消导出该gpio

	echo 54 > /sys/class/gpio/unexport

## libgpiod

Linux 4.8 起,GPIO sysfs 接口已被弃用
用户空间应改用[libgpiod](https://docs.radxa.com/rock5/rock5b/app-development/gpiod)来于GPIO字符设备进行交互

使用libgpiod时需要unexport sysfs中的gpio

安装工具

	apt install -y gpiod

查看管脚信息其中带[used]表明已被使用,无法操作

	gpioinfo

列出所有gpio控制器(rk3588如下)

	gpiodetect

	gpiochip0 [gpio0] (32 lines)
	gpiochip1 [gpio1] (32 lines)
	gpiochip2 [gpio2] (32 lines)
	gpiochip3 [gpio3] (32 lines)
	gpiochip4 [gpio4] (32 lines)
	gpiochip5 [rk806-gpio] (3 lines)

读取gpio3b7值 3 * 32 + 15 = 96 + 15 = 111

	gpioget gpiochip3 15

使用signal模式拉高gpio

	gpioset -m signal gpiochip3 15=1
	(Press Ctrl+C to stop)

使用signal模式拉低gpio

	gpioset -m signal gpiochip3 15=0

	gpiomon gpiochip3 15
