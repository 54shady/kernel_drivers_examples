#ifndef _ACT8846_H_
#define _ACT8846_H_

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

/* 用来表示device tree里的信息,用pdata表示 */
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

#endif
