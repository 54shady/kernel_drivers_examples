#include <linux/module.h>
#include <linux/init.h>
#include <linux/notifier.h>

extern int test_notifier_call_chain(unsigned long val, void *v);

/* 自定义通知事件号 */
enum self_define_event_number {
	evt_no0,
	evt_no1,
};

static int call_notifier(void)
{
	int err;
	printk("%s, Begin to notify:\n", __FUNCTION__);

	printk("%s, event %d emit\n", __FUNCTION__, evt_no0);
	err = test_notifier_call_chain(evt_no0, NULL);

	printk("%s, event %d emit\n", __FUNCTION__, evt_no1);
	err = test_notifier_call_chain(evt_no1, "new_test");
	if (err)
		printk("notifier_call_chain error\n");
	return err;
}

static void uncall_notifier(void)
{
	printk("Endnotify\n");
}

module_init(call_notifier);
module_exit(uncall_notifier);
MODULE_LICENSE("GPL");
