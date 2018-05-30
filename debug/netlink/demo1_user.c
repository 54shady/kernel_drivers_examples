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
	int skfd;
	int ret;
	const char *message = "This is userspace speaking";
	struct sockaddr_nl local, dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct user_define_data udd;

	/* create a socket with USER_MSG type */
	skfd = socket(AF_NETLINK, SOCK_RAW, USER_MSG);
	if (skfd == -1)
	{
		printf("create socket error...%s\n", strerror(errno));
		return -1;
	}

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = 50;
	local.nl_groups = 0;
	if (bind(skfd, (struct sockaddr *)&local, sizeof(local)) != 0)
	{
		printf("bind() error\n");
		close(skfd);
		return -1;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* to kernel */
	dest_addr.nl_groups = 0;

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
	memset(nlh, 0, sizeof(struct nlmsghdr));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type = 0;
	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = local.nl_pid; /* self port */

	/* send message to kernel and wait for replay */
	memcpy(NLMSG_DATA(nlh), message, strlen(message));
	ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_nl));
	if (!ret)
	{
		perror("sendto error1\n");
		close(skfd);
		exit(-1);
	}

	memset(&udd, 0, sizeof(udd));
	ret = recvfrom(skfd, &udd, sizeof(struct user_define_data), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (!ret)
	{
		perror("recv form kernel error\n");
		close(skfd);
		exit(-1);
	}

	printf("USERSPACE receive :%s\n", udd.data);
	close(skfd);

	free((void *)nlh);
	return 0;
}
