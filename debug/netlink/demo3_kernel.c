#include <net/genetlink.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/kobject.h>

#define TEST_GENL_FAMILY_NAME "my-test-family"
#define TEST_GENL_MCAST_GROUP_NAME "my-test-group"
#define TEST_GENL_MSG_FROM_KERNEL   "Hello from kernel space!!!"

/*
 * handler
 * message handling code goes here; return 0 on success, negative
 * values on failure
 */
static int doc_exmpl_echo(struct sk_buff *skb, struct genl_info *info);

/* netlink attributes 可以通过枚举索引找到对应的类型，用户空间应用程序要传递这样的信息 */
enum {
	DOC_EXMPL_A_UNSPEC,
	DOC_EXMPL_A_MSG,
	__DOC_EXMPL_A_MAX,
};
#define DOC_EXMPL_A_MAX (__DOC_EXMPL_A_MAX - 1)

/* attribute policy */
static struct nla_policy doc_exmpl_genl_policy[DOC_EXMPL_A_MAX + 1] = {
	[DOC_EXMPL_A_MSG] = { .type = NLA_NUL_STRING },
};

/* commands 定义命令类型，用户空间以此来表明需要执行的命令 */
enum {
	DOC_EXMPL_C_UNSPEC,
	DOC_EXMPL_C_ECHO,
	__DOC_EXMPL_C_MAX,
};
#define DOC_EXMPL_C_MAX (__DOC_EXMPL_C_MAX - 1)

/* family definition */
static struct genl_family doc_exmpl_genl_family = {
	.id = GENL_ID_GENERATE,   /* request a new channel number, assigned by kernel, NOT driver specific */
	.hdrsize = 0,
	.name = "DOC_EXMPL",
	.version = 1,
	.maxattr = DOC_EXMPL_A_MAX,
};

/* operation definition 将命令command echo和具体的handler对应起来 */
static struct genl_ops doc_exmpl_genl_ops_echo = {
	.cmd = DOC_EXMPL_C_ECHO,
	.flags = 0,
	.policy = doc_exmpl_genl_policy,
	.doit = doc_exmpl_echo,
	.dumpit = NULL,
};

static struct genl_multicast_group doc_exmpl_genl_mcgrp = {
	.name = "DOC_EXMPL_GRP",
};

/* 需要在其他地方主动调用这个函数发送广播 */
static int test_netlink_send(void)
{
	struct sk_buff *skb = NULL;
	void *msg_header = NULL;
	int size;
	int rc;

	/* allocate memory */
	size = nla_total_size(strlen(TEST_GENL_MSG_FROM_KERNEL) + 1) + nla_total_size(0);

	skb = genlmsg_new(size, GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

	/* add the genetlink message header */
	msg_header = genlmsg_put(skb, 0, 0,
			&doc_exmpl_genl_family, 0, DOC_EXMPL_C_ECHO);
	if (!msg_header)
	{
		rc = -ENOMEM;
		goto err_out;
	}

	/* add a DOC_EXMPL_A_MSG attribute */
	rc = nla_put_string(skb, DOC_EXMPL_A_MSG, TEST_GENL_MSG_FROM_KERNEL);
	if (rc != 0)
		goto err_out;

	/* finalize the message */
	genlmsg_end(skb, msg_header);

	/* multicast is send a message to a logical group */
	rc = genlmsg_multicast(skb, 0, doc_exmpl_genl_mcgrp.id, GFP_KERNEL);
	if (rc != 0 && rc != -ESRCH)
	{
		/* if NO one is waitting the message in user space,
		 * genlmsg_multicast return -ESRCH
		 */
		printk("genlmsg_multicast to user failed, return %d\n", rc);

		/*
		 * attention:
		 * If you NOT call genlmsg_unicast/genlmsg_multicast and error occurs,
		 * call nlmsg_free(skb).
		 * But if you call genlmsg_unicast/genlmsg_multicast, NO need to call
		 * nlmsg_free(skb). If NOT, kernel crash.
		 */
		return rc;
	}

	printk("genlmsg_multicast Success\n");

	/*
	 * Attention:
	 * Should NOT call nlmsg_free(skb) here. If NOT, kernel crash!!!
	 */
	return 0;

err_out:
	if (skb)
		nlmsg_free(skb);
	return rc;
}

/* echo 1 > /sys/kernel/genl_test/genl_trigger */
static ssize_t genl_trigger_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	test_netlink_send();
	return count;
}

static DEVICE_ATTR(genl_trigger, S_IRUGO | S_IWUSR, NULL, genl_trigger_store);

static struct attribute *genl_attrs[] = {
	&dev_attr_genl_trigger.attr,
	NULL,
};

static const struct attribute_group genl_group = {
	.attrs = genl_attrs,
};

static inline int genl_msg_prepare_usr_msg(u8 cmd, size_t size, pid_t pid, struct sk_buff **skbp)
{
	struct sk_buff *skb;

	/* create a new netlink msg */
	skb = genlmsg_new(size, GFP_KERNEL);
	if (skb == NULL) {
		return -ENOMEM;
	}

	/* Add a new netlink message to an skb */
	genlmsg_put(skb, pid, 0, &doc_exmpl_genl_family, 0, cmd);

	*skbp = skb;
	return 0;
}

static inline int genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
{
	int rc;

	/* add a netlink attribute to a socket buffer */
	if ((rc = nla_put(skb, type, len, data)) != 0) {
		return rc;
	}
	return 0;
}

/*
 * genl_msg_send_to_user - 通过generic netlink发送数据到netlink
 *
 * @data: 发送数据缓存
 * @len:  数据长度 单位：byte
 * @pid:  发送到的客户端pid
 *
 * return:
 *    0:       成功
 *    -1:      失败
 */
int genl_msg_send_to_user(void *data, int len, pid_t pid)
{
	struct sk_buff *skb;
	size_t size;
	void *head;
	int rc;

	size = nla_total_size(len); /* total length of attribute including padding */

	rc = genl_msg_prepare_usr_msg(DOC_EXMPL_C_ECHO, size, pid, &skb);
	if (rc) {
		return rc;
	}

	rc = genl_msg_mk_usr_msg(skb, DOC_EXMPL_A_MSG, data, len);
	if (rc) {
		kfree_skb(skb);
		return rc;
	}

	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skb)));

	rc = genlmsg_end(skb, head);
	if (rc < 0) {
		kfree_skb(skb);
		return rc;
	}

	rc = genlmsg_unicast(&init_net, skb, pid);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

/* echo command handler, 命令处理函数,当接收到user program发出的命令后,这个函数会被内核调用 */
static int doc_exmpl_echo(struct sk_buff *skb, struct genl_info *info)
{
	/* message handling code goes here; return 0 on success, negative values on failure */
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nlh;
	char *str;
	int ret;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nlh = genlmsg_data(genlhdr);
	str = nla_data(nlh);
	printk("doc_exmpl_echo get: %s\n", str);

	ret = genl_msg_send_to_user(TEST_GENL_MSG_FROM_KERNEL,
			strlen(TEST_GENL_MSG_FROM_KERNEL) + 1,  nlhdr->nlmsg_pid);

	return ret;
}

static int genetlink_init(void)
{
	int rc;
	struct kobject *genl_kobj;

	/**
	 * 1. Registering A Family
	 * This function doesn't exist past linux 3.12
	 */
	rc = genl_register_family(&doc_exmpl_genl_family);
	if (rc != 0)
		goto err_out1;

	rc = genl_register_ops(&doc_exmpl_genl_family, &doc_exmpl_genl_ops_echo);
	if (rc != 0)
		goto err_out2;

	/*
	 * for multicast
	 */
	rc = genl_register_mc_group(&doc_exmpl_genl_family, &doc_exmpl_genl_mcgrp);
	if (rc != 0)
		goto err_out3;

	/* /sys/kerne/genl_test */
	genl_kobj = kobject_create_and_add("genl_test", kernel_kobj);
	rc = sysfs_create_group(genl_kobj, &genl_group);
	if (rc) {
		printk("failed to create sysfs device attributes\n");
		return -1;
	}

	printk("doc_exmpl_genl_mcgrp.id=%d\n", doc_exmpl_genl_mcgrp.id);
	printk("genetlink_init OK\n");

	return 0;

err_out3:
	genl_unregister_ops(&doc_exmpl_genl_family, &doc_exmpl_genl_ops_echo);
err_out2:
	genl_unregister_family(&doc_exmpl_genl_family);
err_out1:
	printk("Error occured while inserting generic netlink example module\n");
	return rc;
}

static void genetlink_exit(void)
{
	printk("Generic Netlink Example Module unloaded.\n");

	genl_unregister_mc_group(&doc_exmpl_genl_family, &doc_exmpl_genl_mcgrp);
	genl_unregister_ops(&doc_exmpl_genl_family, &doc_exmpl_genl_ops_echo);
	genl_unregister_family(&doc_exmpl_genl_family);
}
module_init(genetlink_init);
module_exit(genetlink_exit);
MODULE_LICENSE("GPL");
