#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/mfd/core.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include "rk808.h"

static bool rk808_is_volatile_reg(struct device *dev, unsigned int reg)
{
	/*
	 * Notes:
	 * - Technically the ROUND_30s bit makes RTC_CTRL_REG volatile, but
	 *   we don't use that feature.  It's better to cache.
	 * - It's unlikely we care that RK808_DEVCTRL_REG is volatile since
	 *   bits are cleared in case when we shutoff anyway, but better safe.
	 */

	switch (reg) {
	case RK808_SECONDS_REG ... RK808_WEEKS_REG:
	case RK808_RTC_STATUS_REG:
	case RK808_VB_MON_REG:
	case RK808_THERMAL_REG:
	case RK808_DCDC_UV_STS_REG:
	case RK808_LDO_UV_STS_REG:
	case RK808_DCDC_PG_REG:
	case RK808_LDO_PG_REG:
	case RK808_DEVCTRL_REG:
	case RK808_INT_STS_REG1:
	case RK808_INT_STS_REG2:
		return true;
	}

	return false;
}

/*
 * reg_bits : 寄存器用多少位表示
 * val_bits : 寄存器值用多少位表示
 * max_register : 最大寄存器数
 */
static const struct regmap_config rk808_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = RK808_IO_POL_REG,
	.cache_type = REGCACHE_RBTREE,
	.volatile_reg = rk808_is_volatile_reg,
};

static struct rk8xx_power_data pdata = {
	.name = "rk808",
	.rk8xx_regmap_config = &rk808_regmap_config,
};

static const struct of_device_id rk808_of_match[] = {
	{
		.compatible = "rockchip,rk808",
		.data = &pdata,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, rk808_of_match);

static int rk808_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	/* 获取平台数据 */
	const struct of_device_id *of_id = of_match_device(rk808_of_match, &client->dev);
	const struct rk8xx_power_data *pdata = of_id->data;

	struct rk808 *chip;
	int ret;
	int ldo_en;

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	/* 分配chip */
	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	/* 根据pdata里的配置来设置chip的regmap */
	chip->regmap = devm_regmap_init_i2c(client, pdata->rk8xx_regmap_config);
	if (IS_ERR(chip->regmap)) {
		dev_err(&client->dev, "regmap initialization failed\n");
		return PTR_ERR(chip->regmap);
	}

	/* 使用regmap API来进行I2C读写 */
	ret = regmap_read(chip->regmap, RK818_LDO_EN_REG, &ldo_en);
	printk("read first ldo_en = 0x%x\n", ldo_en);
	ret = regmap_write(chip->regmap, RK818_LDO_EN_REG, 0xf7);
	ret = regmap_read(chip->regmap, RK818_LDO_EN_REG, &ldo_en);
	printk("read after write 0xf7 ldo_en = 0x%x\n", ldo_en);

	chip->i2c = client;
	i2c_set_clientdata(client, chip);

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static int rk808_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id rk808_ids[] = {
	{ "rk808" },
	{ },
};
MODULE_DEVICE_TABLE(i2c, rk808_ids);

static struct i2c_driver rk808_i2c_driver = {
	.driver = {
		.name = "i2c_regmap_test",
		.of_match_table = rk808_of_match,
	},
	.probe    = rk808_probe,
	.remove   = rk808_remove,
	.id_table = rk808_ids,
};

module_i2c_driver(rk808_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("M_O_Bz@163.com");
MODULE_DESCRIPTION("I2C regmap");
