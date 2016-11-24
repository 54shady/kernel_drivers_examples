#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <sound/dmaengine_pcm.h>

#include "rk_i2s.h"

#define I2S_DEFAULT_FREQ	(11289600)
#define I2S_DMA_BURST_SIZE	(16) /* size * width: 16*4 = 64 bytes */
#define ROCKCHIP_I2S_RATES SNDRV_PCM_RATE_8000_192000
#define ROCKCHIP_I2S_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FORMAT_S32_LE)

/* 描述I2S控制器的数据结构 */
struct rk_i2s_dev {
	struct device *dev;
	struct clk *clk; /* bclk */
	struct clk *mclk; /*mclk output only */
	struct clk *hclk; /*ahb clk */

	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;

	struct regmap *regmap;

	/* 发送和接收数据的标志 */
	bool tx_start;
	bool rx_start;
};

static DEFINE_SPINLOCK(lock);

static const struct of_device_id rockchip_i2s_match[] = {
	{ .compatible = "rockchip-i2s", },
	{},
};
MODULE_DEVICE_TABLE(of, rockchip_i2s_match);

int parse_from_dt(struct platform_device *pdev, struct rk_i2s_dev *i2s)
{
	struct device_node *node = pdev->dev.of_node;
	int ret;

	/* get i2s id form dts */
	ret = of_property_read_u32(node, "i2s-id", &pdev->id);
	if (ret < 0) {
		dev_err(&pdev->dev, "Property 'i2s-id' missing or invalid\n");
		ret = -EINVAL;
		goto EXIT;
	}

	/*
	 * 获取I2S控制器需要的时钟
	 * 在dts描述里需要有相关配置
	 */
	i2s->hclk = devm_clk_get(&pdev->dev, "i2s_hclk");
	if (IS_ERR(i2s->hclk)) {
		dev_err(&pdev->dev, "Can't retrieve i2s bus clock\n");
		ret = PTR_ERR(i2s->hclk);
		goto EXIT;
	}

	i2s->clk = devm_clk_get(&pdev->dev, "i2s_clk");
	if (IS_ERR(i2s->clk)) {
		dev_err(&pdev->dev, "Can't retrieve i2s clock\n");
		ret = PTR_ERR(i2s->clk);
		goto EXIT;
	}

	i2s->mclk = devm_clk_get(&pdev->dev, "i2s_mclk");
	if (IS_ERR(i2s->mclk)) {
		dev_info(&pdev->dev, "i2s%d has no mclk\n", pdev->id);
		ret = PTR_ERR(i2s->mclk);
		goto EXIT;
	}

EXIT:
	return ret;
}

void enable_clks(struct rk_i2s_dev *i2s)
{
	clk_prepare_enable(i2s->hclk);
	clk_set_rate(i2s->clk, I2S_DEFAULT_FREQ);
	clk_prepare_enable(i2s->clk);
	clk_prepare_enable(i2s->mclk);
}

/* 可写寄存器 */
static bool rockchip_i2s_wr_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
		case I2S_TXCR:
		case I2S_RXCR:
		case I2S_CKR:
		case I2S_DMACR:
		case I2S_INTCR:
		case I2S_XFER:
		case I2S_CLR:
		case I2S_TXDR:
			return true;
		default:
			return false;
	}
}

/* 可读寄存器 */
static bool rockchip_i2s_rd_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
		case I2S_TXCR:
		case I2S_RXCR:
		case I2S_CKR:
		case I2S_DMACR:
		case I2S_INTCR:
		case I2S_XFER:
		case I2S_CLR:
		case I2S_RXDR:
		case I2S_FIFOLR:
		case I2S_INTSR:
			return true;
		default:
			return false;
	}
}

/* VOLATILE 寄存器 */
static bool rockchip_i2s_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
		case I2S_INTSR:
		case I2S_CLR:
			return true;
		default:
			return false;
	}
}

/* 私有寄存器 */
static bool rockchip_i2s_precious_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
		default:
			return false;
	}
}

/*
 * 从芯片手册可以得到下面信息
 * I2S控制器的寄存器是32位的
 * 每个寄存器步进大小为4
 * 用32位的值来表示
 */
static const struct regmap_config rockchip_i2s_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = I2S_RXDR,
	.writeable_reg = rockchip_i2s_wr_reg,
	.readable_reg = rockchip_i2s_rd_reg,
	.volatile_reg = rockchip_i2s_volatile_reg,
	.precious_reg = rockchip_i2s_precious_reg,
	.cache_type = REGCACHE_FLAT,
};

/* value of /sys/kernel/debug/asoc/dais */
static const struct snd_soc_component_driver rockchip_i2s_component = {
	.name = "rockchip-i2s",
};

static int rockchip_i2s_dai_probe(struct snd_soc_dai *dai)
{
	struct rk_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);

	dai->capture_dma_data = &i2s->capture_dma_data;
	dai->playback_dma_data = &i2s->playback_dma_data;

	return 0;
}

static int rockchip_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
		struct snd_soc_dai *dai)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rockchip_i2s_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rockchip_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rockchip_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
		int div_id, int div)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rockchip_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct snd_soc_dai_ops rockchip_i2s_dai_ops = {
	.trigger = rockchip_i2s_trigger,
	.hw_params = rockchip_i2s_hw_params,
	.set_fmt = rockchip_i2s_set_fmt,
	.set_clkdiv = rockchip_i2s_set_clkdiv,
	.set_sysclk = rockchip_i2s_set_sysclk,
};

/* 这里有两个dai driver, 根据dts里的i2s-id来选择 */
struct snd_soc_dai_driver rockchip_i2s_dai[] = {
	{
		.probe = rockchip_i2s_dai_probe,
		.name = "rockchip-i2s.0",
		.id = 0,
		.playback = {
			.channels_min = 2,
			.channels_max = 8,
			.rates = ROCKCHIP_I2S_RATES,
			.formats = ROCKCHIP_I2S_FORMATS,
		},
		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = ROCKCHIP_I2S_RATES,
			.formats = ROCKCHIP_I2S_FORMATS,
		},
		.ops = &rockchip_i2s_dai_ops,
		.symmetric_rates = 1,
	},
	{
		.probe = rockchip_i2s_dai_probe,
		.name = "rockchip-i2s.1",
		.id = 1,
		.playback = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = ROCKCHIP_I2S_RATES,
			.formats = ROCKCHIP_I2S_FORMATS,
		},
		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = ROCKCHIP_I2S_RATES,
			.formats = ROCKCHIP_I2S_FORMATS,
		},
		.ops = &rockchip_i2s_dai_ops,
		.symmetric_rates = 1,
	},
};

static const struct snd_pcm_hardware rockchip_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_PAUSE |
		SNDRV_PCM_INFO_RESUME,
	.formats = SNDRV_PCM_FMTBIT_S24_LE |
		SNDRV_PCM_FMTBIT_S20_3LE |
		SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = 2*1024*1024,
	.period_bytes_min = 64,
	.period_bytes_max = 512*1024,
	.periods_min = 3,
	.periods_max = 128,
	.fifo_size = 16,
};

static const struct snd_dmaengine_pcm_config rockchip_dmaengine_pcm_config = {
	.pcm_hardware = &rockchip_pcm_hardware,
	.prepare_slave_config = snd_dmaengine_pcm_prepare_slave_config,
	.compat_filter_fn = NULL,
	.prealloc_buffer_size = PAGE_SIZE * 512,
};

static void rockchip_snd_txctrl(struct rk_i2s_dev *i2s, int on)
{
	unsigned long flags;
	unsigned int val = 0;
	int retry = 10;

	spin_lock_irqsave(&lock, flags);

	dev_dbg(i2s->dev, "%s: %d: on: %d\n", __func__, __LINE__, on);

	if (on) {
		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_TDE_MASK, I2S_DMACR_TDE_ENABLE);

		regmap_update_bits(i2s->regmap, I2S_XFER,
				   I2S_XFER_TXS_MASK | I2S_XFER_RXS_MASK,
				   I2S_XFER_TXS_START | I2S_XFER_RXS_START);

		i2s->tx_start = true;
	} else {
		i2s->tx_start = false;

		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_TDE_MASK, I2S_DMACR_TDE_DISABLE);


		if (!i2s->rx_start) {
			regmap_update_bits(i2s->regmap, I2S_XFER,
					   I2S_XFER_TXS_MASK |
					   I2S_XFER_RXS_MASK,
					   I2S_XFER_TXS_STOP |
					   I2S_XFER_RXS_STOP);

			regmap_update_bits(i2s->regmap, I2S_CLR,
					   I2S_CLR_TXC_MASK | I2S_CLR_RXC_MASK,
					   I2S_CLR_TXC | I2S_CLR_RXC);

			regmap_read(i2s->regmap, I2S_CLR, &val);

			/* Should wait for clear operation to finish */
			while (val) {
				regmap_read(i2s->regmap, I2S_CLR, &val);
				retry--;
				if (!retry) {
					dev_warn(i2s->dev, "fail to clear\n");
					break;
				}
			}
			dev_dbg(i2s->dev, "%s: %d: stop xfer\n",
				__func__, __LINE__);
		}
	}

	spin_unlock_irqrestore(&lock, flags);
}

static void rockchip_snd_rxctrl(struct rk_i2s_dev *i2s, int on)
{
	unsigned long flags;
	unsigned int val = 0;
	int retry = 10;

	spin_lock_irqsave(&lock, flags);

	dev_dbg(i2s->dev, "%s: %d: on: %d\n", __func__, __LINE__, on);

	if (on) {
		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_RDE_MASK, I2S_DMACR_RDE_ENABLE);

		regmap_update_bits(i2s->regmap, I2S_XFER,
				   I2S_XFER_TXS_MASK | I2S_XFER_RXS_MASK,
				   I2S_XFER_TXS_START | I2S_XFER_RXS_START);

		i2s->rx_start = true;
	} else {
		i2s->rx_start = false;

		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_RDE_MASK, I2S_DMACR_RDE_DISABLE);

		if (!i2s->tx_start) {
			regmap_update_bits(i2s->regmap, I2S_XFER,
					   I2S_XFER_TXS_MASK |
					   I2S_XFER_RXS_MASK,
					   I2S_XFER_TXS_STOP |
					   I2S_XFER_RXS_STOP);

			regmap_update_bits(i2s->regmap, I2S_CLR,
					   I2S_CLR_TXC_MASK | I2S_CLR_RXC_MASK,
					   I2S_CLR_TXC | I2S_CLR_RXC);

			regmap_read(i2s->regmap, I2S_CLR, &val);

			/* Should wait for clear operation to finish */
			while (val) {
				regmap_read(i2s->regmap, I2S_CLR, &val);
				retry--;
				if (!retry) {
					dev_warn(i2s->dev, "fail to clear\n");
					break;
				}
			}
			dev_dbg(i2s->dev, "%s: %d: stop xfer\n",
				__func__, __LINE__);
		}
	}

	spin_unlock_irqrestore(&lock, flags);
}
static int rockchip_i2s_probe(struct platform_device *pdev)
{
	struct rk_i2s_dev *i2s;
	struct resource *res;
	void __iomem *regs;
	int ret;

	/* 给I2S控制器数据结构分配空间 */
	i2s = devm_kzalloc(&pdev->dev, sizeof(struct rk_i2s_dev), GFP_KERNEL);
	if (!i2s) {
		dev_err(&pdev->dev, "Can't allocate rk_i2s_dev\n");
		ret = -ENOMEM;
		goto EXIT;
	}

	ret = parse_from_dt(pdev, i2s);
	if (ret < 0)
		goto EXIT;

	/* enable hclk, clk, mclk */
	enable_clks(i2s);

	/*
	 * 获取dts里设置的寄存器起始和结束地址
	 * 根据该值来映射内存
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	printk("res: start 0x%x, end 0x%x, name %s\n", res->start, res->end, res->name);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto EXIT;
	}

	/* regmap setup */
	i2s->regmap = devm_regmap_init_mmio(&pdev->dev, regs,
			&rockchip_i2s_regmap_config);
	if (IS_ERR(i2s->regmap)) {
		dev_err(&pdev->dev,
				"Failed to initialise managed register map\n");
		ret = PTR_ERR(i2s->regmap);
		goto EXIT;
	}

	/*
	 * DMA setup for playback and capture
	 * playback --> i2s tx fifo
	 * capture --> i2s rx fifo
	 */
	i2s->playback_dma_data.addr = res->start + I2S_TXDR;
	i2s->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->playback_dma_data.maxburst = I2S_DMA_BURST_SIZE;

	i2s->capture_dma_data.addr = res->start + I2S_RXDR;
	i2s->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->capture_dma_data.maxburst = I2S_DMA_BURST_SIZE;

	/* turn off xfer and recv while init */
	i2s->tx_start = false;
	i2s->rx_start = false;

	/* i2s setup */
	i2s->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, i2s);

	/*
	 * register component(dai)
	 * 根据dts里i2s-id的值来选择注册哪个dai
	 */
	ret = snd_soc_register_component(&pdev->dev, &rockchip_i2s_component,
			&rockchip_i2s_dai[pdev->id], 1);

	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto EXIT;
	}

	/* register platform(dma) */
	ret = snd_dmaengine_pcm_register(&pdev->dev, &rockchip_dmaengine_pcm_config,
			SND_DMAENGINE_PCM_FLAG_COMPAT|
			SND_DMAENGINE_PCM_FLAG_NO_RESIDUE);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		goto EXIT_PLATFORM;
	}

	/* update i2s dma register */
	rockchip_snd_txctrl(i2s, 0);
	rockchip_snd_rxctrl(i2s, 0);

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	return ret;

EXIT_PLATFORM:
	snd_soc_unregister_component(&pdev->dev);
EXIT:
	return ret;
}

static int rockchip_i2s_remove(struct platform_device *pdev)
{
	struct rk_i2s_dev *i2s = dev_get_drvdata(&pdev->dev);
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	clk_disable_unprepare(i2s->mclk);
	clk_disable_unprepare(i2s->clk);
	clk_disable_unprepare(i2s->hclk);

	snd_dmaengine_pcm_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
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
