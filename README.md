# Firefly_RK3399

## 编译脚本参考gen_star

[rk3399编译脚本](https://github.com/54shady/gen_star/tree/rk3399)

## 编译时出现"缺少libtinfo.so.5解决办法"

	ln -s /lib/libncurses.so.5 /lib/libtinfo.so.5

## 烧写

	upgrade_tool ul RK3399MiniLoaderAll_V1.05.bin
	upgrade_tool di uboot uboot.img rk3399_parameter.txt
	upgrade_tool di trust trust.img rk3399_parameter.txt
	upgrade_tool rd

## GPIO类型

如何确定开发板上调试串口电平是3.0v还是1.8v的

![debug port](./debug_port.png)

硬件原理图连接如下

![port1](./port1.png)

TX和RX是从主控AJ4,AK2连出来的

![port2](./port2.png)

AJ4和AK2所在的电源域(GPIO类型)为APIO4

![port3](./port3.png)

APIO4电源域如下图(可以根据硬件电路来配置是1.8v/3.0v)

![apio4](./apio4.png)

电源设置1.8v模式硬件电路

![1p8](./1p8.png)

电源设置3.0v模式硬件电路

![3p0](./3p0.png)

查看开发板原理图如下(所以是3.0v)

![apio4_vdd](./apio4_vdd.png)

## LED使用

Firefly-RK3399开发板上有2个LED灯,如下表所示

|LED|GPIO|PIN NUMBER|
|---|---|---
|Blue|GPIO2_D3|91
|Yellow|GPIO0_B5|13

### 以设备(LED子系统)的方式控制LED

	echo 0 > /sys/class/leds/firefly:blue:power/brightness
	echo 1 > /sys/class/leds/firefly:blue:power/brightness

### 使用trigger方式控制LED(参考leds-class.txt)

首先在DT里将两个LED描述如下

```c
leds {
   compatible = "gpio-leds";
   power {
	   label = "firefly:blue:power";
	   linux,default-trigger = "ir-power-click";
	   default-state = "on";
	   gpios = <&gpio2 D3 GPIO_ACTIVE_HIGH>;
	   pinctrl-names = "default";
	   pinctrl-0 = <&led_power>;
   };
   user {
	   label = "firefly:yellow:user";
	   linux,default-trigger = "ir-user-click";
	   default-state = "off";
	   gpios = <&gpio0 B5 GPIO_ACTIVE_HIGH>;
	   pinctrl-names = "default";
	   pinctrl-0 = <&led_user>;
   };
};
```

#### Simple trigger LED

1. 定义LED触发器

	DEFINE_LED_TRIGGER(ledtrig_default_control);

2. 注册该触发器

	led_trigger_register_simple("ir-user-click", &ledtrig_default_control);

3. 控制LED的亮

	led_trigger_event(ledtrig_default_control, LED_FULL);

#### Complex trigger LED

查看都支持那些trigger

	cat /sys/class/leds/firefly\:blue\:power/trigger

使用某个trigger触发

	echo "timer" > /sys/class/leds/firefly\:blue\:power/trigger

## Misc

DT里rockchip,pins描述(写的不易读,使用下面提供的脚本批量修改)

	rockchip,pins = <4 17 RK_FUNC_3 &pcfg_pull_none>
	4 GPIO bank号,从1开始
	17 GPIO offset,从0开始(A0-A7,B0-B7,C0-C7)
	RK_FUNC_3 GPIO mux功能
	pcfg_pull_none GPIO是否上下拉,高阻配置

- 使用脚本replace_gpio.sh修改DT里的GPIO使代码可读性更强(修改OFFSET为宏)

```shell
GPIO_OFFSET=(
A0 A1 A2 A3 A4 A5 A6 A7
B0 B1 B2 B3 B4 B5 B6 B7
C0 C1 C2 C3 C4 C5 C6 C7
D0 D1 D2 D3 D4 D5 D6 D7)

for (( offset = 31; offset >= 0; offset-- ))
do
	sed -i "/&gpio/s/\ $offset/\ ${GPIO_OFFSET[offset]}/g" $1
done
```

代码修改前

	gpio = <&gpio1 0 GPIO_ACTIVE_HIGH>;

代码修改后

	gpio = <&gpio1 A0 GPIO_ACTIVE_HIGH>;


- 使用脚本replace_pin.sh修改DT里的PIN使代码可读性更强(修改该为BANK_OFFSET宏)

```shell
GPIO_OFFSET=(
A0 A1 A2 A3 A4 A5 A6 A7
B0 B1 B2 B3 B4 B5 B6 B7
C0 C1 C2 C3 C4 C5 C6 C7
D0 D1 D2 D3 D4 D5 D6 D7)

for (( offset = 31; offset >= 0; offset-- ))
do
	for (( bank = 4;  bank >= 0; bank-- ))
	do
		sed -i "/RK_FUNC_/s/<$bank $offset/<GPIO${bank}_${GPIO_OFFSET[offset]}/g" $1
	done
done
```

代码修改前

	rockchip,pins = <4 24 RK_FUNC_1 &pcfg_pull_none>;

代码修改后

	rockchip,pins = <GPIO4_D0 RK_FUNC_1 &pcfg_pull_none>;
