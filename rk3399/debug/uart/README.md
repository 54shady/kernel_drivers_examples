# UART

## Firefly_RK3399

开发板默认使用uart2作为调试串口,在bootargs中有如下设置

	bootargs = "earlycon=uart8250,mmio32,0xff1a0000";

其中0xff1a0000就是uart2的地址

	uart2: serial@ff1a0000

在cmdline中设置了console使用节点ttyFIQ0(等价与ttyS2)

	androidboot.console=ttyFIQ0

ttyFIQ0节点驱动对应的dts如下

    fiq_debugger: fiq-debugger {
        compatible = "rockchip,fiq-debugger";
        rockchip,serial-id = <2>;
        rockchip,signal-irq = <182>;
        rockchip,wake-irq = <0>;
        rockchip,irq-mode-enable = <1>;  /* If enable uart uses irq instead of fiq */
        rockchip,baudrate = <115200>;  /* Only 115200 and 1500000 */
        pinctrl-names = "default";
        pinctrl-0 = <&uart2c_xfer>;
    };

其中设置了对应的串口为uart2

	rockchip,serial-id = <2>;

所以如果要将调试串口改成其它串口,比如uart4

只需要fiq的dts和bootargs,内核日志就将在uart4上输出

	rockchip,serial-id = <4>;
	pinctrl-0 = <&uart4_xfer>;
	bootargs = "earlycon=uart8250,mmio32,ff370000";
