/* test_chain_1.c ：1. 定义回调函数；2. 定义notifier_block；3. 向chain_0注册notifier_block；*/

#include "test_chain.h"

extern int register_test_notifier(struct notifier_block *nb);

/* 收到通知后的处理函数 */
int notify_event_handler(struct notifier_block *nb, unsigned long event, void *v)
{
	printk("<notify handler>Yep I got an notify event, let's handle it\n");
	switch(event){
		case TESTCHAIN_EVENT:
			printk("<notify handler>I got the chain event: test_chain_2 is on the way of init\n");
			break;

		default:
			break;
	}

	return NOTIFY_DONE;
}

/* define a notifier_block */
static struct notifier_block test_init_notifier = {
	.notifier_call = notify_event_handler,
};

static int __init test_chain_1_init(void)
{
	printk("I'm in test_chain_1, register test notifier\n");
	register_test_notifier(&test_init_notifier);
	return 0;
}

static void __exit test_chain_1_exit(void)
{
	printk("Goodbye to test_clain_l\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zeroway");

module_init(test_chain_1_init);
module_exit(test_chain_1_exit);
