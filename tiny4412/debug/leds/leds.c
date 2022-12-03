#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#define LEDS_NUMBER 4

typedef struct
{
	int gpio;
	int state;
	struct pinctrl *pctrl;
	struct pinctrl_state *sleep_pstate;
	struct pinctrl_state *active_pstate;
	char name[20];
} leds_data_t;

static u32 __iomem *GPM4_BASE;
static u32 __iomem *GPM4CON;
static u32 __iomem *GPM4DAT;
static u32 __iomem *GPM4PUD;
static u32 __iomem *GPM4DRV;

static ssize_t leds_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	leds_data_t *data = dev_get_drvdata(dev);
	u32 config, pud, drv, dat;

	config = readl(GPM4CON);
	dat = readl(GPM4DAT);
	pud = readl(GPM4PUD);
	drv = readl(GPM4DRV);

	return snprintf(buf, PAGE_SIZE, "gpio0: %s, gpio1: %s, gpio2: %s, gpio3: %s, cfg: 0x%x, pud: 0x%x, drv: 0x%x, dat = 0x%x\n", data[0].state?"active":"sleep", data[1].state?"active":"sleep", data[2].state?"active":"sleep", data[3].state?"active":"sleep", config, pud, drv, dat);
}

static ssize_t leds_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	leds_data_t *data = dev_get_drvdata(dev);
	char state[10];
	int value = 0;
	int i;

	if (sscanf(buf, "%s", state) != 1)
		return -EINVAL;

	if (strncmp(state, "active", strlen("active")) == 0)
	{
		for (i = 0; i < LEDS_NUMBER; i++)
		{
			pinctrl_select_state(data[i].pctrl, data[i].active_pstate);
			data[i].state = 1;
		}
	}
	else if (strncmp(state, "sleep", strlen("sleep")) == 0)
	{
		for (i = 0; i < LEDS_NUMBER; i++)
		{
			pinctrl_select_state(data[i].pctrl, data[i].sleep_pstate);
			data[i].state = 0;
		}
	}
	else if (sscanf(buf, "%d", &value))
	{
		for (i = 0; i < LEDS_NUMBER; i++)
		{
			if (value & (1 << i))
				gpio_set_value(data[i].gpio, 1);
			else
				gpio_set_value(data[i].gpio, 0);
		}
	}
	else {
		/* do nothing */
	}

	return count;
}

static DEVICE_ATTR(leds, 0644, leds_show, leds_store);

static int leds_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;
	int i = 0;
	int gpio = -1;
	leds_data_t *data = NULL;
	struct resource *res = NULL;
	u32 config, pud, drv, dat;

	printk("%s enter.\n", __FUNCTION__);

	if (!dev->of_node) {
		dev_err(dev, "no platform data.\n");
		goto err1;
	}

	data = devm_kzalloc(dev, sizeof(*data) * LEDS_NUMBER, GFP_KERNEL);
	if (!data) {
		dev_err(dev, "no memory.\n");
		goto err0;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	GPM4_BASE = devm_ioremap(dev, res->start, res->end - res->start);
	printk("remap: 0x%x ---> 0x%x to %p\n", res->start, res->end, GPM4_BASE);
	if (IS_ERR(GPM4_BASE)) {
		dev_err(dev, "remap failed.\n");
		devm_kfree(dev, data);
		return PTR_ERR(GPM4_BASE);
	}

	GPM4CON = (u32 *)((u32)GPM4_BASE + 0x2E0);
	GPM4DAT = (u32 *)((u32)GPM4_BASE + 0x2E4);
	GPM4PUD = (u32 *)((u32)GPM4_BASE + 0x2E8);
	GPM4DRV = (u32 *)((u32)GPM4_BASE + 0x2EC);

	for (i = 0; i < LEDS_NUMBER; i++)
	{
		gpio = of_get_named_gpio(dev->of_node, "tiny4412,gpio", i);
		if (gpio < 0)
		{
			dev_err(dev, "Looking up %s (%d) property in node %s failed %d\n",
					"tiny4412,gpio", i, dev->of_node->full_name, gpio);
			goto err2;
		}

		data[i].gpio = gpio;
		if (gpio_is_valid(gpio))
		{
			sprintf(data[i].name, "tiny4412.led%d", i);
			ret = devm_gpio_request_one(dev, gpio, GPIOF_INIT_HIGH, data[i].name);
			if (ret < 0) {
				dev_err(dev, "request gpio %d failed.\n", gpio);
				goto err2;
			}
		}

		data[i].pctrl = devm_pinctrl_get(dev);
		if (IS_ERR(data[i].pctrl))
		{
			dev_err(dev, "pinctrl get failed.\n");
			devm_gpio_free(dev, gpio);
			goto err2;
		}

		data[i].sleep_pstate = pinctrl_lookup_state(data[i].pctrl, "gpio_sleep");
		if (IS_ERR(data[i].sleep_pstate))
		{
			dev_err(dev, "look up sleep state failed.\n");
			devm_pinctrl_put(data[i].pctrl);
			devm_gpio_free(dev, gpio);
			goto err2;
		}

		data[i].active_pstate = pinctrl_lookup_state(data[i].pctrl, "gpio_active");
		if (IS_ERR(data[i].active_pstate))
		{
			dev_err(dev, "look up sleep state failed.\n");
			devm_pinctrl_put(data[i].pctrl);
			devm_gpio_free(dev, gpio);
			goto err2;
		}
	}

	dev_set_drvdata(dev, data);

	device_create_file(dev, &dev_attr_leds);

	config = readl(GPM4CON);
	dat = readl(GPM4DAT);
	pud = readl(GPM4PUD);
	drv = readl(GPM4DRV);

	printk("default state: cfg: 0x%x, pud: 0x%x, drv: 0x%x, dat = 0x%x\n", config, pud, drv, dat);

	for (i = 0; i < LEDS_NUMBER; i++)
	{
		pinctrl_select_state(data[i].pctrl, data[i].active_pstate);
		data[i].state = 1;
	}

	config = readl(GPM4CON);
	dat = readl(GPM4DAT);
	pud = readl(GPM4PUD);
	drv = readl(GPM4DRV);
	printk("%s, active state: cfg: 0x%x, pud: 0x%x, drv: 0x%x, dat = 0x%x\n", __FUNCTION__, config, pud, drv, dat);

	return 0;

err2:
	devm_iounmap(dev, GPM4_BASE);
err1:
	devm_kfree(dev, data);
err0:
	return -EINVAL;
}

static int leds_remove(struct platform_device *pdev)
{
	printk("%s enter.\n", __FUNCTION__);
	device_remove_file(&pdev->dev, &dev_attr_leds);

	return 0;
}

static const struct of_device_id leds_dt_ids[] = {
	{ .compatible = "tiny4412,leds", },
	{},
};

MODULE_DEVICE_TABLE(of, leds_dt_ids);

static struct platform_driver leds_driver = {
	.driver             = {
		.name           = "leds",
		.of_match_table = of_match_ptr(leds_dt_ids),
	},
	.probe              = leds_probe,
	.remove             = leds_remove,
};

static int leds_init(void)
{
	int ret;

	ret = platform_driver_register(&leds_driver);
	if (ret)
		printk(KERN_ERR "int demo: probe failed: %d\n", ret);

	return ret;
}
module_init(leds_init);

static void leds_exit(void)
{
	platform_driver_unregister(&leds_driver);
}
module_exit(leds_exit);

MODULE_DESCRIPTION("tiny4412 leds");
MODULE_AUTHOR("zeroway");
MODULE_LICENSE("GPL");
