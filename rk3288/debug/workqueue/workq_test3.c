#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

struct self_define_struct *g_sds; /* just for free sds memory */
struct self_define_struct {
	struct delayed_work mywork;
};
static int g_cnt = 0;

static void mywork_handler(struct delayed_work *dwork)
{
	struct self_define_struct *sds = container_of(dwork, struct self_define_struct, mywork);

	printk("%s, %d[%d]\n", __FUNCTION__, __LINE__, g_cnt++);

	/* do work again */
    schedule_delayed_work(&sds->mywork, msecs_to_jiffies(500 + g_cnt * 100));
}

static int workq_test_init(void)
{
	struct self_define_struct *sds;

	sds = kzalloc(sizeof(struct self_define_struct), GFP_KERNEL);
	if (sds == NULL)
		return -ENOMEM;

	/* for free the memory */
	g_sds = sds;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* init mywork */
    INIT_DELAYED_WORK(&sds->mywork, mywork_handler);

	/* do my work */
    schedule_delayed_work(&sds->mywork, msecs_to_jiffies(500));

	return 0;
}

static void workq_test_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	cancel_delayed_work_sync(&g_sds->mywork);
	kfree(g_sds);
}

module_init(workq_test_init)
module_exit(workq_test_exit)
MODULE_LICENSE("GPL");
