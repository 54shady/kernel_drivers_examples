#include "AAAAA_chain.h"

extern int call_test_notifiers(unsigned long val, void *v);

/* CCCCC means the module name */
static int CCCCC_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	call_test_notifiers(TESTCHAIN_EVENT, "no_use");

	return 0;
}

static void CCCCC_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}
module_init(CCCCC_init);
module_exit(CCCCC_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("zeroway");
