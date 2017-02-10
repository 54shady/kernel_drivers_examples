#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/regulator/driver.h>
#include <linux/mfd/rk818.h>
#include <linux/mfd/core.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
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
#include <linux/syscore_ops.h>

#define BUCK_VOL_MASK 0x3f
#define LDO_VOL_MASK 0x3f
#define LDO9_VOL_MASK 0x1f
#define BOOST_VOL_MASK 0xe0

#define VOL_MIN_IDX 0x00
#define VOL_MAX_IDX 0x3f
#define RK818_I2C_ADDR_RATE  200*1000

const static int buck_set_vol_base_addr[] = {
	RK818_BUCK1_ON_REG,
	RK818_BUCK2_ON_REG,
	RK818_BUCK3_CONFIG_REG,
	RK818_BUCK4_ON_REG,
};
const static int buck_set_slp_vol_base_addr[] = {
	RK818_BUCK1_SLP_REG,
	RK818_BUCK2_SLP_REG,
	RK818_BUCK3_CONFIG_REG,
	RK818_BUCK4_SLP_VSEL_REG,
};
const static int buck_contr_base_addr[] = {
	RK818_BUCK1_CONFIG_REG,
	RK818_BUCK2_CONFIG_REG,
	RK818_BUCK3_CONFIG_REG,
	RK818_BUCK4_CONFIG_REG,
};
#define rk818_BUCK_SET_VOL_REG(x) (buck_set_vol_base_addr[x])
#define rk818_BUCK_CONTR_REG(x) (buck_contr_base_addr[x])
#define rk818_BUCK_SET_SLP_VOL_REG(x) (buck_set_slp_vol_base_addr[x])


const static int ldo_set_vol_base_addr[] = {
	RK818_LDO1_ON_VSEL_REG,
	RK818_LDO2_ON_VSEL_REG,
	RK818_LDO3_ON_VSEL_REG,
	RK818_LDO4_ON_VSEL_REG,
	RK818_LDO5_ON_VSEL_REG,
	RK818_LDO6_ON_VSEL_REG,
	RK818_LDO7_ON_VSEL_REG,
	RK818_LDO8_ON_VSEL_REG,
	RK818_BOOST_LDO9_ON_VSEL_REG,
};
const static int ldo_set_slp_vol_base_addr[] = {
	RK818_LDO1_SLP_VSEL_REG,
	RK818_LDO2_SLP_VSEL_REG,
	RK818_LDO3_SLP_VSEL_REG,
	RK818_LDO4_SLP_VSEL_REG,
	RK818_LDO5_SLP_VSEL_REG,
	RK818_LDO6_SLP_VSEL_REG,
	RK818_LDO7_SLP_VSEL_REG,
	RK818_LDO8_SLP_VSEL_REG,
	RK818_BOOST_LDO9_SLP_VSEL_REG,
};

#define rk818_LDO_SET_VOL_REG(x) (ldo_set_vol_base_addr[x])
#define rk818_LDO_SET_SLP_VOL_REG(x) (ldo_set_slp_vol_base_addr[x])

const static int buck_voltage_map[] = {
	712,  725,  737,  750, 762,  775,  787,  800,
	812,  825,  837,  850,862,  875,  887,  900,  912,
	925,  937,  950, 962,  975,  987, 1000, 1012, 1025,
	1037, 1050,1062, 1075, 1087, 1100, 1112, 1125, 1137,
	1150,1162, 1175, 1187, 1200, 1212, 1225, 1237, 1250,
	1262, 1275, 1287, 1300, 1312, 1325, 1337, 1350,1362,
	1375, 1387, 1400, 1412, 1425, 1437, 1450,1462, 1475,
	1487, 1500,
};

const static int buck4_voltage_map[] = {
	1800, 1900, 2000, 2100, 2200,  2300,  2400, 2500, 2600,
	2700, 2800, 2900, 3000, 3100, 3200,3300, 3400,3500,3600,
};

const static int ldo_voltage_map[] = {
	1800, 1900, 2000, 2100, 2200,  2300,  2400, 2500, 2600,
	2700, 2800, 2900, 3000, 3100, 3200,3300, 3400,
};
const static int ldo3_voltage_map[] = {
	800, 900, 1000, 1100, 1200,  1300, 1400, 1500, 1600,
	1700, 1800, 1900,  2000,2100,  2200,  2500,
};
const static int ldo6_voltage_map[] = {
	800, 900, 1000, 1100, 1200,  1300, 1400, 1500, 1600,
	1700, 1800, 1900,  2000,2100,  2200,  2300,2400,2500,
};
const static int boost_voltage_map[] = {
	4700,4800,4900,5000,5100,5200,5300,5400,
};

/////////////////////////////////rk818-irq.c
static inline int irq_to_rk818_irq(struct rk818 *rk818,
		int irq)
{
	return (irq - rk818->chip_irq);
}

/*
 * This is a threaded IRQ handler so can access I2C/SPI.  Since all
 * interrupts are clear on read the IRQ line will be reasserted and
 * the physical IRQ will be handled again if another interrupt is
 * asserted while we run - in the normal course of events this is a
 * rare occurrence so we save I2C/SPI reads.  We're also assuming that
 * it's rare to get lots of interrupts firing simultaneously so try to
 * minimise I/O.
 */
static irqreturn_t rk818_irq(int irq, void *irq_data)
{
	struct rk818 *rk818 = irq_data;
	u32 irq_sts;
	u32 irq_mask;
	u8 reg;
	int i, cur_irq;
	//printk(" rk818 irq %d \n",irq);
	wake_lock(&rk818->irq_wake);
	rk818_i2c_read(rk818, RK818_INT_STS_REG1, 1, &reg);
	irq_sts = reg;
	rk818_i2c_read(rk818, RK818_INT_STS_REG2, 1, &reg);
	irq_sts |= reg << 8;

	rk818_i2c_read(rk818, RK818_INT_STS_MSK_REG1, 1, &reg);
	irq_mask = reg;
	rk818_i2c_read(rk818, RK818_INT_STS_MSK_REG2, 1, &reg);
	irq_mask |= reg << 8;

	irq_sts &= ~irq_mask;

	if (!irq_sts)
	{
		wake_unlock(&rk818->irq_wake);
		return IRQ_NONE;
	}

	for (i = 0; i < rk818->irq_num; i++) {

		if (!(irq_sts & (1 << i)))
			continue;

		cur_irq = irq_find_mapping(rk818->irq_domain, i);

		if (cur_irq)
			handle_nested_irq(cur_irq);
	}

	/* Write the STS register back to clear IRQs we handled */
	reg = irq_sts & 0xFF;
	irq_sts >>= 8;
	rk818_i2c_write(rk818, RK818_INT_STS_REG1, 1, reg);
	reg = irq_sts & 0xFF;
	rk818_i2c_write(rk818, RK818_INT_STS_REG2, 1, reg);
	wake_unlock(&rk818->irq_wake);
	return IRQ_HANDLED;
}

static void rk818_irq_lock(struct irq_data *data)
{
	struct rk818 *rk818 = irq_data_get_irq_chip_data(data);

	mutex_lock(&rk818->irq_lock);
}

static void rk818_irq_sync_unlock(struct irq_data *data)
{
	struct rk818 *rk818 = irq_data_get_irq_chip_data(data);
	u32 reg_mask;
	u8 reg;

	rk818_i2c_read(rk818, RK818_INT_STS_MSK_REG1, 1, &reg);
	reg_mask = reg;
	rk818_i2c_read(rk818, RK818_INT_STS_MSK_REG2, 1, &reg);
	reg_mask |= reg << 8;

	if (rk818->irq_mask != reg_mask) {
		reg = rk818->irq_mask & 0xff;
		//		rk818_i2c_write(rk818, RK818_INT_STS_MSK_REG1, 1, reg);
		reg = rk818->irq_mask >> 8 & 0xff;
		//		rk818_i2c_write(rk818, RK818_INT_STS_MSK_REG2, 1, reg);
	}
	mutex_unlock(&rk818->irq_lock);
}

static void rk818_irq_enable(struct irq_data *data)
{
	struct rk818 *rk818 = irq_data_get_irq_chip_data(data);

	rk818->irq_mask &= ~( 1 << irq_to_rk818_irq(rk818, data->irq));
}

static void rk818_irq_disable(struct irq_data *data)
{
	struct rk818 *rk818 = irq_data_get_irq_chip_data(data);

	rk818->irq_mask |= ( 1 << irq_to_rk818_irq(rk818, data->irq));
}

#ifdef CONFIG_PM_SLEEP
static int rk818_irq_set_wake(struct irq_data *data, unsigned int enable)
{
	struct rk818 *rk818 = irq_data_get_irq_chip_data(data);
	return irq_set_irq_wake(rk818->chip_irq, enable);
}
#else
#define rk818_irq_set_wake NULL
#endif

static struct irq_chip rk818_irq_chip = {
	.name = "rk818",
	.irq_bus_lock = rk818_irq_lock,
	.irq_bus_sync_unlock = rk818_irq_sync_unlock,
	.irq_disable = rk818_irq_disable,
	.irq_enable = rk818_irq_enable,
	.irq_set_wake = rk818_irq_set_wake,
};

static int rk818_irq_domain_map(struct irq_domain *d, unsigned int irq,
		irq_hw_number_t hw)
{
	struct rk818 *rk818 = d->host_data;

	irq_set_chip_data(irq, rk818);
	irq_set_chip_and_handler(irq, &rk818_irq_chip, handle_edge_irq);
	irq_set_nested_thread(irq, 1);
#ifdef CONFIG_ARM
	set_irq_flags(irq, IRQF_VALID);
#else
	irq_set_noprobe(irq);
#endif
	return 0;
}

static struct irq_domain_ops rk818_irq_domain_ops = {
	.map = rk818_irq_domain_map,
};

int rk818_irq_init(struct rk818 *rk818, int irq,struct rk818_board *pdata)
{
	struct irq_domain *domain;
	int ret,val,irq_type,flags;
	u8 reg;

	//	printk("%s,line=%d\n", __func__,__LINE__);
	if (!irq) {
		dev_warn(rk818->dev, "No interrupt support, no core IRQ\n");
		return 0;
	}

	/* Clear unattended interrupts */
	rk818_i2c_read(rk818, RK818_INT_STS_REG1, 1, &reg);
	rk818_i2c_write(rk818, RK818_INT_STS_REG1, 1, reg);
	rk818_i2c_read(rk818, RK818_INT_STS_REG2, 1, &reg);
	rk818_i2c_write(rk818, RK818_INT_STS_REG2, 1, reg);
	rk818_i2c_read(rk818, RK818_RTC_STATUS_REG, 1, &reg);
	rk818_i2c_write(rk818, RK818_RTC_STATUS_REG, 1, reg);//clear alarm and timer interrupt

	/* Mask top level interrupts */
	rk818->irq_mask = 0xFFFFFF;
	mutex_init(&rk818->irq_lock);
	wake_lock_init(&rk818->irq_wake, WAKE_LOCK_SUSPEND, "rk818_irq_wake");
	rk818->irq_num = RK818_NUM_IRQ;
	rk818->irq_gpio = pdata->irq_gpio;
	if (rk818->irq_gpio && !rk818->chip_irq) {
		rk818->chip_irq = gpio_to_irq(rk818->irq_gpio);

		if (rk818->irq_gpio) {
			ret = gpio_request(rk818->irq_gpio, "rk818_pmic_irq");
			if (ret < 0) {
				dev_err(rk818->dev,
						"Failed to request gpio %d with ret:"
						"%d\n",	rk818->irq_gpio, ret);
				return IRQ_NONE;
			}
			gpio_direction_input(rk818->irq_gpio);
			val = gpio_get_value(rk818->irq_gpio);
			if (val){
				irq_type = IRQ_TYPE_LEVEL_LOW;
				flags = IRQF_TRIGGER_FALLING;
			}
			else{
				irq_type = IRQ_TYPE_LEVEL_HIGH;
				flags = IRQF_TRIGGER_RISING;
			}
			gpio_free(rk818->irq_gpio);
			pr_info("%s: rk818_pmic_irq=%x\n", __func__, val);
		}
	}

	domain = irq_domain_add_linear(NULL, RK818_NUM_IRQ,
			&rk818_irq_domain_ops, rk818);
	if (!domain) {
		dev_err(rk818->dev, "could not create irq domain\n");
		return -ENODEV;
	}
	rk818->irq_domain = domain;

	ret = request_threaded_irq(rk818->chip_irq, NULL, rk818_irq, flags | IRQF_ONESHOT, "rk818", rk818);

	irq_set_irq_type(rk818->chip_irq, irq_type);
	enable_irq_wake(rk818->chip_irq);
	if (ret != 0)
		dev_err(rk818->dev, "Failed to request IRQ: %d\n", ret);

	return ret;
}

int rk818_irq_exit(struct rk818 *rk818)
{
	if (rk818->chip_irq)
		free_irq(rk818->chip_irq, rk818);
	return 0;
}
/////////////////////////////////rk818-irq.c

static int rk818_ldo_list_voltage(struct regulator_dev *dev, unsigned index)
{
	int ldo= rdev_get_id(dev) - RK818_LDO1;
	if (ldo == 2){
		if (index >= ARRAY_SIZE(ldo3_voltage_map))
			return -EINVAL;
		return 1000 * ldo3_voltage_map[index];
	}
	else if (ldo == 5 || ldo ==6){
		if (index >= ARRAY_SIZE(ldo6_voltage_map))
			return -EINVAL;
		return 1000 * ldo6_voltage_map[index];
	}
	else{
		if (index >= ARRAY_SIZE(ldo_voltage_map))
			return -EINVAL;
		return 1000 * ldo_voltage_map[index];
	}
}
static int rk818_ldo_is_enabled(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;
	u16 val;

	if (ldo == 8){
		val = rk818_reg_read(rk818, RK818_DCDC_EN_REG);  //ldo9
		if (val < 0)
			return val;
		if (val & (1 << 5))
			return 1;
		else
			return 0;
	}
	val = rk818_reg_read(rk818, RK818_LDO_EN_REG);
	if (val < 0)
		return val;
	if (val & (1 << ldo))
		return 1;
	else
		return 0;
}
static int rk818_ldo_enable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;

	if (ldo == 8)
		rk818_set_bits(rk818, RK818_DCDC_EN_REG, 1 << 5, 1 << 5); //ldo9
	else if (ldo ==9)
		rk818_set_bits(rk818, RK818_DCDC_EN_REG, 1 << 6, 1 << 6); //ldo10 switch
	else
		rk818_set_bits(rk818, RK818_LDO_EN_REG, 1 << ldo, 1 << ldo);

	return 0;
}
static int rk818_ldo_disable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;

	if (ldo == 8)
		rk818_set_bits(rk818, RK818_DCDC_EN_REG, 1 << 5, 1 << 0); //ldo9
	else if(ldo ==9)
		rk818_set_bits(rk818, RK818_DCDC_EN_REG, 1 << 6, 1 << 0); //ldo10 switch
	else
		rk818_set_bits(rk818, RK818_LDO_EN_REG, 1 << ldo, 0);

	return 0;
}
static int rk818_ldo_get_voltage(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;
	u16 reg = 0;
	int val;

	if  (ldo ==9){
		reg = rk818_reg_read(rk818,rk818_BUCK_SET_VOL_REG(3));
		reg &= BUCK_VOL_MASK;
		val = 1000 * buck4_voltage_map[reg];
	}
	else{
		reg = rk818_reg_read(rk818,rk818_LDO_SET_VOL_REG(ldo));
		if (ldo == 8){
			reg &= LDO9_VOL_MASK;
		}
		else
			reg &= LDO_VOL_MASK;

		if (ldo ==2){
			val = 1000 * ldo3_voltage_map[reg];
		}
		else if (ldo == 5 || ldo ==6){
			val = 1000 * ldo6_voltage_map[reg];
		}
		else{
			val = 1000 * ldo_voltage_map[reg];
		}
	}
	return val;
}
static int rk818_ldo_suspend_enable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;

	if (ldo == 8)
		return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG1, 1 << 5, 0); //ldo9
	else if (ldo ==9)
		return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG1, 1 << 6, 0); //ldo10 switch
	else
		return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG2, 1 << ldo, 0);

}
static int rk818_ldo_suspend_disable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;

	if (ldo == 8)
		return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG1, 1 << 5, 1 << 5); //ldo9
	else if (ldo ==9)
		return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG1, 1 << 6, 1 << 6); //ldo10 switch
	else
		return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG2, 1 << ldo, 1 << ldo);

}
static int rk818_ldo_set_sleep_voltage(struct regulator_dev *dev,
		int uV)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;
	const int *vol_map = ldo_voltage_map;
	int min_vol = uV / 1000;
	u16 val;
	int ret = 0,num =0;

	if (ldo ==2){
		vol_map = ldo3_voltage_map;
		num = 15;
	}
	else if (ldo == 5 || ldo ==6){
		vol_map = ldo6_voltage_map;
		num = 17;
	}
	else {
		num = 16;
	}

	if (min_vol < vol_map[0] ||
			min_vol > vol_map[num])
		return -EINVAL;

	for (val = 0; val <= num; val++){
		if (vol_map[val] >= min_vol)
			break;
	}

	if (ldo == 8){
		ret = rk818_set_bits(rk818, rk818_LDO_SET_SLP_VOL_REG(ldo),LDO9_VOL_MASK, val);
	}
	else
		ret = rk818_set_bits(rk818, rk818_LDO_SET_SLP_VOL_REG(ldo),LDO_VOL_MASK, val);

	return ret;
}

static int rk818_ldo_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV,unsigned *selector)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int ldo= rdev_get_id(dev) - RK818_LDO1;
	const int *vol_map;
	int min_vol = min_uV / 1000;
	u16 val;
	int ret = 0,num =0;

	if (ldo ==2){
		vol_map = ldo3_voltage_map;
		num = 15;
	}
	else if (ldo == 5 || ldo ==6){
		vol_map = ldo6_voltage_map;
		num = 17;
	}
	else {
		vol_map = ldo_voltage_map;
		num = 16;
	}

	if (min_vol < vol_map[0] ||
			min_vol > vol_map[num])
		return -EINVAL;

	for (val = 0; val <= num; val++){
		if (vol_map[val] >= min_vol)
			break;
	}

	if (ldo == 8){
		ret = rk818_set_bits(rk818, rk818_LDO_SET_VOL_REG(ldo),LDO9_VOL_MASK, val);
	}
	else
		ret = rk818_set_bits(rk818, rk818_LDO_SET_VOL_REG(ldo),LDO_VOL_MASK, val);

	return ret;

}

static struct regulator_ops rk818_ldo_ops = {
	.set_voltage = rk818_ldo_set_voltage,
	.get_voltage = rk818_ldo_get_voltage,
	.list_voltage = rk818_ldo_list_voltage,
	.is_enabled = rk818_ldo_is_enabled,
	.enable = rk818_ldo_enable,
	.disable = rk818_ldo_disable,
	.set_suspend_enable =rk818_ldo_suspend_enable,
	.set_suspend_disable =rk818_ldo_suspend_disable,
	.set_suspend_voltage = rk818_ldo_set_sleep_voltage,
};

static int rk818_dcdc_list_voltage(struct regulator_dev *dev, unsigned selector)
{
	int volt;
	int buck = rdev_get_id(dev) - RK818_DCDC1;

	if (selector < 0x0 ||selector > BUCK_VOL_MASK )
		return -EINVAL;

	switch (buck) {
		case 0:
		case 1:
			volt = 712500 + selector * 12500;
			break;
		case 3:
			volt = 1800000 + selector * 100000;
			break;
		case 2:
			volt = 1200000;
			break;
		default:
			BUG();
			return -EINVAL;
	}

	return  volt ;
}

static int rk818_dcdc_is_enabled(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;
	u16 val;

	val = rk818_reg_read(rk818, RK818_DCDC_EN_REG);
	if (val < 0)
		return val;
	if (val & (1 << buck))
		return 1;
	else
		return 0;
}

static int rk818_dcdc_enable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;

	return rk818_set_bits(rk818, RK818_DCDC_EN_REG, 1 << buck, 1 << buck);

}

static int rk818_dcdc_disable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;

	return rk818_set_bits(rk818, RK818_DCDC_EN_REG, 1 << buck , 0);
}

static int rk818_dcdc_get_voltage(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;
	u16 reg = 0;
	int val;

	reg = rk818_reg_read(rk818,rk818_BUCK_SET_VOL_REG(buck));

	reg &= BUCK_VOL_MASK;
	val = rk818_dcdc_list_voltage(dev,reg);
	return val;
}

static int rk818_dcdc_select_min_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV ,int buck)
{
	u16 vsel =0;

	if (buck == 0 || buck ==  1){
		if (min_uV < 712500)
			vsel = 0;
		else if (min_uV <= 1500000)
			vsel = ((min_uV - 712500) / 12500) ;
		else
			return -EINVAL;
	}
	else if (buck ==3){
		if (min_uV < 1800000)
			vsel = 0;
		else if (min_uV <= 3300000)
			vsel = ((min_uV - 1800000) / 100000) ;
		else
			return -EINVAL;
	}
	if (rk818_dcdc_list_voltage(dev, vsel) > max_uV)
		return -EINVAL;
	return vsel;
}

static int rk818_dcdc_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV,unsigned *selector)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;
	u16 val;
	int ret = 0;

	if (buck ==2){
		return 0;
	}else if (buck==3){
		val = rk818_dcdc_select_min_voltage(dev,min_uV,max_uV,buck);
		ret = rk818_set_bits(rk818, rk818_BUCK_SET_VOL_REG(buck), BUCK_VOL_MASK, val);
	}
	val = rk818_dcdc_select_min_voltage(dev,min_uV,max_uV,buck);
	ret = rk818_set_bits(rk818, rk818_BUCK_SET_VOL_REG(buck), BUCK_VOL_MASK, val);
	return ret;
}

static int rk818_dcdc_set_sleep_voltage(struct regulator_dev *dev,
		int uV)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;
	u16 val;
	int ret = 0;

	if (buck ==2){
		return 0;
	}else{
		val = rk818_dcdc_select_min_voltage(dev,uV,uV,buck);
		ret = rk818_set_bits(rk818, rk818_BUCK_SET_SLP_VOL_REG(buck) , BUCK_VOL_MASK, val);
	}
	return ret;
}

static unsigned int rk818_dcdc_get_mode(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;
	u16 mask = 0x80;
	u16 val;
	val = rk818_reg_read(rk818, rk818_BUCK_SET_VOL_REG(buck));
	if (val < 0) {
		return val;
	}
	val=val & mask;
	if (val== mask)
		return REGULATOR_MODE_FAST;
	else
		return REGULATOR_MODE_NORMAL;

}

static int rk818_dcdc_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;
	u16 mask = 0x80;
	switch(mode)
	{
		case REGULATOR_MODE_FAST:
			return rk818_set_bits(rk818, rk818_BUCK_SET_VOL_REG(buck), mask, mask);
		case REGULATOR_MODE_NORMAL:
			return rk818_set_bits(rk818, rk818_BUCK_SET_VOL_REG(buck), mask, 0);
		default:
			printk("error:pmu_rk818 only powersave and pwm mode\n");
			return -EINVAL;
	}
}

static int rk818_dcdc_set_voltage_time_sel(struct regulator_dev *dev,   unsigned int old_selector,
		unsigned int new_selector)
{
	int old_volt, new_volt;

	old_volt = rk818_dcdc_list_voltage(dev, old_selector);
	if (old_volt < 0)
		return old_volt;

	new_volt = rk818_dcdc_list_voltage(dev, new_selector);
	if (new_volt < 0)
		return new_volt;

	return DIV_ROUND_UP(abs(old_volt - new_volt)*2, 2500);
}

static int rk818_dcdc_suspend_enable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;

	return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG1, 1 << buck, 0);

}

static int rk818_dcdc_suspend_disable(struct regulator_dev *dev)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;

	return rk818_set_bits(rk818, RK818_SLEEP_SET_OFF_REG1, 1 << buck , 1 << buck);
}

static int rk818_dcdc_set_suspend_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct rk818 *rk818 = rdev_get_drvdata(dev);
	int buck = rdev_get_id(dev) - RK818_DCDC1;
	u16 mask = 0x80;

	switch(mode)
	{
		case REGULATOR_MODE_FAST:
			return rk818_set_bits(rk818, (rk818_BUCK_SET_VOL_REG(buck) + 0x01), mask, mask);
		case REGULATOR_MODE_NORMAL:
			return rk818_set_bits(rk818, (rk818_BUCK_SET_VOL_REG(buck) + 0x01), mask, 0);
		default:
			printk("error:pmu_rk818 only powersave and pwm mode\n");
			return -EINVAL;
	}

}

static struct regulator_ops rk818_dcdc_ops = {
	.set_voltage = rk818_dcdc_set_voltage,
	.get_voltage = rk818_dcdc_get_voltage,
	.list_voltage= rk818_dcdc_list_voltage,
	.is_enabled = rk818_dcdc_is_enabled,
	.enable = rk818_dcdc_enable,
	.disable = rk818_dcdc_disable,
	.get_mode = rk818_dcdc_get_mode,
	.set_mode = rk818_dcdc_set_mode,
	.set_suspend_enable =rk818_dcdc_suspend_enable,
	.set_suspend_disable =rk818_dcdc_suspend_disable,
	.set_suspend_mode = rk818_dcdc_set_suspend_mode,
	.set_suspend_voltage = rk818_dcdc_set_sleep_voltage,
	.set_voltage_time_sel = rk818_dcdc_set_voltage_time_sel,
};

static struct regulator_desc regulators[] = {

	{
		.name = "RK818_DCDC1",
		.id = 0,
		.ops = &rk818_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_DCDC2",
		.id = 1,
		.ops = &rk818_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_DCDC3",
		.id = 2,
		.ops = &rk818_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck4_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_DCDC4",
		.id = 3,
		.ops = &rk818_dcdc_ops,
		.n_voltages = ARRAY_SIZE(buck4_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},

	{
		.name = "RK818_LDO1",
		.id =4,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO2",
		.id = 5,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO3",
		.id = 6,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo3_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO4",
		.id = 7,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},

	{
		.name = "RK818_LDO5",
		.id =8,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO6",
		.id = 9,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo6_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO7",
		.id = 10,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo6_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO8",
		.id = 11,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO9",
		.id = 12,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(ldo_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "RK818_LDO10",
		.id = 13,
		.ops = &rk818_ldo_ops,
		.n_voltages = ARRAY_SIZE(buck4_voltage_map),
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},

};

int rk818_i2c_read(struct rk818 *rk818, char reg, int count,u8 *dest)
{
	struct i2c_client *i2c = rk818->i2c;

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
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].scl_rate = 200*1000;

	msgs[1].buf = dest;
	msgs[1].addr = i2c->addr;
	msgs[1].flags =  I2C_M_RD;
	msgs[1].len = count;
	msgs[1].scl_rate = RK818_I2C_ADDR_RATE;

	ret = i2c_transfer(adap, msgs, 2);

	return 0;
}

int rk818_i2c_write(struct rk818 *rk818, char reg, int count,  const u8 src)
{
	int ret=-1;
	struct i2c_client *i2c = rk818->i2c;
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
	msg.scl_rate = RK818_I2C_ADDR_RATE;

	ret = i2c_transfer(adap, &msg, 1);
	return ret;
}

u8 rk818_reg_read(struct rk818 *rk818, u8 reg)
{
	u8 val = 0;

	mutex_lock(&rk818->io_lock);

	rk818_i2c_read(rk818, reg, 1, &val);

	mutex_unlock(&rk818->io_lock);

	return val & 0xff;
}
EXPORT_SYMBOL_GPL(rk818_reg_read);

int rk818_reg_write(struct rk818 *rk818, u8 reg, u8 val)
{
	int err =0;

	mutex_lock(&rk818->io_lock);

	err = rk818_i2c_write(rk818, reg, 1,val);
	if (err < 0)
		dev_err(rk818->dev, "Write for reg 0x%x failed\n", reg);

	mutex_unlock(&rk818->io_lock);
	return err;
}
EXPORT_SYMBOL_GPL(rk818_reg_write);

int rk818_set_bits(struct rk818 *rk818, u8 reg, u8 mask, u8 val)
{
	u8 tmp;
	int ret;

	mutex_lock(&rk818->io_lock);

	ret = rk818_i2c_read(rk818, reg, 1, &tmp);
	tmp = (tmp & ~mask) | val;
	if (ret == 0) {
		ret = rk818_i2c_write(rk818, reg, 1, tmp);
	}
	rk818_i2c_read(rk818, reg, 1, &tmp);
	mutex_unlock(&rk818->io_lock);

	return 0;//ret;
}
EXPORT_SYMBOL_GPL(rk818_set_bits);

int rk818_clear_bits(struct rk818 *rk818, u8 reg, u8 mask)
{
	u8 data;
	int err;

	mutex_lock(&rk818->io_lock);
	err = rk818_i2c_read(rk818, reg, 1, &data);
	if (err <0) {
		dev_err(rk818->dev, "read from reg %x failed\n", reg);
		goto out;
	}

	data &= ~mask;
	err = rk818_i2c_write(rk818, reg, 1, data);
	if (err <0)
		dev_err(rk818->dev, "write to reg %x failed\n", reg);

out:
	mutex_unlock(&rk818->io_lock);
	return err;
}
EXPORT_SYMBOL_GPL(rk818_clear_bits);

#ifdef CONFIG_OF
static struct of_device_id rk818_of_match[] = {
	{ .compatible = "rockchip,rk818"},
	{ },
};
MODULE_DEVICE_TABLE(of, rk818_of_match);
#endif

#ifdef CONFIG_OF
static struct of_regulator_match rk818_reg_matches[] = {
	{ .name = "rk818_dcdc1", .driver_data = (void *)0 },
	{ .name = "rk818_dcdc2", .driver_data = (void *)1 },
	{ .name = "rk818_dcdc3", .driver_data = (void *)2 },
	{ .name = "rk818_dcdc4", .driver_data = (void *)3 },
	{ .name = "rk818_ldo1", .driver_data = (void *)4 },
	{ .name = "rk818_ldo2", .driver_data = (void *)5 },
	{ .name = "rk818_ldo3", .driver_data = (void *)6 },
	{ .name = "rk818_ldo4", .driver_data = (void *)7 },
	{ .name = "rk818_ldo5", .driver_data = (void *)8 },
	{ .name = "rk818_ldo6", .driver_data = (void *)9 },
	{ .name = "rk818_ldo7", .driver_data = (void *)10 },
	{ .name = "rk818_ldo8", .driver_data = (void *)11 },
	{ .name = "rk818_ldo9", .driver_data = (void *)12 },
	{ .name = "rk818_ldo10", .driver_data = (void *)13 },
};

static struct rk818_board *rk818_parse_dt(struct rk818 *rk818)
{
	struct rk818_board *pdata;
	struct device_node *regs,*rk818_pmic_np;
	int i, count;

	rk818_pmic_np = of_node_get(rk818->dev->of_node);
	if (!rk818_pmic_np) {
		printk("could not find pmic sub-node\n");
		return NULL;
	}

	regs = of_find_node_by_name(rk818_pmic_np, "regulators");
	if (!regs)
		return NULL;

	count = of_regulator_match(rk818->dev, regs, rk818_reg_matches,
			rk818_NUM_REGULATORS);
	of_node_put(regs);
	if ((count < 0) || (count > rk818_NUM_REGULATORS))
		return NULL;

	pdata = devm_kzalloc(rk818->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return NULL;

	for (i = 0; i < count; i++) {
		if (!rk818_reg_matches[i].init_data || !rk818_reg_matches[i].of_node)
			continue;

		pdata->rk818_init_data[i] = rk818_reg_matches[i].init_data;
		pdata->of_node[i] = rk818_reg_matches[i].of_node;
	}
	pdata->irq = rk818->chip_irq;
	pdata->irq_base = -1;

	pdata->irq_gpio = of_get_named_gpio(rk818_pmic_np,"gpios",0);
	if (!gpio_is_valid(pdata->irq_gpio)) {
		printk("invalid gpio: %d\n",  pdata->irq_gpio);
		return NULL;
	}

	pdata->pmic_sleep_gpio = of_get_named_gpio(rk818_pmic_np,"gpios",1);
	if (!gpio_is_valid(pdata->pmic_sleep_gpio)) {
		printk("invalid gpio: %d\n",  pdata->pmic_sleep_gpio);
	}
	pdata->pmic_sleep = true;
	pdata->pm_off = of_property_read_bool(rk818_pmic_np,"rk818,system-power-controller");

	return pdata;
}

#else
static struct rk818_board *rk818_parse_dt(struct i2c_client *i2c)
{
	return NULL;
}
#endif

static int rk818_pre_init(struct rk818 *rk818)
{
	int ret,val;
	printk("%s,line=%d\n", __func__,__LINE__);

	ret = rk818_set_bits(rk818, 0xa1, (0xF<<0),(0x7));
	ret = rk818_set_bits(rk818, 0xa1,(0x7<<4),(0x7<<4)); //close charger when usb low then 3.4V
	ret = rk818_set_bits(rk818, 0x52,(0x1<<1),(0x1<<1)); //no action when vref
	ret = rk818_set_bits(rk818, 0x52,(0x1<<0),(0x1<<0)); //enable HDMI 5V

	/*******enable switch and boost***********/
	val = rk818_reg_read(rk818,RK818_DCDC_EN_REG);
	val |= (0x3 << 5);    //enable switch1/2
	val |= (0x1 << 4);    //enable boost
	ret = rk818_reg_write(rk818,RK818_DCDC_EN_REG,val);
	if (ret <0) {
		printk(KERN_ERR "Unable to write RK818_DCDC_EN_REG reg\n");
		return ret;
	}

	/****************set vbat low **********/
	val = rk818_reg_read(rk818,RK818_VB_MON_REG);
	val &=(~(VBAT_LOW_VOL_MASK | VBAT_LOW_ACT_MASK));
	val |= (RK818_VBAT_LOW_3V0 | EN_VABT_LOW_SHUT_DOWN);
	ret = rk818_reg_write(rk818,RK818_VB_MON_REG,val);
	if (ret <0) {
		printk(KERN_ERR "Unable to write RK818_VB_MON_REG reg\n");
		return ret;
	}

	/**********mask int****************/
	val = rk818_reg_read(rk818,RK818_INT_STS_MSK_REG1);
	val |= (0x1<<0); //mask vout_lo_int
	ret = rk818_reg_write(rk818,RK818_INT_STS_MSK_REG1,val);
	if (ret <0) {
		printk(KERN_ERR "Unable to write RK818_INT_STS_MSK_REG1 reg\n");
		return ret;
	}

	/**********enable clkout2****************/
	ret = rk818_reg_write(rk818,RK818_CLK32OUT_REG,0x01);
	if (ret <0) {
		printk(KERN_ERR "Unable to write RK818_CLK32OUT_REG reg\n");
		return ret;
	}

	ret = rk818_clear_bits(rk818, RK818_INT_STS_MSK_REG1,(0x3<<5)); //open rtc int when power on
	ret = rk818_set_bits(rk818, RK818_RTC_INT_REG,(0x1<<3),(0x1<<3)); //open rtc int when power on

	/*****disable otg and boost when in sleep mode****/
	val = rk818_reg_read(rk818, RK818_SLEEP_SET_OFF_REG1);
	val |= ((0x1 << 7) | (0x1 << 4));
	ret =  rk818_reg_write(rk818, RK818_SLEEP_SET_OFF_REG1, val);
	if (ret < 0) {
		pr_err("Unable to write RK818_SLEEP_SET_OFF_REG1 reg\n");
		return ret;
	}

	return 0;
}

static int  rk818_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct rk818 *rk818;
	struct rk818_board *pdev;
	const struct of_device_id *match;
	struct regulator_config config = { };
	struct regulator_dev *rk818_rdev;
	struct regulator_init_data *reg_data;
	const char *rail_name = NULL;
	int ret, i = 0;

	printk("%s,line=%d\n", __func__,__LINE__);

	if (i2c->dev.of_node) {
		match = of_match_device(rk818_of_match, &i2c->dev);
		if (!match) {
			dev_err(&i2c->dev,"Failed to find matching dt id\n");
			return -EINVAL;
		}
	}

	rk818 = devm_kzalloc(&i2c->dev,sizeof(struct rk818), GFP_KERNEL);
	if (rk818 == NULL) {
		ret = -ENOMEM;
	}
	rk818->i2c = i2c;
	rk818->dev = &i2c->dev;
	i2c_set_clientdata(i2c, rk818);

	mutex_init(&rk818->io_lock);

	ret = rk818_reg_read(rk818,0x2f);
	if ((ret < 0) || (ret == 0xff)){
		printk("The device is not rk818 %d\n",ret);
	}

	ret = rk818_pre_init(rk818);
	if (ret < 0){
		printk("The rk818_pre_init failed %d\n",ret);
	}

	if (rk818->dev->of_node)
		pdev = rk818_parse_dt(rk818);

	/******************************set sleep vol & dcdc mode******************/
#ifdef CONFIG_OF
	rk818->pmic_sleep_gpio = pdev->pmic_sleep_gpio;
	if (rk818->pmic_sleep_gpio) {
		ret = gpio_request(rk818->pmic_sleep_gpio, "rk818_pmic_sleep");
		if (ret < 0) {
			dev_err(rk818->dev,"Failed to request gpio %d with ret:""%d\n",	rk818->pmic_sleep_gpio, ret);
			return IRQ_NONE;
		}
		gpio_direction_output(rk818->pmic_sleep_gpio,0);
		ret = gpio_get_value(rk818->pmic_sleep_gpio);
		gpio_free(rk818->pmic_sleep_gpio);
		pr_info("%s: rk818_pmic_sleep=%x\n", __func__, ret);
	}
#endif
	/**********************************************************/

	if (pdev) {
		rk818->num_regulators = rk818_NUM_REGULATORS;
		rk818->rdev = kcalloc(rk818_NUM_REGULATORS,sizeof(struct regulator_dev *), GFP_KERNEL);
		if (!rk818->rdev) {
			return -ENOMEM;
		}
		/* Instantiate the regulators */
		for (i = 0; i < rk818_NUM_REGULATORS; i++) {
			reg_data = pdev->rk818_init_data[i];
			if (!reg_data)
				continue;
			config.dev = rk818->dev;
			config.driver_data = rk818;
			if (rk818->dev->of_node)
				config.of_node = pdev->of_node[i];
			if (reg_data && reg_data->constraints.name)
				rail_name = reg_data->constraints.name;
			else
				rail_name = regulators[i].name;
			reg_data->supply_regulator = rail_name;

			config.init_data =reg_data;

			rk818_rdev = regulator_register(&regulators[i],&config);
			if (IS_ERR(rk818_rdev)) {
				printk("failed to register %d regulator\n",i);
			}
			rk818->rdev[i] = rk818_rdev;
		}
	}

	rk818->irq_gpio = pdev->irq_gpio;
	ret = rk818_irq_init(rk818, rk818->irq_gpio, pdev);
	if (ret < 0)
		printk("rk818 irq init error\n");

	return 0;
}

static int rk818_i2c_remove(struct i2c_client *i2c)
{
	struct rk818 *rk818 = i2c_get_clientdata(i2c);
	int i;

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	for (i = 0; i < rk818->num_regulators; i++)
		if (rk818->rdev[i])
			regulator_unregister(rk818->rdev[i]);
	i2c_set_clientdata(i2c, NULL);

	return 0;
}

static const struct i2c_device_id rk818_i2c_id[] = {
	{ "rk818", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, rk818_i2c_id);

static struct i2c_driver rk818_i2c_driver = {
	.driver = {
		.name = "rk818",
		.owner = THIS_MODULE,
		.of_match_table =of_match_ptr(rk818_of_match),
	},
	.probe    = rk818_i2c_probe,
	.remove   = rk818_i2c_remove,
	.id_table = rk818_i2c_id,
};

static int rk818_module_init(void)
{
	int ret;
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	ret = i2c_add_driver(&rk818_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);
	return ret;
}
module_init(rk818_module_init);
//subsys_initcall_sync(rk818_module_init);

static void rk818_module_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	i2c_del_driver(&rk818_i2c_driver);
}
module_exit(rk818_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("M_O_Bz@163.com");
MODULE_DESCRIPTION("rk818 PMIC driver");
