#include <linux/module.h>
#include <linux/init.h>
#include <linux/notifier.h>

extern int unregister_test_notifier(struct notifier_block*);
extern int register_test_notifier(struct notifier_block *nb);

static int test_event1(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("In %s: evt_no = %ld\n", __FUNCTION__, event);

	if (NULL != ptr)
		printk("%s get %s\n", __FUNCTION__, (char *)ptr);

	return 0;
}

static int test_event2(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("In %s: evt_no = %ld\n", __FUNCTION__, event);

	if (NULL != ptr)
		printk("%s get %s\n", __FUNCTION__, (char *)ptr);

	return 0;
}

static int test_event3(struct notifier_block *this, unsigned long event, void *ptr)
{
	printk("In %s: evt_no = %ld\n", __FUNCTION__, event);

	if (NULL != ptr)
		printk("%s get %s\n", __FUNCTION__, (char *)ptr);

	return 0;
}

static struct notifier_block test_notifier1 =
{
	.notifier_call = test_event1,
};

static struct notifier_block test_notifier2 =
{
	.notifier_call = test_event2,
};

static struct notifier_block test_notifier3 =
{
	.notifier_call = test_event3,
};

static int reg_notifier(void)
{
	int err;
	printk("Begin to register:\n");

	err = register_test_notifier(&test_notifier1);
	if (err)
	{
		printk("register test_notifier1 error\n");
		return -1;
	}
	printk("register test_notifier1 completed\n");

	err = register_test_notifier(&test_notifier2);
	if (err)
	{
		printk("register test_notifier2 error\n");
		return -1;
	}
	printk("register test_notifier2 completed\n");

	err = register_test_notifier(&test_notifier3);
	if (err)
	{
		printk("register test_notifier3 error\n");
		return -1;
	}
	printk("register test_notifier3 completed\n");

	return err;
}

static void unreg_notifier(void)
{
	printk("Begin to unregister\n");
	unregister_test_notifier(&test_notifier1);
	unregister_test_notifier(&test_notifier2);
	unregister_test_notifier(&test_notifier3);
	printk("Unregister finished\n");
}

module_init(reg_notifier);
module_exit(unreg_notifier);
MODULE_LICENSE("GPL");
