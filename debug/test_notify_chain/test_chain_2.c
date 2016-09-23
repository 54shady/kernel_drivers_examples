/* test_chain_2.c：发出通知链事件*/
#include "test_chain.h"

extern int call_test_notifiers(unsigned long val, void *v);

static int __init test_chain_2_init(void)
{
	printk("<notify sender>send out a notify event\n");
	call_test_notifiers(TESTCHAIN_EVENT, "no_use");

	return 0;
}

static void __exit test_chain_2_exit(void)
{
	printk("Goodbye to test_chain_2\n");
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("zeroway");

module_init(test_chain_2_init);
module_exit(test_chain_2_exit);
