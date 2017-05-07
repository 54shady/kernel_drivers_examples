#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#define DEBUG
#define FIREFLY_SPI_READ_ID_CMD 0x9F
#define FIREFLY_SPI_PP 0x2
#define FIREFLY_SPI_READ 0x3
#define USE_TIMER

static int w25q128fv_spi_read_w25x_id_0(struct spi_device *spi)
{
	int	status;
	char tbuf[]={FIREFLY_SPI_READ_ID_CMD};
	char rbuf[5];

	struct spi_transfer	t = {
		.tx_buf		= tbuf,
		.len		= sizeof(tbuf),
	};

	struct spi_transfer     r = {
		.rx_buf         = rbuf,
		.len            = sizeof(rbuf),
	};
	struct spi_message      m;

	/* 初始化message */
	spi_message_init(&m);

	/* 将tx rx buffer加入到message */
	spi_message_add_tail(&t, &m);
	spi_message_add_tail(&r, &m);

	/* 发送message */
	status = spi_sync(spi, &m);
	dev_err(&spi->dev, "%s: ID = %02x %02x %02x %02x %02x\n", __FUNCTION__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4]);

	return status;
}

static int w25q128fv_spi_read_w25x_id_1(struct spi_device *spi)
{
	int	status;
	char tbuf[] = {FIREFLY_SPI_READ_ID_CMD};
	char rbuf[5];

	/* 先发送写数据后读回数据 */
	status = spi_write_then_read(spi, tbuf, sizeof(tbuf), rbuf, sizeof(rbuf));
	dev_err(&spi->dev, "%s: ID = %02x %02x %02x %02x %02x\n", __FUNCTION__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4]);

	return status;
}

#ifdef USE_TIMER
struct self_define_struct {
	struct timer_list timer;
};

struct self_define_struct g_sds;
#define EXPIRES_PERIOD	(5*HZ)

static void timeout_handler(unsigned long tdata)
{
	struct spi_device *spi = (struct spi_device *)tdata;

	g_sds.timer.expires = jiffies + EXPIRES_PERIOD;
	add_timer(&g_sds.timer);
	w25q128fv_spi_read_w25x_id_0(spi);
	w25q128fv_spi_read_w25x_id_1(spi);
}
#endif

static int w25q128fv_spi_probe(struct spi_device *spi)
{
    int ret = 0;
    struct device_node __maybe_unused *np = spi->dev.of_node;

    dev_dbg(&spi->dev, "Firefly SPI demo program\n");

	if(!spi)
		return -ENOMEM;

	dev_err(&spi->dev, "w25q128fv_spi_probe: setup mode %d, %s%s%s%s%u bits/w, %u Hz max\n",
			(int) (spi->mode & (SPI_CPOL | SPI_CPHA)),
			(spi->mode & SPI_CS_HIGH) ? "cs_high, " : "",
			(spi->mode & SPI_LSB_FIRST) ? "lsb, " : "",
			(spi->mode & SPI_3WIRE) ? "3wire, " : "",
			(spi->mode & SPI_LOOP) ? "loopback, " : "",
			spi->bits_per_word, spi->max_speed_hz);

#ifdef USE_TIMER
	/* init timer */
	init_timer(&g_sds.timer);

	/* setup the timer */
	g_sds.timer.expires = jiffies + EXPIRES_PERIOD;
	g_sds.timer.function = timeout_handler;
	g_sds.timer.data = (unsigned long)spi;

	/* add to system */
	add_timer(&g_sds.timer);
#else
#endif
    return ret;
}

static struct of_device_id firefly_match_table[] = {
	{ .compatible = "firefly,w25q128fv",},
	{},
};

static struct spi_driver w25q128fv_spi_driver = {
	.driver = {
		.name = "firefly-spi",
		.owner = THIS_MODULE,
		.of_match_table = firefly_match_table,
	},
	.probe = w25q128fv_spi_probe,
};

static int w25q128fv_spi_init(void)
{
	return spi_register_driver(&w25q128fv_spi_driver);
}
module_init(w25q128fv_spi_init);

static void w25q128fv_spi_exit(void)
{
	spi_unregister_driver(&w25q128fv_spi_driver);
#ifdef USE_TIMER
	del_timer(&g_sds.timer);
#endif
}
module_exit(w25q128fv_spi_exit);

MODULE_AUTHOR("M_O_Bz@163.com");
MODULE_DESCRIPTION("Firefly SPI demo driver, by zeroway");
MODULE_ALIAS("platform:firefly-spi");
MODULE_LICENSE("GPL");
