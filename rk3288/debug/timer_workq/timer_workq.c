#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#define EXPIRES_PERIOD	(5*HZ)

struct self_define_struct {
	struct timer_list mytimer;
	struct work_struct	mywork;
};

static int g_cnt = 1;

/* just for delete mytimer in the exit function */
struct timer_list *g_timer;

/* just for free the sds */
struct self_define_struct *g_sds;

static void mywork_handler(struct work_struct *work)
{
	struct self_define_struct *sds = container_of(work, struct self_define_struct, mywork);

	printk("do work[%d]\n", g_cnt++);

	sds->mytimer.expires = jiffies + EXPIRES_PERIOD;
	add_timer(&sds->mytimer);
}

static void timeout_handler(unsigned long tdata)
{
	struct self_define_struct *tmp = (struct self_define_struct *)tdata;

	printk("timeout[%d]...", g_cnt);

	/* do my work */
	schedule_work(&tmp->mywork);
}

static int timer_workq_init(void)
{
	struct self_define_struct *sds;
	sds = kzalloc(sizeof(struct self_define_struct), GFP_KERNEL);
	if (sds == NULL)
		return -ENOMEM;

	g_sds = sds;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* init mytimer */
	init_timer(&sds->mytimer);
	g_timer = &sds->mytimer;

	/* setup the timer */
	sds->mytimer.expires = jiffies + EXPIRES_PERIOD;
	sds->mytimer.function = timeout_handler;
	sds->mytimer.data = (unsigned long)sds;

	/* add to system */
	add_timer(&sds->mytimer);

	/* init mywork */
	INIT_WORK(&sds->mywork, mywork_handler);
}

static void timer_workq_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	del_timer(g_timer);
	kfree(g_sds);
}

module_init(timer_workq_init)
module_exit(timer_workq_exit)
MODULE_LICENSE("GPL");
