#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>

/*
 * TestScript: Input the shell script below
 *
 * while true
 * do
 * insmod regulator.ko
 * sleep 5
 * rmmod regulator
 * sleep 5
 * done
 */

/* describe in device tree */
#if 0
goodix_ts@5d {
	compatible = "goodix,gt9xx";
	status = "okay";
	reg = <0x5d>;
	VCC_TP-supply = <&ldo4_reg>;
}
#endif

struct myi2c_chip {
	struct i2c_client	*client;
	struct device		*dev;
	struct regulator *supply;
};

static ssize_t myi2c_debug_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 1;
}

/* FIXME:Don't figure it out how it cann't work */
static ssize_t myi2c_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct myi2c_chip *chip;
	int ret;
	int tmp;
	struct regulator *supply;
	tmp = simple_strtoul(buf, NULL, 0);
	chip = container_of(&dev, struct myi2c_chip, dev);
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

static DEVICE_ATTR(myi2c_debug, S_IRUGO | S_IWUSR, myi2c_debug_show,
		   myi2c_debug_store);

static struct attribute *myi2c_attrs[] = {
	&dev_attr_myi2c_debug.attr,
	NULL,
};

static const struct attribute_group myi2c_attr_group = {
	.attrs = myi2c_attrs,
};

static const struct i2c_device_id myi2c_id_table[] = {
	{ "myi2c", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, myi2c_id_table);

static int myi2c_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	static struct regulator *supply;
	struct myi2c_chip *chip;
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

	chip = kzalloc(sizeof(struct myi2c_chip), GFP_KERNEL);
	if (chip == NULL) {
		printk("kzalloc error\n");
		return -ENOMEM;
	}

	chip->supply = supply;
	chip->client = client;
	chip->dev = &client->dev;
	i2c_set_clientdata(client, chip);

	ret = sysfs_create_group(&client->dev.kobj, &myi2c_attr_group);
	if (ret) {
		printk("failed to create sysfs device attributes\n");
		return -1;
	}

	/* Enable the regulator */
	ret = regulator_enable(chip->supply);
	printk("Enable regulator :) ret = %d\n", ret);

	return 0;
}

static int myi2c_remove(struct i2c_client *client)
{
	int ret;
	struct myi2c_chip *chip = i2c_get_clientdata(client);

	/* Disable the regulator */
	regulator_disable(chip->supply);
	printk("Disable regulator :( ret = %d\n", ret);
	sysfs_remove_group(&client->dev.kobj, &myi2c_attr_group);
	kfree(chip);

	return 0;
}

/* Use gt9xx device to test */
static const struct of_device_id of_myi2c_match[] = {
	{ .compatible = "goodix,gt9xx" },
	{ /* Sentinel */ }
};

static struct i2c_driver myi2c_driver = {
	.driver	= {
		.name	= "myi2c",
		.owner	= THIS_MODULE,
		.of_match_table = of_myi2c_match,
	},
	.probe		= myi2c_probe,
	.remove		= myi2c_remove,
	.id_table	= myi2c_id_table,
};

static int myi2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&myi2c_driver);
	return ret;
}

static void myi2c_exit(void)
{
	i2c_del_driver(&myi2c_driver);
}

module_init(myi2c_init);
module_exit(myi2c_exit);
MODULE_LICENSE("GPL");
