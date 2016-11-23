# Intro

- es8323.c codec驱动
- rk_es8323.c rockchip平台machine驱动

# Debug

codec probe成功后在下面目录会有相关信息

	ls /dev/snd/
	cat /proc/asound/cards
	cat /sys/kernel/debug/asoc/codecs
	cat /sys/kernel/debug/asoc/dais
	cat /sys/kernel/debug/asoc/platforms

# Usage

将es8323.dtsi包含到主dts中

	#include "es8323.dtsi"
