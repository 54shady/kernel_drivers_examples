# rk3588 gmac and ethernet phy

[Doc: GMAC RGMII DELAYLINE](Rockchip_Developer_Guide_Linux_GMAC_RGMII_Delayline_CN.pdf)

[Doc: Linux GMAC](Rockchip_Developer_Guide_Linux_GMAC_CN.pdf)

[Doc: GMAC mode configuration](Rockchip_Developer_Guide_Linux_GMAC_Mode_Configuration_CN.pdf)

[rk3588 调试 phy](https://blog.51cto.com/u_15904120/5949187)

## basic

RK3588芯片拥有2个GMAC控制器,提供RMII或RGMII接口连接外置的Ethernet PHY

RK3588 平台默认支持的是PHY: RTL8211f(带loopback功能),toybrick开发板使用的是这个phy芯片

- 使用100M PHY(phy-mode: rmii), gmac clk 必须为50M(可以由phy提供,或gmac自己提供)
- 使用1000M PHY(phy-mode: rgmii), gmac clk必须为125M(由phy提供)

## phy debug

查看phy寄存器,往reg0写0xabcd

	cat /sys/bus/mdio_bus/devices/stmmac-0:01/phy_registers
	echo 0x00 0xabcd > /sys/bus/mdio_bus/devices/stmmac-0:01/phy_registers

## clk debug

查看gmac clock

	cat /sys/kernel/debug/clk/clk_gmac_125m/clk_enable_count
	grep gmac /sys/kernel/debug/clk/clk_summary

	head /sys/kernel/debug/clk/clk_summary
								 enable  prepare  protect                                duty
	clock                          count    count    count        rate   accuracy phase  cycle

## 查看pinmux

排查pinmux(比如排查gpio2b6是否是gmac pin)

	grep gpio2-14 /sys/kernel/debug/pinctrl/pinctrl-rockchip-pinctrl/pinmux-pins
	grep gpio2-15 /sys/kernel/debug/pinctrl/pinctrl-rockchip-pinctrl/pinmux-pins
	grep gpio2-16 /sys/kernel/debug/pinctrl/pinctrl-rockchip-pinctrl/pinmux-pins

## 设置delayline(取值范围在0x00~0x7f)

为了兼容不同硬件的信号差异,rgmii提供调整tx,rx的延时值

首先扫描delayline窗口,获得tx/rx delayline

### 千兆测试

使用千兆速度1000来扫描

	echo 1000 > /sys/devices/platform/fe1b0000.ethernet/phy_lb_scan
	...
	Find available tx_delay = 0x3e, rx_delay = disable
	...

对于rk3588上(rtl8211f)开启了硬件phy rx delay, 所以rx delay是固定的
所以tx delay的值大小反应了窗口大小:值约大，窗口越大，信号越好
这里测试得到的0x3e值相对来说比较小(最大值是0x7f):说明窗口小，信号不好

设置上面获得的delayline后测试loopback是否可以成功

	echo 0x3d 0x7f > /sys/devices/platform/fe1b0000.ethernet/rgmii_delayline
	echo 0x3e 0x7f > /sys/devices/platform/fe1b0000.ethernet/rgmii_delayline
	cat /sys/devices/platform/fe1b0000.ethernet/rgmii_delayline

loop back 测试

	echo 1000 > /sys/devices/platform/fe1b0000.ethernet/phy_lb
	...
	[ 1973.912370] PHY loopback: FAIL //信号不好会有这条日志
	...

如果测试成功则可以把相应的值写入到dts中,否则写入dts也无效

### 百兆测试

下面是百兆的扫描情况,窗口相比上面千兆来的大(0x65距离0x7f已经非常近了),因为百兆要求不如千兆要求高

	echo 100 > /sys/devices/platform/fe1b0000.ethernet/phy_lb_scan
	...
	[ 1148.323228] Find available tx_delay = 0x65, rx_delay = disable
	...

loop back 测试

	echo 100 > /sys/devices/platform/fe1b0000.ethernet/phy_lb
	...
	[ 2062.580645] PHY loopback: PASS //测试成功
	...

## Link Up/Down

确认是否link上,1表示link up

	cat /sys/devices/platform/fe1b0000.ethernet/net/eth0/carrier

## IO驱动强度设置

配置gpio2b6的io驱动强度

在trm中搜索gpio2b6_ds

	gpio2b6_ds (9:8)
	2'b00: 2.5mA 100ohm
	2'b10: 5mA 50ohm
	2'b01: 7.5mA 33ohm
	2'b11: 10mA 25ohm

找到其属于VCCIO3_5_IOC下的VCCIO3_5_IOC_GPIO2B_DS_H

	VCCIO3_5_IOC = 0xfd5fa000
		VCCIO3_5_IOC_GPIO2B_DS_H (0x004C)

dts里配置drv_leave_15时对应的io驱动强度(<2 RK_PB6 1 &pcfg_pull_none_drv_level_15>)

	io -4 0xfd5fa04c
	fd5fa04c:  0000ff11

## DTS中gmac的配置

gmac的dts配置如下(参考 Documentation/devicetree/bindings/net/rockchip-dwmac.yaml)

&gmac0 {
	/* Use rgmii-rxid mode to disable rx delay inside Soc */
	phy-mode = "rgmii-rxid"; //模式为rgmii-rxid
	clock_in_out = "output"; //这里设置成input反而会报错?应该是用的主控的pll提供txclk
	//input为使用phy提供的125M,input方式作为备选方案
	//原理图上(rk3588)gmac0_mclkinout连接的是(rtl8211f)phy0_clkout125

内核文档中对clock_in_out的解释

	clock_in_out:
	description:
	  For RGMII, it must be "input", means main clock(125MHz)
	  is not sourced from SoC's PLL, but input from PHY.
	  For RMII, "input" means PHY provides the reference clock(50MHz),
	  "output" means GMAC provides the reference clock.
