#ifndef __LINUX_REGULATOR_rk808_H
#define __LINUX_REGULATOR_rk808_H

#include <linux/regulator/machine.h>
#include <linux/regmap.h>

/*
 * rk808 Global Register Map.
 */

#define RK808_DCDC1	0 /* (0+RK808_START) */
#define RK808_LDO1	4 /* (4+RK808_START) */
#define RK808_NUM_REGULATORS   14

enum rk808_reg {
	RK808_ID_DCDC1,
	RK808_ID_DCDC2,
	RK808_ID_DCDC3,
	RK808_ID_DCDC4,
	RK808_ID_LDO1,
	RK808_ID_LDO2,
	RK808_ID_LDO3,
	RK808_ID_LDO4,
	RK808_ID_LDO5,
	RK808_ID_LDO6,
	RK808_ID_LDO7,
	RK808_ID_LDO8,
	RK808_ID_SWITCH1,
	RK808_ID_SWITCH2,
};

enum rk818_reg {
	RK818_ID_DCDC1,
	RK818_ID_DCDC2,
	RK818_ID_DCDC3,
	RK818_ID_DCDC4,
	RK818_ID_LDO1,
	RK818_ID_LDO2,
	RK818_ID_LDO3,
	RK818_ID_LDO4,
	RK818_ID_LDO5,
	RK818_ID_LDO6,
	RK818_ID_LDO7,
	RK818_ID_LDO8,
	RK818_ID_LDO9,
	RK818_ID_SWITCH,
};

#define RK808_SECONDS_REG	0x00
#define RK808_MINUTES_REG	0x01
#define RK808_HOURS_REG		0x02
#define RK808_DAYS_REG		0x03
#define RK808_MONTHS_REG	0x04
#define RK808_YEARS_REG		0x05
#define RK808_WEEKS_REG		0x06
#define RK808_ALARM_SECONDS_REG	0x08
#define RK808_ALARM_MINUTES_REG	0x09
#define RK808_ALARM_HOURS_REG	0x0a
#define RK808_ALARM_DAYS_REG	0x0b
#define RK808_ALARM_MONTHS_REG	0x0c
#define RK808_ALARM_YEARS_REG	0x0d
#define RK808_RTC_CTRL_REG	0x10
#define RK808_RTC_STATUS_REG	0x11
#define RK808_RTC_INT_REG	0x12
#define RK808_RTC_COMP_LSB_REG	0x13
#define RK808_RTC_COMP_MSB_REG	0x14
#define RK808_CLK32OUT_REG	0x20
#define RK808_VB_MON_REG	0x21
#define RK808_THERMAL_REG	0x22
#define RK808_DCDC_EN_REG	0x23
#define RK808_LDO_EN_REG	0x24
#define RK808_SLEEP_SET_OFF_REG1	0x25
#define RK808_SLEEP_SET_OFF_REG2	0x26
#define RK808_DCDC_UV_STS_REG	0x27
#define RK808_DCDC_UV_ACT_REG	0x28
#define RK808_LDO_UV_STS_REG	0x29
#define RK808_LDO_UV_ACT_REG	0x2a
#define RK808_DCDC_PG_REG	0x2b
#define RK808_LDO_PG_REG	0x2c
#define RK808_VOUT_MON_TDB_REG	0x2d
#define RK808_BUCK1_CONFIG_REG		0x2e
#define RK808_BUCK1_ON_VSEL_REG		0x2f
#define RK808_BUCK1_SLP_VSEL_REG	0x30
#define RK808_BUCK1_DVS_VSEL_REG	0x31
#define RK808_BUCK2_CONFIG_REG		0x32
#define RK808_BUCK2_ON_VSEL_REG		0x33
#define RK808_BUCK2_SLP_VSEL_REG	0x34
#define RK808_BUCK2_DVS_VSEL_REG	0x35
#define RK808_BUCK3_CONFIG_REG		0x36
#define RK808_BUCK4_CONFIG_REG		0x37
#define RK808_BUCK4_ON_VSEL_REG		0x38
#define RK808_BUCK4_SLP_VSEL_REG	0x39
#define RK808_BOOST_CONFIG_REG		0x3a
#define RK808_LDO1_ON_VSEL_REG		0x3b
#define RK808_LDO1_SLP_VSEL_REG		0x3c
#define RK808_LDO2_ON_VSEL_REG		0x3d
#define RK808_LDO2_SLP_VSEL_REG		0x3e
#define RK808_LDO3_ON_VSEL_REG		0x3f
#define RK808_LDO3_SLP_VSEL_REG		0x40
#define RK808_LDO4_ON_VSEL_REG		0x41
#define RK808_LDO4_SLP_VSEL_REG		0x42
#define RK808_LDO5_ON_VSEL_REG		0x43
#define RK808_LDO5_SLP_VSEL_REG		0x44
#define RK808_LDO6_ON_VSEL_REG		0x45
#define RK808_LDO6_SLP_VSEL_REG		0x46
#define RK808_LDO7_ON_VSEL_REG		0x47
#define RK808_LDO7_SLP_VSEL_REG		0x48
#define RK808_LDO8_ON_VSEL_REG		0x49
#define RK808_LDO8_SLP_VSEL_REG		0x4a
#define RK808_DEVCTRL_REG	0x4b
#define RK808_INT_STS_REG1	0x4c
#define RK808_INT_STS_MSK_REG1	0x4d
#define RK808_INT_STS_REG2	0x4e
#define RK808_INT_STS_MSK_REG2	0x4f
#define RK808_IO_POL_REG	0x50

#define RK818_VB_MON_REG		0x21
#define RK818_THERMAL_REG		0x22
#define RK818_DCDC_EN_REG		0x23
#define RK818_LDO_EN_REG		0x24
#define RK818_SLEEP_SET_OFF_REG1	0x25
#define RK818_SLEEP_SET_OFF_REG2	0x26
#define RK818_DCDC_UV_STS_REG		0x27
#define RK818_DCDC_UV_ACT_REG		0x28
#define RK818_LDO_UV_STS_REG		0x29
#define RK818_LDO_UV_ACT_REG		0x2a
#define RK818_DCDC_PG_REG		0x2b
#define RK818_LDO_PG_REG		0x2c
#define RK818_VOUT_MON_TDB_REG		0x2d
#define RK818_BUCK1_CONFIG_REG		0x2e
#define RK818_BUCK1_ON_VSEL_REG		0x2f
#define RK818_BUCK1_SLP_VSEL_REG	0x30
#define RK818_BUCK2_CONFIG_REG		0x32
#define RK818_BUCK2_ON_VSEL_REG		0x33
#define RK818_BUCK2_SLP_VSEL_REG	0x34
#define RK818_BUCK3_CONFIG_REG		0x36
#define RK818_BUCK4_CONFIG_REG		0x37
#define RK818_BUCK4_ON_VSEL_REG		0x38
#define RK818_BUCK4_SLP_VSEL_REG	0x39
#define RK818_BOOST_CONFIG_REG		0x3a
#define RK818_LDO1_ON_VSEL_REG		0x3b
#define RK818_LDO1_SLP_VSEL_REG		0x3c
#define RK818_LDO2_ON_VSEL_REG		0x3d
#define RK818_LDO2_SLP_VSEL_REG		0x3e
#define RK818_LDO3_ON_VSEL_REG		0x3f
#define RK818_LDO3_SLP_VSEL_REG		0x40
#define RK818_LDO4_ON_VSEL_REG		0x41
#define RK818_LDO4_SLP_VSEL_REG		0x42
#define RK818_LDO5_ON_VSEL_REG		0x43
#define RK818_LDO5_SLP_VSEL_REG		0x44
#define RK818_LDO6_ON_VSEL_REG		0x45
#define RK818_LDO6_SLP_VSEL_REG		0x46
#define RK818_LDO7_ON_VSEL_REG		0x47
#define RK818_LDO7_SLP_VSEL_REG		0x48
#define RK818_LDO8_ON_VSEL_REG		0x49
#define RK818_LDO8_SLP_VSEL_REG		0x4a
#define RK818_DEVCTRL_REG		0x4b
#define RK818_INT_STS_REG1		0X4c
#define RK818_INT_STS_MSK_REG1		0X4d
#define RK818_INT_STS_REG2		0X4e
#define RK818_INT_STS_MSK_REG2		0X4f
#define RK818_IO_POL_REG		0X50
#define RK818_OTP_VDD_EN_REG		0x51
#define RK818_H5V_EN_REG		0x52
#define RK818_SLEEP_SET_OFF_REG3	0x53
#define RK818_BOOST_LDO9_ON_VSEL_REG	0x54
#define RK818_BOOST_LDO9_SLP_VSEL_REG	0x55
#define RK818_BOOST_CTRL_REG		0x56
#define RK818_DCDC_ILMAX_REG		0x90
#define RK818_CHRG_COMP_REG		0x9a
#define RK818_SUP_STS_REG		0xa0
#define RK818_USB_CTRL_REG		0xa1
#define RK818_CHRG_CTRL_REG1		0xa3
#define RK818_CHRG_CTRL_REG2		0xa4
#define RK818_CHRG_CTRL_REG3		0xa5
#define RK818_BAT_CTRL_REG		0xa6
#define RK818_BAT_HTS_TS1_REG		0xa8
#define RK818_BAT_LTS_TS1_REG		0xa9
#define RK818_BAT_HTS_TS2_REG		0xaa
#define RK818_BAT_LTS_TS2_REG		0xab
#define RK818_TS_CTRL_REG		0xac
#define RK818_ADC_CTRL_REG		0xad
#define RK818_ON_SOURCE_REG		0xae
#define RK818_OFF_SOURCE_REG		0xaf
#define RK818_GGCON_REG			0xb0
#define RK818_GGSTS_REG			0xb1
#define RK818_FRAME_SMP_INTERV_REG	0xb2
#define RK818_AUTO_SLP_CUR_THR_REG	0xb3
#define RK818_GASCNT_CAL_REG3		0xb4
#define RK818_GASCNT_CAL_REG2		0xb5
#define RK818_GASCNT_CAL_REG1		0xb6
#define RK818_GASCNT_CAL_REG0		0xb7
#define RK818_GASCNT3_REG		0xb8
#define RK818_GASCNT2_REG		0xb9
#define RK818_GASCNT1_REG		0xba
#define RK818_GASCNT0_REG		0xbb
#define RK818_BAT_CUR_AVG_REGH		0xbc
#define RK818_BAT_CUR_AVG_REGL		0xbd
#define RK818_TS1_ADC_REGH		0xbe
#define RK818_TS1_ADC_REGL		0xbf
#define RK818_TS2_ADC_REGH		0xc0
#define RK818_TS2_ADC_REGL		0xc1
#define RK818_BAT_OCV_REGH		0xc2
#define RK818_BAT_OCV_REGL		0xc3
#define RK818_BAT_VOL_REGH		0xc4
#define RK818_BAT_VOL_REGL		0xc5
#define RK818_RELAX_ENTRY_THRES_REGH	0xc6
#define RK818_RELAX_ENTRY_THRES_REGL	0xc7
#define RK818_RELAX_EXIT_THRES_REGH	0xc8
#define RK818_RELAX_EXIT_THRES_REGL	0xc9
#define RK818_RELAX_VOL1_REGH		0xca
#define RK818_RELAX_VOL1_REGL		0xcb
#define RK818_RELAX_VOL2_REGH		0xcc
#define RK818_RELAX_VOL2_REGL		0xcd
#define RK818_BAT_CUR_R_CALC_REGH	0xce
#define RK818_BAT_CUR_R_CALC_REGL	0xcf
#define RK818_BAT_VOL_R_CALC_REGH	0xd0
#define RK818_BAT_VOL_R_CALC_REGL	0xd1
#define RK818_CAL_OFFSET_REGH		0xd2
#define RK818_CAL_OFFSET_REGL		0xd3
#define RK818_NON_ACT_TIMER_CNT_REG	0xd4
#define RK818_VCALIB0_REGH		0xd5
#define RK818_VCALIB0_REGL		0xd6
#define RK818_VCALIB1_REGH		0xd7
#define RK818_VCALIB1_REGL		0xd8
#define RK818_IOFFSET_REGH		0xdd
#define RK818_IOFFSET_REGL		0xde
#define RK818_SOC_REG			0xe0
#define RK818_REMAIN_CAP_REG3		0xe1
#define RK818_REMAIN_CAP_REG2		0xe2
#define RK818_REMAIN_CAP_REG1		0xe3
#define RK818_REMAIN_CAP_REG0		0xe4
#define RK818_UPDAT_LEVE_REG		0xe5
#define RK818_NEW_FCC_REG3		0xe6
#define RK818_NEW_FCC_REG2		0xe7
#define RK818_NEW_FCC_REG1		0xe8
#define RK818_NEW_FCC_REG0		0xe9
#define RK818_NON_ACT_TIMER_CNT_SAVE_REG 0xea
#define RK818_OCV_VOL_VALID_REG		0xeb
#define RK818_REBOOT_CNT_REG		0xec
#define RK818_POFFSET_REG		0xed
#define RK818_MISC_MARK_REG		0xee
#define RK818_HALT_CNT_REG		0xef
#define RK818_CALC_REST_REGH		0xf0
#define RK818_CALC_REST_REGL		0xf1
#define RK818_SAVE_DATA19		0xf2
#define RK818_NUM_REGULATORS		14

/* IRQ Definitions */
#define RK808_IRQ_VOUT_LO	0
#define RK808_IRQ_VB_LO		1
#define RK808_IRQ_PWRON		2
#define RK808_IRQ_PWRON_LP	3
#define RK808_IRQ_HOTDIE	4
#define RK808_IRQ_RTC_ALARM	5
#define RK808_IRQ_RTC_PERIOD	6
#define RK808_IRQ_PLUG_IN_INT	7
#define RK808_IRQ_PLUG_OUT_INT	8
#define RK808_NUM_IRQ		9

#define RK808_IRQ_VOUT_LO_MSK		BIT(0)
#define RK808_IRQ_VB_LO_MSK		BIT(1)
#define RK808_IRQ_PWRON_MSK		BIT(2)
#define RK808_IRQ_PWRON_LP_MSK		BIT(3)
#define RK808_IRQ_HOTDIE_MSK		BIT(4)
#define RK808_IRQ_RTC_ALARM_MSK		BIT(5)
#define RK808_IRQ_RTC_PERIOD_MSK	BIT(6)
#define RK808_IRQ_PLUG_IN_INT_MSK	BIT(0)
#define RK808_IRQ_PLUG_OUT_INT_MSK	BIT(1)

#define RK808_VBAT_LOW_2V8	0x00
#define RK808_VBAT_LOW_2V9	0x01
#define RK808_VBAT_LOW_3V0	0x02
#define RK808_VBAT_LOW_3V1	0x03
#define RK808_VBAT_LOW_3V2	0x04
#define RK808_VBAT_LOW_3V3	0x05
#define RK808_VBAT_LOW_3V4	0x06
#define RK808_VBAT_LOW_3V5	0x07
#define VBAT_LOW_VOL_MASK	(0x07 << 0)
#define EN_VABT_LOW_SHUT_DOWN	(0x00 << 4)
#define EN_VBAT_LOW_IRQ		(0x1 << 4)
#define VBAT_LOW_ACT_MASK	(0x1 << 4)

#define BUCK_ILMIN_MASK		(7 << 0)
#define BOOST_ILMIN_MASK	(7 << 0)
#define BUCK1_RATE_MASK		(3 << 3)
#define BUCK2_RATE_MASK		(3 << 3)
#define MASK_ALL	0xff

#define BUCK_UV_ACT_MASK	0x0f
#define BUCK_UV_ACT_DISABLE	0

#define SWITCH2_EN	BIT(6)
#define SWITCH1_EN	BIT(5)
#define DEV_OFF_RST	BIT(3)
#define DEV_OFF		BIT(0)

#define VB_LO_ACT		BIT(4)
#define VB_LO_SEL_3500MV	(7 << 0)

#define VOUT_LO_INT	BIT(0)
#define CLK32KOUT2_EN	BIT(0)
#define H5V_EN_MASK		BIT(0)
#define H5V_EN_ENABLE		BIT(0)
#define REF_RDY_CTRL_MASK	BIT(1)
#define REF_RDY_CTRL_ENABLE	BIT(1)

/*RK818_DCDC_EN_REG*/
#define BUCK1_EN_MASK		BIT(0)
#define BUCK2_EN_MASK		BIT(1)
#define BUCK3_EN_MASK		BIT(2)
#define BUCK4_EN_MASK		BIT(3)
#define BOOST_EN_MASK		BIT(4)
#define LDO9_EN_MASK		BIT(5)
#define SWITCH_EN_MASK		BIT(6)
#define OTG_EN_MASK		BIT(7)

#define BUCK1_EN_ENABLE		BIT(0)
#define BUCK2_EN_ENABLE		BIT(1)
#define BUCK3_EN_ENABLE		BIT(2)
#define BUCK4_EN_ENABLE		BIT(3)
#define BOOST_EN_ENABLE		BIT(4)
#define LDO9_EN_ENABLE		BIT(5)
#define SWITCH_EN_ENABLE	BIT(6)
#define OTG_EN_ENABLE		BIT(7)

/* IRQ Definitions */
#define RK818_IRQ_VOUT_LO	0
#define RK818_IRQ_VB_LO		1
#define RK818_IRQ_PWRON		2
#define RK818_IRQ_PWRON_LP	3
#define RK818_IRQ_HOTDIE	4
#define RK818_IRQ_RTC_ALARM	5
#define RK818_IRQ_RTC_PERIOD	6
#define RK818_IRQ_USB_OV	7
#define RK818_IRQ_PLUG_IN	8
#define RK818_IRQ_PLUG_OUT	9
#define RK818_IRQ_CHG_OK	10
#define RK818_IRQ_CHG_TE	11
#define RK818_IRQ_CHG_TS1	12
#define RK818_IRQ_TS2		13
#define RK818_IRQ_CHG_CVTLIM	14
#define RK818_IRQ_DISCHG_ILIM	15

#define BUCK1_SLP_SET_MASK	BIT(0)
#define BUCK2_SLP_SET_MASK	BIT(1)
#define BUCK3_SLP_SET_MASK	BIT(2)
#define BUCK4_SLP_SET_MASK	BIT(3)
#define BOOST_SLP_SET_MASK	BIT(4)
#define LDO9_SLP_SET_MASK	BIT(5)
#define SWITCH_SLP_SET_MASK	BIT(6)
#define OTG_SLP_SET_MASK	BIT(7)

#define BUCK1_SLP_SET_OFF	BIT(0)
#define BUCK2_SLP_SET_OFF	BIT(1)
#define BUCK3_SLP_SET_OFF	BIT(2)
#define BUCK4_SLP_SET_OFF	BIT(3)
#define BOOST_SLP_SET_OFF	BIT(4)
#define LDO9_SLP_SET_OFF	BIT(5)
#define SWITCH_SLP_SET_OFF	BIT(6)
#define OTG_SLP_SET_OFF		BIT(7)

#define BUCK1_SLP_SET_ON	BIT(0)
#define BUCK2_SLP_SET_ON	BIT(1)
#define BUCK3_SLP_SET_ON	BIT(2)
#define BUCK4_SLP_SET_ON	BIT(3)
#define BOOST_SLP_SET_ON	BIT(4)
#define LDO9_SLP_SET_ON		BIT(5)
#define SWITCH_SLP_SET_ON	BIT(6)
#define OTG_SLP_SET_ON		BIT(7)

#define VOUT_LO_MASK		BIT(0)
#define VB_LO_MASK		BIT(1)
#define PWRON_MASK		BIT(2)
#define PWRON_LP_MASK		BIT(3)
#define HOTDIE_MASK		BIT(4)
#define RTC_ALARM_MASK		BIT(5)
#define RTC_PERIOD_MASK		BIT(6)
#define USB_OV_MASK		BIT(7)

#define VOUT_LO_DISABLE		BIT(0)
#define VB_LO_DISABLE		BIT(1)
#define PWRON_DISABLE		BIT(2)
#define PWRON_LP_DISABLE	BIT(3)
#define HOTDIE_DISABLE		BIT(4)
#define RTC_ALARM_DISABLE	BIT(5)
#define RTC_PERIOD_DISABLE	BIT(6)
#define USB_OV_INT_DISABLE	BIT(7)

#define VOUT_LO_ENABLE		(0 << 0)
#define VB_LO_ENABLE		(0 << 1)
#define PWRON_ENABLE		(0 << 2)
#define PWRON_LP_ENABLE		(0 << 3)
#define HOTDIE_ENABLE		(0 << 4)
#define RTC_ALARM_ENABLE	(0 << 5)
#define RTC_PERIOD_ENABLE	(0 << 6)
#define USB_OV_INT_ENABLE	(0 << 7)

#define PLUG_IN_MASK		BIT(0)
#define PLUG_OUT_MASK		BIT(1)
#define CHGOK_MASK		BIT(2)
#define CHGTE_MASK		BIT(3)
#define CHGTS1_MASK		BIT(4)
#define TS2_MASK		BIT(5)
#define CHG_CVTLIM_MASK		BIT(6)
#define DISCHG_ILIM_MASK	BIT(7)

#define PLUG_IN_DISABLE		BIT(0)
#define PLUG_OUT_DISABLE	BIT(1)
#define CHGOK_DISABLE		BIT(2)
#define CHGTE_DISABLE		BIT(3)
#define CHGTS1_DISABLE		BIT(4)
#define TS2_DISABLE		BIT(5)
#define CHG_CVTLIM_DISABLE	BIT(6)
#define DISCHG_ILIM_DISABLE	BIT(7)

#define PLUG_IN_ENABLE		BIT(0)
#define PLUG_OUT_ENABLE		BIT(1)
#define CHGOK_ENABLE		BIT(2)
#define CHGTE_ENABLE		BIT(3)
#define CHGTS1_ENABLE		BIT(4)
#define TS2_ENABLE		BIT(5)
#define CHG_CVTLIM_ENABLE	BIT(6)
#define DISCHG_ILIM_ENABLE	BIT(7)

enum {
	BUCK_ILMIN_50MA,
	BUCK_ILMIN_100MA,
	BUCK_ILMIN_150MA,
	BUCK_ILMIN_200MA,
	BUCK_ILMIN_250MA,
	BUCK_ILMIN_300MA,
	BUCK_ILMIN_350MA,
	BUCK_ILMIN_400MA,
};

enum {
	BOOST_ILMIN_75MA,
	BOOST_ILMIN_100MA,
	BOOST_ILMIN_125MA,
	BOOST_ILMIN_150MA,
	BOOST_ILMIN_175MA,
	BOOST_ILMIN_200MA,
	BOOST_ILMIN_225MA,
	BOOST_ILMIN_250MA,
};

/*
 * 该数据结构表示rk808芯片
 * 用这个结构体定义的变量名为chip
 */
struct rk808 {
	struct i2c_client *i2c;
	struct regmap_irq_chip_data *irq_data;
	struct regmap *regmap;
	int hold_gpio;
	int stby_gpio;
};

struct rk808_reg_data {
	int addr;
	int mask;
	int value;
};

/*
 * 该结构体表示芯片的硬件平台数据
 * 这个结构体定义的变量名为pdata
 */
struct rk8xx_power_data {
	char *name;
	const struct rk808_reg_data *rk8xx_pre_init_reg;
	int reg_num;
	const struct regmap_config *rk8xx_regmap_config;
	const struct mfd_cell *rk8xx_cell;
	int cell_num;
	struct regmap_irq_chip *rk8xx_irq_chip;
	int (*pm_shutdown)(struct regmap *regmap);
};

#endif /* __LINUX_REGULATOR_rk808_H */
