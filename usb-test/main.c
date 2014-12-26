/*
 * ci20-tools
 * Copyright (C) 2014 Paul Burton <paulburton89@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/ci20.h"

static struct ci20_ctx *ctx;
static struct ci20_dev *dev;
static struct ci20_otp otp;

static void usage(FILE *f)
{
	fprintf(f, "Usage: ci20-usb-test <options>\n");
	fprintf(f, "\n");
	fprintf(f, "Where options is some combination of:\n");
	fprintf(f, "\n");
	fprintf(f, "  --help                 Display this message and exit\n");
	fprintf(f, "  --serial=<sn>          Use the board with this serial number\n");
}

static void discover_cb(struct ci20_ctx *c, struct ci20_dev *_dev, void *user)
{
	int serial = *(int *)user;
	int err;

	err = ci20_read_otp(_dev, &otp);
	if (err) {
		fprintf(stderr, "Failed to read OTP\n");
		return;
	}

	if ((serial != -1) && (serial != otp.serial)) {
		/* this is not the board you're looking for */
		return;
	}

	if (dev) {
		fprintf(stderr, "Multiple boards detected. Please specify --serial.\n");
		exit(EXIT_FAILURE);
	}

	dev = _dev;
}

int main(int argc, char *argv[])
{
	unsigned i;
	int ch, serial = -1;
	struct option options[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "serial", required_argument, NULL, 's' },
		{ NULL, 0, NULL, 0 },
	};

	do {
		ch = getopt_long(argc, argv, "", options, NULL);

		switch (ch) {
		case 'h':
			usage(stdout);
			return EXIT_SUCCESS;

		case 's':
			serial = atoi(optarg);
			break;

		case '?':
		case ':':
			usage(stderr);
			return EXIT_FAILURE;
		}
	} while (ch != -1);

	ctx = ci20_init();
	if (!ctx) {
		fprintf(stderr, "Failed to initialise libci20\n");
		return EXIT_FAILURE;
	}

	ci20_discover(ctx, discover_cb, &serial);

	if (!dev) {
		fprintf(stderr, "CI20 not found\n");
		return EXIT_FAILURE;
	}

	printf("Serial:           %" PRIu32 "\n", otp.serial);
	printf("Manufacture date: %" PRIu32 "\n", otp.manufacture_date);
	printf("Manufacturer:     %.2s (%s)\n", otp.manufacturer,
	       ci20_manufacturer_long(otp.manufacturer, "Unknown"));
	printf("MAC:              %02x:%02x:%02x:%02x:%02x:%02x\n",
	       otp.mac[0], otp.mac[1], otp.mac[2],
	       otp.mac[3], otp.mac[4], otp.mac[5]);

	for (i = 0; i < 10; i++) {
		ci20_pin_config(dev, 5, 15, PIN_GPIO_OUT_LOW + (i % 2));
		sleep(1);
	}

	ci20_fini(ctx);
	return EXIT_SUCCESS;
}
