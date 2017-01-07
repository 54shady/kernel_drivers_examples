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
#include <linux/workqueue.h>
#include <asm/irq.h>
#include <asm/io.h>
#include "wk2xxx.h"

#define WK2XXX_PAGE1        1
#define WK2XXX_PAGE0        0

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



static struct uart_driver wk2xxx_uart_driver = {
	.owner       = THIS_MODULE,
	.major       = SERIAL_WK2XXX_MAJOR,
	.driver_name = "ttySWK",
	.dev_name    = "ttysWK",
	.minor       = MINOR_START,
	.nr          = NR_PORTS,
	.cons        = NULL
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

/*
 * echo 1 > /dev/ttysWK<0|1|2|3>
 * will call function below
 */
static void wk2xxx_start_tx(struct uart_port *port)
{
	struct wk2xxx_port *chip = container_of(port, struct wk2xxx_port, port);

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	queue_work(chip->workqueue, &chip->work);
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

static void wk2xxx_rx_chars(struct uart_port *port)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static void wk2xxx_tx_chars(struct uart_port *port)
{
    uint8_t fsr,tfcnt,dat[1],txbuf[255]={0};
    int count,tx_count,i;

    struct wk2xxx_port *s = container_of(port,struct wk2xxx_port,port);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	wk2xxx_write_reg(s->spi_wk,s->port.iobase,WK2XXX_SPAGE,WK2XXX_PAGE0);
	if (s->port.x_char)
	{
		printk("x_char = 0x%x\n", s->port.x_char);
		wk2xxx_write_reg(s->spi_wk,s->port.iobase,WK2XXX_FDAT,s->port.x_char);
		s->port.icount.tx++;
		s->port.x_char = 0;
		goto out;
	}

	if(uart_circ_empty(&s->port.state->xmit) || uart_tx_stopped(&s->port))
	{
		printk("uart circ empty\n");
		goto out;
	}

	wk2xxx_read_reg(s->spi_wk,s->port.iobase,WK2XXX_FSR,dat);
	fsr = dat[0];

	wk2xxx_read_reg(s->spi_wk,s->port.iobase,WK2XXX_TFCNT,dat);
	tfcnt= dat[0];
	printk("tfcnt = %d\n", tfcnt);

    if(tfcnt==0)
    {
      if(fsr & WK2XXX_TFULL)
        {
        tfcnt=255;
        tx_count=0;
        }
      else
        {
        tfcnt=0;
        tx_count=255;
        }
    }
    else
    {
        tx_count=255-tfcnt;
        printk(KERN_ALERT "wk2xxx_tx_chars2   tx_count:%x,port = %x\n",tx_count,s->port.iobase);
    }

    printk(KERN_ALERT "fsr:%x\n",fsr);
printk("tx_count = %d\n", tx_count);
count = tx_count;
	i=0;
	do
	{
		if(uart_circ_empty(&s->port.state->xmit))
		{
			printk("zeroway, circ empty\n");
			break;
		}
		txbuf[i]=s->port.state->xmit.buf[s->port.state->xmit.tail];
		s->port.state->xmit.tail = (s->port.state->xmit.tail + 1) & (UART_XMIT_SIZE - 1);
		s->port.icount.tx++;
		i++;
		printk("tx_chars:0x%x--\n",txbuf[i-1]);

	}while(--count>0);
	printk("tx_chars I:0x%x--\n",i);

	for(count=0;count<i;count++)
	{
		printk("txbuf[%d] = %c\n", count, txbuf[count]);
		wk2xxx_write_reg(s->spi_wk,s->port.iobase,WK2XXX_FDAT,txbuf[count]);
	}

out:wk2xxx_read_reg(s->spi_wk,s->port.iobase,WK2XXX_FSR,dat);
	fsr = dat[0];
	if(((fsr&WK2XXX_TDAT)==0)&&((fsr&WK2XXX_TBUSY)==0))
	{
		if (uart_circ_chars_pending(&s->port.state->xmit) < WAKEUP_CHARS)
			uart_write_wakeup(&s->port);

		if (uart_circ_empty(&s->port.state->xmit))
		{
			wk2xxx_stop_tx(&s->port);
		}
	}
}

static void wk2xxx_work(struct work_struct *w)
{
#if 0
	unsigned char val;
	unsigned char val_bak;
	unsigned char tmp;
	struct wk2xxx_port *chip = container_of(w, struct wk2xxx_port, work);

	wk2xxx_read_reg(chip->spi_wk, chip->port.iobase, WK2XXX_SIER, &val);
	val |= WK2XXX_TFTRIG_IEN;
	val_bak = val;
	while (1)
	{
		wk2xxx_write_reg(chip->spi_wk,WK2XXX_GPORT,WK2XXX_GENA,0x0F);
		wk2xxx_write_reg(chip->spi_wk, chip->port.iobase, WK2XXX_SIER, val);
		msleep(100);
		wk2xxx_read_reg(chip->spi_wk, chip->port.iobase,WK2XXX_SIER, &val);
		printk("val = 0x%x, val_bak = 0x%x\n", val, val_bak);
	}
#endif
#if 1
	uint8_t gier,sifr0,sifr1,sifr2,sifr3,sier1,sier0,sier2,sier3;
    uint8_t fsr,tfcnt,dat[1],txbuf[255]={0};
	unsigned int  pass_counter = 0;
	uint8_t sifr,gifr,sier;

	unsigned char val;
	unsigned char val_bak;
	struct wk2xxx_port *chip = container_of(w, struct wk2xxx_port, work);

	/* xfer */
	wk2xxx_read_reg(chip->spi_wk, chip->port.iobase, WK2XXX_SIER, &val);
	val |= WK2XXX_TFTRIG_IEN;
	val_bak = val;
	printk("val = 0x%x\n", val);
	printk("port %d\n", chip->port.iobase);
	do
	{
		wk2xxx_write_reg(chip->spi_wk,WK2XXX_GPORT,WK2XXX_GENA,0x0F);
		wk2xxx_write_reg(chip->spi_wk, chip->port.iobase, WK2XXX_SIER, val);
		msleep(5);
		wk2xxx_read_reg(chip->spi_wk, chip->port.iobase,WK2XXX_SIER, &val);
		printk("val = 0x%x, val_bak = 0x%x\n", val, val_bak);
	}while(val != val_bak);


	printk("%s SIER = 0x%x\n", __FUNCTION__, val);

	/* config */
	wk2xxx_read_reg(chip->spi_wk,WK2XXX_GPORT,WK2XXX_GIFR ,dat);
	gifr = dat[0];
	wk2xxx_read_reg(chip->spi_wk,WK2XXX_GPORT,WK2XXX_GIER ,dat);
	gier = dat[0];
	wk2xxx_write_reg(chip->spi_wk,1,WK2XXX_SPAGE,WK2XXX_PAGE0);//set register in page0
	wk2xxx_write_reg(chip->spi_wk,2,WK2XXX_SPAGE,WK2XXX_PAGE0);//set register in page0
	wk2xxx_write_reg(chip->spi_wk,3,WK2XXX_SPAGE,WK2XXX_PAGE0);//set register in page0
	wk2xxx_write_reg(chip->spi_wk,4,WK2XXX_SPAGE,WK2XXX_PAGE0);//set register in page0

	wk2xxx_read_reg(chip->spi_wk,1,WK2XXX_SIFR,&sifr0);
	wk2xxx_read_reg(chip->spi_wk,2,WK2XXX_SIFR,&sifr1);
	wk2xxx_read_reg(chip->spi_wk,3,WK2XXX_SIFR,&sifr2);
	wk2xxx_read_reg(chip->spi_wk,4,WK2XXX_SIFR,&sifr3);

	wk2xxx_read_reg(chip->spi_wk,1,WK2XXX_SIER,&sier0);
	wk2xxx_read_reg(chip->spi_wk,2,WK2XXX_SIER,&sier1);
	wk2xxx_read_reg(chip->spi_wk,3,WK2XXX_SIER,&sier2);
	wk2xxx_read_reg(chip->spi_wk,4,WK2XXX_SIER,&sier3);

	printk("gifr = 0x%x, port = %d\n", chip->port.iobase);

	wk2xxx_read_reg(chip->spi_wk,chip->port.iobase,WK2XXX_SIFR,dat);
	sifr = dat[0];
	wk2xxx_read_reg(chip->spi_wk,chip->port.iobase,WK2XXX_SIER,dat);
	sier = dat[0];

	printk("irq_app.sifr:%x sier:%x \n",sifr,sier);
	do {
		wk2xxx_tx_chars(&chip->port);
	} while ((sifr&WK2XXX_RXOVT_INT)||(sifr & WK2XXX_RFTRIG_INT)||((sifr & WK2XXX_TFTRIG_INT)&&(sier & WK2XXX_TFTRIG_IEN)));

	wk2xxx_read_reg(chip->spi_wk,chip->port.iobase,WK2XXX_SIFR,dat);
	sifr = dat[0];
	wk2xxx_read_reg(chip->spi_wk,chip->port.iobase,WK2XXX_SIER,dat);
	sier = dat[0];
	printk("irq_app..rx....tx  sifr:%x sier:%x port:%x\n",sifr,sier,chip->port.iobase);
#endif
}

static irqreturn_t wk2xxx_irq(int irq, void *dev_id)
{
    struct wk2xxx_port *chip = dev_id;
    disable_irq_nosync(chip->port.irq);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
    return IRQ_HANDLED;
}

/* 配置串口 */
static void wk2xxx_config_port(struct uart_port *port, int flags)
{
	struct wk2xxx_port *chip = container_of(port, struct wk2xxx_port, port);

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

static int wk2xxx_remove(struct spi_device *spidev)
{
	int i;
	struct wk2xxx_port *chip;

	for (i = 0;i < NR_PORTS; i++)
	{
		chip = &wk2xxxs[i];
		uart_remove_one_port(&wk2xxx_uart_driver, &chip->port);
		free_irq(chip->port.irq, chip);
		destroy_workqueue(chip->workqueue);
		chip->workqueue = NULL;
	}

	uart_unregister_driver(&wk2xxx_uart_driver);
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	return 0;
}



static int wk2xxx_probe(struct spi_device *spidev)
{
	char name[12];
	int i;
	int status;
	unsigned char val = 0;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

#define TEST_DRIVER
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

        sprintf(name, "wk2xxx-%d", (uint8_t)chip->port.iobase);
		chip->workqueue = create_workqueue(name);
		if (!chip->workqueue)
		{
			printk("create workqueue, ERROR\n");
			return -1;
		}

		INIT_WORK(&chip->work, wk2xxx_work);

		/* request irq */
		if (request_irq(chip->port.irq, wk2xxx_irq, IRQF_SHARED|IRQF_TRIGGER_LOW, "wk2xxxspi", chip) < 0)
		{
			printk("request IRQ ERROR\n");
			return -1;
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
