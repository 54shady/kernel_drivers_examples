#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define KTIME_DEMO
#ifdef KTIME_DEMO
#include <linux/ktime.h>
#endif

static int cpu_index = 0;

struct self_define_struct {
	struct work_struct	mywork;
	struct workqueue_struct *my_workqueue;
};
struct self_define_struct sds;

static double waste_time(long n)
{
	double res = 0;
	long i = 0;
	while (i < n * 2000)
	{
		i++;
		res += int_sqrt(i);
	}
	return res;
}

static void work_function(struct work_struct *work)
{
	int cpu = smp_processor_id();

#ifdef KTIME_DEMO
    ktime_t start_time, end_time;
    s64 time_delta;
#endif

    printk(KERN_INFO "Work handler function executed on CPU %d\n", cpu);

#ifdef KTIME_DEMO
    start_time = ktime_get();
#endif

	/* time consuming work */
	waste_time(1);
	/* msleep(1000); */

#ifdef KTIME_DEMO
    end_time = ktime_get();
    time_delta = ktime_to_ns(ktime_sub(end_time, start_time));
    printk(KERN_INFO "Time taken by function: %lld ns\n", time_delta);
#endif

	schedule_work_on(cpu_index, work);
}

static int workq_test_init(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* init mywork */
	INIT_WORK(&sds.mywork, work_function);

	sds.my_workqueue = alloc_workqueue("myworkq_name",
		WQ_HIGHPRI | WQ_MEM_RECLAIM | WQ_SYSFS, 0);
	/* sds.my_workqueue = create_singlethread_workqueue("myworkq_name"); */
	if (!sds.my_workqueue) {
		printk("create single thread workqueue error\n");
	}

	schedule_work_on(cpu_index, &(sds.mywork));

	return 0;
}

static void workq_test_exit(void)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	/* must cancel work before destroy, or will cause crash */
	cancel_work_sync(&sds.mywork);
	destroy_workqueue(sds.my_workqueue);
}

/* insmod schedule_work_on_cpu.ko cpu_index=3 */
module_param(cpu_index, int, S_IRUGO);
MODULE_PARM_DESC(cpu_index, "CPU Index to Run");

module_init(workq_test_init)
module_exit(workq_test_exit)
MODULE_LICENSE("GPL");
