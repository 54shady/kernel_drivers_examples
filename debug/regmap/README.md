# Regmap

[参考链接http://www.tinylab.org/kernel-explore-regmap-framework/](http://www.tinylab.org/kernel-explore-regmap-framework/)

Regmap主要目的是减少慢速I/O驱动上的重复逻辑,提供一种通用的接口来操作底层硬件上的寄存器.Regmap除了能做到统一的I/O接口,还可以在驱动和硬件IC之间做一层缓存,从而能减少底层I/O的操作次数

![regmap](./regmap.png)

[I2C Regmap example](./i2c_regmap.c)
