# audio(es8338 codec)

[rk audio develop guide](Rockchip_Developer_Guide_Audio_CN.pdf)

## basic

查看系统中声卡设备

	/dev/snd/by-path/platform-es8388-sound -> ../controlC1

显示系统中所有可用声卡

	aplay -l

	**** List of PLAYBACK Hardware Devices ****
	card 0: rockchiphdmi0 [rockchip-hdmi0], device 0: rockchip-hdmi0 i2s-hifi-0 [rockchip-hdmi0 i2s-hifi-0]
	  Subdevices: 1/1
	  Subdevice #0: subdevice #0
	card 1: rockchipes8388 [rockchip,es8388], device 0: dailink-multicodecs ES8323.7-0011-0 [dailink-multicodecs ES8323.7-0011-0]
	  Subdevices: 1/1
	  Subdevice #0: subdevice #0

	cat /proc/asound/cards
	0 [rockchiphdmi0  ]: rockchip-hdmi0 - rockchip-hdmi0
					  rockchip-hdmi0
	1 [rockchipes8388 ]: rockchip_es8388 - rockchip,es8388
					  rockchip,es8388
	2 [rockchiphdmiin ]: rockchip_hdmiin - rockchip,hdmiin
					  rockchip,hdmiin

使用card1测试

	arecord -D plughw:1,0 -f cd -t wav a.wav
	aplay -D plughw:1,0 /usr/share/sounds/alsa/Noise.wav

## xrun profile

如下两种情况统称xrun,音频路径上,所有buffer节点都可能触发xrun

- 当音频播放buffer empty的时候,触发underrun
- 当音频录制buffer full的时候,触发overrun

使能xrun config

	CONFIG_SND_DEBUG
	CONFIG_SND_PCM_XRUN_DEBUG
	CONFIG_SND_VERBOSE_PROCFS

使能声卡1的所有xrun调试开关

	echo 7 > /proc/asound/card1/pcm0p/xrun_debug

## FAQ

issue: i2c 没有外部上拉导致的通信失败(state 3 表示sda sdl都是低)

	[    4.132960] rk3x-i2c fec90000.i2c: timeout, ipd: 0x11, state: 3
	[    4.205112] rk3x-i2c fec90000.i2c: timeout, ipd: 0x10, state: 3

fix: 通过硬件外部加上拉电阻或软件实现内部上拉

	将原pcfg_pull_none_smt改为pcfg_pull_up_drv_level_0
	i2c7 {
		/omit-if-no-ref/
		i2c7m0_xfer: i2c7m0-xfer {
			rockchip,pins =
				/* i2c7_scl_m0 */
				<1 RK_PD0 9 &pcfg_pull_up_drv_level_0>,
				/* i2c7_sda_m0 */
				<1 RK_PD1 9 &pcfg_pull_up_drv_level_0>;
		};
