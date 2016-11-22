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
/* (tinyplay test.wav) when sound is played the functions below will be called */
static int es8323_pcm_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	/* 根据DAI来获取chip */
	struct snd_soc_codec *codec = dai->codec;
	struct es8323_chip *chip = snd_soc_codec_get_drvdata(codec);

	/* 采样率是由提供给codec的MCLK决定的 */
	printk("es8323 sysclk = %d\n", chip->sysclk);

	snd_pcm_hw_constraint_list(substream->runtime, 0,
				   SNDRV_PCM_HW_PARAM_RATE,
				   chip->sysclk_constraints);

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
	/* 8k */
	{12288000, 8000, 1536, 0xa, 0x0},
	{11289600, 8000, 1408, 0x9, 0x0},
	{18432000, 8000, 2304, 0xc, 0x0},
	{16934400, 8000, 2112, 0xb, 0x0},
	{12000000, 8000, 1500, 0xb, 0x1},

	/* 11.025k */
	{11289600, 11025, 1024, 0x7, 0x0},
	{16934400, 11025, 1536, 0xa, 0x0},
	{12000000, 11025, 1088, 0x9, 0x1},

	/* 16k */
	{12288000, 16000, 768, 0x6, 0x0},
	{18432000, 16000, 1152, 0x8, 0x0},
	{12000000, 16000, 750, 0x7, 0x1},

	/* 22.05k */
	{11289600, 22050, 512, 0x4, 0x0},
	{16934400, 22050, 768, 0x6, 0x0},
	{12000000, 22050, 544, 0x6, 0x1},

	/* 32k */
	{12288000, 32000, 384, 0x3, 0x0},
	{18432000, 32000, 576, 0x5, 0x0},
	{12000000, 32000, 375, 0x4, 0x1},

	/* 44.1k */
	{11289600, 44100, 256, 0x2, 0x0},
	{16934400, 44100, 384, 0x3, 0x0},
	{12000000, 44100, 272, 0x3, 0x1},

	/* 48k */
	{12288000, 48000, 256, 0x2, 0x0},
	{18432000, 48000, 384, 0x3, 0x0},
	{12000000, 48000, 250, 0x2, 0x1},

	/* 88.2k */
	{11289600, 88200, 128, 0x0, 0x0},
	{16934400, 88200, 192, 0x1, 0x0},
	{12000000, 88200, 136, 0x1, 0x1},

	/* 96k */
	{12288000, 96000, 128, 0x0, 0x0},
	{18432000, 96000, 192, 0x1, 0x0},
	{12000000, 96000, 125, 0x0, 0x1},
};

static inline int get_coeff(int mclk, int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
			return i;
	}

	return -EINVAL;
}

/* 设置DAI硬件配置 */
static int es8323_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct es8323_chip *chip = snd_soc_codec_get_drvdata(codec);
	u16 srate = snd_soc_read(codec, ES8323_IFACE) & 0x80;
	u16 adciface = snd_soc_read(codec, ES8323_ADC_IFACE) & 0xE3;
	u16 daciface = snd_soc_read(codec, ES8323_DAC_IFACE) & 0xC7;
	int coeff;

	coeff = get_coeff(chip->sysclk, params_rate(params));
	if (coeff < 0) {
		coeff = get_coeff(chip->sysclk / 2, params_rate(params));
		srate |= 0x40;
	}
	if (coeff < 0) {
		dev_err(codec->dev,
				"Unable to configure sample rate %dHz with %dHz MCLK\n",
				params_rate(params), chip->sysclk);
		return coeff;
	}

	/* bit size */
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			adciface |= 0x000C;
			daciface |= 0x0018;
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			adciface |= 0x0004;
			daciface |= 0x0008;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			adciface |= 0x0010;
			daciface |= 0x0020;
			break;
	}

	/* set iface & srate*/
	snd_soc_write(codec, ES8323_DAC_IFACE, daciface);
	snd_soc_write(codec, ES8323_ADC_IFACE, adciface);

	if (coeff >= 0) {
		snd_soc_write(codec, ES8323_IFACE, srate);
		snd_soc_write(codec, ES8323_ADCCONTROL5, coeff_div[coeff].sr | (coeff_div[coeff].usb) << 4);
		snd_soc_write(codec, ES8323_DACCONTROL2, coeff_div[coeff].sr | (coeff_div[coeff].usb) << 4);
	}

	return 0;
}

/* 设置CODEC为主或从模式, I2S接口模式 */
static int es8323_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u8 iface = 0;
	u8 adciface = 0;
	u8 daciface = 0;
	printk("%s, %d fmt[%02x]\n", __FUNCTION__, __LINE__, fmt);

	iface    = snd_soc_read(codec, ES8323_IFACE);
	adciface = snd_soc_read(codec, ES8323_ADC_IFACE);
	daciface = snd_soc_read(codec, ES8323_DAC_IFACE);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBM_CFM:    // MASTER MODE
			printk("es8323 in master mode\n");
			iface |= 0x80;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:    // SLAVE MODE
			printk("es8323 in slave mode\n");
			iface &= 0x7F;
			break;
		default:
			return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:
			adciface &= 0xFC;
			daciface &= 0xF9;
			printk("I2S FORMAT\n");
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			break;
		case SND_SOC_DAIFMT_DSP_A:
			break;
		case SND_SOC_DAIFMT_DSP_B:
			break;
		default:
			return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			printk("NORMAL BCLK and NORMAL FRAME\n");
			iface    &= 0xDF;
			adciface &= 0xDF;
			daciface &= 0xBF;
			break;
		case SND_SOC_DAIFMT_IB_IF:
			printk("INVERT BCLK and INVERT FRAME\n");
			iface    |= 0x20;
			adciface |= 0x20;
			daciface |= 0x40;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			printk("INVERT BCLK and NORMAL FRAME\n");
			iface    |= 0x20;
			adciface &= 0xDF;
			daciface &= 0xBF;
			break;
		case SND_SOC_DAIFMT_NB_IF:
			printk("NORMAL BCLK and INVERT FRAME\n");
			iface    &= 0xDF;
			adciface |= 0x20;
			daciface |= 0x40;
			break;
		default:
			return -EINVAL;
	}

	snd_soc_write(codec, ES8323_IFACE, iface);
	snd_soc_write(codec, ES8323_ADC_IFACE, adciface);
	snd_soc_write(codec, ES8323_DAC_IFACE, daciface);

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

/* 是否静音操作 */
static int es8323_mute(struct snd_soc_dai *dai, int mute)
{
	/* 根据DAI来获取chip */
	struct snd_soc_codec *codec = dai->codec;
	struct es8323_chip *chip = snd_soc_codec_get_drvdata(codec);

	printk("%s, %d mute = %d\n", __FUNCTION__, __LINE__, mute);
	if (mute)
	{
		printk("set spk = %d\n", !chip->spk_gpio_level);
		printk("set hp = %d\n", !chip->hp_gpio_level);
		gpio_set_value(chip->spk_ctl_gpio, !chip->spk_gpio_level);
		gpio_set_value(chip->hp_ctl_gpio, !chip->hp_gpio_level);
		msleep(100);
		snd_soc_write(codec, ES8323_DACCONTROL3, 0x06);
	}
	else
	{
		printk("set spk = %d\n", !chip->spk_gpio_level);
		printk("set hp = %d\n", !chip->hp_gpio_level);
		snd_soc_write(codec, ES8323_DACCONTROL3, 0x02);
		snd_soc_write(codec, 0x30, ES8323_DEF_VOL);
		snd_soc_write(codec, 0x31, ES8323_DEF_VOL);
		msleep(130);

		if(chip->hp_det_level != gpio_get_value(chip->hp_det_gpio))
			gpio_set_value(chip->spk_ctl_gpio, chip->spk_gpio_level);
		else
			gpio_set_value(chip->hp_ctl_gpio, !chip->hp_gpio_level);

		msleep(150);
	}

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
