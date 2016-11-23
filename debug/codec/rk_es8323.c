#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

static int rockchip_es8323_audio_probe(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rockchip_es8323_audio_remove(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static const struct of_device_id rockchip_es8323_of_match[] = {
	{ .compatible = "rockchip-es8323", },
	{},
};
MODULE_DEVICE_TABLE(of, rockchip_es8323_of_match);

static struct platform_driver rockchip_es8323_audio_driver = {
	.driver         = {
		.name   = "ROCKCHIP ES8323",
		.owner  = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(rockchip_es8323_of_match),
	},
	.probe          = rockchip_es8323_audio_probe,
	.remove         = rockchip_es8323_audio_remove,
};
module_platform_driver(rockchip_es8323_audio_driver);

MODULE_AUTHOR("zeroway");
MODULE_DESCRIPTION("ROCKCHIP i2s ASoC Interface");
MODULE_LICENSE("GPL");
