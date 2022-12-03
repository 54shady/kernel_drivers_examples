#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/debugfs.h>

#define TEST_NODE_NAME "showtask"
#define TEST_DIR_NAME "tasks"

#ifdef PROC_FS_TEST
static struct proc_dir_entry *entry;
#endif

static loff_t *gpos = NULL;
struct dentry *d_dir_name;

/*
 *The pos passed to start() will always be either zero, or
 *the most recent pos used in the previous session.
 */
static void *myseq_start(struct seq_file *m, loff_t *pos)
{
	int i;
	struct task_struct *task;
	gpos = pos;

	/*
	 * 传入start的参数pos要么是0,要么是上一次会话的pos值
	 * start从position 0开始(pos = 0),意味着从文件开始
	 * 可以在此时打印文件头
	 */
	if (*pos == 0) {
		seq_printf(m, "Current all the processes in system:\n"
				"%-24s%-5s\n", "name", "pid");
#ifdef PROC_FS_TEST
		task = &init_task;
#else
		task = m->private;
#endif
		pr_debug("start first:%-24s%-5d\n", task->comm, task->pid);
		return task;
	} else {
		/* linux中task被挂入到环形链表中 */
#ifdef PROC_FS_TEST
		for (i = 0, task = &init_task; i < *pos; i++)
#else
		for (i = 0, task = m->private; i < *pos; i++)
#endif
			task = next_task(task);
		BUG_ON(i != *pos);

		/* 读完所有内容, 结束 */
#ifdef PROC_FS_TEST
		if (task == &init_task)
#else
		if (task == m->private)
#endif
			return NULL;

		pr_debug("start return %-24s%-5d\n", task->comm, task->pid);
		return task;
	}
}

/*
 * 1. 每执行一次都要步进一次position
 * 2. 将返回值传递给show作为第二个参数
 * 参数p是当前会话中start的返回值的下一个内容
 */
static void *myseq_next(struct seq_file *m, void *p, loff_t *pos)
{
	struct task_struct * task = (struct task_struct *)p;

	task = next_task(task);

	/* step forward, must update position before return NULL */
	++(*pos);
#ifdef PROC_FS_TEST
	if ((*pos != 0) && (task == &init_task))
#else
	if ((*pos != 0) && (task == m->private))
#endif
		return NULL;

	return task;
}

/* 执行了stop说明结束一次会话,下次绘画从这个pos重新开始 */
static void myseq_stop(struct seq_file *m, void *p)
{
	struct task_struct * task = (struct task_struct *)p;
	if (task != NULL)
		pr_debug("stop->:%-24s%-5d, pos: %lld\n", task->comm, task->pid, *gpos);
}

/*
 * 第二个参数是当前会话中start或next的返回值
 * 第一次是start的返回值
 * 使用过程中是next或是start的返回值
 */
static int myseq_show(struct seq_file *m, void *p)
{
	struct task_struct * task = (struct task_struct *)p;
	seq_printf(m, "%-24s%-5d\n", task->comm, task->pid);

	return 0;
}

static struct seq_operations myseq_sops = {
	.start = myseq_start,
	.next  = myseq_next,
	.stop  = myseq_stop,
	.show  = myseq_show
};

#ifdef PROC_FS_TEST
static int myseq_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &myseq_sops);
}

static const struct proc_ops proc_fops = {
	.proc_open = myseq_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};
#endif

#ifndef PROC_FS_TEST
DEFINE_SEQ_ATTRIBUTE(myseq);
#endif
static int seq_test_init(void)
{
#ifdef PROC_FS_TEST
	entry = proc_create(TEST_NODE_NAME, 0444, NULL, &proc_fops);
	if (!entry)
		printk(KERN_EMERG "proc_create error.\n");
#else
	d_dir_name = debugfs_create_dir(TEST_DIR_NAME, NULL);
	/*
	 * 第四个参数就是赋值给s->private的
	 * init_task是内核中的全局变量,初始化为第一个进程swapper或是idel
	 * 内核中将所有进程都挂载在环形链表中
	 * 这里将init_task作为参数传递给seq_file->private
	 */
	debugfs_create_file(TEST_NODE_NAME, 0444, d_dir_name, &init_task,
			&myseq_fops);
#endif
	return 0;
}

static void seq_test_exit(void)
{
#ifdef PROC_FS_TEST
	remove_proc_entry(TEST_NODE_NAME, NULL);
#else
	debugfs_remove(d_dir_name);
#endif
}

module_init(seq_test_init);
module_exit(seq_test_exit);
MODULE_LICENSE("GPL");
