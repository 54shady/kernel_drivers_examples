#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>

#define MAX_SPI_DEV_NUM 10
#define BUFF_LEN_MAX 1024

struct self_define_struct {
    struct device *dev;
    struct spi_device *spi;
    char *rx_buf;
    int rx_len;
    char *tx_buf;
    int tx_len;

	struct delayed_work dwork;
	struct workqueue_struct *wq;
};

static struct self_define_struct *g_sds[MAX_SPI_DEV_NUM];

static const struct of_device_id rockchip_spi_test_dt_match[] = {
    {
        .compatible = "rockchip,spi_test_bus4_cs0",
    },
    {},
};
MODULE_DEVICE_TABLE(of, rockchip_spi_test_dt_match);

int spi_read_slt(int id, char *rxbuf, size_t n)
{
	int ret = -1;
	struct spi_device *spi = NULL;
	struct spi_transfer t = {
		.rx_buf         = rxbuf,
		.len            = n,
	};

	struct spi_message      m;

	if (id >= MAX_SPI_DEV_NUM)
		return ret;
	if (!g_sds[id]) {
		pr_err("g_spi.%d is NULL\n", id);
		return ret;
	} else {
		spi = g_sds[id]->spi;
	}

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);
}

static int rockchip_spi_test_remove(struct spi_device* spi)
{
    printk("%s, %d\n", __func__, __LINE__);
    return 0;
}

static ssize_t spi_test_read(struct file* file, char __user* buf, size_t count, loff_t* offset)
{
	return 0;
}

static ssize_t spi_test_write(struct file* file,
    const char __user* buf, size_t n, loff_t* offset)
{
	return 0;
}

static long spi_test_ioctl(struct file* file, unsigned cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations spi_test_fops = {
    .write = spi_test_write,
    .read = spi_test_read,
    .unlocked_ioctl = spi_test_ioctl,
};

static struct miscdevice spi_test_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "spimisc",
    .fops = &spi_test_fops,
};

void dwork_fn(struct work_struct *work)
{
	/* avoid using malloc while the heavily call work function */
	//char *rxbuf = NULL;
    //rxbuf = kzalloc(BUFF_LEN_MAX, GFP_KERNEL);
	char rxbuf[BUFF_LEN_MAX];
	struct self_define_struct *psds = NULL;

    pr_info("Executing workqueue function!\n");

	if (spi_read_slt(0, rxbuf, BUFF_LEN_MAX) < 0) {
		pr_info("spi read error\n");
	} else {
		print_hex_dump(KERN_ERR, "SPI RX: ",
				   DUMP_PREFIX_OFFSET,
				   16,
				   1,
				   rxbuf,
				   BUFF_LEN_MAX,
				   1);
	}

	//kfree(rxbuf);

    /* Reschedule itself in the workqueue */
	psds = container_of(work, struct self_define_struct, dwork.work);
    queue_delayed_work(psds->wq, &psds->dwork, msecs_to_jiffies(20));
}

static int rockchip_spi_test_probe(struct spi_device* spi)
{
    int ret;
    int id = 0;
    struct self_define_struct *psds = NULL;

    if (!spi) {
        printk("spi_setup erro:%d", __LINE__);
        return -ENOMEM;
    }

    if (!spi->dev.of_node) {
        printk("spi_setup erro:%d", __LINE__);
        return -ENOMEM;
    }

    psds = kzalloc(sizeof(struct self_define_struct), GFP_KERNEL);
    if (!psds) {
        dev_err(&spi->dev, "ERR: no memory for psds\n");
        printk("spi_setup erro:%d", __LINE__);
        return -ENOMEM;
    }

    spi->bits_per_word = 8;
    spi->max_speed_hz = 10000000;
    psds->spi = spi;
    psds->dev = &spi->dev;
    ret = spi_setup(spi);
    if (ret < 0) {
        dev_err(psds->dev, "ERR: fail to setup spi\n");
        printk("spi_setup erro:%d", __LINE__);
        return -1;
    }

    if (of_property_read_u32(spi->dev.of_node, "id", &id)) {
        dev_warn(&spi->dev, "fail to get id, default set 0\n");
        id = 0;
    }

    g_sds[id] = psds;

    printk("name=%s,bus_num=%d,cs=%d,mode=%d,speed=%d\n",
		spi->modalias, spi->master->bus_num, spi->chip_select, spi->mode, spi->max_speed_hz);

	/* setup work */
    psds->wq = create_singlethread_workqueue("spi-test-wq");
    if (!psds->wq) {
        pr_err("Failed to create workqueue\n");
        return -ENOMEM;
    }

    /* Initialize work */
    INIT_DELAYED_WORK(&psds->dwork, dwork_fn);

    /* Queue up the work */
    queue_delayed_work(psds->wq, &psds->dwork, msecs_to_jiffies(20));

    return ret;
}

static struct spi_driver spi_rockchip_test_driver = {
    .driver = {
        .name = "spi_test",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(rockchip_spi_test_dt_match),
    },
    .probe = rockchip_spi_test_probe,
    .remove = rockchip_spi_test_remove,
};

static int __init workqueue_module_init(void)
{
    pr_info("Initializing workqueue module\n");

    misc_register(&spi_test_misc);
    spi_register_driver(&spi_rockchip_test_driver);

    return 0;
}

static void __exit workqueue_module_exit(void)
{
	struct self_define_struct *psds = g_sds[0];

    pr_info("Exiting spi delay work\n");

    /* Cleanup workqueue */
    cancel_delayed_work_sync(&psds->dwork);
    destroy_workqueue(psds->wq);

	misc_deregister(&spi_test_misc);
    spi_unregister_driver(&spi_rockchip_test_driver);
}

module_init(workqueue_module_init);
module_exit(workqueue_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux workqueue example module");
MODULE_VERSION("1.0");
