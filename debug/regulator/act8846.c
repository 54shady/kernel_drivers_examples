#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#define ACT8846_BUCK1_SET_VOL_BASE 0x10
#define ACT8846_LDO8_CONTR_BASE 0xA1
#define ACT8846_NUM_REGULATORS 12

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

	if ((reg >= ACT8846_BUCK1_SET_VOL_BASE) && (reg <= ACT8846_LDO8_CONTR_BASE)) {
		return true;
	}
	return true;
}

static const struct regmap_config act8846_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = is_volatile_reg,
	.max_register = ACT8846_NUM_REGULATORS - 1,
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

/* piece of shit */
static int act8846_i2c_read(struct i2c_client *i2c, char reg, int count,	u16 *dest)
{
      int ret;
    struct i2c_adapter *adap;
    struct i2c_msg msgs[2];

    if(!i2c)
		return ret;

	if (count != 1)
		return -EIO;

    adap = i2c->adapter;

    msgs[0].addr = i2c->addr;
    msgs[0].buf = &reg;
    msgs[0].flags = i2c->flags;
    msgs[0].len = 1;
    msgs[0].scl_rate = 200*1000;

    msgs[1].buf = (u8 *)dest;
    msgs[1].addr = i2c->addr;
    msgs[1].flags = i2c->flags | I2C_M_RD;
    msgs[1].len = 1;
    msgs[1].scl_rate = 200*1000;
    ret = i2c_transfer(adap, msgs, 2);

	//DBG("***run in %s %d msgs[1].buf = %d\n",__FUNCTION__,__LINE__,*(msgs[1].buf));

	return ret;
}

static int act8846_i2c_write(struct i2c_client *i2c, char reg, int count, const u16 src)
{
	int ret=-1;

	struct i2c_adapter *adap;
	struct i2c_msg msg;
	char tx_buf[2];

	if(!i2c)
		return ret;
	if (count != 1)
		return -EIO;

	adap = i2c->adapter;
	tx_buf[0] = reg;
	tx_buf[1] = src;

	msg.addr = i2c->addr;
	msg.buf = &tx_buf[0];
	msg.len = 1 +1;
	msg.flags = i2c->flags;
	msg.scl_rate = 200*1000;

	ret = i2c_transfer(adap, &msg, 1);
	return ret;
}
static int act8846_set_bits(struct act8846 *act8846, u8 reg, u16 mask, u16 val)
{
	u16 tmp;
	int ret;

	//mutex_lock(&act8846->io_lock);

	ret = act8846_i2c_read(act8846->client, reg, 1, &tmp);
	if(ret < 0){
		//mutex_unlock(&act8846->io_lock);
		return ret;
	}
	//DBG("1 reg read 0x%02x -> 0x%02x\n", (int)reg, (unsigned)tmp&0xff);
	tmp = (tmp & ~mask) | val;
	ret = act8846_i2c_write(act8846->client, reg, 1, tmp);
	if(ret < 0){
	//	mutex_unlock(&act8846->io_lock);
		return ret;
	}
	//DBG("reg write 0x%02x -> 0x%02x\n", (int)reg, (unsigned)val&0xff);

	ret = act8846_i2c_read(act8846->client, reg, 1, &tmp);
	if(ret < 0){
	//	mutex_unlock(&act8846->io_lock);
		return ret;
	}
	//DBG("2 reg read 0x%02x -> 0x%02x\n", (int)reg, (unsigned)tmp&0xff);
	//mutex_unlock(&act8846->io_lock);

	return 0;//ret;
}

/* buck and ldo voltage map */
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
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_dcdc_get_voltage(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

/* 根据index返回一个电压值 */
static int act8846_dcdc_list_voltage(struct regulator_dev *dev, unsigned index)
{
	if (index >= ARRAY_SIZE(buck_voltage_map))
		return -EINVAL;
	return 1000 * buck_voltage_map[index];
}

static int act8846_dcdc_is_enabled(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_dcdc_enable(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_dcdc_disable(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static unsigned int act8846_dcdc_get_mode(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_dcdc_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_dcdc_set_sleep_voltage(struct regulator_dev *dev,
					    int uV)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_dcdc_set_voltage_time_sel(struct regulator_dev *dev,   unsigned int old_selector,
				     unsigned int new_selector)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
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
static int act8846_ldo_set_voltage(struct regulator_dev *dev,
				  int min_uV, int max_uV,unsigned *selector)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_ldo_get_voltage(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

/* 根据index返回一个电压值 */
static int act8846_ldo_list_voltage(struct regulator_dev *dev, unsigned index)
{
	if (index >= ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;
	return 1000 * ldo_voltage_map[index];
}

static int act8846_ldo_is_enabled(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_ldo_enable(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_ldo_disable(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static unsigned int act8846_ldo_get_mode(struct regulator_dev *dev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int act8846_ldo_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
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
#if 0//Error in caching of register
	ret = regmap_write(chip->regmap, 0xf4, 1);
	if (ret < 0) {
		printk("act8846 set 0xf4 error!\n");
		return -1;
	}
#endif
	ret = act8846_set_bits(chip, 0xf4,(0x1<<7),(0x0<<7));
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
		//gpio_free(act8846->pmic_sleep_gpio);
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
			act_rdev = regulator_register(&regulators[i], &rcfg);
			if (IS_ERR(act_rdev)) {
				printk("failed to register %d regulator\n",i);
				ret = -1;
			}
			chip->rdev[i] = act_rdev;
		}
	}

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return ret;
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
