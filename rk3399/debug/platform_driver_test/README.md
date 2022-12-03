# Usage

把dtsi文件包含到所使用的dts里

	#include "skeleton_ss.dtsi"

## debug

拉高pin_a_name(GPIO1_B2)

	echo 0 1 > /sys/devices/platform/skeleton_gpios/egpios_debug

拉低pin_a_name(GPIO1_B2)

	echo 0 0 > /sys/devices/platform/skeleton_gpios/egpios_debug

查看相应管脚状态

	cat /sys/devices/platform/skeleton_gpios/egpios_debug
	cat /sys/kernel/debug/gpio | grep pin_
