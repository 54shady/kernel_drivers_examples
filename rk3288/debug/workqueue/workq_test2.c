#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

struct self_define_struct *g_sds; /* just for free sds memory */
struct self_define_struct {
	struct work_struct	mywork;
	struct workqueue_struct *my_workqueue;
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

	sds->my_workqueue = create_singlethread_workqueue("myworkq_name");
	if (!sds->my_workqueue) {
		printk("create single thread workqueue error\n");
	}

	/* do my work */
	if (!work_pending(&sds->mywork))
		queue_work(sds->my_workqueue, &sds->mywork);

	return 0;
}

static void workq_test_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	destroy_workqueue(g_sds->my_workqueue);
	kfree(g_sds);
}

module_init(workq_test_init)
module_exit(workq_test_exit)
MODULE_LICENSE("GPL");
