# SPDX-License-Identifier: GPL-2.0
# apt install -y libusb-1.0-0-dev
CC = gcc
LIBUSB_CFLAGS = $(shell pkg-config --cflags libusb-1.0)
LIBUSB_LIBS = $(shell pkg-config --libs libusb-1.0)
WARNINGS = -Wall -Wextra
CFLAGS = $(LIBUSB_CFLAGS) $(WARNINGS)
LDFLAGS = $(LIBUSB_LIBS)

all: simple_test multibuff_test
%: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) simple_test multibuff_test
