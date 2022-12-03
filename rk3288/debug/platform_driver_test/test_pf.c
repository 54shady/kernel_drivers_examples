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
#include <linux/regulator/consumer.h>

#if 0
/ {
	my_test_node {
		compatible = "test_platform";
		VCC_TP-supply = <&ldo4_reg>;
	};
};
#endif

static struct regulator *supply;
static int test_pf_probe(struct platform_device *pdev)
{
	int ret;

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	supply = devm_regulator_get(&pdev->dev, "VCC_TP");
	if (IS_ERR(supply)) {
		printk("regulator get of vdd_ana failed");
		ret = PTR_ERR(supply);
		supply = NULL;
		return -1;
	}

	/* Enable the regulator */
	ret = regulator_enable(supply);
	printk("Enable regulator :) ret = %d\n", ret);

	return 0;
}

static const struct of_device_id test_pf_dt_ids[] = {
	{ .compatible = "test_platform", },
	{}
};

static int test_pf_remove(struct platform_device *dev)
{
	int ret;

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	/* Disable the regulator */
	ret = regulator_disable(supply);
	printk("Disable regulator :( ret = %d\n", ret);
	return 0;
}

static int test_pf_suspend(struct platform_device *dev, pm_message_t state)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int test_pf_resume(struct platform_device *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void test_pf_shutdown(struct platform_device *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static struct platform_driver test_pf_driver = {
	.driver		= {
		.name	= "test platform driver",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(test_pf_dt_ids),
	},
	.probe		= test_pf_probe,
	.remove 	= test_pf_remove,
	.suspend 	= test_pf_suspend,
	.resume 	= test_pf_resume,
	.shutdown 	= test_pf_shutdown,
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
