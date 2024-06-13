#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/irq.h>
#include <linux/version.h>
#include <uapi/linux/sched/types.h>

#define MAX_SPI_DEV_NUM 10
#define BUFF_LEN_MAX 1024

#define MEASURE_CUSUME_TIME
#define DEDICATED_RUN

#ifdef DEDICATED_RUN
#define RUN_ON_CPU 6
#endif

#define DELAY_MSEC 20

struct self_define_struct {
	int id;
    struct device *dev;
    struct spi_device *spi;

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

	int cpu = smp_processor_id();
    pr_debug(KERN_INFO "Work handler function executed on CPU %d\n", cpu);

	/* no recommended to set workqueue to FIFO or RR */
	//struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	//sched_setscheduler(current, SCHED_RR, &param);

	psds = container_of(work, struct self_define_struct, dwork.work);

#ifdef MEASURE_CUSUME_TIME
    ktime_t start_time, end_time;
    s64 time_delta;

    start_time = ktime_get();
#endif
	if (spi_read_slt(psds->id, rxbuf, BUFF_LEN_MAX) < 0) {
		pr_info("spi read error\n");
	} else {
#ifdef MEASURE_CUSUME_TIME
		end_time = ktime_get();
		time_delta = ktime_to_ns(ktime_sub(end_time, start_time));
		pr_info(KERN_INFO "Time taken by function: %lld ns\n", time_delta);
#endif

		//print_hex_dump(KERN_ERR, "SPI RX: ",
		//		   DUMP_PREFIX_OFFSET,
		//		   16,
		//		   1,
		//		   rxbuf,
		//		   BUFF_LEN_MAX,
		//		   1);
	}

	//kfree(rxbuf);

    /* Reschedule itself in the workqueue */
#ifdef DEDICATED_RUN
    schedule_delayed_work_on(RUN_ON_CPU, &psds->dwork, msecs_to_jiffies(DELAY_MSEC));
#else
    queue_delayed_work(psds->wq, &psds->dwork, msecs_to_jiffies(DELAY_MSEC));
#endif
}

static irqreturn_t gpio_irq_handler(int irq, void* dev_id)
{
    return IRQ_HANDLED;
}

static int gpio_init(struct device* dev)
{
    int gpio;
    enum of_gpio_flags flag;
    int ret = -ENODEV;
    unsigned int irq_number = 0;
    gpio = of_get_named_gpio_flags(dev->of_node, "gpio_mcu", 0, &flag);
    if (!gpio_is_valid(gpio)) {
        printk("gpio: %d is invalid\n", gpio);
        return -ENODEV;
    }
    irq_number = gpio_to_irq(gpio);
    if (0 != irq_number) {
		/* /sys/kernel/debug/gpio */
        if (devm_gpio_request(dev, gpio, "spi-test-gpio")) {
            dev_err(dev, "irq-gpio: %d request failed!\n", gpio);
            return IRQ_NONE;
        }

        //ret = devm_request_irq(dev, irq_number, gpio_irq_handler, flag, "spi-test-irq", NULL);
		/*
		 * thread irq set default SCHED_FIFO policy by kernel
		 * if CONFIG_PREEMPT_RT is enable
		 *
		 * FF : SCHED_FIFO
		 * RR : SCHED_RR
		 * TS : SCHED_OTHER
		 * ps -eLfc | grep spi-tes
		 * root      519723       2  519723    1 FF   90 16:58 ?        00:00:01 [irq/122-spi-tes]
		 */
        ret = devm_request_threaded_irq(dev, irq_number, NULL, gpio_irq_handler,
				IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				"spi-test-thread-irq", NULL);
        if (ret != 0) {
            dev_err(dev, "Failed to request IRQ: %d\n", ret);
        }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0))
		/* /proc/irq/<irq_number>/affinity_hint */
		irq_set_affinity_hint(irq_number, cpumask_of(RUN_ON_CPU));
#else
		irq_set_affinity(irq_number, cpumask_of(RUN_ON_CPU));
#endif

	}

    pr_info("irq_number %d flag:%d \n", irq_number, flag);

    return ret;
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

	psds->id = id;
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
#ifdef DEDICATED_RUN
    schedule_delayed_work_on(RUN_ON_CPU, &psds->dwork, msecs_to_jiffies(DELAY_MSEC));
#else
    queue_delayed_work(psds->wq, &psds->dwork, msecs_to_jiffies(DELAY_MSEC));
#endif

    gpio_init(&spi->dev);

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
