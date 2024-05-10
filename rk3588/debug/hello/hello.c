#include <linux/module.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/delay.h>

static void foo(void)
{
	msleep(1000);
}

static int __init hello_init(void)
{
    ktime_t start_time, end_time;
    s64 time_delta;

    start_time = ktime_get();

	/* function logic here */
	foo();

    end_time = ktime_get();

    time_delta = ktime_to_ns(ktime_sub(end_time, start_time));

    printk(KERN_INFO "Time taken by function: %lld ns\n", time_delta);

	return 0;
}

static void hello_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
