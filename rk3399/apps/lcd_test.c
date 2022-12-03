#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#ifdef ANDROID_SYSTEM
#define DEFAULT_FB_DEVICE "/dev/graphics/fb0"
#else
#define DEFAULT_FB_DEVICE "/dev/fb0"
#endif

/*
 * Usage:
 * Android:
 * 		# stop
 * 		./lcd_test
 * 		# start
 * Linux:
 * 	./lcd_test
 */
int main(int argc, char *argv[])
{
	int fd = 0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int screensize = 0;
	char *fbp = 0;
	int x = 0, y = 0;
	long int location = 0;

	/* Open the file for reading and writing */
	fd = open(DEFAULT_FB_DEVICE, O_RDWR);
	if (fd == -1) {
		perror("Error: cannot open framebuffer device");
		exit(1);
	}
	printf("The framebuffer device was opened successfully.\n");

	/* Get fixed screen information */
	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		exit(2);
	}

	/* Get variable screen information */
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		exit(3);
	}

	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	/* Figure out the size of the screen in bytes */
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	/* Map the device to memory */
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fbp < 0) {
		perror("Error: failed to map framebuffer device to memory");
		exit(4);
	}
	printf("The framebuffer device was mapped to memory successfully.\n");

	/* Figure out where in memory to put the pixel */
	for (y = 0; y < vinfo.yres; y++)
	{
		for (x = 0; x < vinfo.xres; x++)
		{
			location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
				(y+vinfo.yoffset) * finfo.line_length;

			if (vinfo.bits_per_pixel == 32)
			{
				*(fbp + location) = 100;// Some blue
				*(fbp + location + 1) = 15+(x-100)/2; // A little green
				*(fbp + location + 2) = 200-(y-100)/5;// A lot of red
				*(fbp + location + 3) = 0;// No transparency
			}
			else //assume 16bpp
			{
				int b = 10;
				int g = (x-100)/6; // A little green
				int r = 31-(y-100)/16;// A lot of red
				unsigned short int t = r<<11 | g << 5 | b;
				*((unsigned short int*)(fbp + location)) = t;
			}
		}
	}

	munmap(fbp, screensize);
	close(fd);
	return 0;
}
