#include <linux/module.h>
#include <linux/i2c.h>

void es8323_i2c_shutdown(struct i2c_client *client)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

static int es8323_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int es8323_i2c_remove(struct i2c_client *client)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static const struct i2c_device_id es8323_i2c_id[] = {
	{ "es8323", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, es8323_i2c_id);

static struct i2c_driver es8323_i2c_driver = {
	.driver = {
		.name = "my ES8323 driver test",
		.owner = THIS_MODULE,
	},
	.shutdown = es8323_i2c_shutdown,
	.probe = es8323_i2c_probe,
	.remove = es8323_i2c_remove,
	.id_table = es8323_i2c_id,
};

static int es8323_init(void)
{
	return i2c_add_driver(&es8323_i2c_driver);
}

static void es8323_exit(void)
{
	i2c_del_driver(&es8323_i2c_driver);
}

module_init(es8323_init);
module_exit(es8323_exit);

MODULE_DESCRIPTION("ASoC es8323 driver");
MODULE_AUTHOR("M_O_Bz@163.com");
MODULE_LICENSE("GPL");
