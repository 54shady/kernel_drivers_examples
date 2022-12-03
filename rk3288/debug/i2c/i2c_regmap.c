#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regmap.h>

/* describe in device tree */
#if 0
&i2c0 {
	myi2c@5a {
		compatible = "myi2c";
		status = "okay";
		reg = <0x5a>;
	};
}
#endif

struct myi2c_chip {
	char name[100];
	struct i2c_client	*client;
	struct device		*dev;
	struct regmap *regmap;
};
struct myi2c_chip chip;

/* regmap config */
#define REGISTER_NUMBERS 0xF5
static const struct regmap_config regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = REGISTER_NUMBERS,
	.cache_type = REGCACHE_RBTREE,
};

static ssize_t myi2c_debug_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
    unsigned int rval;
	//struct myi2c_chip *chip;
	//chip = container_of(dev, struct myi2c_chip, dev);
	printk("chip name = %s\n", chip.name);

	printk("regmap = %p\n", &chip.regmap);
	if (regmap_read(chip.regmap, 0x22, &rval) == 0)
	{
		printk("reg map read ERROR\n");
		return -1;
	}
	else
	{
		printk("reg map read ok\n");
		if ((rval < 0) || (rval == 0xff))
			printk("The device is not act8846\n");
	}

//	printk("myi2c reg[0x22] = %d\n", rval);
	return sprintf(buf,"REG[0x22]=%d\n", rval);
}

static ssize_t myi2c_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	tmp = simple_strtoul(buf, NULL, 16);
	return count;
}

static DEVICE_ATTR(myi2c_debug, S_IRUGO | S_IWUSR, myi2c_debug_show, myi2c_debug_store);

static struct attribute *myi2c_attrs[] = {
	&dev_attr_myi2c_debug.attr,
	NULL,
};

static const struct attribute_group myi2c_attr_group = {
	.attrs = myi2c_attrs,
};

/* I2C device id */
static const struct i2c_device_id myi2c_id_table[] = {
	{ "myi2c", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, myi2c_id_table);

static const struct of_device_id of_myi2c_match[] = {
	{ .compatible = "myi2c" },
	{ /* Sentinel */ }
};

static int myi2c_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	//struct myi2c_chip *chip;
	int ret;
	const struct of_device_id *match;
	unsigned int rval;

	/* 检查DT */
	if (client->dev.of_node) {
		match = of_match_device(of_myi2c_match, &client->dev);
		if (!match) {
			printk("Failed to find matching dt id\n");
			return -EINVAL;
		}
	}

#if 0
	/* 为自定义结构提分配数据空间 */
	chip = kzalloc(sizeof(struct myi2c_chip), GFP_KERNEL);
	//chip = devm_kzalloc(&client->dev, sizeof(struct myi2c_chip), GFP_KERNEL);
	if (chip == NULL) {
		ret = -ENOMEM;
		printk("No enough memory ERROR!!\n");
	}
#endif
	/* 设置chip */
	strcpy(chip.name, "myi2ctest");
	chip.client = client;
	chip.dev = &client->dev;

	/* 设置client 的driver data 指向chip */
	i2c_set_clientdata(client, &chip);

	/* 映射寄存器地址 */
	chip.regmap = devm_regmap_init_i2c(client, &regmap_config);
	if (IS_ERR(chip.regmap)) {
		ret = PTR_ERR(chip.regmap);
		printk("regmap initialization failed: %d\n", ret);
		return ret;
	}

	/* I2C 通讯 */
	regmap_read(chip.regmap, 0x22, &rval);
	if ((rval < 0) || (rval == 0xff)){
		printk("The device is not act8846 %x \n",ret);
		ret = -1;
	}
	printk("myi2c reg[0x22] = %d\n", rval);
	printk("regmap = %p\n", &chip.regmap);

	/* read write 0xF4 for test */
	regmap_read(chip.regmap, 0xf4, &rval);
	if ((rval < 0) || (rval == 0xff)){
		printk("The device is not act8846 %x \n",ret);
		ret = -1;
	}
	printk("myi2c read reg[0xf4] = %d\n", rval);

	ret = regmap_write(chip.regmap, 0xf4, 1);
	if (ret < 0) {
		printk("myi2c set 0xf4 error!\n");
		return -1;
	}
	printk("write 0xf4 = 1\n");
	regmap_read(chip.regmap, 0xf4, &rval);
	if ((rval < 0) || (rval == 0xff)){
		printk("The device is not act8846 %x \n",ret);
		ret = -1;
	}
	printk("myi2c read reg[0xf4] = %d\n", rval);

	ret = sysfs_create_group(&client->dev.kobj, &myi2c_attr_group);
	if (ret) {
		printk("failed to create sysfs device attributes\n");
		return -1;
	}
	printk("Probe Done\n");

	return 0;
}

static int myi2c_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &myi2c_attr_group);

	return 0;
}

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
