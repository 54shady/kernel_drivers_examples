#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/ratelimit.h>

#define EXPIRES_PERIOD	(5*HZ)

struct self_define_struct {
	struct timer_list timer;
};

static int g_cnt = 0;
struct self_define_struct g_sds;
const char *important_code = "this is a secret";

/*
 * printk_ratelimit函数的速率控制根据两个文件来确定,分别是:
 * 1. /proc/sys/kernel/printk_ratelimit 限制的时间间隔,默认值是5
 * 2. /proc/sys/kernel/printk_ratelimit_burst
 * 时间间隔内的最大打印条数,默认值是10
 * 所以默认的打印速率是每5秒最多打印10条
 */
static void timeout_handler(unsigned long tdata)
{
	int i;
	const char *tmp = (const char *)tdata;

	for (i = 0; i < 100; i++)
		if(printk_ratelimit())
			printk("%s got data[%s] %d times\n", __FUNCTION__, tmp, g_cnt++);

	g_cnt = 0;
	g_sds.timer.expires = jiffies + EXPIRES_PERIOD;
	add_timer(&g_sds.timer);
}

static int mytimer_test_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* init timer */
	init_timer(&g_sds.timer);

	/* setup the timer */
	g_sds.timer.expires = jiffies + EXPIRES_PERIOD;
	g_sds.timer.function = timeout_handler;
	g_sds.timer.data = (unsigned long)important_code;

	/* add to system */
	add_timer(&g_sds.timer);

	return 0;
}

static void  mytimer_test_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	del_timer(&g_sds.timer);
}

module_init(mytimer_test_init)
module_exit(mytimer_test_exit)
