#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>

static struct workqueue_struct *my_workqueue;
static DECLARE_WORK(my_work, NULL);

static void my_work_function(struct work_struct *work) {
    pr_info("Executing workqueue function!\n");

    /* Reschedule itself */
    queue_work(my_workqueue, work);
}

static int __init my_init_module(void) {
    pr_info("Initializing module\n");

    /* Create the workqueue */
    my_workqueue = create_workqueue("my_workqueue");

    if (!my_workqueue) {
        pr_err("Failed to create workqueue\n");
        return -ENOMEM;
    }

    INIT_WORK(&my_work, my_work_function);

    /* Queue the work */
    queue_work(my_workqueue, &my_work);

    return 0;
}

static void __exit my_exit_module(void) {
    pr_info("Exiting module\n");

    /* Cleanup */
    cancel_work_sync(&my_work);
    destroy_workqueue(my_workqueue);
}

module_init(my_init_module);
module_exit(my_exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux work queue example module");
MODULE_VERSION("1.0");
