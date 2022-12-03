#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/slab.h>
#include <linux/delay.h>

struct pwm_bl_data {
	struct pwm_device	*pwm;
	struct device		*dev;
	unsigned int		period;
	unsigned int		lth_brightness;
	unsigned int		*levels;
	bool			enabled;
	int			enable_gpio;
	unsigned long		enable_gpio_flags;
	unsigned int		scale;
	int			(*notify)(struct device *,
					  int brightness);
	void			(*notify_after)(struct device *,
					int brightness);
	int			(*check_fb)(struct device *, struct fb_info *);
	void			(*exit)(struct device *);
};

/* 开背光 */
static void pwm_backlight_power_on(struct pwm_bl_data *pbd, int brightness)
{
	/* 背光使能标志已经是开则不需要再开 */
	if (pbd->enabled)
		return;

	/* 否则判断引脚是否有效, 判断是低电平有效还是高电平有效, 设置成有效的电平 */
	if (gpio_is_valid(pbd->enable_gpio)) {
		if (pbd->enable_gpio_flags & PWM_BACKLIGHT_GPIO_ACTIVE_LOW)
			gpio_set_value(pbd->enable_gpio, 0);
		else
			gpio_set_value(pbd->enable_gpio, 1);
	}

	/* 设置相关状态 */
	pwm_enable(pbd->pwm);
	pbd->enabled = true;
}

/* 关背光 */
static void pwm_backlight_power_off(struct pwm_bl_data *pbd)
{
	/* 背光使能标志已经是关则不需要再关 */
	if (!pbd->enabled)
		return;

	pwm_config(pbd->pwm, 0, pbd->period);
	pwm_disable(pbd->pwm);

	/* PIN脚的操作正好和开是相反的 */
	if (gpio_is_valid(pbd->enable_gpio)) {
		if (pbd->enable_gpio_flags & PWM_BACKLIGHT_GPIO_ACTIVE_LOW)
			gpio_set_value(pbd->enable_gpio, 1);
		else
			gpio_set_value(pbd->enable_gpio, 0);
	}

	pbd->enabled = false;
}

/* 根据亮度值来计算占空比 */
static int compute_duty_cycle(struct pwm_bl_data *pbd, int brightness)
{
	unsigned int lth = pbd->lth_brightness;
	int duty_cycle;

	if (pbd->levels)
		duty_cycle = pbd->levels[brightness];
	else
		duty_cycle = brightness;

	return (duty_cycle * (pbd->period - lth) / pbd->scale) + lth;
}

static int pwm_backlight_update_status(struct backlight_device *bl)
{
	struct pwm_bl_data *pbd = bl_get_data(bl);
	int brightness = bl->props.brightness;
	int duty_cycle;

	printk("set brightness %d to backlight\n", brightness);

	if (brightness > 0) {
		duty_cycle = compute_duty_cycle(pbd, brightness);
		pwm_config(pbd->pwm, duty_cycle, pbd->period);
		pwm_backlight_power_on(pbd, brightness);
	} else {
		pwm_backlight_power_off(pbd);
	}

	return 0;
}

static const struct backlight_ops pwm_backlight_ops = {
	.update_status	= pwm_backlight_update_status,
};

static int pwm_backlight_parse_dt(struct device *dev,
				  struct platform_pwm_backlight_data *data)
{
	struct device_node *node = dev->of_node;
	enum of_gpio_flags flags;
	struct property *prop;
	int length;
	u32 value;
	int ret;

	if (!node)
		return -ENODEV;

	memset(data, 0, sizeof(*data));

	/* determine the number of brightness levels */
	prop = of_find_property(node, "brightness-levels", &length);
	if (!prop)
		return -EINVAL;

	data->max_brightness = length / sizeof(u32);

	/* read brightness levels from DT property */
	if (data->max_brightness > 0) {
		size_t size = sizeof(*data->levels) * data->max_brightness;

		data->levels = devm_kzalloc(dev, size, GFP_KERNEL);
		if (!data->levels)
			return -ENOMEM;

		ret = of_property_read_u32_array(node, "brightness-levels", data->levels, data->max_brightness);
		if (ret < 0)
			return ret;

		ret = of_property_read_u32(node, "default-brightness-level", &value);
		if (ret < 0)
			return ret;

		data->dft_brightness = value;
		data->max_brightness--;
	}

	data->enable_gpio = of_get_named_gpio_flags(node, "enable-gpios", 0, &flags);
	if (data->enable_gpio == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (gpio_is_valid(data->enable_gpio) && (flags & OF_GPIO_ACTIVE_LOW))
		data->enable_gpio_flags |= PWM_BACKLIGHT_GPIO_ACTIVE_LOW;

	return 0;
}

static struct of_device_id pwm_backlight_of_match[] = {
	{ .compatible = "pwm-backlight" },
	{ }
};
MODULE_DEVICE_TABLE(of, pwm_backlight_of_match);

static int pwm_backlight_probe(struct platform_device *pdev)
{
	struct platform_pwm_backlight_data *data = pdev->dev.platform_data;
	struct platform_pwm_backlight_data defdata;
	struct backlight_properties props;
	struct backlight_device *bl;
	struct pwm_bl_data *pbd;
	int ret;

	/* 先使用老方法获得data, 否则用DT, 数据保存在defdata中 */
	if (!data) {
		ret = pwm_backlight_parse_dt(&pdev->dev, &defdata);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to find platform data\n");
			return ret;
		}

		data = &defdata;
	}

	/* 分配pwm bl data */
	pbd = devm_kzalloc(&pdev->dev, sizeof(*pbd), GFP_KERNEL);
	if (!pbd) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	/* DT中有设置背光等级 */
	if (data->levels) {
		unsigned int i;
		/* scale 是最小背光值 */
		for (i = 0; i <= data->max_brightness; i++)
			if (data->levels[i] > pbd->scale)
				pbd->scale = data->levels[i];

		/* 设置背光可调范围 */
		pbd->levels = data->levels;
	} else {
		pbd->scale = data->max_brightness;
	}

	pbd->enable_gpio = data->enable_gpio;
	pbd->enable_gpio_flags = data->enable_gpio_flags;
	pbd->dev = &pdev->dev;
	pbd->enabled = false;

	/* PIN脚判断 */
	if (gpio_is_valid(pbd->enable_gpio)) {
		unsigned long flags;

		if (pbd->enable_gpio_flags & PWM_BACKLIGHT_GPIO_ACTIVE_LOW)
			flags = GPIOF_OUT_INIT_HIGH;
		else
			flags = GPIOF_OUT_INIT_LOW;

		ret = devm_gpio_request_one(&pdev->dev, pbd->enable_gpio, flags, "enable");
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request GPIO#%d: %d\n", pbd->enable_gpio, ret);
			goto err_alloc;
		}
	}

	/*
	 * 从指定设备(dev)的DTS节点中,获得对应的PWM句柄
	 * 可以通过con_id指定一个名称,或者会获取和该设备绑定的第一个PWM句柄
	 */
	pbd->pwm = devm_pwm_get(&pdev->dev, NULL);
	if (IS_ERR(pbd->pwm)) {
		dev_err(&pdev->dev, "unable to request PWM, trying legacy API\n");
		pbd->pwm = pwm_request(data->pwm_id, "pwm-backlight");
		if (IS_ERR(pbd->pwm)) {
			dev_err(&pdev->dev, "unable to request legacy PWM\n");
			ret = PTR_ERR(pbd->pwm);
			goto err_alloc;
		}
	}

	dev_dbg(&pdev->dev, "got pwm for backlight\n");

	/* 获取到PWM后就可以使用它来操作了 */
	pbd->period = pwm_get_period(pbd->pwm);
	pbd->lth_brightness = data->lth_brightness * (pbd->period / pbd->scale);

	/* 由于在代码drivers/video/backlight/backlight.c 对max_brightness有一个判断, 所以需要设置这个 */
	memset(&props, 0, sizeof(struct backlight_properties));
	props.max_brightness = data->max_brightness;

	dev_set_name(&pdev->dev, "my_backlight");
	bl = backlight_device_register(dev_name(&pdev->dev), &pdev->dev, pbd, &pwm_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err_alloc;
	}

	backlight_update_status(bl);
	platform_set_drvdata(pdev, bl);
	return 0;

err_alloc:
	if (data->exit)
		data->exit(&pdev->dev);
	return ret;
}

static int pwm_backlight_remove(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pbd = bl_get_data(bl);

	backlight_device_unregister(bl);
	pwm_backlight_power_off(pbd);

	if (pbd->exit)
		pbd->exit(&pdev->dev);

	return 0;
}

static void pwm_backlight_shutdown(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pbd = bl_get_data(bl);

	backlight_device_unregister(bl);
	pwm_backlight_power_off(pbd);

	if (pbd->exit)
		pbd->exit(&pdev->dev);
}

static struct platform_driver pwm_backlight_driver = {
	.driver		= {
		.name		= "pwm-backlight",
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(pwm_backlight_of_match),
	},
	.probe		= pwm_backlight_probe,
	.remove		= pwm_backlight_remove,
	.shutdown   = pwm_backlight_shutdown,
};

module_platform_driver(pwm_backlight_driver);

MODULE_DESCRIPTION("PWM based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-backlight");
