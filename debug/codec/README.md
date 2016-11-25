# Intro

## es8323.c codec驱动

### DeviceTree Describe

	&i2c2 {
	es8323: es8323@10 {
				compatible = "es8323";
				reg = <0x10>;
				spk-con-gpio = <&gpio7 GPIO_B7 GPIO_ACTIVE_HIGH>;
				hp-con-gpio = <&gpio0 GPIO_B5 GPIO_ACTIVE_HIGH>;
				hp-det-gpio = <&gpio7 GPIO_A4 GPIO_ACTIVE_HIGH>;
				hub_rest = <&gpio0 GPIO_B6 GPIO_ACTIVE_LOW>;
				hub_en = <&gpio7 GPIO_B2 GPIO_ACTIVE_HIGH>;
			};
	};

## rk_es8323.c rockchip平台machine驱动

### DeviceTree Describe

	/ {
		rockchip-es8323 {
			compatible = "rockchip-es8323";
			dais {
				dai0 {
					audio-codec = <&es8323>;
					audio-controller = <&i2s>;
					format = "i2s";
				};
			};
		};
	};

## rk_i2s.c rockchip平台platform驱动

### DeviceTree Describe

	i2s: rockchip-i2s@0xff890000 {
			 compatible = "rockchip-i2s";
			 reg = <0xff890000 0x10000>;
			 i2s-id = <0>;
			 clocks = <&clk_i2s>, <&clk_i2s_out>, <&clk_gates10 8>;
			 clock-names = "i2s_clk","i2s_mclk", "i2s_hclk";
			 interrupts = <GIC_SPI 85 IRQ_TYPE_LEVEL_HIGH>;
			 dmas = <&pdma0 0>, <&pdma0 1>;
			 dma-names = "tx", "rx";
			 pinctrl-names = "default", "sleep";
			 pinctrl-0 = <&i2s_mclk &i2s_sclk &i2s_lrckrx &i2s_lrcktx &i2s_sdi &i2s_sdo0 &i2s_sdo1 &i2s_sdo2 &i2s_sdo3>;
			 pinctrl-1 = <&i2s_gpio>;
		 };


### 从datasheet里Address Mapping可以找到I2S控制器被映射到的位置

![I2S MAP](./I2S_MAP.png)

### 中断号(SPI[85])

![I2S INT](./I2S_INT.png)

### DMA编号(tx[0], rx[1])

![I2S DMA](./I2S_DMA.png)

### I2S寄存器信息,32bit,步进4

![I2S REG](./I2S_REG.png)

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
