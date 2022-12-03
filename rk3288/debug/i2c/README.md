# I2C

## UBOOT 下 I2C

在UBOOT下测试硬件是否正常

确认UBOOT中对I2C支持,一般会有相应命令

	uboot # i2c
	i2c - I2C sub-system

	Usage:
	i2c crc32 chip address[.0, .1, .2] count - compute CRC32 checksum
	i2c dev [dev] - show or set current I2C bus
	i2c loop chip address[.0, .1, .2] [# of objects] - looping read of device
	i2c md chip address[.0, .1, .2] [# of objects] - read from I2C device
	i2c mm chip address[.0, .1, .2] - write to I2C device (auto-incrementing)
	i2c mw chip address[.0, .1, .2] value [count] - write to I2C device (fill)
	i2c nm chip address[.0, .1, .2] - write to I2C device (constant address)
	i2c probe [address] - test for and show device(s) on the I2C bus
	i2c read chip address[.0, .1, .2] length memaddress - read to memory
	i2c write memaddress chip address[.0, .1, .2] length - write memory to i2c
	i2c reset - re-init the I2C Controller
	i2c speed [speed] - show or set I2C bus speed

查看当前使用的I2C BUS

	uboot # i2c dev
	Current bus is 0

查询该总线下接的所有I2C设备

	uboot # i2c probe
	Valid chip addresses: 14 40 41 51 55 5A

	如果是下面这样的输出,则说明该总线有异常,极大可能是有I2C设备拉低了总线

	uboot # i2c probe
	Valid chip addresses:I2C Send Start Bit Timeout
	I2C Send Start Bit Timeout

切换I2C BUS

	uboot # i2c dev 1
	Setting bus to 1
