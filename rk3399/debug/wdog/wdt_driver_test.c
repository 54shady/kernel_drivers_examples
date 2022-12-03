#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define WDT_DEV_NODE "/dev/watchdog"

void help_info(void)
{
	printf("Usage: wdt_driver_test <timeout> <sleep> <test>\n");
	printf("    timeout: value in seconds to cause wdt timeout/reset\n");
	printf("    sleep: value in seconds to service the wdt\n");
	printf("    test: 0 - Service wdt with ioctl(), 1 - with write()\n");
}

int main(int argc, const char *argv[])
{
	int fd, timeout, sleep_sec, test;

	if (argc < 2) {
		help_info();
		return 1;
	}

	timeout = atoi(argv[1]);
	sleep_sec = atoi(argv[2]);
	if (sleep_sec <= 0) {
		sleep_sec = 1;
		printf("correct 0 or negative sleep time to %d seconds\n",
				sleep_sec);
	}
	test = atoi(argv[3]);
	printf("Starting wdt_driver (timeout: %d, sleep: %d, test: %s)\n",
			timeout, sleep_sec, (test == 0) ? "ioctl" : "write");
	fd = open(WDT_DEV_NODE, O_WRONLY);
	if (fd == -1) {
		perror("watchdog");
		exit(1);
	}
	printf("Trying to set timeout value=%d seconds\n", timeout);
	ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
	printf("The actual timeout was set to %d seconds\n", timeout);
	ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
	printf("Now reading back -- The timeout is %d seconds\n", timeout);

	while (1)
	{
		if (test == 0)
		{
			ioctl(fd, WDIOC_KEEPALIVE, 0);
		}
		else
		{
			write(fd, "\0", 1);
		}

		sleep(sleep_sec);
	}

	return 0;
}
