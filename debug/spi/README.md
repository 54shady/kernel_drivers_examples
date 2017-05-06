# SPI

以W25Q128FV为例子介绍

## 软件基础

在设备树中每一个spi节点对应一个SPI控制器(一般情况下软件将bus和控制器配置成如下对应关系)

	spi0 <==> bus 0
	spi1 <==> bus 1
	spi2 <==> bus 2

其中每个SPI控制器上片选数有一个或多个,具体看芯片

	SPI0_CSN0
	SPI0_CSN1

	SPI1_CSN0

	SPI2_CSN0
	SPI2_CSN1

## 硬件连接

	CS		<--> 	SPI1_CSN0
	VCC		<--> 	VCC3V3_SYS
	DO		<--> 	SPI1_RXD
	DI		<--> 	SPI1_TXD
	GND		<--> 	GND
	HOLD	<--> 	TP_RST(需要拉高到3V)
	CLK 	<--> 	SPI1_CLK

## DeviceTree

	&spi1 {
		status = "okay";
		max-freq = <48000000>;
		dev-port = <1>;

		w25q128fv@10{
			status = "okay";
			compatible = "firefly,w25q128fv";
			reg = <0x0>;
			spi-max-frequency = <48000000>;
		};
	};

dev-port

	表示bus_num,因为这里用的是spi1,所以配置为1

@10的含义

	1表示bus_num,需要和dev-port一致, 0表示spi设备使用CSN0作为片选

reg = <0x0>

	表示spi设备使用的片选,需要和上面一致,即CSN0
