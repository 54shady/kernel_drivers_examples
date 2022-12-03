#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/of_device.h>

#define START_REG_ADDR 0x00
#define END_REG_ADDR 0xF5
#define MAX_REGISTER 0XF5

struct myregmap_test {
	struct device *dev;
	struct i2c_client *i2c;
	struct regmap *regmap;
};

/* 使用ACT8846来测试 */
static struct of_device_id myregmap_test_of_match[] = {
	{ .compatible = "act,act8846"},
	{ },
};
MODULE_DEVICE_TABLE(of, myregmap_test_of_match);

static bool is_volatile_reg(struct device *dev, unsigned int reg)
{

	if ((reg >= START_REG_ADDR) && (reg <= END_REG_ADDR)) {
		return true;
	}
	return false;
}

static const struct regmap_config myregmap_test_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = is_volatile_reg,
	.max_register = MAX_REGISTER - 1,
	.cache_type = REGCACHE_RBTREE,
};

static int myregmap_test_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	unsigned int rval;
	struct myregmap_test *myregmap_test;
	const struct of_device_id *match;
	int ret,i=0;

	/* 检查DT */
	if (i2c->dev.of_node) {
		match = of_match_device(myregmap_test_of_match, &i2c->dev);
		if (!match) {
			printk("Failed to find matching dt id\n");
			return -EINVAL;
		}
	}

	/* 为自定义结构提分配数据空间 */
	myregmap_test = devm_kzalloc(&i2c->dev,sizeof(struct myregmap_test), GFP_KERNEL);
	if (myregmap_test == NULL) {
		ret = -ENOMEM;
		return -1;
	}
	myregmap_test->i2c = i2c;
	myregmap_test->dev = &i2c->dev;
	i2c_set_clientdata(i2c, myregmap_test);

	/* 映射寄存器地址 */
	myregmap_test->regmap = devm_regmap_init_i2c(i2c, &myregmap_test_config);
	if (IS_ERR(myregmap_test->regmap)) {
		ret = PTR_ERR(myregmap_test->regmap);
		printk("regmap initialization failed: %d\n", ret);
		return ret;
	}

	/* I2C 通讯 */
	ret = regmap_read(myregmap_test->regmap, 0x22, &rval);
	printk("ret = %d, rval = %d\n", ret, rval);

	return ret;
}

static int  myregmap_test_remove(struct i2c_client *i2c)
{
	struct myregmap_test *myregmap_test = i2c_get_clientdata(i2c);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	i2c_set_clientdata(i2c, NULL);

	return 0;
}

static const struct i2c_device_id myregmap_test_id[] = {
       { "myregmap_test", 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, myregmap_test_id);

static struct i2c_driver myregmap_test_driver = {
	.driver = {
		.name = "myregmap_test",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(myregmap_test_of_match),
	},
	.probe    = myregmap_test_probe,
	.remove   = myregmap_test_remove,
	.id_table = myregmap_test_id,
};

static int __init myregmap_test_module_init(void)
{
	int ret;
	ret = i2c_add_driver(&myregmap_test_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);
	return ret;
}
module_init(myregmap_test_module_init);

static void __exit myregmap_test_module_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	i2c_del_driver(&myregmap_test_driver);
}
module_exit(myregmap_test_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zeroway");
MODULE_DESCRIPTION("myregmap_test driver");
