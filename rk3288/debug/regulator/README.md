# Regulator Usage

## DCDC & LDO

电源类型概述

系统中各路电源总体分为两种:DCDC和LDO

两种电源的总体特性如下

DCDC:输入输出压差大时,效率高,但是有纹波问题,成本高,所以打压差,大电流时使用

LDC:输入输出压差大时,效率低,成本低

为提高LDO的转换效率,系统上会进行相关优化如:

LDO输入电压为1.1V,为了提高效率,其输入电压可以从VCCIO_3.3V的DCDC给出,所以电路上如果允许尽量将LDO接到DCDC输出回路,但是要注意上电时序

DCDC一般有两种工作模式:

- PWM 纹波瞬态相应好,效率低

- PFM 效率高,但是负载能力差

## ACT8846 PMU驱动

框架图如下

![BLOCK DIAGRAM](./dcdc_ldo.png)

由图可知

REG[1, 4]对应DCDC[1, 4]

REG[5, 13]对应LDO[1, 9]

驱动代码中的voltage map可以参考手册的table 5

	 600, 625, 650, 675, 700, 725, 750, 775,
	 800, 825, 850, 875, 900, 925, 950, 975,
	 1000, 1025, 1050, 1075, 1100, 1125, 1150,
	 1175, 1200, 1250, 1300, 1350, 1400, 1450,
	 1500, 1550, 1600, 1650, 1700, 1750, 1800,
	 1850, 1900, 1950, 2000, 2050, 2100, 2150,
	 2200, 2250, 2300, 2350, 2400, 2500, 2600,
	 2700, 2800, 2900, 3000, 3100, 3200,
	 3300, 3400, 3500, 3600, 3700, 3800, 3900,

![Table5](./table5.png)

### 设备树配置

在所使用的主dts文件里包含act8846.dtsi的设备树文件即可

	#include "act8846.dtsi"

假设act8846接到主控I2C的I2C0上则且I2C0标号为i2c0则可以直接使用,否则只需改下I2C的编号即可

### 触摸屏硬件连接和设备树配置

以下图所示的一个TP模块连接方式来说明

![vcc_tp_1](./vcc_tp_1.png)

![vcc_tp_2](./vcc_tp_2.png)

VCC_TP给TP供电,由PMU上的LDO提供

TP的device tree如下描述这里并不完整,少里对PIN脚等的描述

```shell
goodix_ts@5d {
    compatible = "goodix,gt9xx";
    status = "okay";
    reg = <0x5d>;
    VCC_TP-supply = <&ldo4_reg>;
}
```

其中VCC_TP-supply这样写比较规范

VCC_TP和原理图上标的对应(纯粹为了好记,让代码和图对应)

supply是固定后缀,可以从代码里得知

为什么对应ldo4_reg?

regulator的寄存器从0开始计算

OUT8 对应就对应regulator 7

而regulator 7 的标号就是ldo4_reg,这里是device tree的语法

```shell
ldo4_reg:regulator@7 {
	reg = <0x7>;
	regulator-compatible = "act_ldo4";
	regulator-name = "act_ldo4";
	regulator-min-microvolt = <0x325aa0>;
	regulator-max-microvolt = <0x325aa0>;
	linux,phandle = <0xc2>;
	phandle = <0xc2>;
};
```

### 测试方法

- 加载PMU驱动

```shell
	insmod act8846.ko
```

- 不断开关regulator7的电压

```shell
while true
do
insmod regulator.ko
sleep 5
rmmod regulator
sleep 5
done
```

- 查看regulator7开关状态

```shell
while true
do
cat /sys/class/regulator/regulator.8/state
sleep 2
done
```

## RK818 PMU驱动

![PowerTree](./rk818_powertree.png)

可以从上图中看出TP使用的LDO2,下面就测试这个LDO

### 测试代码简单说明

模拟一个I2C驱动(实际可以没有物理设备),其DeviceTree描述如下

```c
&i2c0 {
	consumer1@5d {
		compatible = "Consumer1";
		status = "okay";
		reg = <0x5d>;
		VCC_TP-supply = <&rk818_ldo2_reg>;
	};
};
```

其中LDO2的设置如下(不能设置成regulator-always-on)
否则无法看到关闭regulator的现象

```c
rk818_ldo2_reg: regulator@5 {
					reg = <5>;
					regulator-compatible = "rk818_ldo2";
					regulator-boot-on;
					regulator-name= "vcc_tp";
					regulator-min-microvolt = <3300000>;
					regulator-max-microvolt = <3300000>;
					regulator-initial-state = <3>;
					regulator-state-mem {
						regulator-state-enabled;
						regulator-state-uv = <3300000>;
					};
				};
```

测试代码中通过下面的方法来获取和使用这个LDO

	supply = devm_regulator_get(&client->dev, "VCC_TP");
	regulator_enable(supply);
	regulator_disable(supply);

### 如何使用本文中的代码

编译相应代码得到相应模块,在主dts文件中包含下面的dtsi文件

	#include "rk818.dtsi"
	#include "rk818_test.dtsi"

### 测试方法1(1个consumer)

- 加载PMU驱动

```shell
	insmod rk818.ko
```

- 不断开关LDO2的电压

```shell
	while true
	do
	insmod consumer1.ko
	sleep 5
	rmmod consumer1
	sleep 5
	done
```

- 查看regulator开关状态

```shell
	while true
	do
	cat /sys/class/regulator/regulator.7/state
	cat /sys/class/regulator/regulator.7/num_users
	sleep 2
	done
```

### 测试方法2(2个consumer)

- 加载PMU驱动

```shell
	insmod rk818.ko
```

- 查看regulator开关状态

```shell
while true
do
cat /sys/class/regulator/regulator.7/state
cat /sys/class/regulator/regulator.7/num_users
sleep 2
done
```

- 分别加载consumer1,2,并查看信息

```shell
	insmod consumer1.ko
	insmod consumer2.ko
```

- 分别卸载consumer1,2(只有当num_users为0时state才会disabled)

```shell
	rmmod consumer1.ko
	rmmod consumer2.ko
```
