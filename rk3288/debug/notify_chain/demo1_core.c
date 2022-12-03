/*
 * 1. 申明一个通知链
 * 2. 向内核注册通知链
 * 3. 定义事件
 * 4. 导出符号,因而必需最后退出
 */
#include "AAAAA_chain.h"

/* 自定义一个通知链头 */
static RAW_NOTIFIER_HEAD(AAAAA_nh);

/* 定义调用通知链的函数 */
static int call_test_notifiers(unsigned long val, void *v)
{
	return raw_notifier_call_chain(&AAAAA_nh, val, v);
}
EXPORT_SYMBOL(call_test_notifiers);

/* 定义注册自己通知链的函数  */
static int register_BBBBB_notifier(struct notifier_block *nb)
{
	int err = 0;
	err = raw_notifier_chain_register(&AAAAA_nh, nb);

	return err;
}
EXPORT_SYMBOL(register_BBBBB_notifier);

static int unregister_BBBBB_notifier(struct notifier_block *nb)
{
	int err = 0;
	err = raw_notifier_chain_unregister(&AAAAA_nh, nb);

	return err;
}
EXPORT_SYMBOL(unregister_BBBBB_notifier);

/* AAAAA means the module name */
static int AAAAA_core_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void AAAAA_core_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}
module_init(AAAAA_core_init);
module_exit(AAAAA_core_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("zeroway");
