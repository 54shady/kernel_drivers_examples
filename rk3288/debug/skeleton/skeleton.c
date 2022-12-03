#include <linux/module.h>
#include <linux/init.h>

static int VENDOR_MODULE_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void VENDOR_MODULE_exit (void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

module_init(VENDOR_MODULE_init);
module_exit(VENDOR_MODULE_exit);
MODULE_LICENSE("GPL");
