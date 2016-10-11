#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>

#define EXPIRES_PERIOD	(5*HZ)

struct self_define_struct {
	struct timer_list timer;
};

struct self_define_struct g_sds;
static int g_cnt = 0;

static void timeout_handler(unsigned long tdata)
{
	printk("%s being called %d times\n", __FUNCTION__, g_cnt++);
	g_sds.timer.expires = jiffies + EXPIRES_PERIOD;
	add_timer(&g_sds.timer);
}

static int mytimer_test_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	init_timer(&g_sds.timer);
	g_sds.timer.expires = jiffies + EXPIRES_PERIOD;
	g_sds.timer.function = timeout_handler;
	add_timer(&g_sds.timer);
}



static void  mytimer_test_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	del_timer(&g_sds.timer);
}

module_init(mytimer_test_init)
module_exit(mytimer_test_exit)
