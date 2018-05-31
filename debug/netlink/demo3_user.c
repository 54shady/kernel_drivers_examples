#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

#include <linux/genetlink.h>

#define GENLMSG_DATA(glh)       ((void*)(((char*)glh) + GENL_HDRLEN))
#define NLA_DATA(nla)           ((void *)((char*)(nla) + NLA_HDRLEN))
#define NLA_NEXT(nla,len)       ((len) -= NLA_ALIGN((nla)->nla_len), \ 
		(struct nlattr*)(((char*)(nla)) + NLA_ALIGN((nla)->nla_len)))
#define NLA_OK(nla,len)         ((len) >= (int)sizeof(struct nlattr) && \ 
		(nla)->nla_len >= sizeof(struct nlattr) && \ 
		(nla)->nla_len <= (len))

/* copy from kernel driver genl_ops's cmd */
enum {
	DOC_EXMPL_C_UNSPEC,
	DOC_EXMPL_C_ECHO,
	__DOC_EXMPL_C_MAX,
};

/* copy from kernel driver netlink attribute */
enum {
	DOC_EXMPL_A_UNSPEC,
	DOC_EXMPL_A_MSG,
	__DOC_EXMPL_A_MAX,
};

/**
 * nla_attr_size - length of attribute size, NOT including padding
 * @param payload   length of payload
 * @return
 */
static inline int nla_attr_size(int payload)
{
	return NLA_HDRLEN + payload;
}

/**
 * nla_total_size - total length of attribute including padding
 * @param payload   length of payload, NOT including NLA_HDR
 */
static inline int nla_total_size(int payload)
{
	return NLA_ALIGN(nla_attr_size(payload));
}

static int genlmsg_open(void)
{
	int sockfd;
	struct sockaddr_nl nladdr;
	int ret;

	sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if (sockfd < 0)
	{
		printf("socket: %m");
		return -1;
	}

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	nladdr.nl_groups = 0xffffffff;

	ret = bind(sockfd, (struct sockaddr *)&nladdr, sizeof(nladdr));
	if (ret < 0)
	{
		printf("bind: %m");
		ret = -1;
		goto err_out;
	}

	return sockfd;

err_out:
	close(sockfd);
	return ret;
}

static void *genlmsg_alloc(int *size)
{
	unsigned char *buf;
	int len;

	/*
	 * attribute len
	 * attr len = (nla_hdr + pad) + (payload(user data) + pad)
	 */
	len = nla_total_size(*size);
	/*
	 * family msg len,
	 * but actually we have NOT custom family header
	 * family msg len = family_hdr + payload(attribute)
	 */
	len += 0;
	/*
	 * generic netlink msg len
	 * genlmsg len = (genlhdr + pad) + payload(family msg)
	 */
	len += GENL_HDRLEN;
	/*
	 * netlink msg len
	 * nlmsg len = (nlmsghdr + pad) + (payload(genlmsg) + pad)
	 */
	len = NLMSG_SPACE(len);

	buf = malloc(len);
	if (!buf)
		return NULL;

	memset(buf, 0, len);
	*size = len;

	return buf;
}

static void genlmsg_free(void *buf)
{
	if (buf)
		free(buf);
}

static int genlmsg_send(int sockfd, unsigned short nlmsg_type, unsigned int nlmsg_pid,
		unsigned char genl_cmd, unsigned char genl_version,
		unsigned short nla_type, const void *nla_data, unsigned int nla_len)
{
	struct nlmsghdr *nlh;    /* netlink message header */
	struct genlmsghdr *glh;  /* generic netlink message header */
	struct nlattr *nla;      /* netlink attribute header */

	struct sockaddr_nl nladdr;
	unsigned char *buf;
	int len;

	int count;
	int ret;

	if ((nlmsg_type == 0) || (!nla_data) || (nla_len <= 0))
	{
		return -1;
	}

	len = nla_len;
	buf = genlmsg_alloc(&len);
	if (!buf)
		return -1;

	nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = len;
	nlh->nlmsg_type = nlmsg_type;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = nlmsg_pid;

	glh = (struct genlmsghdr *)NLMSG_DATA(nlh);
	glh->cmd = genl_cmd;
	glh->version = genl_version;


	nla = (struct nlattr *)GENLMSG_DATA(glh);
	nla->nla_type = nla_type;
	nla->nla_len = nla_attr_size(nla_len);
	memcpy(NLA_DATA(nla), nla_data, nla_len);

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	count = 0;
	ret = 0;
	do {
		ret = sendto(sockfd, &buf[count], len - count, 0,
				(struct sockaddr *)&nladdr, sizeof(nladdr));
		if (ret < 0)
		{
			if (errno != EAGAIN)
			{
				count = -1;
				goto out;
			}
		}
		else
		{
			count += ret;
		}

	}while (count < len);

out:
	genlmsg_free(buf);

	printf("send return %d", count);
	return count;
}

/**
 *
 * @param sockfd    generic netlink socket fd
 * @param buf       the 'buf' is including the struct nlmsghdr,
 *                  struct genlmsghdr and struct nlattr
 * @param len       size of 'buf'
 * @return  >0      size of genlmsg
 *          <0      error occur
 */
static int genlmsg_recv(int sockfd, unsigned char *buf, unsigned int len)
{
	struct sockaddr_nl nladdr;
	struct msghdr msg;
	struct iovec iov;
	int ret;

	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	nladdr.nl_groups = 0xffffffff;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_name = (void *)&nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	ret = recvmsg(sockfd, &msg, 0);
	ret = ret > 0 ? ret : -1;
	printf("recv return %d\n", ret);
	return ret;
}

static int genlmsg_dispatch(struct nlmsghdr *nlmsghdr, unsigned int nlh_len,
		int nlmsg_type, int nla_type, unsigned char *buf, int *len)
{
	struct nlmsghdr *nlh;
	struct genlmsghdr *glh;
	struct nlattr *nla;
	int nla_len;

	int l;
	int i;
	int ret = -1;

	if (!nlmsghdr || !buf || !len)
		return -1;

	printf("nlmsg_type = %d\n", nlmsghdr->nlmsg_type);
	if (nlmsg_type && (nlmsghdr->nlmsg_type != nlmsg_type))
		return -1;

	for (nlh = nlmsghdr; NLMSG_OK(nlh, nlh_len); nlh = NLMSG_NEXT(nlh, nlh_len))
	{
		/* The end of multipart message. */
		if (nlh->nlmsg_type == NLMSG_DONE)
		{
			printf("get NLMSG_DONE\n");
			ret = 0;
			break;
		}

		if (nlh->nlmsg_type == NLMSG_ERROR)
		{
			printf("get NLMSG_ERROR\n");
			ret = -1;
			break;
		}

		glh = (struct genlmsghdr *)NLMSG_DATA(nlh);
		nla = (struct nlattr *)GENLMSG_DATA(glh);   /* the first attribute */
		nla_len = nlh->nlmsg_len - GENL_HDRLEN;           /* len of attributes */
		for (i = 0; NLA_OK(nla, nla_len); nla = NLA_NEXT(nla, nla_len), ++i)
		{
			printf("%d. nla->nla_type = %d\n", i, nla->nla_type);
			/* Match the family ID, copy the data to user */
			if (nla_type == nla->nla_type)
			{
				l = nla->nla_len - NLA_HDRLEN;  /* data length in one attribute */
				*len = *len > l ? l : *len;
				memcpy(buf, NLA_DATA(nla), *len);
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

static int genlmsg_get_family_id(int sockfd, const char *family_name)
{
	void *buf;
	int len;
	__u16 id;
	int l;
	int ret;

	ret = genlmsg_send(sockfd, GENL_ID_CTRL, 0, CTRL_CMD_GETFAMILY, 1,
			CTRL_ATTR_FAMILY_NAME, family_name, strlen(family_name) + 1);
	if (ret < 0)
		return -1;

	len = 256;
	buf = genlmsg_alloc(&len);
	if (!buf)
		return -1;

	len = genlmsg_recv(sockfd, buf, len);
	if (len < 0)
		return len;

	id = 0;
	l = sizeof(id);
	genlmsg_dispatch((struct nlmsghdr *)buf, len, 0, CTRL_ATTR_FAMILY_ID, (unsigned char *)&id, &l);

	genlmsg_free(buf);

	return id > 0 ? id : -1;
}

static void genlmsg_close(int sockfd)
{
	if (sockfd >= 0)
		close(sockfd);
}

#define BUF_SIZE    256
static int test_netlink_multicast(void)
{
	int sockfd = -1;
	unsigned char buf[BUF_SIZE];
	int len;
	struct nlmsghdr *nlh = NULL;
	int id;
	int ret;

	len = BUF_SIZE;
	nlh = genlmsg_alloc(&len);
	if (!nlh)
		return -1;

	sockfd = genlmsg_open();
	if (sockfd < 0)
	{
		ret = -1;
		goto out;
	}

	id = genlmsg_get_family_id(sockfd, "DOC_EXMPL");
	printf("get family ID[%d]\n", id);
	if (id <= 0)
	{
		ret = -1;
		goto out;
	}

	ret = genlmsg_recv(sockfd, (unsigned char *)nlh, len);
	printf("recv return %d\n", ret);
	if (ret > 0)
	{
		memset(buf, 0, sizeof(buf));
		len = sizeof(buf);
		ret = genlmsg_dispatch(nlh, ret, id, DOC_EXMPL_A_MSG, buf, &len);
		if (ret == 0)
		{
			printf("get: %s\n", buf);
		}
	}

out:
	genlmsg_close(sockfd);
	genlmsg_free(nlh);

	return 0;
}

int main(int argc, char *argv[])
{
	/* test netlink broadcast */
	test_netlink_multicast();

	return 0;
}
