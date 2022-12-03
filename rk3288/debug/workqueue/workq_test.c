#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

struct self_define_struct *g_sds; /* just for free sds memory */
struct self_define_struct {
	struct work_struct	mywork;
};

static void mywork_handler(struct work_struct *work)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
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
	INIT_WORK(&sds->mywork, mywork_handler);

	/* do my work */
	schedule_work(&sds->mywork);

	/* make it happend imediatly, the result is unexpected ? */
	flush_scheduled_work();

	return 0;
}

static void workq_test_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	kfree(g_sds);
}

module_init(workq_test_init)
module_exit(workq_test_exit)
MODULE_LICENSE("GPL");
