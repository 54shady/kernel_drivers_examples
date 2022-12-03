#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>

struct consumer2_chip {
	struct i2c_client	*client;
	struct device		*dev;
	struct regulator *supply;
};

static ssize_t consumer2_debug_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 1;
}

/* FIXME:Don't figure it out how it cann't work */
static ssize_t consumer2_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct consumer2_chip *chip;
	int ret;
	int tmp;
	struct regulator *supply;
	tmp = simple_strtoul(buf, NULL, 0);
	chip = container_of(&dev, struct consumer2_chip, dev);
	switch (tmp)
	{
		case 0:
			ret = regulator_disable(chip->supply);
			printk("Disable regulator :( ret = %d\n", ret);
			msleep(2000);
			break;
		case 1:
			ret = regulator_enable(chip->supply);
			printk("Enable regulator :) ret = %d\n", ret);
			msleep(2000);
			break;
		default:
			break;
	};
	return count;
}

static DEVICE_ATTR(consumer2_debug, S_IRUGO | S_IWUSR, consumer2_debug_show,
		   consumer2_debug_store);

static struct attribute *consumer2_attrs[] = {
	&dev_attr_consumer2_debug.attr,
	NULL,
};

static const struct attribute_group consumer2_attr_group = {
	.attrs = consumer2_attrs,
};

static const struct i2c_device_id consumer2_id_table[] = {
	{ "consumer2", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, consumer2_id_table);

static int consumer2_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	static struct regulator *supply;
	struct consumer2_chip *chip;
	int ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		printk("i2c_check_functionality error\n");
		return -ENODEV;
	}

	supply = devm_regulator_get(&client->dev, "VCC_TP");
	if (IS_ERR(supply)) {
		printk("regulator get of vdd_ana failed");
		ret = PTR_ERR(supply);
		supply = NULL;
		return -1;
	}

	chip = kzalloc(sizeof(struct consumer2_chip), GFP_KERNEL);
	if (chip == NULL) {
		printk("kzalloc error\n");
		return -ENOMEM;
	}

	chip->supply = supply;
	chip->client = client;
	chip->dev = &client->dev;
	i2c_set_clientdata(client, chip);

	ret = sysfs_create_group(&client->dev.kobj, &consumer2_attr_group);
	if (ret) {
		printk("failed to create sysfs device attributes\n");
		return -1;
	}

	/* Enable the regulator */
	ret = regulator_enable(chip->supply);
	printk("Enable regulator :) ret = %d\n", ret);

	return 0;
}

static int consumer2_remove(struct i2c_client *client)
{
	int ret;
	struct consumer2_chip *chip = i2c_get_clientdata(client);

	/* Disable the regulator */
	regulator_disable(chip->supply);
	printk("Disable regulator :( ret = %d\n", ret);
	sysfs_remove_group(&client->dev.kobj, &consumer2_attr_group);
	kfree(chip);

	return 0;
}

static const struct of_device_id of_consumer2_match[] = {
	{ .compatible = "Consumer1" },
	{ /* Sentinel */ }
};

static struct i2c_driver consumer2_driver = {
	.driver	= {
		.name	= "consumer2",
		.owner	= THIS_MODULE,
		.of_match_table = of_consumer2_match,
	},
	.probe		= consumer2_probe,
	.remove		= consumer2_remove,
	.id_table	= consumer2_id_table,
};

static int consumer2_init(void)
{
	int ret;
	ret = i2c_add_driver(&consumer2_driver);
	return ret;
}

static void consumer2_exit(void)
{
	i2c_del_driver(&consumer2_driver);
}

module_init(consumer2_init);
module_exit(consumer2_exit);
MODULE_LICENSE("GPL");
