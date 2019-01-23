#include <linux/of.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/kthread.h>

/* modules data */
struct self_define_data {
	struct task_struct *task;
	wait_queue_head_t wqh;
	char data[1024];
};

static struct self_define_data sdd;
static int g_condition = 0;

static ssize_t BBBBB_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_debug("g_condition = %d\n", g_condition);

	return 0;
}

/* echo hello > /sys/devices/my_test_node/BBBBB_debug */
static ssize_t BBBBB_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_debug("trigger g_condition ok\n");

	/* copy data from userspace */
	memset(sdd.data, 0, 1024);
	memcpy(sdd.data, buf, count);
	g_condition = 1;

	wake_up(&sdd.wqh);

	return count;
}

/* /sys/devices/my_test_node/BBBBB_debug */
static DEVICE_ATTR(BBBBB_debug, S_IRUGO | S_IWUSR, BBBBB_debug_show, BBBBB_debug_store);

static struct attribute *BBBBB_attrs[] = {
	&dev_attr_BBBBB_debug.attr,
	NULL,
};

static const struct attribute_group BBBBB_attr_group = {
	.attrs = BBBBB_attrs,
};

static int A_test_kthread(void *arg)
{
	int ret;
	struct self_define_data *psdd;

	psdd = (struct self_define_data *)arg;

	/*
	 * running the thread while the g_condition equal to 1
	 * other wise just sleeping here
	 */
	while(!kthread_should_stop())
	{
		pr_debug("g_condition = %d\n", g_condition);
		ret = wait_event_interruptible(psdd->wqh, g_condition);
		g_condition = 0;
		if (ret)
			continue;

		/* contition true, do something */
		printk("CheckData ==> %s\n", psdd->data);
	}
}

static int AAAAA_platform_probe(struct platform_device *pdev)
{
	int ret;

	/* create a kthread */
	sdd.task = kthread_run(A_test_kthread, &sdd, "A_test_kthread");
	if(IS_ERR(sdd.task)){
		printk("create A_test_kthread failed");
		return -1;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &BBBBB_attr_group);
	if (ret) {
		printk("failed to create sysfs device attributes\n");
		return -1;
	}
	return 0;
}

static const struct of_device_id AAAAA_platform_dt_ids[] = {
	{ .compatible = "test_platform", },
	{}
};

static int AAAAA_platform_remove(struct platform_device *pdev)
{
	int ret;

	sysfs_remove_group(&pdev->dev.kobj, &BBBBB_attr_group);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct platform_driver AAAAA_platform_driver = {
	.driver		= {
		.name	= "test platform driver",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(AAAAA_platform_dt_ids),
	},
	.probe	= AAAAA_platform_probe,
	.remove = AAAAA_platform_remove,
};

static int AAAAA_platform_init(void)
{
	init_waitqueue_head(&sdd.wqh);
	return platform_driver_register(&AAAAA_platform_driver);
}

static void AAAAA_platform_exit(void)
{
	platform_driver_unregister(&AAAAA_platform_driver);
}

module_init(AAAAA_platform_init);
module_exit(AAAAA_platform_exit);
MODULE_LICENSE("GPL");
