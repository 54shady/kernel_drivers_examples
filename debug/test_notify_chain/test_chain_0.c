/* test_chain_0.c ：0. 申明一个通知链；1. 向内核注册通知链；2. 定义事件； 3. 导出符号，因而必需最后退出*/
#include "test_chain.h"

/* 自定义一个通知链头 */
static RAW_NOTIFIER_HEAD(test_chain);

/* 定义调用通知链的函数 */
static int call_test_notifiers(unsigned long val, void *v)
{
	return raw_notifier_call_chain(&test_chain, val, v);
}
EXPORT_SYMBOL(call_test_notifiers);

/* 定义注册自己通知链的函数  */
static int register_test_notifier(struct notifier_block *nb)
{
	int err = 0;
	err = raw_notifier_chain_register(&test_chain, nb);

	return err;
}
EXPORT_SYMBOL(register_test_notifier);

static int __init test_chain_0_init(void)
{
	printk(KERN_DEBUG "I'm in test_chain_0\n");

	return 0;
}

static void __exit test_chain_0_exit(void)
{
	printk(KERN_DEBUG "Goodbye to test_chain_0\n");
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("zeroway");

module_init(test_chain_0_init);
module_exit(test_chain_0_exit);
