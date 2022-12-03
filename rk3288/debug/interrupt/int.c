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
#include <linux/of_gpio.h>


#if 0
中断连接的是GPIO0_B1
/ {
	my_test_node {
		compatible = "test_platform";
		irq_gpio = <&gpio0 GPIO_B1 IRQ_TYPE_EDGE_FALLING>;
	};
};
#endif

struct work_struct  g_irq_work;
int g_irq;

static irqreturn_t test_irq_handler(int irq, void *data)
{
	disable_irq_nosync(irq);
	(void)schedule_work(&g_irq_work);
	return IRQ_HANDLED;
}

static void test_irq_work(struct work_struct *work)
{
	printk("read irq\n");
	enable_irq(g_irq);
}

static int int_probe(struct platform_device *pdev)
{
	int ret;
	int irq_pin;
	struct device_node *node = pdev->dev.of_node;

	/* 获取DeviceTree中配置的引脚 */
	irq_pin = of_get_named_gpio(node, "irq_gpio", 0);

	/* 引脚有效则将其转换为对应的IRQ */
	if (gpio_is_valid(irq_pin)) {
		printk("irq pin valid = %d\n", irq_pin);
		g_irq = gpio_to_irq(irq_pin);
	} else {
		printk("irq gpio NOT valid\n");
	}

	/* 申请IQR,成功后可在/proc/interrupts里看到相关信息 */
	ret = request_irq(g_irq , test_irq_handler, IRQF_TRIGGER_LOW|IRQF_DISABLED , "my_test_irq", NULL);
	if (ret) {
		printk("request irq error\n");
		return -1;
	}

	INIT_WORK(&g_irq_work, test_irq_work);

	printk("Probe ok\n");

	return 0;
}

static const struct of_device_id int_dt_ids[] = {
	{ .compatible = "test_platform", },
	{}
};

static int int_remove(struct platform_device *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* 释放中断 */
	free_irq(g_irq, NULL);
	return 0;
}

static int int_suspend(struct platform_device *dev, pm_message_t state)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int int_resume(struct platform_device *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void int_shutdown(struct platform_device *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static struct platform_driver int_driver = {
	.driver		= {
		.name	= "test platform driver",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(int_dt_ids),
	},
	.probe		= int_probe,
	.remove 	= int_remove,
	.suspend 	= int_suspend,
	.resume 	= int_resume,
	.shutdown 	= int_shutdown,
};

static int __init int_init(void)
{
	return platform_driver_register(&int_driver);
}

static void __exit int_exit(void)
{
	platform_driver_unregister(&int_driver);
}

module_init(int_init);
module_exit(int_exit);
MODULE_LICENSE("GPL");
