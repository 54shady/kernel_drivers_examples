# Firefly_RK3399

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
