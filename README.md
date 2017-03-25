# Firefly_RK3399

## 编译脚本参考gen_star

[rk3399编译脚本](https://github.com/54shady/gen_star/tree/rk3399)

## 编译时出现"缺少libtinfo.so.5解决办法"

	ln -s /lib/libncurses.so.5 /lib/libtinfo.so.5

## 使用upgrade_tool烧写

	upgrade_tool ul RK3399MiniLoaderAll_V1.05.bin
	upgrade_tool di uboot uboot.img rk3399_parameter.txt
	upgrade_tool di trust trust.img rk3399_parameter.txt
	upgrade_tool rd

## 使用rkflashtool烧写(nsector = 512byte)

参考rkflashtool帮助

读出misc分区(假设起始地址为0x6000)前48K的内容

	rkflashtool r 0x6000 96 > misc.img

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

## GPIO使用(以GPIO0_B4为例)

DT里描述GPIO0_B4如下

配置管脚MUX为GPIO模式(默认是GPIO模式,这里是为了读者更明白原理)
```c
gpio_demo_pin: gpio_demo_pin {
	rockchip,pins = <GPIO0_B4 RK_FUNC_GPIO &pcfg_pull_none>;
};
```

以GPIO的模式使用该PIN脚
```c
gpio_demo: gpio_demo {
	status = "okay";
	compatible = "firefly,rk3399-gpio";
	firefly-gpio = <&gpio0 B4 GPIO_ACTIVE_HIGH>;
	pinctrl-names = "default";
	pinctrl-0 = <&gpio_demo_pin>;
};
```

### IO-Domain
在复杂的片上系统(SOC)中,设计者一般会将系统的供电分为多个独立的block,这称作电源域(Power Domain),这样做有很多好处,例如:

- 在IO-Domain的DTS节点统一配置电压域,不需要每个驱动都去配置一次,便于管理
- 依照的是Upstream的做法,以后如果需要Upstream比较方便
- IO-Domain的驱动支持运行过程中动态调整电压域,例如PMIC的某个Regulator可以1.8v和3.3v的动态切换,一旦Regulator电压发生改变,会通知IO-Domain驱动去重新设置电压域

### 使用工具IO来调试

查看GPIO1_B3引脚的复用情况

1. 从主控的datasheet查到GPIO1对应寄存器基地址为:0xff320000
2. 从主控的datasheet查到GPIO1B_IOMUX的偏移量为:0x00014
3. GPIO1_B3的iomux寄存器地址为:基址(Operational Base) + 偏移量(offset)=0xff320000+0x00014=0xff320014
4. 用以下指令查看GPIO1_B3的复用情况:

	io -4 -r 0xff320014
	ff320014:  0000816a

5. 如果想复用为GPIO,可以使用以下指令设置

	io -4 -w 0xff320014 0x0000812a

## Misc

### pincontrl,gpio修改

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

### 使用7yuv显示fb里的图像

抓取fb里的图像数据

	echo bmp > /sys/class/graphics/fb0/dump_buf

会在/data/dmp_buf/里保存图片数据,假设名为frame0_win0_0_1920x1080_XBGR888.bin

使用7yuv设置好分辨率1920x1080和格式RGBA888就能显示该图片
