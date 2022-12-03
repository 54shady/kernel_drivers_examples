#include <linux/module.h>
#include <linux/init.h>

struct test {
	char name;
	int age;
};

static int __init hello_init(void)
{
	struct test *t = NULL;

	printk("%s, %d, tmp = %s\n", __FUNCTION__, __LINE__, t->name);
	return 0;
}

static void hello_exit (void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
