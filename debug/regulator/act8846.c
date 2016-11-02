#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regmap.h>

/* 自定义数据结构,用chip表示 */
struct act8846 {
	struct device *dev;
	struct i2c_client *client;
	struct regmap *regmap;
};

/* DEVICE TABLE */
static struct of_device_id act8846_of_match[] = {
	{ .compatible = "act,act8846"},
	{ },
};
MODULE_DEVICE_TABLE(of, act8846_of_match);

static const struct i2c_device_id act8846_i2c_id[] = {
       { "act8846", 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, act8846_i2c_id);

/* regmap config */
#define ACT8846_BUCK1_SET_VOL_BASE 0x10
#define ACT8846_LDO8_CONTR_BASE 0xA1
#define ACT8846_NUM_REGULATORS 12
static bool is_volatile_reg(struct device *dev, unsigned int reg)
{

	if ((reg >= ACT8846_BUCK1_SET_VOL_BASE) && (reg <= ACT8846_LDO8_CONTR_BASE)) {
		return true;
	}
	return true;
}

static const struct regmap_config act8846_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = is_volatile_reg,
	.max_register = ACT8846_NUM_REGULATORS - 1,
	.cache_type = REGCACHE_RBTREE,
};

static int act8846_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct act8846 *chip;
	const struct of_device_id *match;
	unsigned int rval;

	/* 检查DT */
	if (client->dev.of_node) {
		match = of_match_device(act8846_of_match, &client->dev);
		if (!match) {
			printk("Failed to find matching dt id\n");
			return -EINVAL;
		}
	}

	/* 为自定义结构提分配数据空间 */
	chip = devm_kzalloc(&client->dev,sizeof(struct act8846), GFP_KERNEL);
	if (chip == NULL) {
		ret = -ENOMEM;
		printk("No enough memory ERROR!!\n");
	}

	/* 设置chip */
	chip->client = client;
	chip->dev = &client->dev;

	/* 设置client 的driver data 指向chip */
	i2c_set_clientdata(client, chip);

	/* 映射寄存器地址 */
	chip->regmap = devm_regmap_init_i2c(client, &act8846_regmap_config);
	if (IS_ERR(chip->regmap)) {
		ret = PTR_ERR(chip->regmap);
		printk("regmap initialization failed: %d\n", ret);
		return ret;
	}

	/* I2C 通讯 */
	regmap_read(chip->regmap, 0x22, &rval);
	if ((rval < 0) || (rval == 0xff)){
		printk("The device is not act8846 %x \n",ret);
		ret = -1;
	}
	printk("act8846 reg[0x22] = %d\n", rval);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return ret;
}

static int act8846_i2c_remove(struct i2c_client *client)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct i2c_driver act8846_i2c_driver = {
	.driver = {
		.name = "act8846",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(act8846_of_match),
	},
	.probe    = act8846_i2c_probe,
	.remove   = act8846_i2c_remove,
	.id_table = act8846_i2c_id,
};

static int act8846_module_init(void)
{
	int ret;
	ret = i2c_add_driver(&act8846_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);
	return ret;
}
module_init(act8846_module_init);

static void act8846_module_exit(void)
{
	i2c_del_driver(&act8846_i2c_driver);
}
module_exit(act8846_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zeroway");
MODULE_DESCRIPTION("act8846 PMIC driver");
