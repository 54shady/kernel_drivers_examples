# Intro

- es8323.c codec驱动
- rk_es8323.c rockchip平台machine驱动
- rk_i2s.c rockchip平台platform驱动

# Debug

编译后得到连个模块es8323.ko, rk_es8323.ko, rk_i2s.ko

加载codec驱动

	insmod es8323.ko

	cat /sys/kernel/debug/asoc/codecs

	内容如下(i2c_driver name.i2c控制器号-I2C设备地址)
	my ES8323 driver test.2-0010

	cat /sys/kernel/debug/asoc/dais
	ES8323 HiFi

加载platform驱动

	insmod rk_i2s.ko
	cat /sys/kernel/debug/asoc/dais
	内容如下(i2s控制器号地址.dainame)
	ff890000.rockchip-i2s

	cat /sys/kernel/debug/asoc/platforms
	内容如下(i2s控制器号地址.dainame)
	ff890000.rockchip-i2s

加载machine驱动

	insmod rk_es8323.ko 有如下信息表示成功,练接连个dai
	ES8323 HiFi <-> ff890000.rockchip-i2s mapping ok

codec probe成功后在下面目录会有相关信息

	/dev/snd/
	/proc/asound/cards
	/sys/kernel/debug/asoc/
	/sys/class/sound/

# Usage

将es8323.dtsi包含到主dts中

	#include "es8323.dtsi"

# test

录音

	tinycap test.wav

播放

	tinyplay test.wav
