#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "spnavdev.h"

static struct spndev *dev;
static int quit = 0;

int main(int argc, char **argv)
{
	int fd;
	union spndev_event ev;
	const char *s;

	if(!(dev = spndev_open(argv[1]))) {
		fprintf(stderr, "Failed to open 6dof device %s\n", argv[1] ? argv[1] : "");
		return 1;
	}
	fd = spndev_fd(dev);

	printf("Monitoring device, ctrl-c to quit\n");

	while(!quit) {

		if(1 > 0) {
			if(1) {
				if(spndev_process(dev, &ev)) {
					switch(ev.type) {
					case SPNDEV_MOTION:
						printf("motion: T[%+6d %+6d %+6d]  R[%+6d %+6d %+6d]\n",
								ev.mot.v[0], ev.mot.v[1], ev.mot.v[2], ev.mot.v[3],
								ev.mot.v[4], ev.mot.v[5]);
						break;

					case SPNDEV_BUTTON:
						if((s = spndev_button_name(dev, ev.bn.num))) {
							printf("button %d (%s) ", ev.bn.num, s);
						} else {
							printf("button %d ", ev.bn.num);
						}
						puts(ev.bn.press ? "pressed" : "released");
						break;

					default:
						break;
					}
				}
			}
		}

	}

	spndev_close(dev);
	return 0;
}
