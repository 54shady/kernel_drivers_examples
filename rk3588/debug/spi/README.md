# SPI

## config dt both support user and non-user mode spi

配置设备树同时支持user mode和非user mode device

	&spi4 {
		status = "okay";
		pinctrl-names = "default", "high_speed";
		pinctrl-0 = <&spi4m2_cs0  &spi4m2_pins &gpio_mcu_in>;
		dma-names = "tx","rx";
		spi_test@0 {
			//多个compatible字符串,可以匹配不同的驱动
			compatible = "rockchip,spi_test_bus4_cs0", "rockchip,spidev";
			gpio_mcu = <&gpio3 RK_PB1 IRQ_TYPE_EDGE_FALLING>;
			id = <0>;
			reg = <0>; //chip select 0:cs0 1:cs1
			spi-max-frequency = <24000000>; //spi output clock
		};
	};

## user mode spi device(用户空间直接操作spi接口)

- 优点: 不需要写内核驱动,用默认的内核框架
- 缺点: 不支持中断,只能用同步的read/write,不支持异步

在驱动driver/spi/spidev.c中可以匹配如下设备

	static const struct of_device_id spidev_dt_ids[] = {
		{ .compatible = "rockchip,spidev" },
		{},
	};
	MODULE_DEVICE_TABLE(of, spidev_dt_ids);

加载spidev驱动,让设备匹配spidev驱动(设备名为spi4.0)

	insmod spidev.ko

使用内核代码中带的测试代码[spidev_test.c](linux/tools/spi/spidev_test.c)

	./spidev_test -D /dev/spidev4.0 -v

## non-user mode spi device

在驱动driver/spi/myspi.c中可以匹配如下设备

	static const struct of_device_id rockchip_spi_test_dt_match[] = {
		{ .compatible = "rockchip,spi_test_bus4_cs0", },
		{},
	};
	MODULE_DEVICE_TABLE(of, rockchip_spi_test_dt_match);
