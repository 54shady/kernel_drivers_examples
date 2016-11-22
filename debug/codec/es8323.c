#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include "es8323.h"

#define INVALID_GPIO -1
#define ES8323_RATES SNDRV_PCM_RATE_8000_96000
#define ES8323_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
		SNDRV_PCM_FMTBIT_S24_LE)

/* chip data */
struct es8323_chip {
	/* alway include these two member */
	struct device *dev;
	struct i2c_client *client;

	/* sys clock */
	unsigned int sysclk;
	struct snd_pcm_hw_constraint_list *sysclk_constraints;

	int spk_ctl_gpio; /* speak control gpio */
	int hp_ctl_gpio; /* headphone control gpio*/
	int hp_det_gpio; /* headphone detect gpio */

	bool spk_gpio_level;
	bool hp_gpio_level;
	bool hp_det_level;
};

/* global chip point */
struct es8323_chip *g_chip;

static u16 es8323_reg[] = {
	0x06, 0x1C, 0xC3, 0xFC,  /*  0 *////0x0100 0x0180
	0xC0, 0x00, 0x00, 0x7C,  /*  4 */
	0x80, 0x00, 0x00, 0x06,  /*  8 */
	0x00, 0x06, 0x30, 0x30,  /* 12 */
	0xC0, 0xC0, 0x38, 0xB0,  /* 16 */
	0x32, 0x06, 0x00, 0x00,  /* 20 */
	0x06, 0x30, 0xC0, 0xC0,  /* 24 */
	0x08, 0x06, 0x1F, 0xF7,  /* 28 */
	0xFD, 0xFF, 0x1F, 0xF7,  /* 32 */
	0xFD, 0xFF, 0x00, 0x38,  /* 36 */
	0x38, 0x38, 0x38, 0x38,  /* 40 */
	0x38, 0x00, 0x00, 0x00,  /* 44 */
	0x00, 0x00, 0x00, 0x00,  /* 48 */
	0x00, 0x00, 0x00, 0x00,  /* 52 */
};

static int es8323_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level);

static unsigned int es8323_read_reg_cache(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg >= ARRAY_SIZE(es8323_reg))
		return -1;
	return es8323_reg[reg];
}

static int es8323_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	u8 data[2];
	int ret;

	BUG_ON(codec->volatile_register);
	data[0] = reg;
	data[1] = value & 0x00ff;

	if (reg < ARRAY_SIZE(es8323_reg))
		es8323_reg[reg] = value;
	ret = codec->hw_write(codec->control_data, data, 2);
	if (ret == 2)
		return 0;
	if (ret < 0)
		return ret;
	else
		return -EIO;
}

static int es8323_reset(struct snd_soc_codec *codec)
{
	snd_soc_write(codec, ES8323_CONTROL1, 0x80);
	return snd_soc_write(codec, ES8323_CONTROL1, 0x00);
}

/* codec 硬件操作 */
static int es8323_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	/* 设置codec读写寄存器的函数, i2c read write wrapper */
	codec->read  = es8323_read_reg_cache;
	codec->write = es8323_write;
	codec->hw_write = (hw_write_t)i2c_master_send; /* low level i2c write function */

	/* without the control data i2c xfer will cause kernel crash  */
	codec->control_data = container_of(codec->dev, struct i2c_client, dev);

	/* 复位芯片 */
	ret = es8323_reset(codec);
	if (ret < 0)
		dev_err(codec->dev, "Failed to issue reset\n");

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* 寄存器操作 */
	msleep(100);
	snd_soc_write(codec, 0x02,0xf3);
	snd_soc_write(codec, 0x2B,0x80);
	snd_soc_write(codec, 0x08,0x00);   //ES8388 salve
	snd_soc_write(codec, 0x00,0x35);   //
	snd_soc_write(codec, 0x01,0x50);   //PLAYBACK & RECORD Mode,EnRefr=1
	snd_soc_write(codec, 0x03,0x59);   //pdn_ana=0,ibiasgen_pdn=0
	snd_soc_write(codec, 0x05,0x00);   //pdn_ana=0,ibiasgen_pdn=0
	snd_soc_write(codec, 0x06,0x00);   //pdn_ana=0,ibiasgen_pdn=0
	snd_soc_write(codec, 0x07,0x7c);
	snd_soc_write(codec, 0x09,0x88);  //ADC L/R PGA =  +24dB
	snd_soc_write(codec, 0x0a,0xf0);  //ADC INPUT=LIN2/RIN2
	snd_soc_write(codec, 0x0b,0x82);  //ADC INPUT=LIN2/RIN2 //82
	snd_soc_write(codec, 0x0C,0x4c);  //I2S-24BIT
	snd_soc_write(codec, 0x0d,0x02);  //MCLK/LRCK=256
	snd_soc_write(codec, 0x10,0x00);  //ADC Left Volume=0db
	snd_soc_write(codec, 0x11,0x00);  //ADC Right Volume=0db
	snd_soc_write(codec, 0x12,0xea); // ALC stereo MAXGAIN: 35.5dB,  MINGAIN: +6dB (Record Volume increased!)
	snd_soc_write(codec, 0x13,0xc0);
	snd_soc_write(codec, 0x14,0x05);
	snd_soc_write(codec, 0x15,0x06);
	snd_soc_write(codec, 0x16,0x53);
	snd_soc_write(codec, 0x17,0x18);  //I2S-16BIT
	snd_soc_write(codec, 0x18,0x02);
	snd_soc_write(codec, 0x1A,0x18);  //DAC VOLUME=0DB
	snd_soc_write(codec, 0x1B,0x18);
	snd_soc_write(codec, 0x26,0x12);  //Left DAC TO Left IXER
	snd_soc_write(codec, 0x27,0xb8);  //Left DAC TO Left MIXER
	snd_soc_write(codec, 0x28,0x38);
	snd_soc_write(codec, 0x29,0x38);
	snd_soc_write(codec, 0x2A,0xb8);
	snd_soc_write(codec, 0x02,0x00); //aa //START DLL and state-machine,START DSM
	snd_soc_write(codec, 0x19,0x02);  //SOFT RAMP RATE=32LRCKS/STEP,Enable ZERO-CROSS CHECK,DAC MUTE
	snd_soc_write(codec, 0x04,0x0c);   //pdn_ana=0,ibiasgen_pdn=0
	msleep(100);
	snd_soc_write(codec, 0x2e,0x00);
	snd_soc_write(codec, 0x2f,0x00);
	snd_soc_write(codec, 0x30,0x08);
	snd_soc_write(codec, 0x31,0x08);
	msleep(200);
	snd_soc_write(codec, 0x30,0x0f);
	snd_soc_write(codec, 0x31,0x0f);
	msleep(200);
	snd_soc_write(codec, 0x30,0x18);
	snd_soc_write(codec, 0x31,0x18);
	msleep(100);
	snd_soc_write(codec, 0x04,0x2c);   //pdn_ana=0,ibiasgen_pdn=0
	snd_soc_write(codec, ES8323_DACCONTROL3, 0x06);

	es8323_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

static int es8323_remove(struct snd_soc_codec *codec)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

/* 设置偏压值 */
static int es8323_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level)
{
	switch (level) {
		case SND_SOC_BIAS_ON:
			printk("%s on\n", __func__);
			break;
		case SND_SOC_BIAS_PREPARE:
			printk("%s prepare\n", __func__);
			snd_soc_write(codec, ES8323_ANAVOLMANAG, 0x7C);
			snd_soc_write(codec, ES8323_CHIPLOPOW1, 0x00);
			snd_soc_write(codec, ES8323_CHIPLOPOW2, 0x00);
			snd_soc_write(codec, ES8323_CHIPPOWER, 0x00);
			snd_soc_write(codec, ES8323_ADCPOWER, 0x59);
			break;
		case SND_SOC_BIAS_STANDBY:
			printk("%s standby\n", __func__);
			snd_soc_write(codec, ES8323_ANAVOLMANAG, 0x7C);
			snd_soc_write(codec, ES8323_CHIPLOPOW1, 0x00);
			snd_soc_write(codec, ES8323_CHIPLOPOW2, 0x00);
			snd_soc_write(codec, ES8323_CHIPPOWER, 0x00);
			snd_soc_write(codec, ES8323_ADCPOWER, 0x59);
			break;
		case SND_SOC_BIAS_OFF:
			printk("%s off\n", __func__);
			snd_soc_write(codec, ES8323_ADCPOWER, 0xFF);
			snd_soc_write(codec, ES8323_DACPOWER, 0xC0);
			snd_soc_write(codec, ES8323_CHIPLOPOW1, 0xFF);
			snd_soc_write(codec, ES8323_CHIPLOPOW2, 0xFF);
			snd_soc_write(codec, ES8323_CHIPPOWER, 0xFF);
			snd_soc_write(codec, ES8323_ANAVOLMANAG, 0x7B);
			break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

/* codec driver */
static struct snd_soc_codec_driver soc_codec_dev_es8323 = {
	.probe          = es8323_probe,
	.remove         = es8323_remove,
	.set_bias_level = es8323_set_bias_level,
};

/* dai ops  */
static int es8323_pcm_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int es8323_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int es8323_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

/* 采样率 */
static unsigned int rates_12288[] = {
	8000, 12000, 16000, 24000, 24000, 32000, 48000, 96000,
};

/* 硬件约束条件 */
static struct snd_pcm_hw_constraint_list constraints_12288 = {
	.count	= ARRAY_SIZE(rates_12288),
	.list	= rates_12288,
};

/* 采样率 */
static unsigned int rates_112896[] = {
	8000, 11025, 22050, 44100,
};

/* 硬件约束条件 */
static struct snd_pcm_hw_constraint_list constraints_112896 = {
	.count	= ARRAY_SIZE(rates_112896),
	.list	= rates_112896,
};

/* 采样率 */
static unsigned int rates_12[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
	48000, 88235, 96000,
};

/* 硬件约束条件 */
static struct snd_pcm_hw_constraint_list constraints_12 = {
	.count	= ARRAY_SIZE(rates_12),
	.list	= rates_12,
};

/* Note that this should be called from init rather than from hw_params */
/* 根据相关硬件参数来设置时钟 */
static int es8323_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	/* 根据DAI来获取chip */
	struct snd_soc_codec *codec = codec_dai->codec;
	struct es8323_chip *chip = snd_soc_codec_get_drvdata(codec);

	/* 根据不同的屏率来设置时钟 */
	switch (freq)
	{
		case 11289600:
		case 18432000:
		case 22579200:
		case 36864000:
			chip->sysclk_constraints = &constraints_112896;
			chip->sysclk = freq;
			break;

		case 12288000:
		case 16934400:
		case 24576000:
		case 33868800:
			chip->sysclk_constraints = &constraints_12288;
			chip->sysclk = freq;
			break;

		case 12000000:
		case 24000000:
			chip->sysclk_constraints = &constraints_12;
			chip->sysclk = freq;
			break;
	}

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int es8323_mute(struct snd_soc_dai *dai, int mute)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct snd_soc_dai_ops es8323_ops = {
	.startup      = es8323_pcm_startup,
	.hw_params    = es8323_pcm_hw_params,
	.set_fmt      = es8323_set_dai_fmt,
	.set_sysclk   = es8323_set_dai_sysclk,
	.digital_mute = es8323_mute,
};

/* 描述声卡播放和录音硬件相关信息和参数设置 */
static struct snd_soc_dai_driver es8323_dai = {
	.name = "ES8323 HiFi",
	.playback = {
		.stream_name  = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates        = ES8323_RATES,
		.formats      = ES8323_FORMATS,
	},
	.capture = {
		.stream_name  = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates        = ES8323_RATES,
		.formats      = ES8323_FORMATS,
	},
	.ops = &es8323_ops,
};

void es8323_i2c_shutdown(struct i2c_client *client)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

/*
 * return value
 * equal zero success
 * less then zero failed
 * To see more info about the gpios
 * cat /sys/kernel/debug/gpio
 */
int gpio_setup(struct es8323_chip *chip, struct i2c_client *client)
{
	int hub_rest = -1;
	int hub_en = -1;
	int ret = 0;
	enum of_gpio_flags flags;

	chip->spk_ctl_gpio = of_get_named_gpio_flags(client->dev.of_node, "spk-con-gpio", 0, &flags);
	if (chip->spk_ctl_gpio < 0)
	{
		printk("%s() Can not read property spk codec-en-gpio\n", __FUNCTION__);
		ret = -1;
	}
	else
	{
		chip->spk_gpio_level = (flags & OF_GPIO_ACTIVE_LOW)? 0:1;
		printk("request gpio%d for speak control gpio\n", chip->spk_ctl_gpio);
		ret = devm_gpio_request(chip->dev, chip->spk_ctl_gpio, "spk_ctl_gpio");
		if (ret != 0)
		{
			printk("%s request SPK_CON error", __func__);
			return ret;
		}
		gpio_direction_output(chip->spk_ctl_gpio, !chip->spk_gpio_level);
	}

	chip->hp_ctl_gpio = of_get_named_gpio_flags(client->dev.of_node, "hp-con-gpio", 0, &flags);
	if (chip->hp_ctl_gpio < 0)
	{
		printk("%s() Can not read property hp codec-en-gpio\n", __FUNCTION__);
		chip->hp_ctl_gpio = INVALID_GPIO;
	}
	else
	{
		chip->hp_gpio_level = (flags & OF_GPIO_ACTIVE_LOW)? 0:1;
		printk("request gpio%d for headphone control gpio\n", chip->hp_ctl_gpio);
		ret = devm_gpio_request(chip->dev, chip->hp_ctl_gpio, "hp_ctl_gpio");
		if (ret != 0)
		{
			printk("%s request hp_ctl error", __func__);
			return ret;
		}
		gpio_direction_output(chip->hp_ctl_gpio, !chip->hp_gpio_level);
	}

	chip->hp_det_gpio = of_get_named_gpio_flags(client->dev.of_node, "hp-det-gpio", 0, &flags);
	if (chip->hp_det_gpio < 0)
	{
		printk("%s() Can not read property hp_det gpio\n", __FUNCTION__);
		chip->hp_det_gpio = INVALID_GPIO;
	}
	else
	{
		chip->hp_det_level = (flags & OF_GPIO_ACTIVE_LOW)? 0:1;
		printk("request gpio%d for headphone detect gpio\n", chip->hp_det_gpio);
		ret = devm_gpio_request(chip->dev, chip->hp_det_gpio, "hp_det_gpio");
		if (ret != 0)
		{
			printk("%s request HP_DET error", __func__);
			return ret;
		}
		gpio_direction_input(chip->hp_det_gpio);
	}

	hub_en = of_get_named_gpio_flags(client->dev.of_node, "hub_en", 0, &flags);
	if (hub_en > 0)
	{
		printk("request gpio%d for hub enable gpio\n", hub_en);
		ret = devm_gpio_request(chip->dev, hub_en, "hub_en");
		if (ret != 0)
		{
			printk("%s request hub en error", __func__);
			return ret;
		}
		gpio_direction_output(hub_en, 1);
		msleep(10);
	}

	hub_rest = of_get_named_gpio_flags(client->dev.of_node, "hub_rest", 0, &flags);
	if (hub_rest > 0)
	{
		printk("request gpio%d for hub reset gpio\n", hub_rest);
		ret = devm_gpio_request(chip->dev, hub_rest, "hub_reset");
		if (ret != 0)
		{
			printk("%s request hub rst error", __func__);
			return ret;
		}
		gpio_direction_output(hub_rest, 0);
		msleep(20);
		gpio_set_value(hub_rest, 1);
	}

	return 0;
}

static irqreturn_t hp_det_irq_handler(int irq, void *dev_id)
{
	/* disable irq */
	return IRQ_HANDLED;
}

static int es8323_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct es8323_chip *chip;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	/* Do i2c functionality check */
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_warn(&adapter->dev, "I2C-Adapter doesn't support I2C_FUNC_I2C\n");
		return -EIO;
	}

	/* alloc memory for chip data */
	chip = devm_kzalloc(&client->dev, sizeof(struct es8323_chip), GFP_KERNEL);
	if (chip == NULL)
	{
		printk("ERROR: No memory\n");
		return -ENOMEM;
	}

	/* 设置chip */
	chip->client = client;
	chip->dev = &client->dev;

	/* set client data */
	i2c_set_clientdata(client, chip);

	/* gpio setup */
	ret = gpio_setup(chip, client);
	if (ret < 0)
	{
		printk("parse gpios error\n");
	}

	/* irq setup */
	ret = devm_request_threaded_irq(chip->dev, gpio_to_irq(chip->hp_det_gpio), NULL, hp_det_irq_handler, IRQF_TRIGGER_LOW |IRQF_ONESHOT, "ES8323", NULL);
	if(ret == 0)
		printk("%s:register ISR (irq=%d)\n", __FUNCTION__, gpio_to_irq(chip->hp_det_gpio));

	/* register codec */
	ret = snd_soc_register_codec(&client->dev, &soc_codec_dev_es8323, &es8323_dai, 1);
	/*
	 * after register the codec
	 * the codec probe(es8323_probe) and es8323_set_dai_sysclk will be called
	 */

	printk("%s, %d probe done\n", __FUNCTION__, __LINE__);
	return ret;
}

static int es8323_i2c_remove(struct i2c_client *client)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	snd_soc_unregister_codec(&client->dev);
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
