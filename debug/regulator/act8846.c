#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>

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

static int act8846_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	const struct of_device_id *match;

	/* 检查DT */
	if (client->dev.of_node) {
		match = of_match_device(act8846_of_match, &client->dev);
		if (!match) {
			printk("Failed to find matching dt id\n");
			return -EINVAL;
		}
	}
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
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
