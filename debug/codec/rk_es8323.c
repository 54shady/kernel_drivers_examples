#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

int parse_card_info_from_dt(struct snd_soc_card *card)
{
	struct device_node *dai_node, *child_dai_node;
	int dai_num;

	dai_node = of_get_child_by_name(card->dev->of_node, "dais");
	if (!dai_node) {
		dev_err(card->dev, "%s() Can not get child: dais\n",
				__func__);
		return -EINVAL;
	}

	dai_num = 0;

	for_each_child_of_node(dai_node, child_dai_node)
	{
		card->dai_link[dai_num].dai_fmt = snd_soc_of_parse_daifmt(child_dai_node, NULL);
		if ((card->dai_link[dai_num].dai_fmt & SND_SOC_DAIFMT_MASTER_MASK) == 0)
		{
			dev_err(card->dev, "Property 'format' missing or invalid\n");
			return -EINVAL;
		}

		card->dai_link[dai_num].codec_name = NULL;
		card->dai_link[dai_num].cpu_dai_name = NULL;
		card->dai_link[dai_num].platform_name = NULL;

		/* 获取dai_link里codec */
		card->dai_link[dai_num].codec_of_node = of_parse_phandle(child_dai_node, "audio-codec", 0);
		if (!card->dai_link[dai_num].codec_of_node)
		{
			dev_err(card->dev, "Property 'audio-codec' missing or invalid\n");
			return -EINVAL;
		}

		/* 获取dai_link里i2s控制器 */
		card->dai_link[dai_num].cpu_of_node = of_parse_phandle( child_dai_node, "audio-controller", 0);
		if (!card->dai_link[dai_num].cpu_of_node)
		{
			dev_err(card->dev, "Property 'audio-controller' missing or invalid\n");
			return -EINVAL;
		}

		/* platform node = cpu node */
		card->dai_link[dai_num].platform_of_node = card->dai_link[dai_num].cpu_of_node;

		if (++dai_num >= card->num_links)
			break;
	}

	if (dai_num < card->num_links) {
		dev_err(card->dev, "%s() Can not get enough property for dais, dai: %d, max dai num: %d\n",
				__func__, dai_num, card->num_links);
		return -EINVAL;
	}

	return 0;
}

static int rk3288_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct snd_soc_ops rk3288_ops = {
	  .hw_params = rk3288_hw_params,
};

static int rk3288_es8323_init(struct snd_soc_pcm_runtime *rtd)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct snd_soc_dai_link rk3288_dai = {
	.name = "ES8323",
	.stream_name = "ES8323 PCM",
	.codec_dai_name = "ES8323 HiFi",
	.init = rk3288_es8323_init,
	.ops = &rk3288_ops,
};

static struct snd_soc_card rockchip_es8323_snd_card = {
	.name = "RK_ES8323",
	.dai_link = &rk3288_dai,
	.num_links = 1,
};

/* 向内核注册card信息 */
static int rockchip_es8323_audio_probe(struct platform_device *pdev)
{
	int ret;
	struct snd_soc_card *card = &rockchip_es8323_snd_card;

	card->dev = &pdev->dev;

	/* 从DT中获取card信息 */
	ret = parse_card_info_from_dt(card);
	if (ret) {
		printk("%s() get sound card info failed:%d\n", __FUNCTION__, ret);
		return ret;
	}

	/* 注册card */
	ret = snd_soc_register_card(card);
	if (ret)
		printk("%s() register card failed:%d\n", __FUNCTION__, ret);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rockchip_es8323_audio_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	snd_soc_unregister_card(card);
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
