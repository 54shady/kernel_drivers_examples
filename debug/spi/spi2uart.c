#include <mach/gpio.h>
#include <mach/iomux.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/console.h>
#include <linux/serial_core.h>
#include <asm/irq.h>
#include <asm/io.h>
#include "wk2xxx.h"

/* global mutex lock */
static DEFINE_MUTEX(wk2xxxs_reg_lock);

struct wk2xxx_port
{
	struct uart_port port;
	struct spi_device *spi_wk;
	spinlock_t conf_lock;
	struct workqueue_struct *workqueue;
	struct work_struct work;
	int suspending;
	void (*wk2xxx_hw_suspend) (int suspend);
	int tx_done;

	int force_end_work;
	int irq;
	int minor;
	int tx_empty;
	int tx_empty_flag;

	int start_tx_flag;
	int stop_tx_flag;
	int stop_rx_flag;
	int irq_flag;
	int conf_flag;

	int tx_empty_fail;
	int start_tx_fail;
	int stop_tx_fail;
	int stop_rx_fail;
	int irq_fail;
	int conf_fail;

	uint8_t new_lcr;
	uint8_t new_scr;
	uint8_t new_baud1;
	uint8_t new_baud0;
	uint8_t new_pres;
};

static u_int wk2xxx_tx_empty(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void wk2xxx_set_mctrl(struct uart_port *port, u_int mctrl)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static void wk2xxx_stop_tx(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static u_int wk2xxx_get_mctrl(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void wk2xxx_start_tx(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static void wk2xxx_stop_rx(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static void wk2xxx_enable_ms(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static void wk2xxx_break_ctl(struct uart_port *port, int break_state)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static int wk2xxx_startup(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void wk2xxx_shutdown(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static void wk2xxx_termios( struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static const char *wk2xxx_type(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return port->type == PORT_WK2XXX ? "wk2xxx" : NULL;
}

static void wk2xxx_release_port(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

/* Request the memory region(s) being used by port */
static int wk2xxx_request_port(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	return 0;
}

/* 配置串口 */
static void wk2xxx_config_port(struct uart_port *port, int flags)
{
	struct wk2xxx_port *chip = container_of(port,struct wk2xxx_port,port);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	if (flags & UART_CONFIG_TYPE && wk2xxx_request_port(port) == 0)
		chip->port.type = PORT_WK2XXX;
}

static int wk2xxx_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct uart_ops wk2xxx_pops = {
	.tx_empty     = wk2xxx_tx_empty,
	.set_mctrl    = wk2xxx_set_mctrl,
	.get_mctrl    = wk2xxx_get_mctrl,
	.stop_tx      = wk2xxx_stop_tx,
	.start_tx     = wk2xxx_start_tx,
	.stop_rx      = wk2xxx_stop_rx,
	.enable_ms    = wk2xxx_enable_ms,
	.break_ctl    = wk2xxx_break_ctl,
	.startup      = wk2xxx_startup,
	.shutdown     = wk2xxx_shutdown,
	.set_termios  = wk2xxx_termios,
	.type         = wk2xxx_type,
	.release_port = wk2xxx_release_port,
	.request_port = wk2xxx_request_port,
	.config_port  = wk2xxx_config_port,
	.verify_port  = wk2xxx_verify_port,
};

static struct wk2xxx_port wk2xxxs[NR_PORTS] = {
	{
		.tx_done       = 0,
		.port = {
			.line     = 0,
			.ops      = &wk2xxx_pops,
			.uartclk  = WK_CRASTAL_CLK,
			.fifosize = 64,
			.iobase   = 1,
			.iotype   = SERIAL_IO_PORT,
			.flags    = ASYNC_BOOT_AUTOCONF,
		},
	},
	{
		.tx_done       = 0,
		.port = {
			.line     = 1,
			.ops      = &wk2xxx_pops,
			.uartclk  = WK_CRASTAL_CLK,
			.fifosize = 64,
			.iobase   = 2,
			.iotype   = SERIAL_IO_PORT,
			.flags    = ASYNC_BOOT_AUTOCONF,
		},
	},
	{
		.tx_done       = 0,
		.port = {
			.line     = 2,
			.ops      = &wk2xxx_pops,
			.uartclk  = WK_CRASTAL_CLK,
			.fifosize = 64,
			.iobase   = 3,
			.iotype   = SERIAL_IO_PORT,
			.flags    = ASYNC_BOOT_AUTOCONF,
		},
	},
	{
		.tx_done       = 0,
		.port = {
			.line     = 3,
			.ops      = &wk2xxx_pops,
			.uartclk  = WK_CRASTAL_CLK,
			.fifosize = 64,
			.iobase   = 4,
			.iotype   = SERIAL_IO_PORT,
			.flags    = ASYNC_BOOT_AUTOCONF,
		},
	},
};

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

static int wk2xxx_write_reg(struct spi_device *spidev, uint8_t port, uint8_t reg, uint8_t data)
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
	status = spi_sync(spidev, &msg);
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

static int wk2xxx_read_reg(struct spi_device *spidev, uint8_t port, uint8_t reg, uint8_t *data)
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
	status = spi_sync(spidev, &msg);
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

static struct uart_driver wk2xxx_uart_driver = {
	.owner       = THIS_MODULE,
	.major       = SERIAL_WK2XXX_MAJOR,
	.driver_name = "ttySWK",
	.dev_name    = "ttysWK",
	.minor       = MINOR_START,
	.nr          = NR_PORTS,
	.cons        = NULL
};

static int wk2xxx_remove(struct spi_device *spidev)
{
	int i;
	struct wk2xxx_port *chip;
	for (i = 0;i < NR_PORTS; i++)
	{
		chip = &wk2xxxs[i];
		uart_remove_one_port(&wk2xxx_uart_driver, &chip->port);
	}

	uart_unregister_driver(&wk2xxx_uart_driver);
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static int wk2xxx_probe(struct spi_device *spidev)
{
	int i;
	int status;
	unsigned char val = 0;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

#ifdef TEST_DRIVER
	for (i = 0; i < 10; i++)
	{
		/* do 10 time read write */
		wk2xxx_read_reg(spidev, WK2XXX_GPORT, WK2XXX_GENA, &val);
		printk("rval = 0x%x, time %d\n", val, i);
		if ((val & 0xF0) == 0x30)
			printk("Nice, :) SPI OK %d time %d\n", val, i);
		wk2xxx_write_reg(spidev, WK2XXX_GPORT, WK2XXX_GENA, i);
	}
#endif

	/* 注册串口驱动 */
	status = uart_register_driver(&wk2xxx_uart_driver);
	if (status)
	{
		printk("register uart error\n");
		return status;
	}
	printk("Register Uart Done\n");

	/* 初始化各个子串口 */
	for (i = 0; i < NR_PORTS; i++)
	{
		struct wk2xxx_port *chip = &wk2xxxs[i];
		chip->spi_wk = spidev;
		chip->port.irq  = gpio_to_irq(RK30_PIN0_PA7);
		status = uart_add_one_port(&wk2xxx_uart_driver, &chip->port);
		if(status < 0)
		{
			printk("uart_add_one_port failed for uart %d \n", i);
			return status;
		}
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
