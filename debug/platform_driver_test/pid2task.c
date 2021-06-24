#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/mm.h>

/* Self define object struct */
struct skeleton_kobject {
	struct kobject kobj;
	int value;
	char buf[1024];
};
#define to_skeleton_kobject(x) container_of(x, struct skeleton_kobject, kobj)

/* a custom attribute that works just for a struct skeleton_kobject. */
struct skeleton_attribute {
	struct attribute attr;
	ssize_t (*show)(struct skeleton_kobject *foo, struct skeleton_attribute *attr, char *buf);
	ssize_t (*store)(struct skeleton_kobject *foo, struct skeleton_attribute *attr, const char *buf, size_t count);
};
#define to_foo_attr(x) container_of(x, struct skeleton_attribute, attr)

/* global variable */
static struct kset *g_skeleton_kset;
static struct skeleton_kobject *g_ske_obj;

/*
 * The default show function that must be passed to sysfs.  This will be
 * called by sysfs for whenever a show function is called by the user on a
 * sysfs file associated with the kobjects we have registered.  We need to
 * transpose back from a "default" kobject to our custom struct skeleton_kobject and
 * then call the show function for that specific object.
 */
static ssize_t skeleton_sysfs_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf)
{
	struct skeleton_attribute *attribute;
	struct skeleton_kobject *ske_obj;

	attribute = to_foo_attr(attr);
	ske_obj = to_skeleton_kobject(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(ske_obj, attribute, buf);
}

/* echo pid > /sys/kernel/KSET_NAME/KOBJECT_NAME/attr_one */
static ssize_t skeleton_sysfs_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	int pid;
	struct skeleton_attribute *attribute;
	struct skeleton_kobject *ske_obj;
	struct pid *pid_t;
	struct task_struct *task;

	/* get task_struct according the PID */
	sscanf(buf, "%d", &pid);
	pid_t = find_get_pid(pid);
	task = get_pid_task(pid_t, PIDTYPE_PID);

	pr_debug("pid:%d user:%lld gtime:%lld sys:%lld, %s (%d-%d-%d)",
		task->pid,
		task->utime,
		task->gtime,
		task->stime,
		task->comm,
		task->prio,
		task->static_prio,
		task->normal_prio);

	attribute = to_foo_attr(attr);
	ske_obj = to_skeleton_kobject(kobj);

	sprintf(ske_obj->buf, "pid:%d user:%lld gtime:%lld sys:%lld, %s (%d-%d-%d)",
		task->pid,
		task->utime,
		task->gtime,
		task->stime,
		task->comm,
		task->prio,
		task->static_prio,
		task->normal_prio);

	if (!attribute->store)
		return -EIO;

	return attribute->store(ske_obj, attribute, buf, len);
}

/* Our custom sysfs_ops that we will associate with our ktype later on */
static const struct sysfs_ops skeleton_sysfs_ops = {
	.show = skeleton_sysfs_show,
	.store = skeleton_sysfs_store,
};

/*
 * The release function for our object.  This is REQUIRED by the kernel to
 * have.  We free the memory held in our object here.
 *
 * NEVER try to get away with just a "blank" release function to try to be
 * smarter than the kernel.  Turns out, no one ever is...
 */
static void skeleton_release(struct kobject *kobj)
{
	struct skeleton_kobject *ske_obj;

	ske_obj = to_skeleton_kobject(kobj);
	kfree(ske_obj);
}

/* cat /sys/kernel/KSET_NAME/KOBJECT_NAME/attr_one */
static ssize_t attr_one_show(struct skeleton_kobject *ske_obj, struct skeleton_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%s\n", ske_obj->buf);
}

static ssize_t attr_one_store(struct skeleton_kobject *ske_obj, struct skeleton_attribute *attr,
			 const char *buf, size_t count)
{
	return count;
}

/* Sysfs attributes cannot be world-writable. */
static struct skeleton_attribute attribute_one =
	__ATTR(attr_one, 0664, attr_one_show, attr_one_store);

/*
 * More complex function where we determine which variable is being accessed by
 * looking at the attribute for the "baz" and "bar" files.
 */
/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *skeleton_default_attrs[] = {
	&attribute_one.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * Our own ktype for our kobjects.  Here we specify our sysfs ops, the
 * release function, and the set of default attributes we want created
 * whenever a kobject of this type is registered with the kernel.
 */
static struct kobj_type skeleton_ktype = {
	.sysfs_ops = &skeleton_sysfs_ops,
	.release = skeleton_release,
	.default_attrs = skeleton_default_attrs,
};

static struct skeleton_kobject *create_kobject_skeleton(const char *kobj_name)
{
	struct skeleton_kobject *ske_obj;
	int retval;

	/* allocate the memory for the whole object */
	ske_obj = kzalloc(sizeof(struct skeleton_kobject), GFP_KERNEL);
	if (!ske_obj)
		return NULL;

	/*
	 * As we have a kset for this kobject, we need to set it before calling
	 * the kobject core.
	 */
	ske_obj->kobj.kset = g_skeleton_kset;

	/*
	 * Initialize and add the kobject to the kernel.  All the default files
	 * will be created here.  As we have already specified a kset for this
	 * kobject, we don't have to set a parent for the kobject, the kobject
	 * will be placed beneath that kset automatically.
	 */
	retval = kobject_init_and_add(&ske_obj->kobj, &skeleton_ktype, NULL, "%s", kobj_name);
	if (retval) {
		kobject_put(&ske_obj->kobj);
		return NULL;
	}

	/*
	 * We are always responsible for sending the uevent that the kobject
	 * was added to the system.
	 */
	kobject_uevent(&ske_obj->kobj, KOBJ_ADD);

	return ske_obj;
}

static void destroy_skeleton_kobject(struct skeleton_kobject *ske_obj)
{
	kobject_put(&ske_obj->kobj);
}

static int example_init(void)
{
	/*
	 * Create a kset with the name of "KSET_NAME",
	 * located under /sys/kernel/
	 */
	g_skeleton_kset = kset_create_and_add("KSET_NAME", NULL, kernel_kobj);
	if (!g_skeleton_kset)
		return -ENOMEM;

	/* Create a object and register with our kset */
	g_ske_obj = create_kobject_skeleton("KOBJECT_NAME");
	if (!g_ske_obj)
	{
		kset_unregister(g_skeleton_kset);
		return -EINVAL;
	}

	return 0;
}

static void example_exit(void)
{
	destroy_skeleton_kobject(g_ske_obj);
	kset_unregister(g_skeleton_kset);
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("zeroway <M_O_Bz@163.com>");
