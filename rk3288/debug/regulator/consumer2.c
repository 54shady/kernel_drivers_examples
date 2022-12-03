#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>

struct consumer1_chip {
	struct i2c_client	*client;
	struct device		*dev;
	struct regulator *supply;
};

static ssize_t consumer1_debug_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 1;
}

/* FIXME:Don't figure it out how it cann't work */
static ssize_t consumer1_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct consumer1_chip *chip;
	int ret;
	int tmp;
	struct regulator *supply;
	tmp = simple_strtoul(buf, NULL, 0);
	chip = container_of(&dev, struct consumer1_chip, dev);
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

static DEVICE_ATTR(consumer1_debug, S_IRUGO | S_IWUSR, consumer1_debug_show,
		   consumer1_debug_store);

static struct attribute *consumer1_attrs[] = {
	&dev_attr_consumer1_debug.attr,
	NULL,
};

static const struct attribute_group consumer1_attr_group = {
	.attrs = consumer1_attrs,
};

static const struct i2c_device_id consumer1_id_table[] = {
	{ "consumer1", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, consumer1_id_table);

static int consumer1_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	static struct regulator *supply;
	struct consumer1_chip *chip;
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

	chip = kzalloc(sizeof(struct consumer1_chip), GFP_KERNEL);
	if (chip == NULL) {
		printk("kzalloc error\n");
		return -ENOMEM;
	}

	chip->supply = supply;
	chip->client = client;
	chip->dev = &client->dev;
	i2c_set_clientdata(client, chip);

	ret = sysfs_create_group(&client->dev.kobj, &consumer1_attr_group);
	if (ret) {
		printk("failed to create sysfs device attributes\n");
		return -1;
	}

	/* Enable the regulator */
	ret = regulator_enable(chip->supply);
	printk("Enable regulator :) ret = %d\n", ret);

	return 0;
}

static int consumer1_remove(struct i2c_client *client)
{
	int ret;
	struct consumer1_chip *chip = i2c_get_clientdata(client);

	/* Disable the regulator */
	regulator_disable(chip->supply);
	printk("Disable regulator :( ret = %d\n", ret);
	sysfs_remove_group(&client->dev.kobj, &consumer1_attr_group);
	kfree(chip);

	return 0;
}

static const struct of_device_id of_consumer1_match[] = {
	{ .compatible = "Consumer2" },
	{ /* Sentinel */ }
};

static struct i2c_driver consumer1_driver = {
	.driver	= {
		.name	= "consumer1",
		.owner	= THIS_MODULE,
		.of_match_table = of_consumer1_match,
	},
	.probe		= consumer1_probe,
	.remove		= consumer1_remove,
	.id_table	= consumer1_id_table,
};

static int consumer1_init(void)
{
	int ret;
	ret = i2c_add_driver(&consumer1_driver);
	return ret;
}

static void consumer1_exit(void)
{
	i2c_del_driver(&consumer1_driver);
}

module_init(consumer1_init);
module_exit(consumer1_exit);
MODULE_LICENSE("GPL");
