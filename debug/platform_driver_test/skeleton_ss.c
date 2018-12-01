#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

#include "skeleton_ss.h"

/* cat /sys/skeleton_pins/first_attr */
/* cat /sys/skeleton_pins/second_attr */
ssize_t ops_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int size = 0;
	struct skeleton_chip *chip;

	chip = container_of(kobj, struct skeleton_chip, kobj);
	printk("attr->name:%s \n",attr->name);
	printk("%s, 0x%p:%s\n", __FUNCTION__, chip, chip->name);
	size = sprintf(buf, "%s value is %d\n", chip->pd[chip->pin_name_index]->name, gpio_get_value(chip->pd[chip->pin_name_index]->gpio_pin));

	return size;
}

/* echo <pin_index> <val> > /sys/skeleton_pins/first_attr */
/* echo <pin_index> <val> > /sys/skeleton_pins/second_attr */
ssize_t ops_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	struct skeleton_chip *chip;

	chip = container_of(kobj, struct skeleton_chip, kobj);
	sscanf(buf, "%d %d", &chip->pin_name_index, &chip->value);
	gpio_set_value(chip->pd[chip->pin_name_index]->gpio_pin, chip->value);

	return count;
}

struct sysfs_ops sysops = {
	.show = ops_show,
	.store = ops_store,
};

void kobj_release(struct kobject *kobj)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

struct attribute first_attr = {
	.name = "first_attr",
	.mode = S_IRWXUGO,
};

struct attribute second_attr = {
	.name = "second_attr",
	.mode = S_IRUGO,
};

static struct attribute *def_attrs[] = {
	&first_attr,
	&second_attr,
	NULL,
};

struct kobj_type ktype = {
	.release       = kobj_release,
	.sysfs_ops     = &sysops,
	.default_attrs = def_attrs,
};

static int skeleton_ss_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int i;
	enum of_gpio_flags flag;
	struct skeleton_chip *chip;

	int gpio_nr;
	int ret;

	printk("%s, %d\n", __FUNCTION__, __LINE__);
	gpio_nr = sizeof(dts_name) / sizeof(dts_name[0]);

	/* alloc for chip point */
	chip = devm_kzalloc(&pdev->dev, sizeof(struct skeleton_chip), GFP_KERNEL);
	if (!chip)
	{
		printk("no memory\n");
		ret = -ENOMEM;
	}

	/* alloc memory for array head */
	chip->pd = devm_kzalloc(&pdev->dev, sizeof(struct pin_desc *) * gpio_nr, GFP_KERNEL);
	if (!chip->pd)
	{
		printk("no memory\n");
		ret = -ENOMEM;
	}

	strcpy(chip->name, "Skeleton Chip");
	printk("%s, 0x%p:%s\n", __FUNCTION__, chip, chip->name);

	/* defualt pin index */
	chip->pin_name_index = 0;

	/* alloc memory for every point in the array */
	for (i = 0; i < gpio_nr; i++)
	{
		chip->pd[i] = devm_kzalloc(&pdev->dev, sizeof(struct pin_desc), GFP_KERNEL);
		if (!chip->pd[i])
		{
			printk("no memory\n");
			ret = -ENOMEM;
		}
	}

	/* set pdata */
	dev_set_drvdata(&pdev->dev, (void *)chip);

	/* make the pin desc names */
	for (i = 0; i < gpio_nr; i++)
		strcpy(chip->pd[i]->name, dts_name[i]);

	/* make all pins active */
	for (i = 0; i < gpio_nr; i++)
	{
		chip->pd[i]->gpio_pin = of_get_named_gpio_flags(node, chip->pd[i]->name, 0, &flag);

		/* ROCKCHIP gpio active high and low is reverse */
		chip->pd[i]->gpio_active_flag = !flag;

		ret = devm_gpio_request(&pdev->dev, chip->pd[i]->gpio_pin, chip->pd[i]->name);
		if (ret)
		{
			printk("%s error\n", chip->pd[i]->name);
		}
		else
		{
			ret = gpio_direction_output(chip->pd[i]->gpio_pin, chip->pd[i]->gpio_active_flag);
#ifdef READ_VALUE_BACK
			printk("%s[%d]\n", chip->pd[i]->name, gpio_get_value(chip->pd[i]->gpio_pin));
#endif
		}
	}

	memset(&chip->kobj, 0, sizeof(struct kobject));
	kobject_init_and_add(&chip->kobj, &ktype, NULL, "skeleton_pins");

	return 0;
}

static int skeleton_ss_remove(struct platform_device *pdev)
{
	struct skeleton_chip *chip;

	chip = dev_get_drvdata(&pdev->dev);
	printk("%s, 0x%p:%s\n", __FUNCTION__, chip, chip->name);
	kobject_del(&chip->kobj);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static const struct of_device_id skeleton_ss_dt_ids[] = {
	{.compatible = "skeleton, compatible",},
	{},
};

int skeleton_ss_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

int skeleton_ss_resume(struct platform_device *pdev)
{
	int i;
	int gpio_nr;
	struct skeleton_chip *chip;

	gpio_nr = sizeof(dts_name) / sizeof(dts_name[0]);
	chip = dev_get_drvdata(&pdev->dev);

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* active all */
	for (i = 0; i < gpio_nr; i++)
		gpio_set_value(chip->pd[i]->gpio_pin, chip->pd[i]->gpio_active_flag);

	return 0;
}

static struct platform_driver skeleton_ss = {
	.driver		= {
		.name	= "skeleton example",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(skeleton_ss_dt_ids),
	},
	.probe		= skeleton_ss_probe,
	.remove 	= skeleton_ss_remove,
	.suspend    = skeleton_ss_suspend,
	.resume     = skeleton_ss_resume,
};

module_platform_driver(skeleton_ss);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zeroway <M_O_Bz@163.com>");
MODULE_DESCRIPTION("skeleton");
