#include <stdio.h>
#include "dev.h"

int spndev_usb_open(struct spndev *dev, const char *devstr, int vend, int prod)
{
	fprintf(stderr, "spndev_open: USB devices not supported yet\n");
	return -1;
}
