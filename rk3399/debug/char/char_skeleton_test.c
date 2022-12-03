#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define SKELETON_DEVICE_NAME "/dev/char_skeleton"

struct self_define_data
{
	int arg1;
	int arg2;
};

void print_usage(const char *name)
{
	printf("%s <argv1> <argv2>\n", name);
}

int main(int argc, char *argv[])
{
	int fd;
	struct self_define_data sdd;
	int wBytes;

	if (argc < 3)
	{
		print_usage(argv[0]);
		return 1;
	}

	fd = open(SKELETON_DEVICE_NAME, O_RDWR);
	if (fd < 0)
	{
		printf("Can't open %s\n", SKELETON_DEVICE_NAME);
		return -1;
	}
	sdd.arg1 = atoi(argv[1]);
	sdd.arg2 = atoi(argv[2]);
	printf("argv1 = %d, argv2 = %d\n", sdd.arg1, sdd.arg2);

	wBytes = write(fd, &sdd, sizeof(sdd));
	printf("wBytes = %dbytes\n", wBytes);
	close(fd);

	return 0;
}
