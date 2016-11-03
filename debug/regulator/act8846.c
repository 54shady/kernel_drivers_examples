#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/regulator/driver.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regmap.h>
#include <asm/system_misc.h>

#define REGISTER_NUMBERS 0xF5
#define ACT8846_BUCK1_SET_VOL_BASE 0x10
#define ACT8846_NUM_REGULATORS 12

#define ACT8846_DCDC1  0
#define ACT8846_LDO1 4
#define BUCK_VOL_MASK 0x3f
#define LDO_VOL_MASK 0x3f
#define VOL_MIN_IDX 0x00
#define VOL_MAX_IDX 0x3f
#define ACT8846_BUCK1_SET_VOL_BASE 0x10
#define ACT8846_BUCK2_SET_VOL_BASE 0x20
#define ACT8846_BUCK3_SET_VOL_BASE 0x30
#define ACT8846_BUCK4_SET_VOL_BASE 0x40

#define ACT8846_BUCK2_SLP_VOL_BASE 0x21
#define ACT8846_BUCK3_SLP_VOL_BASE 0x31
#define ACT8846_BUCK4_SLP_VOL_BASE 0x41

#define ACT8846_LDO1_SET_VOL_BASE 0x50
#define ACT8846_LDO2_SET_VOL_BASE 0x58
#define ACT8846_LDO3_SET_VOL_BASE 0x60
#define ACT8846_LDO4_SET_VOL_BASE 0x68
#define ACT8846_LDO5_SET_VOL_BASE 0x70
#define ACT8846_LDO6_SET_VOL_BASE 0x80
#define ACT8846_LDO7_SET_VOL_BASE 0x90
#define ACT8846_LDO8_SET_VOL_BASE 0xa0

#define ACT8846_BUCK1_CONTR_BASE 0x12
#define ACT8846_BUCK2_CONTR_BASE 0x22
#define ACT8846_BUCK3_CONTR_BASE 0x32
#define ACT8846_BUCK4_CONTR_BASE 0x42

#define ACT8846_LDO1_CONTR_BASE 0x51
#define ACT8846_LDO2_CONTR_BASE 0x59
#define ACT8846_LDO3_CONTR_BASE 0x61
#define ACT8846_LDO4_CONTR_BASE 0x69
#define ACT8846_LDO5_CONTR_BASE 0x71
#define ACT8846_LDO6_CONTR_BASE 0x81
#define ACT8846_LDO7_CONTR_BASE 0x91
#define ACT8846_LDO8_CONTR_BASE 0xa1
#define ACT8846_BUCK_SET_VOL_REG(x) (buck_set_vol_base_addr[x])
#define ACT8846_BUCK_CONTR_REG(x) (buck_contr_base_addr[x])
#define ACT8846_LDO_SET_VOL_REG(x) (ldo_set_vol_base_addr[x])
#define ACT8846_LDO_CONTR_REG(x) (ldo_contr_base_addr[x])

const static int ldo_set_vol_base_addr[] = {
	ACT8846_LDO1_SET_VOL_BASE,
	ACT8846_LDO2_SET_VOL_BASE,
	ACT8846_LDO3_SET_VOL_BASE,
	ACT8846_LDO4_SET_VOL_BASE,
	ACT8846_LDO5_SET_VOL_BASE,
	ACT8846_LDO6_SET_VOL_BASE,
	ACT8846_LDO7_SET_VOL_BASE,
	ACT8846_LDO8_SET_VOL_BASE,
};
const static int ldo_contr_base_addr[] = {
	ACT8846_LDO1_CONTR_BASE,
	ACT8846_LDO2_CONTR_BASE,
	ACT8846_LDO3_CONTR_BASE,
	ACT8846_LDO4_CONTR_BASE,
	ACT8846_LDO5_CONTR_BASE,
	ACT8846_LDO6_CONTR_BASE,
	ACT8846_LDO7_CONTR_BASE,
	ACT8846_LDO8_CONTR_BASE,
};

const static int buck_set_vol_base_addr[] = {
	ACT8846_BUCK1_SET_VOL_BASE,
	ACT8846_BUCK2_SET_VOL_BASE,
	ACT8846_BUCK3_SET_VOL_BASE,
	ACT8846_BUCK4_SET_VOL_BASE,
};
const static int buck_contr_base_addr[] = {
	ACT8846_BUCK1_CONTR_BASE,
	ACT8846_BUCK2_CONTR_BASE,
	ACT8846_BUCK3_CONTR_BASE,
	ACT8846_BUCK4_CONTR_BASE,
};

/* 用来表示device tree里的信息, 用pdata表示 */
struct act8846_board {
	struct regulator_init_data *rid[ACT8846_NUM_REGULATORS];
	struct device_node *np[ACT8846_NUM_REGULATORS];

	/* pins */
	int pmic_sleep_gpio;
	int pmic_hold_gpio;

	/* functions */
	bool pmic_sleep;
};

/* 自定义数据结构,用chip表示 */
struct act8846 {
	int num_regulators;
	struct regulator_dev **rdev;
	struct device *dev;
	struct i2c_client *client;
	struct regmap *regmap;

	/* pins */
	int pmic_sleep_gpio;
	int pmic_hold_gpio;
};

/* DEVICE TABLE */
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

/* regmap config */
static bool is_volatile_reg(struct device *dev, unsigned int reg)
{
	return true;
}

static const struct regmap_config act8846_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = REGISTER_NUMBERS,
	.cache_type = REGCACHE_RBTREE,
};

/*
 * 用名字和device tree里的信息匹配
 * 将每个regulator的信息填到这个数据结构
 */
static struct of_regulator_match act8846_reg_matches[] = {
	{ .name = "act_dcdc1"} ,
	{ .name = "act_dcdc2"} ,
	{ .name = "act_dcdc3"} ,
	{ .name = "act_dcdc4"} ,
	{ .name = "act_ldo1" } ,
	{ .name = "act_ldo2" } ,
	{ .name = "act_ldo3" } ,
	{ .name = "act_ldo4" } ,
	{ .name = "act_ldo5" } ,
	{ .name = "act_ldo6" } ,
	{ .name = "act_ldo7" } ,
	{ .name = "act_ldo8" } ,
};

static struct act8846_board *act8846_parse_dt(struct act8846 *chip)
{
	struct act8846_board *pdata;
	struct device_node *regulators_np;
	struct device_node *chip_np;
	int i, count;
	int gpio;

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	/* 增加引用计数 */
	chip_np = of_node_get(chip->dev->of_node);
	if (!chip_np) {
		printk("could not find chip act8846 node\n");
		return NULL;
	}

	/* 获取regulators的node */
	regulators_np = of_find_node_by_name(chip_np, "regulators");
	if (!regulators_np)
	{
		printk("Cann't get regulators_np\n");
		return NULL;
	}

	/* 获取regulator的个数,并且初始化init_data */
	count = of_regulator_match(chip->dev, regulators_np, act8846_reg_matches, ACT8846_NUM_REGULATORS);
	printk("Regulators = %d\n", count);

	/* 引用计数减一 */
	of_node_put(regulators_np);

	if ((count < 0) || (count > ACT8846_NUM_REGULATORS))
	{
		printk("reuglator count number error\n");
		return NULL;
	}

	/* 自定义的平台数据 */
	pdata = devm_kzalloc(chip->dev, sizeof(struct act8846_board), GFP_KERNEL);
	if (!pdata)
	{
		printk("No enough memory!!!\n");
		return NULL;
	}

	for (i = 0; i < count; i++)
	{
		/* 保存regulator_init_data和pnode */
		pdata->rid[i] = act8846_reg_matches[i].init_data;
		pdata->np[i] = act8846_reg_matches[i].of_node;
	}

	/* 获取相关的PIN脚 */
	gpio = of_get_named_gpio(chip_np, "gpios", 0);
	if (!gpio_is_valid(gpio))
		printk("invalid gpio: %d\n",gpio);
	pdata->pmic_sleep_gpio = gpio;
	pdata->pmic_sleep = true;

	gpio = of_get_named_gpio(chip_np, "gpios", 1);
	if (!gpio_is_valid(gpio))
		printk("invalid gpio: %d\n",gpio);
	pdata->pmic_hold_gpio = gpio;

	return pdata;
}

/*
 * buck and ldo voltage map
 * see Table 5 of the data sheet
 */
const static int buck_voltage_map[] = {
	 600, 625, 650, 675, 700, 725, 750, 775,
	 800, 825, 850, 875, 900, 925, 950, 975,
	 1000, 1025, 1050, 1075, 1100, 1125, 1150,
	 1175, 1200, 1250, 1300, 1350, 1400, 1450,
	 1500, 1550, 1600, 1650, 1700, 1750, 1800,
	 1850, 1900, 1950, 2000, 2050, 2100, 2150,
	 2200, 2250, 2300, 2350, 2400, 2500, 2600,
	 2700, 2800, 2900, 3000, 3100, 3200,
	 3300, 3400, 3500, 3600, 3700, 3800, 3900,
};

const static int ldo_voltage_map[] = {
	 600, 625, 650, 675, 700, 725, 750, 775,
	 800, 825, 850, 875, 900, 925, 950, 975,
	 1000, 1025, 1050, 1075, 1100, 1125, 1150,
	 1175, 1200, 1250, 1300, 1350, 1400, 1450,
	 1500, 1550, 1600, 1650, 1700, 1750, 1800,
	 1850, 1900, 1950, 2000, 2050, 2100, 2150,
	 2200, 2250, 2300, 2350, 2400, 2500, 2600,
	 2700, 2800, 2900, 3000, 3100, 3200,
	 3300, 3400, 3500, 3600, 3700, 3800, 3900,
};

/* dcdc ops */
static int act8846_dcdc_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV,unsigned *selector)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8846_DCDC1;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = buck_voltage_map;
	unsigned int val;
	int ret = 0;

	/* 设置的电压是否超出上下界限判断 */
	if (min_vol < vol_map[VOL_MIN_IDX] ||
	    min_vol > vol_map[VOL_MAX_IDX])
		return -EINVAL;

	for (val = VOL_MIN_IDX; val <= VOL_MAX_IDX; val++)
	{
		if (vol_map[val] >= min_vol)
			break;
    }

	if (vol_map[val] > max_vol)
		printk("WARNING:this voltage is not support!voltage set is %d mv\n",vol_map[val]);

	regmap_update_bits(chip->regmap, ACT8846_BUCK_SET_VOL_REG(buck), BUCK_VOL_MASK, val);

	if(ret < 0)
		printk("##################:set voltage error!voltage set is %d mv\n",vol_map[val]);

	return ret;
}

/* bit[0:5]表示电压值 */
static int act8846_dcdc_get_voltage(struct regulator_dev *rdev)
{
	struct act8846 *chip = rdev_get_drvdata(rdev);
	int buck = rdev_get_id(rdev) - ACT8846_DCDC1;
	unsigned int reg;
	int val;

	regmap_read(chip->regmap, ACT8846_BUCK_SET_VOL_REG(buck), &reg);
	reg &= BUCK_VOL_MASK;
	val = 1000 * buck_voltage_map[reg];
	return val;
}

/* 根据index返回一个电压值 */
static int act8846_dcdc_list_voltage(struct regulator_dev *dev, unsigned index)
{
	if (index >= ARRAY_SIZE(buck_voltage_map))
		return -EINVAL;
	return 1000 * buck_voltage_map[index];
}

/* 读bit7判断是否使能regulator */
static int act8846_dcdc_is_enabled(struct regulator_dev *rdev)
{
	struct act8846 *chip = rdev_get_drvdata(rdev);
	int buck = rdev_get_id(rdev) - ACT8846_DCDC1;
	unsigned int val;
	u16 mask = 0x80;
	regmap_read(chip->regmap, ACT8846_BUCK_CONTR_REG(buck), &val);
	if (val < 0)
		return val;
	 val = val & ~0x7f;
	if (val & mask)
		return 1;
	else
		return 0;
}

/* 设置bit7=1来使能regulator */
static int act8846_dcdc_enable(struct regulator_dev *rdev)
{
	struct act8846 *chip = rdev_get_drvdata(rdev);
	int buck = rdev_get_id(rdev) - ACT8846_DCDC1;
	u16 mask = 0x80;

	return regmap_update_bits(chip->regmap, ACT8846_BUCK_CONTR_REG(buck), mask, 0x80);
}

static int act8846_dcdc_disable(struct regulator_dev *dev)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8846_DCDC1;
	u16 mask = 0x80;
	return regmap_update_bits(chip->regmap, ACT8846_BUCK_CONTR_REG(buck), mask, 0);
}

static unsigned int act8846_dcdc_get_mode(struct regulator_dev *dev)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8846_DCDC1;
	u16 mask = 0x08;
	unsigned int val;
	regmap_read(chip->regmap, ACT8846_BUCK_CONTR_REG(buck), &val);
	if (val < 0)
		return val;

	val = val & mask;
	if (val== mask)
		return REGULATOR_MODE_NORMAL;
	else
		return REGULATOR_MODE_STANDBY;
}

static int act8846_dcdc_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8846_DCDC1;
	u16 mask = 0x80;

	switch(mode)
	{
	case REGULATOR_MODE_STANDBY:
		return regmap_update_bits(chip->regmap, ACT8846_BUCK_CONTR_REG(buck), mask, 0);
	case REGULATOR_MODE_NORMAL:
		return regmap_update_bits(chip->regmap, ACT8846_BUCK_CONTR_REG(buck), mask, 0x80);
	default:
		printk("error:pmu_act8846 only powersave and pwm mode\n");
		return -EINVAL;
	}
}

static int act8846_dcdc_set_sleep_voltage(struct regulator_dev *dev,
					    int uV)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - ACT8846_DCDC1;
	int min_vol = uV / 1000,max_vol = uV / 1000;
	const int *vol_map = buck_voltage_map;
	unsigned int val;
	int ret = 0;

	if (min_vol < vol_map[VOL_MIN_IDX] ||
	    min_vol > vol_map[VOL_MAX_IDX])
		return -EINVAL;

	for (val = VOL_MIN_IDX; val <= VOL_MAX_IDX; val++){
		if (vol_map[val] >= min_vol)
			break;
        }

	if (vol_map[val] > max_vol)
		printk("WARNING:this voltage is not support!voltage set is %d mv\n",vol_map[val]);
	regmap_update_bits(chip->regmap, (ACT8846_BUCK_SET_VOL_REG(buck) +0x01), BUCK_VOL_MASK, val);

	return ret;
}

static int act8846_dcdc_set_voltage_time_sel(struct regulator_dev *dev,   unsigned int old_selector,
				     unsigned int new_selector)
{
	int old_volt, new_volt;

	old_volt = act8846_dcdc_list_voltage(dev, old_selector);
	if (old_volt < 0)
		return old_volt;

	new_volt = act8846_dcdc_list_voltage(dev, new_selector);
	if (new_volt < 0)
		return new_volt;

	return DIV_ROUND_UP(abs(old_volt - new_volt)*2, 25000);
}


static struct regulator_ops act8846_dcdc_ops = {
	.set_voltage = act8846_dcdc_set_voltage,
	.get_voltage = act8846_dcdc_get_voltage,
	.list_voltage= act8846_dcdc_list_voltage,
	.is_enabled = act8846_dcdc_is_enabled,
	.enable = act8846_dcdc_enable,
	.disable = act8846_dcdc_disable,
	.get_mode = act8846_dcdc_get_mode,
	.set_mode = act8846_dcdc_set_mode,
	.set_suspend_voltage = act8846_dcdc_set_sleep_voltage,
	.set_voltage_time_sel = act8846_dcdc_set_voltage_time_sel,
};

/* ldo ops */
static int act8846_ldo_list_voltage(struct regulator_dev *dev, unsigned index)
{
	if (index >= ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;
	return 1000 * ldo_voltage_map[index];
}
static int act8846_ldo_is_enabled(struct regulator_dev *dev)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8846_LDO1;
	unsigned int val;
	u16 mask=0x80;
	regmap_read(chip->regmap, ACT8846_LDO_CONTR_REG(ldo), &val);
	if (val < 0)
		return val;
	val=val&~0x7f;
	if (val & mask)
		return 1;
	else
		return 0;
}
static int act8846_ldo_enable(struct regulator_dev *dev)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - ACT8846_LDO1;
	u16 mask=0x80;

	return regmap_update_bits(chip->regmap, ACT8846_LDO_CONTR_REG(ldo), mask, 0x80);

}
static int act8846_ldo_disable(struct regulator_dev *dev)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - ACT8846_LDO1;
	u16 mask=0x80;

	return regmap_update_bits(chip->regmap, ACT8846_LDO_CONTR_REG(ldo), mask, 0);
}
static int act8846_ldo_get_voltage(struct regulator_dev *dev)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - ACT8846_LDO1;
	unsigned int reg;
	int val;
	regmap_read(chip->regmap, ACT8846_LDO_SET_VOL_REG(ldo), &val);
	reg &= LDO_VOL_MASK;
	val = 1000 * ldo_voltage_map[reg];
	return val;
}
static int act8846_ldo_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV,unsigned *selector)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - ACT8846_LDO1;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map =ldo_voltage_map;
	unsigned int val;
	int ret = 0;

	if (min_vol < vol_map[VOL_MIN_IDX] ||
	    min_vol > vol_map[VOL_MAX_IDX])
		return -EINVAL;

	for (val = VOL_MIN_IDX; val <= VOL_MAX_IDX; val++){
		if (vol_map[val] >= min_vol)
			break;
        }

	if (vol_map[val] > max_vol)
		return -EINVAL;

	regmap_update_bits(chip->regmap, ACT8846_LDO_SET_VOL_REG(ldo), LDO_VOL_MASK, val);
	return ret;

}
static unsigned int act8846_ldo_get_mode(struct regulator_dev *dev)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8846_LDO1;
	u16 mask = 0x80;
	unsigned int val;
	regmap_read(chip->regmap, ACT8846_LDO_CONTR_REG(ldo), &val);
        if (val < 0) {
                return val;
        }
	val=val & mask;
	if (val== mask)
		return REGULATOR_MODE_NORMAL;
	else
		return REGULATOR_MODE_STANDBY;

}
static int act8846_ldo_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct act8846 *chip = rdev_get_drvdata(dev);
	int ldo = rdev_get_id(dev) - ACT8846_LDO1;
	u16 mask = 0x80;
	switch(mode)
	{
	case REGULATOR_MODE_NORMAL:
		return regmap_update_bits(chip->regmap, ACT8846_LDO_CONTR_REG(ldo), mask, mask);
	case REGULATOR_MODE_STANDBY:
		return regmap_update_bits(chip->regmap, ACT8846_LDO_CONTR_REG(ldo), mask, 0);
	default:
		printk("error:pmu_act8846 only lowpower and nomal mode\n");
		return -EINVAL;
	}


}

static struct regulator_ops act8846_ldo_ops = {
	.set_voltage = act8846_ldo_set_voltage,
	.get_voltage = act8846_ldo_get_voltage,
	.list_voltage = act8846_ldo_list_voltage,
	.is_enabled = act8846_ldo_is_enabled,
	.enable = act8846_ldo_enable,
	.disable = act8846_ldo_disable,
	.get_mode = act8846_ldo_get_mode,
	.set_mode = act8846_ldo_set_mode,
};

static struct regulator_desc regulators[] =
{
	{
		.name = "ACT_DCDC1",
		.id = 0,
		.ops = &act8846_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_DCDC2",
		.id = 1,
		.ops = &act8846_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_DCDC3",
		.id = 2,
		.ops = &act8846_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_DCDC4",
		.id = 3,
		.ops = &act8846_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},

	{
		.name = "ACT_LDO1",
		.id =4,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_LDO2",
		.id = 5,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_LDO3",
		.id = 6,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_LDO4",
		.id = 7,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},

	{
		.name = "ACT_LDO5",
		.id =8,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_LDO6",
		.id = 9,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_LDO7",
		.id = 10,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_LDO8",
		.id = 11,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "ACT_LDO9",
		.id = 12,
		.ops = &act8846_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
};

/* FIXME should not extern */
extern struct regulator_dev *devm_regulator_register(struct device *dev,
				  const struct regulator_desc *regulator_desc,
				  const struct regulator_config *config);

static int act8846_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i;
	int ret = 0;
	struct act8846 *chip;
	const struct of_device_id *match;
	unsigned int rval;
	struct act8846_board *pdata ;
	struct regulator_init_data *rid;
	struct regulator_config rcfg;
	struct regulator_dev *act_rdev;
	const char *rail_name = NULL;

	/* 检查DT */
	if (client->dev.of_node) {
		match = of_match_device(act8846_of_match, &client->dev);
		if (!match) {
			printk("Failed to find matching dt id\n");
			return -EINVAL;
		}
	}

	/* 为自定义结构提分配数据空间 */
	chip = devm_kzalloc(&client->dev,sizeof(struct act8846), GFP_KERNEL);
	if (chip == NULL) {
		ret = -ENOMEM;
		printk("No enough memory ERROR!!\n");
	}

	/* 设置chip */
	chip->client = client;
	chip->dev = &client->dev;

	/* 设置client 的driver data 指向chip */
	i2c_set_clientdata(client, chip);

	/* 映射寄存器地址 */
	chip->regmap = devm_regmap_init_i2c(client, &act8846_regmap_config);
	if (IS_ERR(chip->regmap)) {
		ret = PTR_ERR(chip->regmap);
		printk("regmap initialization failed: %d\n", ret);
		return ret;
	}

	/* I2C 通讯 */
	regmap_read(chip->regmap, 0x22, &rval);
	if ((rval < 0) || (rval == 0xff)){
		printk("The device is not act8846 %x \n",ret);
		ret = -1;
	}
	printk("act8846 reg[0x22] = %d\n", rval);

	/* enable pwm function of gpio3  */
	ret = regmap_write(chip->regmap, 0xf4, 1);
	if (ret < 0) {
		printk("act8846 set 0xf4 error!\n");
		return -1;
	}

	/* 解析DT */
	if (chip->dev->of_node)
		pdata = act8846_parse_dt(chip);

	/* 拉高HOLD管脚 */
	chip->pmic_hold_gpio = pdata->pmic_hold_gpio;
	if (chip->pmic_hold_gpio) {
		ret = devm_gpio_request(chip->dev, chip->pmic_hold_gpio, "act8846_pmic_hold");
		if (ret < 0) {
			dev_err(chip->dev,"Failed to request gpio %d with ret:""%d\n",	chip->pmic_hold_gpio, ret);
			return -1;
		}
		gpio_direction_output(chip->pmic_hold_gpio, 1);
		ret = gpio_get_value(chip->pmic_hold_gpio);
		printk("act8846_pmic_hold : %s\n", ret == 0 ? "Low" : "High");
	}

	/* set sleep and dcdc mode */
	chip->pmic_sleep_gpio = pdata->pmic_sleep_gpio;
	if (chip->pmic_sleep_gpio) {
		ret = devm_gpio_request(chip->dev, chip->pmic_sleep_gpio, "act8846_pmic_sleep");
		if (ret < 0) {
			dev_err(chip->dev,"Failed to request gpio %d with ret:""%d\n",	chip->pmic_sleep_gpio, ret);
			return -1;
		}
		gpio_direction_output(chip->pmic_sleep_gpio, 1);
		ret = gpio_get_value(chip->pmic_sleep_gpio);
		printk("act8846_pmic_sleep=%x\n", ret);
	}

	/* If we got pdata, let's do some important */
	if (pdata) {
		chip->num_regulators = ACT8846_NUM_REGULATORS;
		chip->rdev = kcalloc(ACT8846_NUM_REGULATORS, sizeof(struct regulator_dev *), GFP_KERNEL);
		if (!chip->rdev) {
			printk("No memeory!!!\n");
			return -ENOMEM;
		}

		/* Instantiate the regulators */
		for (i = 0; i < ACT8846_NUM_REGULATORS; i++)
		{
			rid = pdata->rid[i];
			if (!rid)
				continue;

			rcfg.dev = chip->dev;
			rcfg.driver_data = chip;
			rcfg.regmap = chip->regmap;
			if (chip->dev->of_node)
				rcfg.of_node = pdata->np[i];

			if (rid && rid->constraints.name)
				rail_name = rid->constraints.name;
			else
				rail_name = regulators[i].name;

			rid->supply_regulator = rail_name;

			rcfg.init_data = rid;
			act_rdev = devm_regulator_register(chip->dev, &regulators[i], &rcfg);
			if (IS_ERR(act_rdev)) {
				printk("failed to register %d regulator\n",i);
				ret = -1;
			}
			chip->rdev[i] = act_rdev;
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
