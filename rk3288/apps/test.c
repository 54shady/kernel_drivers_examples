#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define MISC_DEV_NAME "/dev/myledmiscname"

#define IOCTL_INT_VALUE		0
#define IOCTL_STRING_VALUE	1

int main (int argc, char *argv[])
{
	int ret;
	int fd;
	char datas[] = "abcdefg";

	fd = open(MISC_DEV_NAME, O_RDWR);
	if (fd < 0)
		return -1;

	ret = ioctl(fd, IOCTL_INT_VALUE, 911);
	if (ret < 0)
	{
		printf("error, %d\n", __LINE__);
		return -1;
	}

	ret = ioctl(fd, IOCTL_STRING_VALUE, datas);
	if (ret < 0)
	{
		printf("error, %d\n", __LINE__);
		return -1;
	}

	ret = close(fd);

	return 0;
}
