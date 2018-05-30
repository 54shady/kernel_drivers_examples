#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <linux/netlink.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define NETLINK_USER 22
#define USER_MSG    (NETLINK_USER + 1)
#define MSG_LEN 100
#define MAX_PLOAD 100

/* user define data */
struct user_define_data
{
	struct nlmsghdr hdr;
	char  data[MSG_LEN];
};

int main(int argc, char **argv)
{
	int sock_fd;
	int ret;
	const char *message = "This is userspace speaking";
	struct sockaddr_nl src_addr, dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct user_define_data udd;

	/* create a socket with USER_MSG type */
	sock_fd = socket(AF_NETLINK, SOCK_RAW, USER_MSG);
	if (sock_fd == -1)
	{
		printf("create socket error...%s\n", strerror(errno));
		return -1;
	}

	/* source address */
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	/*
	 * In scenarios where different threads of the same process want to
	 * have different netlink sockets opened under the same netlink protocol
	 */
#ifndef DIFFERENT_NETLINK_IN_SAME_NETLINK
	src_addr.nl_pid = getpid();
#else
	src_addr.nl_pid = pthread_self() << 16 | getpid();
#endif
	src_addr.nl_groups = 0; /* not in mcast groups */
	if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) != 0)
	{
		printf("bind() error\n");
		close(sock_fd);
		return -1;
	}

	/* destination address */
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* For Linux Kernel */
	dest_addr.nl_groups = 0; /* unicast */

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));

	/* Fill the netlink message header */
	memset(nlh, 0, sizeof(struct nlmsghdr));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type = 0;
	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = src_addr.nl_pid; /* self pid */
	printf("calling process pid = %d\n", nlh->nlmsg_pid);

	/* Fill in the netlink message payload */
	memcpy(NLMSG_DATA(nlh), message, strlen(message));

	ret = sendto(sock_fd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_nl));
	if (!ret)
	{
		perror("sendto error1\n");
		close(sock_fd);
		exit(-1);
	}

	memset(&udd, 0, sizeof(udd));
	ret = recvfrom(sock_fd, &udd, sizeof(struct user_define_data), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (!ret)
	{
		perror("recv form kernel error\n");
		close(sock_fd);
		exit(-1);
	}

	printf("USERSPACE receive :%s\n", udd.data);
	close(sock_fd);
	free((void *)nlh);

	return 0;
}
