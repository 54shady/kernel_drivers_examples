# Linux PWM

[参考文章http://www.wowotech.net/comm/pwm_overview.html?utm_source=tuicool](http://www.wowotech.net/comm/pwm_overview.html?utm_source=tuicool)

# 硬件连接图

![pwm](./pwm01.png)

![pwm](./pwm02.png)

从下面pwm控制器的设备树描述可以知道GPIO7_A0已经是PWM

	#define PWM0 0x7a01
	GPIO7_A0 配置成了PWM功能

	pwm0_pin:pwm0 {
		rockchip,pins = <PWM0>;
		rockchip,pull = <VALUE_PULL_DISABLE>;
		rockchip,drive = <VALUE_DRV_DEFAULT>;
	};

	pwm0: pwm@ff680000 {
		compatible = "rockchip,rk-pwm";
		reg = <0xff680000 0x10>;

		/* used by driver on remotectl'pwm */
		interrupts = <GIC_SPI 78 IRQ_TYPE_LEVEL_HIGH>;
		#pwm-cells = <2>;
		pinctrl-names = "default";
		pinctrl-0 = <&pwm0_pin>;
		clocks = <&clk_gates11 11>;
		clock-names = "pclk_pwm";
		status = "disabled";
	};

# Usage

在主dts文件中包含pwm的dtsi文件

	#include "pwm_backlight.dtsi"

# 测试方法

### 每一秒钟修改一次背光

```shell
# i=0;while (($i < 250)); do echo $i > /sys/devices/backlight.25/backlight/my_backlight/brightness; ((i+=20)); sleep 1;done
```

### 循环修改背光

```shell
# i=0;while (($i < 250)); do echo $i > /sys/devices/backlight.25/backlight/my_backlight/brightness; ((i+=20)); sleep 1;done
# while true; do if (($i > 250)) then;i=0 fi; echo $i; i+=1;done
```

```shell
i=0
while true
do
if(($i > 250)) then
i=0
fi
echo $i > /sys/devices/backlight.25/backlight/my_backlight/brightness;
((i+=10))
sleep 1
done
```
