#include <linux/module.h>
#include <linux/init.h>

static int __init hello_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void hello_exit (void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
