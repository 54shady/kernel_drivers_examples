#include "AAAAA_chain.h"

extern int register_BBBBB_notifier(struct notifier_block *nb);
extern int unregister_BBBBB_notifier(struct notifier_block *nb);

/* 收到通知后的处理函数 */
int BBBBB_event_handler(struct notifier_block *nb, unsigned long event, void *v)
{
	switch(event) {
		case TESTCHAIN_EVENT:
			printk("Register handling the event(%d)\n", event);
			break;
		default:
			break;
	}

	return NOTIFY_DONE;
}

/* define a notifier_block */
static struct notifier_block BBBBB_nb = {
	.notifier_call = BBBBB_event_handler,
};

/* BBBBB means the module name */
static int BBBBB_register_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	register_BBBBB_notifier(&BBBBB_nb);
	return 0;
}

static void BBBBB_register_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	unregister_BBBBB_notifier(&BBBBB_nb);
}
module_init(BBBBB_register_init);
module_exit(BBBBB_register_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zeroway");
