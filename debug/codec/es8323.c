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

#define INVALID_GPIO -1

/* chip data */
struct es8323_chip {
	/* alway include these two member */
	struct device *dev;
	struct i2c_client *client;

	int spk_ctl_gpio; /* speak control gpio */
	int hp_ctl_gpio; /* headphone control gpio*/
	int hp_det_gpio; /* headphone detect gpio */

	bool spk_gpio_level;
	bool hp_gpio_level;
	bool hp_det_level;
};

/* global chip point */
struct es8323_chip *g_chip;

static struct snd_soc_codec_driver soc_codec_dev_es8323 = {
};

static struct snd_soc_dai_driver es8323_dai = {
	.name = "ES8323 HiFi",
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

	printk("%s, %d probe done\n", __FUNCTION__, __LINE__);
	return ret;
}

static int es8323_i2c_remove(struct i2c_client *client)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	//snd_soc_unregister_codec(&client->dev);
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
