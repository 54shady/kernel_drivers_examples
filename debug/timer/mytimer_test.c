#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>

#define EXPIRES_PERIOD	(HZ / 2)

struct self_define_struct {
	struct timer_list timer;
};

static int g_cnt = 1;
struct self_define_struct g_sds;

const char *important_code = "this is a secret";

static void timeout_handler(unsigned long tdata)
{
	const char *tmp = (const char *)tdata;

	printk("%s got data[%s] %d times\n", __FUNCTION__, tmp, g_cnt++);
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
}

static void mytimer_test_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	del_timer(&g_sds.timer);
}

module_init(mytimer_test_init)
module_exit(mytimer_test_exit)
