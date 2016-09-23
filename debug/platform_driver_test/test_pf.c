#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <asm/div64.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/gpio.h>

#if 0
/ {
	my_test_node {
		compatible = "test_platform";
	};
};
#endif
static int test_pf_probe(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static const struct of_device_id test_pf_dt_ids[] = {
	{ .compatible = "test_platform", },
	{}
};

static struct platform_driver test_pf_driver = {
	.probe		= test_pf_probe,
	.driver		= {
		.name	= "test platform driver",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(test_pf_dt_ids),
	},
};

static int __init test_pf_init(void)
{
	return platform_driver_register(&test_pf_driver);
}

static void __exit test_pf_exit(void)
{
	platform_driver_unregister(&test_pf_driver);
}

module_init(test_pf_init);
module_exit(test_pf_exit);
MODULE_LICENSE("GPL");
