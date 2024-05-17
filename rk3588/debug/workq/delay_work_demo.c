#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>

struct workqueue_struct *our_workqueue;
DECLARE_DELAYED_WORK(our_work, NULL);

void our_work_fn(struct work_struct *work) {
    pr_info("Executing workqueue function!\n");

    /* Reschedule itself in the workqueue */
    queue_delayed_work(our_workqueue, &our_work, msecs_to_jiffies(1000));
}

static int __init workqueue_module_init(void) {
    pr_info("Initializing workqueue module\n");

    /* Initialize workqueue */
    our_workqueue = create_singlethread_workqueue("our_workqueue");

    if (!our_workqueue) {
        pr_err("Failed to create workqueue\n");
        return -ENOMEM;
    }

    /* Initialize work */
    INIT_DELAYED_WORK(&our_work, our_work_fn);

    /* Queue up the work */
    queue_delayed_work(our_workqueue, &our_work, msecs_to_jiffies(1000));

    return 0;
}

static void __exit workqueue_module_exit(void) {
    pr_info("Exiting workqueue module\n");

    /* Cleanup workqueue */
    cancel_delayed_work_sync(&our_work);
    destroy_workqueue(our_workqueue);
}

module_init(workqueue_module_init);
module_exit(workqueue_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux workqueue example module");
MODULE_VERSION("1.0");
