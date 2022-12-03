#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>

typedef struct
{
	int gpio;
	int irq;
	char name[20];
}int_demo_data_t;

static irqreturn_t int_demo_isr(int irq, void *dev_id)
{
	int_demo_data_t *data = dev_id;

	printk("%s enter, %s: gpio:%d, irq: %d\n", __FUNCTION__, data->name, data->gpio, data->irq);

	return IRQ_HANDLED;
}

static int int_demo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int irq_gpio = -1;
	int irq = -1;
	int ret = 0;
	int i = 0;
	int_demo_data_t *data = NULL;

	if (!dev->of_node)
	{
		dev_err(dev, "no platform data.\n");
		goto err1;
	}

	data = devm_kmalloc(dev, sizeof(*data)*4, GFP_KERNEL);
	if (!data)
	{
		dev_err(dev, "no memory.\n");
		goto err0;
	}

	for (i = 0; i < 4; i++)
	{
		sprintf(data[i].name, "tiny4412,int_gpio%d", i + 1);
		irq_gpio = of_get_named_gpio(dev->of_node, data[i].name, 0);
		if (irq_gpio < 0)
		{
			dev_err(dev, "Looking up %s property in node %s failed %d\n",
					data[i].name, dev->of_node->full_name, irq_gpio);
			goto err1;
		}

		data[i].gpio = irq_gpio;

		irq = gpio_to_irq(irq_gpio);
		if (irq < 0) {
			dev_err(dev,
					"Unable to get irq number for GPIO %d, error %d\n",
					irq_gpio, irq);
			goto err1;
		}

		data[i].irq = irq;

		printk("%s: gpio: %d ---> irq (%d)\n", __FUNCTION__, irq_gpio, irq);

		ret = devm_request_any_context_irq(dev, irq,
				int_demo_isr, IRQF_TRIGGER_FALLING, data[i].name, data+i);
		if (ret < 0) {
			dev_err(dev, "Unable to claim irq %d; error %d\n",
					irq, ret);
			goto err1;
		}
	}

	return 0;

err1:
	devm_kfree(dev, data);
err0:
	return -EINVAL;
}

static int int_demo_remove(struct platform_device *pdev)
{
	printk("%s enter.\n", __FUNCTION__);
	return 0;
}

static const struct of_device_id int_demo_dt_ids[] = {
	{ .compatible = "tiny4412,buttons", },
	{},
};

MODULE_DEVICE_TABLE(of, int_demo_dt_ids);

static struct platform_driver int_demo_driver = {
	.driver             = {
		.name           = "buttons",
		.of_match_table = of_match_ptr(int_demo_dt_ids),
	},
	.probe              = int_demo_probe,
	.remove             = int_demo_remove,
};

static int int_demo_init(void)
{
	int ret;

	ret = platform_driver_register(&int_demo_driver);
	if (ret)
		printk(KERN_ERR "int demo: probe failed: %d\n", ret);

	return ret;
}
module_init(int_demo_init);

static void int_demo_exit(void)
{
	platform_driver_unregister(&int_demo_driver);
}
module_exit(int_demo_exit);

MODULE_DESCRIPTION("tiny4412 buttons");
MODULE_AUTHOR("zeroway");
MODULE_LICENSE("GPL");
