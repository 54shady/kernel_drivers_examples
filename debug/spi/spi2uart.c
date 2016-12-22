#include <mach/gpio.h>
#include <mach/iomux.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include "wk2xxx.h"

/* global mutex lock */
static DEFINE_MUTEX(wk2xxxs_reg_lock);

#define GPIO_DRIVER_CS_SUCK
#ifdef GPIO_DRIVER_CS_SUCK
#define CS_HIGH 1
#define CS_LOW 0
void chip_select_enable(int enable)
{
	iomux_set(GPIO0_D7);
	gpio_set_value(RK30_PIN0_PD7, enable);
	iomux_set(SPI1_CS0);
}
#endif

static int wk2xxx_write_reg(struct spi_device *chip, uint8_t port, uint8_t reg, uint8_t data)
{
	struct spi_message msg;
	uint8_t buffer_w[2];
	int status;
	struct spi_transfer index_xfer = {
		.len            = 2,
		.cs_change      = 1,
	};
	int sub_number;

	sub_number = ((port - 1) << 4);

	mutex_lock(&wk2xxxs_reg_lock);

	spi_message_init(&msg);

	/* 构造控制字节和数据, 参考wk2xxx手册 */
	buffer_w[0] = sub_number | reg;
	buffer_w[1] = data;

	/* 设置发送缓冲区 */
	index_xfer.tx_buf = buffer_w;

	spi_message_add_tail(&index_xfer, &msg);
#ifdef GPIO_DRIVER_CS_SUCK
	chip_select_enable(CS_LOW);
#endif
	status = spi_sync(chip, &msg);
	if(status)
	{
		printk("spi sync error\n");
		return status;
	}
	udelay(3);
	mutex_unlock(&wk2xxxs_reg_lock);

#ifdef GPIO_DRIVER_CS_SUCK
	chip_select_enable(CS_HIGH);
#endif

	return status;
}

static int wk2xxx_read_reg(struct spi_device *chip, uint8_t port, uint8_t reg, uint8_t *data)
{
	struct spi_message msg;
	uint8_t buffer_w[2];
	uint8_t buffer_r[2];
	int status = 0;
	int sub_number;

	struct spi_transfer index_xfer = {
		.len            = 2,
		.cs_change      = 1,
	};

	sub_number = ((port - 1) << 4);

	mutex_lock(&wk2xxxs_reg_lock);
	spi_message_init(&msg);
	buffer_w[0] = 0x40 | sub_number | reg;
	buffer_w[1] = 0x00;
	buffer_r[0] = 0x00;
	buffer_r[1] = 0x00;

	index_xfer.tx_buf = buffer_w;
	index_xfer.rx_buf = buffer_r;

	spi_message_add_tail(&index_xfer, &msg);

#ifdef GPIO_DRIVER_CS_SUCK
	chip_select_enable(CS_LOW);
#endif
	status = spi_sync(chip, &msg);
	if(status)
	{
		printk("spi sync error\n");
		return status;
	}
	udelay(3);
	mutex_unlock(&wk2xxxs_reg_lock);

	/* 读出的数据 */
	*data = buffer_r[1];

#ifdef GPIO_DRIVER_CS_SUCK
	chip_select_enable(CS_HIGH);
#endif

	return 0;
}

static int wk2xxx_remove(struct spi_device *chip)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static int wk2xxx_probe(struct spi_device *chip)
{
	int i;
    unsigned char val = 0;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	for (i = 0; i < 10; i++)
	{
		/* do 10 time read write */
		wk2xxx_read_reg(chip, WK2XXX_GPORT, WK2XXX_GENA, &val);
		printk("rval = 0x%x, time %d\n", val, i);
		if ((val & 0xF0) == 0x30)
			printk("Nice, :) SPI OK %d time %d\n", val, i);
         wk2xxx_write_reg(chip, WK2XXX_GPORT, WK2XXX_GENA, i);
	}

	return 0;
}

static struct spi_driver wk2xxx_driver = {
	.driver = {
		.name           = "wk2xxxspi",
		.bus            = &spi_bus_type,
		.owner          = THIS_MODULE,
	},

	.probe          = wk2xxx_probe,
	.remove         = wk2xxx_remove,
};

static int wk2xxx_init(void)
{
	int retval;
	retval = spi_register_driver(&wk2xxx_driver);
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return retval;
}

static void wk2xxx_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	spi_unregister_driver(&wk2xxx_driver);
}

module_init(wk2xxx_init);
module_exit(wk2xxx_exit);

MODULE_AUTHOR("zeroway");
MODULE_DESCRIPTION("wk2xxx spi2uart");
MODULE_LICENSE("GPL");
