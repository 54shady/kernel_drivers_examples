# 按键驱动

## 硬件连接

当按键按下后电平由高变低

![button1](./button1.png)

![button2](./button2.png)

## 使用方法

在主设备树文件里包含下面的文件

	#include "buttons.dtsi"

## 测试

按下每个按键后会打印日志
