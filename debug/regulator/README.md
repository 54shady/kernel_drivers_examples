# Regulator Usage

## 硬件连接和设备树配置

以下图所示的一个TP模块连接方式来说明

![vcc_tp_1](./vcc_tp_1.png)

![vcc_tp_2](./vcc_tp_2.png)

VCC_TP给TP供电,由PMU上的LDO提供

TP的device tree如下描述

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

## 测试方法

在shell下输入下面的命令测试

```shell
while true
do
insmod regulator.ko
sleep 5
rmmod regulator
sleep 5
done
```
