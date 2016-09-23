#include <linux/module.h>
#include <linux/init.h>
#include <linux/notifier.h>
extern int test_notifier_call_chain(unsigned long val, void*v);

static int __init call_notifier(void)
{
	int err;
	printk("Begin to notify:\n");

	printk("==============================\n");
	err = test_notifier_call_chain(1, NULL);
	err = test_notifier_call_chain(2, "new_test");
	printk("==============================\n");
	if (err)
		printk("notifier_call_chain error\n");
	return err;
}

static void __exit uncall_notifier(void)
{
	printk("Endnotify\n");
}

module_init(call_notifier);
module_exit(uncall_notifier);
MODULE_LICENSE("GPL");
