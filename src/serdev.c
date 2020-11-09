#include <stdio.h>
#include "dev.h"

#if  defined(__i386__) || defined(__ia64__) || defined(WIN32) || \
    (defined(__alpha__) || defined(__alpha)) || \
     defined(__arm__) || \
    (defined(__mips__) && defined(__MIPSEL__)) || \
     defined(__SYMBIAN32__) || \
     defined(__x86_64__) || \
     defined(__LITTLE_ENDIAN__)
#define SPNDEV_LITTLE_ENDIAN
#else
#define SPNDEV_BIG_ENDIAN
#endif

#define INP_BUF_SZ	256


enum {
	SB4000	= 1,
	FLIPXY	= 2
};

struct sball {
	int type;
	unsigned int flags;

	char buf[INP_BUF_SZ];
	int len;

	unsigned int keystate, keymask;

	struct termios saved_term;
	int saved_mstat;

	int (*parse)(struct sball*, int, char*, int);
};

enum {
	DEV_UNKNOWN,
	DEV_SB2003,
	DEV_SB2003C,
	DEV_SB3003,
	DEV_SB4000,
	DEV_SMOUSE,
	DEV_SB5000,
	DEV_CADMAN,
	DEV_SPEXP
};

static struct {
	const char *name;
	int nbuttons;
	const char *bnames[16];
} devinfo[] = {
	{"Unknown serial device", 16, {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16"}},
	{"Spaceball 1003/2003", 8, {"1", "2", "3", "4", "5", "6", "7", "pick"}},
	{"Spaceball 2003C", 8, {"1", "2", "3", "4", "5", "6", "7", "8"}},
	{"Spaceball 3003/3003C", 2, {"R", "L"}},
	{"Spaceball 4000FLX/5000FLX-A", 12, {"1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C"}},
	{"Magellan SpaceMouse", 9, {"1", "2", "3", "4", "5", "6", "7", "8", "*"}},
	{"Spaceball 5000FLX", 12, {"1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C"}},
	{"CadMan", 4, {"1", "2", "3", "4"}},
	{"Space Explorer", 14, {"1", "2", "T", "L", "R", "F", "ALT", "ESC", "SHIFT", "CTRL", "Fit", "Panel", "+", "-", "2D"}},
	{0, 0, {0}}
};

static int init_dev(struct spndev *dev, int type);

static int stty_sball(int fd, struct sball *sb);
static int stty_mag(int fd, struct sball *sb);
static void stty_save(int fd, struct sball *sb);
static void stty_restore(int fd, struct sball *sb);

static int proc_input(struct sball *sb);

static int mag_parsepkt(struct sball *sb, int id, char *data, int len);
static int sball_parsepkt(struct sball *sb, int id, char *data, int len);

static int guess_device(const char *verstr);

static void make_printable(char *buf, int len);
static int read_timeout(int fd, char *buf, int bufsz, long tm_usec);



int spndev_ser_open(struct spndev *dev, const char *devstr)
{
	int fd, sz;
	char buf[128];
	struct sball *sb = 0;

	if((fd = open(devstr, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
		fprintf(stderr, "spndev_open: failed to open device: %s: %s\n", dev, strerror(errno));
		return -1;
	}
	dev->fd = fd;
	dev->path = strdup(devstr);
	dev->usb_vendor = dev->usb_product = -1;
	dev->handle = 0;

	if(!(sb = calloc(1, sizeof *sb))) {
		fprintf(stderr, "spndev_open: failed to allocate sball object\n");
		goto err;
	}
	dev->drvdata = sb;

	stty_save(fd, sb);

	if(stty_sball(fd, sb) == -1) {
		goto err;
	}
	write(fd, "\r@RESET\r", 8);

	if((sz = read_timeout(fd, buf, sizeof buf - 1, 2000000)) > 0 && strstr(buf, "\r@1")) {
		/* we got a response, so it's a spaceball */
		make_printable(buf, sz);
		if(init_dev(dev, guess_device(buf)) == -1) {
			fprintf(stderr, "spndev_open: failed to initialized device structure\n");
			goto err;
		}

		printf("Spaceball detected: %s\n", dev->name);

		/* set binary mode and enable automatic data packet sending. also request
		 * a key event to find out as soon as possible if this is a 4000flx with
		 * 12 buttons
		*/
		write(fd, "\rCB\rMSSV\rk\r", 11);

		sb->parse = sball_parsepkt;
		return dev;
	}

	/* try as a magellan spacemouse */
	if(stty_mag(fd, sb) == -1) {
		goto err;
	}
	write(fd, "vQ\r", 3);

	if((sz = read_timeout(fd, buf, sizeof buf - 1, 250000)) > 0 && buf[0] == 'v') {
		make_printable(buf, sz);
		if(init_dev(dev, guess_device(buf)) == -1) {
			fprintf(stderr, "spndev_open: failed to initialize device structure\n");
			goto err;
		}

		printf("Magellan SpaceMouse detected: %s\n", dev->name);

		/* set 3D mode, not-dominant-axis, pass through motion and button packets */
		write(fd, "m3\r", 3);

		sb->parse = mag_parsepkt;
		return dev;
	}

err:
	stty_restore(fd, sb);
	close(fd);
	free(sb);
	return -1;
}

static int init_dev(struct spndev *dev, int type)
{
	int i;
	struct sball *sb = dev->drvdata;
	static const char *axnames[] = {"Tx", "Ty", "Tz", "Rx", "Ry", "Rz"};

	if(!(dev->name = strdup(devinfo[type].name))) {
		return -1;
	}
	dev->num_axes = 6;
	dev->num_buttons = devinfo[type].nbuttons;

	if(!(dev->aprop = malloc(dev->num_axes * sizeof *dev->aprop))) {
		free(dev->name);
		return -1;
	}
	if(!(dev->bn_name = malloc(dev->num_buttons * sizeof *dev->bn_name))) {
		free(dev->aprop);
		free(dev->name);
	}

	for(i=0; i<6; i++) {
		dev->aprop[i].name = axnames[i];
		/* TODO min/max/deadz */
	}
	for(i=0; i<dev->num_buttons; i++) {
		dev->bn_name[i] = devinfo[type].bnames[i];
	}

	/* TODO setup callbacks */
}

static int guess_device(const char *verstr)
{
	int major, minor;
	const char *s;

	if((s = strstr(verstr, "Firmware version"))) {	/* spaceball */
		/* try to guess based on firmware number */
		if(sscanf(s + 17, "%d.%d", &major, &minor) == 2 && major == 2) {
			switch(minor) {
			case 35:
			case 62:
			case 63:
				return DEV_SB3003;

			case 43:
			case 45:
				return DEV_SB4000;

			case 2:
			case 13:
			case 15:
				return DEV_SB2003;

			case 42:
				/* 2.42 is also used by spaceball 4000flx. we'll guess 2003c for
				 * now, and change the buttons to 12 first time we get a '.'
				 * packet. I'll also request a key report during init to make
				 * sure this happens as soon as possible, before clients have a
				 * chance to connect.
				 */
				return DEV_SB2003C;
			}
		}
	}

	if(strstr(verstr, "MAGELLAN")) {
		return DEV_SMOUSE;
	}

	if(strstr(verstr, "SPACEBALL")) {
		return DEV_SB5000;
	}

	if(strstr(verstr, "CadMan")) {
		return DEV_CADMAN;
	}

	if(strstr(verstr, "SpaceExplorer")) {	/* no idea what the ver string is, this is just a guess */
		return DEV_SPEXP;
	}

	fprintf(stderr, "Unknown serial device. Even if it works, please report this"
			"as a bug at https://github.com/FreeSpacenav/libspnavdev/issues\n");
	fprintf(stderr, "Please include the following version string in your bug report: \"%s\"\n", verstr);
	return DEV_UNKNOWN;
}
