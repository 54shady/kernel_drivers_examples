#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/netlink.h>

#define NETLINK_USER  22
#define USER_MSG    (NETLINK_USER + 1)

static struct sock *netlinkfd = NULL;
const char *message = "This is kernel speaking";

int send_msg(const char *pbuf, unsigned short len, unsigned int pid)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
    int ret;

    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if (!nl_skb)
    {
        printk("netlink_alloc_skb error\n");
        return -1;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, USER_MSG, len, 0);
    if (nlh == NULL)
    {
        printk("nlmsg_put() error\n");
        nlmsg_free(nl_skb);
        return -1;
    }

    memcpy(nlmsg_data(nlh), pbuf, len);

    ret = netlink_unicast(netlinkfd, nl_skb, pid, MSG_DONTWAIT);

    return ret;
}

/* wait for message coming down from user-space */
static void recv_cb(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    void *data = NULL;
	unsigned int pid;

    printk("skb->len:%u\n", skb->len);
    if (skb->len >= nlmsg_total_size(0))
    {
        nlh = nlmsg_hdr(skb);
		pid = nlh->nlmsg_pid; /*pid of sending process */
		printk("calling process pid = %d\n", pid);

        data = NLMSG_DATA(nlh);
        if (data)
        {
            printk("KERNEL receive : %s\n", (int8_t *)data);
            send_msg(message, strlen(message), pid);
        }
    }
}

struct netlink_kernel_cfg nl_cfg = {
    .input = recv_cb,
};

static int test_netlink_init(void)
{
    printk("%s, %d\n", __FUNCTION__, __LINE__);

    netlinkfd = netlink_kernel_create(&init_net, USER_MSG, &nl_cfg);
    if (!netlinkfd)
    {
        printk(KERN_ERR "can not create a netlink socket!\n");
        return -1;
    }

    return 0;
}

static void test_netlink_exit(void)
{
    printk("%s, %d\n", __FUNCTION__, __LINE__);
    sock_release(netlinkfd->sk_socket);
}

module_init(test_netlink_init);
module_exit(test_netlink_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("netlink_demo");
