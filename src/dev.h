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
#ifndef SPNAVDEV_DEVICE_H_
#define SPNAVDEV_DEVICE_H_

struct spndev {
	char *name, *path;
	int usb_vendor, usb_product;

	int fd;			/* UNIX file descriptor */
	void *handle;	/* Win32 handle */

	int num_axes, num_buttons;
	char **axis_name, **bn_name;
	int *minval, *maxval;
	int *deadz;
	int led;

	void *uptr;

	void (*close)(struct spndev*);
	int (*read)(struct spndev*, union spndev_event*);

	void (*setled)(struct spndev*, int led);
	int (*getled)(struct spndev*);
};


struct spndev *spndev_usb_open(const char *devstr, int vend, int prod);
struct spndev *spndev_ser_open(const char *devstr);


#endif	/* SPNAVDEV_DEVICE_H_ */
