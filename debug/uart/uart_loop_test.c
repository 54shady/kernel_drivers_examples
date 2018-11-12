#include <stdarg.h>
#include <termio.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#define DEFAULT_RATE 115200
#define ALPHABET_LEN 26
#define DATA_NR 500

/* uart device handler */
int fd;

/* thrad worker swither */
int trun, rrun;

/* recv and xfer count */
unsigned int tcount, rcount;

/* fp for read file */
FILE *fp_read;

/* totoal data size for test */
int data_size = DATA_NR * ALPHABET_LEN;

/* test data */
static char alphabet_table[ALPHABET_LEN];

static speed_t baudrate_map(unsigned long b)
{
	speed_t retval;

	switch (b)
	{
		case 110:
			retval = B110;
			break;

		case 300:
			retval = B300;
			break;

		case 1200:
			retval = B1200;
			break;

		case 2400:
			retval = B2400;
			break;

		case 4800:
			retval = B4800;
			break;

		case 9600:
			retval = B9600;
			break;

		case 19200:
			retval = B19200;
			break;

		case 38400:
			retval = B38400;
			break;

		case 57600:
			retval = B57600;
			break;

		case 115200:
			retval = B115200;
			break;

#ifdef B230400
		case 230400:
			retval = B230400;
			break;
#endif

#ifdef B460800
		case 460800:
			retval = B460800;
			break;
#endif

#ifdef B500000
		case 500000:
			retval = B500000;
			break;
#endif

#ifdef B576000
		case 576000:
			retval = B576000;
			break;
#endif

#ifdef B921600
		case 921600:
			retval = B921600;
			break;
#endif

#ifdef B1000000
		case 1000000:
			retval = B1000000;
			break;
#endif

#ifdef B1152000
		case 1152000:
			retval = B1152000;
			break;
#endif

#ifdef B1500000
		case 1500000:
			retval = B1500000;
			break;
#endif

#ifdef B2000000
		case 2000000:
			retval = B2000000;
			break;
#endif

#ifdef B2500000
		case 2500000:
			retval = B2500000;
			break;
#endif

#ifdef B3000000
		case 3000000:
			retval = B3000000;
			break;
#endif

#ifdef B3500000
		case 3500000:
			retval = B3500000;
			break;
#endif

#ifdef B4000000
		case 4000000:
			retval = B4000000;
			break;
#endif

		default:
			retval = 0;
			break;
	}

	return(retval);
}

/* xfer DATA_NR number of alphabet data */
void *Uartsend(void * threadParameter)
{
	int i;
	char *tx_buf;
	double speed;
	time_t start;
	time_t end;
	time_t time_consuming;

	tcount = 0;

	for (i = 0; i < ALPHABET_LEN; i++)
		alphabet_table[i] = i + 'A';

	tx_buf = malloc(data_size);
	for (i = 0; i < DATA_NR; i++)
		memcpy(tx_buf + i*ALPHABET_LEN, alphabet_table, ALPHABET_LEN);

	while (trun)
	{
		sleep(1);
		start = time(NULL);
		write(fd, tx_buf, data_size);
		end = time(NULL);
		time_consuming = end - start;
		speed = data_size * 8 / time_consuming;
		tcount += data_size;
		printf("sent %d bytes with speed %fbps\n", data_size, speed);
	}

	free(tx_buf);

	return 0;
}

/* recv uart data, and write it into file */
void *Uartread(void * threadParameter)
{
	char *rx_buf;
	int iores, iocount;
	rcount = 0;

	while (rrun)
	{
		iocount = 0;
		iores = ioctl(fd, FIONREAD, &iocount);
		if(!iocount)
			continue;
		rx_buf = malloc(iocount);
		/* Read in and wrap around the list */
		iores = read(fd, rx_buf, iocount);
		rcount += iores;
#ifdef DEBUG_MESSAGE
		rx_buf[iores] = '\0';
		printf("%s[%d:%d]\n", rx_buf, rcount, iores);
#endif
		fwrite(rx_buf, 1, iores, fp_read);
		free(rx_buf);
	}

	return 0;
}

static void print_usage(const char *name)
{
	printf("Usage: %s <tty name> [-S] [-O] [-E] [-HW] [-B baudrate]"
			"\n\t'-S' for 2 stop bit"
			"\n\t'-O' for PARODD "
			"\n\t'-E' for PARENB"
			"\n\t'-HW' for HW flow control enable"
			"\n\t'-B baudrate' for different baudrate\n", name);
}

int main(int argc, char *argv[])
{
	int i, ret;
	struct termios options;
	unsigned long baudrate = DEFAULT_RATE;
	char c = 0;
	pthread_t p_Uartsend, p_Uartread;
	void *thread_res;
	char *test_result = "Failed";

	if (argc < 2) {
		print_usage(argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd == -1)
	{
		printf("open_port: Unable to open serial port - %s", argv[1]);
		return -1;
	}

	fcntl(fd, F_SETFL, 0);

	tcgetattr(fd, &options);
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= PARENB;
	options.c_cflag &= ~PARODD;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CRTSCTS;

	options.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
	options.c_oflag &= ~OPOST;
	options.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT );

	options.c_cc[VMIN] = 1;
	options.c_cc[VTIME] = 0;

	options.c_cflag |= (CLOCAL | CREAD);
	for (i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "-S")) {
			options.c_cflag |= CSTOPB;
			continue;
		}
		if (!strcmp(argv[i], "-O")) {
			options.c_cflag |= PARODD;
			options.c_cflag &= ~PARENB;
			continue;
		}
		if (!strcmp(argv[i], "-E")) {
			options.c_cflag &= ~PARODD;
			options.c_cflag |= PARENB;
			continue;
		}
		if (!strcmp(argv[i], "-HW")) {
			options.c_cflag |= CRTSCTS;
			continue;
		}
		if (!strcmp(argv[i], "-B")) {
			i++;
			baudrate = atoi(argv[i]);
			if(!baudrate_map(baudrate))
				baudrate = DEFAULT_RATE;
			continue;
		}
	}

	if (baudrate) {
		cfsetispeed(&options, baudrate_map(baudrate));
		cfsetospeed(&options, baudrate_map(baudrate));
	}
	tcsetattr(fd, TCSANOW, &options);
	printf("UART %lu, %dbit, %dstop, %s, HW flow %s\n", baudrate, 8,
			(options.c_cflag & CSTOPB) ? 2 : 1,
			(options.c_cflag & PARODD) ? "PARODD" : "PARENB",
			(options.c_cflag & CRTSCTS) ? "enabled" : "disabled");

	trun = 1;
	rrun = 1;
	fp_read = fopen("uart_read.txt","wb");

	/* create and start the xfer thread */
	ret = pthread_create(&p_Uartsend, NULL, Uartsend, NULL);
	if (ret < 0)
		goto error;

	/*
	 * create and start the recv thread
	 * stop the xfer thread if and error happend
	 */
	ret = pthread_create(&p_Uartread, NULL, Uartread, NULL);
	if (ret < 0) {
		ret = pthread_join(p_Uartsend, &thread_res);
		goto error;
	}

	/* stop the test */
	printf("test begin, press 'c' to exit\n");
	while (c != 'c') {
		c = getchar();
	}

	/* stop xfer thread */
	trun = 0;
	ret = pthread_join(p_Uartsend, &thread_res);
	if (ret < 0)
		printf("fail to stop Uartsend thread\n");

	printf("tcount=%d Bytes\n", tcount);

	/* wait 5 more seconds, when recv thread not yet completed */
	i = 5;
	while ((tcount > rcount) && i) {
		i--;
		sleep(1);
	}

	/* stop the recv thread */
	rrun = 0;
	ret = pthread_join(p_Uartread, &thread_res);
	if (ret < 0)
		printf("fail to stop Uartread thread\n");

	printf("rcount=%d Bytes\n", rcount);

	if (rcount == tcount)
		test_result = "Successed";
error:
	fclose(fp_read);
	close(fd);
	printf("Test %s\n", test_result);

	return 0;
}
