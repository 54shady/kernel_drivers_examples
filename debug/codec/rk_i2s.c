#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

static const struct of_device_id rockchip_i2s_match[] = {
	{ .compatible = "rockchip-i2s", },
	{},
};
MODULE_DEVICE_TABLE(of, rockchip_i2s_match);

static int rockchip_i2s_probe(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rockchip_i2s_remove(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct platform_driver rockchip_i2s_driver = {
	.probe  = rockchip_i2s_probe,
	.remove = rockchip_i2s_remove,
	.driver = {
		.name   = "rockchip-i2s",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(rockchip_i2s_match),
	},
};

static int rockchip_i2s_init(void)
{
	return platform_driver_register(&rockchip_i2s_driver);
}
module_init(rockchip_i2s_init);

static void rockchip_i2s_exit(void)
{
	platform_driver_unregister(&rockchip_i2s_driver);
}
module_exit(rockchip_i2s_exit);

MODULE_AUTHOR("zeroway");
MODULE_DESCRIPTION("Rockchip I2S Controller Driver");
MODULE_LICENSE("GPL v2");
