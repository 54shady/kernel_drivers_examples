#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

struct locktest_chip {
	struct mutex lock;
};

static int mutexlock_test_probe(struct platform_device *pdev)
{
	struct locktest_chip *chip;
	int ret = 0;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* alloc for chip point */
	chip = devm_kzalloc(&pdev->dev, sizeof(struct locktest_chip), GFP_KERNEL);
	if (!chip)
	{
		printk("no memory\n");
		ret = -ENOMEM;
	}

	/* init mutext lock */
	mutex_init(&chip->lock);

	/* set pdata */
	dev_set_drvdata(&pdev->dev, (void *)chip);

	/* visit the race conditon area */
	if (!mutex_trylock(&chip->lock))
	{
		printk("can not get the lock\n");
		return -1;
	}
	printk("Accessing race condition area...\n");

	/*
	 * mutext can sleep
	 * sleep will cause context switch
	 */
	msleep(1);
	mutex_unlock(&chip->lock);

	return ret;
}

static int mutexlock_test_remove(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static const struct of_device_id mutexlock_test_dt_ids[] = {
	{.compatible = "lock-test",},
	{},
};

static struct platform_driver mutexlock_test = {
	.driver		= {
		.name	= "locktest example",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(mutexlock_test_dt_ids),
	},
	.probe		= mutexlock_test_probe,
	.remove 	= mutexlock_test_remove,
};

module_platform_driver(mutexlock_test);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zeroway <M_O_Bz@163.com>");
MODULE_DESCRIPTION("locktest");
