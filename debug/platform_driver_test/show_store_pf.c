#include <linux/of.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static int g_x = -1;
static int g_y = -1;
static int g_z = -1;

static ssize_t BBBBB_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("x = %d y = %d z = %d\n", g_x, g_y, g_z);
	return 0;
}

static ssize_t BBBBB_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%d %d %d", &g_x, &g_y, &g_z);
	return count;
}

static DEVICE_ATTR(BBBBB_debug, S_IRUGO | S_IWUSR, BBBBB_debug_show, BBBBB_debug_store);

static struct attribute *BBBBB_attrs[] = {
	&dev_attr_BBBBB_debug.attr,
	NULL,
};

static const struct attribute_group BBBBB_attr_group = {
	.attrs = BBBBB_attrs,
};

static int AAAAA_platform_probe(struct platform_device *pdev)
{
	int ret;

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
	.probe		= AAAAA_platform_probe,
};

static int AAAAA_platform_init(void)
{
	return platform_driver_register(&AAAAA_platform_driver);
}

static void AAAAA_platform_exit(void)
{
	platform_driver_unregister(&AAAAA_platform_driver);
}

module_init(AAAAA_platform_init);
module_exit(AAAAA_platform_exit);
MODULE_LICENSE("GPL");
