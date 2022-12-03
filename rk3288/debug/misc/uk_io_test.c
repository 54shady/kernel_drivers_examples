#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#define DEVICE_NODE "/dev/user_kernel_io"

#define GET_VALUE 10
#define PUT_VALUE 11

struct self_define_data {
	int age;
	char name[20];
};

static struct self_define_data sdd;

int main(int argc, char *argv[])
{
	int ret;
	int i;
	int fd;
	struct self_define_data tmp_sdd;

	fd = open(DEVICE_NODE, O_RDWR);
	if (fd < 0)
		printf("Open failed\n");

	sdd.age = 111;
	strcpy(sdd.name, "noname");

	ioctl(fd, GET_VALUE, &tmp_sdd);
	printf("get %s, %d\n", tmp_sdd.name, tmp_sdd.age);

	ioctl(fd, PUT_VALUE, &sdd);
	printf("set %s, %d\n", sdd.name, sdd.age);

	ioctl(fd, GET_VALUE, &tmp_sdd);
	printf("get %s, %d\n", tmp_sdd.name, tmp_sdd.age);

	return 0;
}
