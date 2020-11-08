/*
libspnavdev - direct 6dof device handling library
Copyright (C) 2020 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include "spnavdev.h"
#include "dev.h"

struct spndev *spndev_open(const char *devstr)
{
	struct spndev *dev;
	int vendor = -1, product = -1;

	if(!devstr || sscanf(devstr, "%x:%x", &vendor, &product) == 2) {
		if(!(dev = spndev_usb_open(devstr, vendor, product))) {
			return 0;
		}
	} else {
		if(!(dev = spndev_ser_open(devstr))) {
			return 0;
		}
	}
	return dev;
}

void spndev_close(struct spndev *dev)
{
	if(dev) {
		dev->close(dev);
		free(dev);
	}
}

void spndev_set_userptr(struct spndev *dev, void *uptr)
{
	dev->uptr = uptr;
}

void *spndev_get_userptr(struct spndev *dev)
{
	return dev->uptr;
}

/* device information */
const char *spndev_name(struct spndev *dev)
{
	return dev->name;
}

const char *spndev_path(struct spndev *dev)
{
	return dev->path;
}

int spndev_usbid(struct spndev *dev, int *vend, int *prod)
{
	if(dev->usb_vendor == -1) return -1;
	*vend = dev->usb_vendor;
	*prod = dev->usb_product;
	return 0;
}


int spndev_num_axes(struct spndev *dev)
{
	return dev->num_axes;
}

const char *spndev_axis_name(struct spndev *dev, int axis)
{
	if(axis < 0 || axis >= dev->num_axes) {
		return 0;
	}
	return dev->axis_name[axis];
}

int spndev_axis_min(struct spndev *dev, int axis)
{
	if(axis < 0 || axis >= dev->num_axes) {
		return 0;
	}
	return dev->minval[axis];
}

int spndev_axis_max(struct spndev *dev, int axis)
{
	if(axis < 0 || axis >= dev->num_axes) {
		return 0;
	}
	return dev->maxval[axis];
}

int spndev_num_buttons(struct spndev *dev)
{
	return dev->num_buttons;
}

const char *spndev_button_name(struct spndev *dev, int bidx)
{
	if(bidx < 0 || bidx >= dev->num_buttons) {
		return 0;
	}
	return dev->bn_name[bidx];
}

int spndev_fd(struct spndev *dev)
{
	return dev->fd;
}

void *spndev_handle(struct spndev *dev)
{
	return dev->handle;
}

/* device operations */
int spndev_process(struct spndev *dev, union spndev_event *ev)
{
	return dev->read(dev, ev);
}

int spndev_set_led(struct spndev *dev, int state)
{
	if(!dev->setled) {
		return -1;
	}
	dev->setled(dev, state);
	return 0;
}

int spndev_get_led(struct spndev *dev)
{
	if(!dev->getled) {
		return 0;
	}
	return dev->getled(dev);
}

/* axis == -1: set all deadzones */
int spndev_set_deadzone(struct spndev *dev, int axis, int dead)
{
	return -1;	/* TODO */
}

int spndev_get_deadzone(struct spndev *dev, int axis)
{
	return -1;	/* TODO */
}
