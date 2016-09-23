#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>

/* describe in device tree */
#if 0
&i2c {
	myi2c@36 {
		compatible = "myi2c";
		status = "okay";
		reg = <0x36>;
	};
}
#endif

struct myi2c_chip {
	struct i2c_client	*client;
	struct device		*dev;
};

static inline int __myi2c_read(struct i2c_client *client,
				int reg, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;
	return 0;
}

static inline int __myi2c_write(struct i2c_client *client,
				 int reg, uint8_t val)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}
	return 0;
}

int myi2c_write(struct device *dev, int reg, uint8_t val)
{
	return __myi2c_write(to_i2c_client(dev), reg, val);
}

int myi2c_read(struct device *dev, int reg, uint8_t *val)
{
	return __myi2c_read(to_i2c_client(dev), reg, val);
}

static uint8_t myi2c_regs_addr = 0;
static ssize_t myi2c_debug_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
    uint8_t val;
	myi2c_read(dev,myi2c_regs_addr,&val);
	return sprintf(buf,"REG[%x]=0x%x\n",myi2c_regs_addr,val);
}

static ssize_t myi2c_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 16);
	/* val = low 8 bit, addr = high 8 bit */
	val = tmp & 0x00FF;
	myi2c_regs_addr= (tmp >> 8) & 0x00FF;
	myi2c_write(dev, myi2c_regs_addr, val);
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
	struct myi2c_chip *chip;
	int ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		printk("i2c_check_functionality error\n");
		return -ENODEV;
	}

	chip = kzalloc(sizeof(struct myi2c_chip), GFP_KERNEL);
	if (chip == NULL) {
		printk("kzalloc error\n");
		return -ENOMEM;
	}

	chip->client = client;
	chip->dev = &client->dev;
	i2c_set_clientdata(client, chip);

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
	struct myi2c_chip *chip = i2c_get_clientdata(client);
	sysfs_remove_group(&client->dev.kobj, &myi2c_attr_group);
	kfree(chip);

	return 0;
}

static const struct of_device_id of_myi2c_match[] = {
	{ .compatible = "myi2c" },
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
