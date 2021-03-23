/*
libspnavdev - USB HID 6dof device handling (through hidapi)
Copyright (C) 2021 Peter Ruevski <dpr@ruevs.com>

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
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include "hidapi.h"
#include "dev.h"

#define max_num_btn 48
#define max_buf_size 80

static const size_t oldposrotreportsize = 7;
static const size_t newposrotreportsize = 13;
static const int axis_max = 511;
static const int axis_min = -512;
static const int axis_deadz = 0;

typedef struct spnav_hid {
    unsigned char btn[max_num_btn];
    unsigned char buf[max_buf_size];
} tspnav_hid;

static const struct {
    uint16_t vid;
    uint16_t pid;
    enum {O=0, N=1} posrotreport;   // Old (two reports) or New (single report) positon and rotation data
    const char* name;
    const char* pn;     // "P/N:" part number "Part No." from the label or box
    int nbuttons;
    const char* const bnames[32];
}
devinfo[] = {
    {0x046d, 0xc603, O, "SpaceMouse Plus XT USB" ,          "",           10, {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"}},  // Manual says 11 is the * reported? Buttons order? Side button names? "L" "R"?
    {0x046d, 0xc605, O, "CADman",                           "",            4, {"1", "2", "3", "4"}},  // Buttons order? Names?
    {0x046d, 0xc606, O, "SpaceMouse Classic USB",           "",            8, {"1", "2", "3", "4", "5", "6", "7", "8"}},  // Manual says 11 is the * reported?
    {0x046d, 0xc621, O, "Spaceball 5000 USB",               "5000 USB",   12, {"1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C"}},  // Buttons order?
    {0x046d, 0xc623, O, "SpaceTraveler",                    "",            8, {"1", "2", "3", "4", "5", "6", "7", "8"}},
    {0x046d, 0xc625, O, "SpacePilot",                       "SP1 USB",    21, {"1", "2", "3", "4", "5", "6", "T", "L", "R", "F", "ESC", "ALT", "SHIFT", "CTRL", "FIT", "PANEL", "+", "-", "Dom", "3D Lock", "Config"}},  // IBM "(P) P/N: 60K9206", "(P) FRU P/N: 4K9204"
    {0x046d, 0xc626, O, "SpaceNavigator",                   "3DX-700028",  2, {"MENU", "FIT"} },  // also "3DX-600028" "3DX-600029"? "SpaceNavigator USB" on the label
    {0x046d, 0xc627, O, "SpaceExplorer",                    "3DX-700026", 15, {"1", "2", "T", "L", "R", "F", "ESC", "ALT", "SHIFT", "CTRL", "FIT", "PANEL", "+", "-", "2D"}},  // Also "3DX-600029"? "3DX-600025" both "SpaceExplorer USB" "DLZ-3DX-700026 (B)" on the other side,  "Part No. 3DX-700026" on the box
    {0x046d, 0xc628, O, "SpaceNavigator for Notebooks",     "3DX-700034",  2, {"MENU", "FIT"}},
    {0x046d, 0xc629, O, "SpacePilot Pro",                   "3DX-700036", 31,       {"MENU", "FIT", "T", "L", "R", "F", "B", "BK", "Roll +", "Roll -", "ISO1", "ISO2", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "ESC", "ALT", "SHIFT", "CTRL", "Rot", "Pan/Zoom", "Dom", "+", "-"}},
    {0x046d, 0xc62b, O, "SpaceMouse Pro",                   "3DX-700040", 27/*15*/, {"Menu", "FIT", "T",  "", "R", "F",  "",   "", "Roll +",       "",     "",     "", "1", "2", "3", "4",  "",  "",  "",  "",  "",   "", "ESC", "ALT", "SHIFT", "CTRL", "Rot"}},   // < ReportID>16  < Mask>00001000  LongPressButton_13 LongPressButton_14 LongPressButton_15 <Mask>00008000 LongPressButton_16
    {0x046d, 0xc640, O, "NuLOOQ",                           "",            5, {""}},  // Logitech Amazon First Available December 19, 2005  From https://github.com/microdee/UE4-SpaceMouse/blob/3584fab85147a7806c15040b5625ddb07414bbcb/Source/SpaceMouseReader/Private/SpaceMouseReader.cpp#L26
    {0x256f, 0xc62c, N, "LIPARI",                           "",           22,       {"MENU", "FIT", "T", "L", "R", "F", "B", "BK", "Roll +", "Roll -", "ISO1", "ISO2", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"}},
    {0x256f, 0xc62e, N, "SpaceMouse Wireless (cabled)",     "3DX-700043",  2, {"MENU", "FIT"}},  // 3DX-700066 3DX-600044?
    {0x256f, 0xc62f, N, "SpaceMouse Wireless Receiver",     "3DX-700043",  2, {"MENU", "FIT"}},  // 3DX-700066 3DX-600044?
    {0x256f, 0xc631, N, "SpaceMouse Pro Wireless (cabled)", "3DX-700075", 27/*15*/, {"MENU", "FIT", "T",  "", "R", "F",  "",   "", "Roll +",       "",     "",     "", "1", "2", "3", "4",  "",  "",  "",  "",  "",   "", "ESC", "ALT", "SHIFT", "CTRL", "Rot"}},  // < ReportID>16 < Mask > 00001000  LongPressButton_13 LongPressButton_14 LongPressButton_15 <Mask>00008000 LongPressButton_16    3DX-600047
    {0x256f, 0xc632, N, "SpaceMouse Pro Wireless Receiver", "3DX-700075", 27/*15*/, {"MENU", "FIT", "T",  "", "R", "F",  "",   "", "Roll +",       "",     "",     "", "1", "2", "3", "4",  "",  "",  "",  "",  "",   "", "ESC", "ALT", "SHIFT", "CTRL", "Rot"}},  // < ReportID>16 < Mask > 00001000  LongPressButton_13 LongPressButton_14 LongPressButton_15 <Mask>00008000 LongPressButton_16    3DX-600047
    {0x256f, 0xc633, N, "SpaceMouse Enterprise",            "3DX-700056", 31/*31*/, {"MENU", "FIT", "T",  "", "R", "F",  "",   "", "Roll +",       "", "ISO1",     "", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "ESC", "ALT", "SHIFT", "CTRL", "Rot"}},   // report1C  ENTER, DELETE, 11, 12, V1, V2, V3, TAB, SPACE    < ReportID>16  < Mask>00001000  LongPressButton_13 LongPressButton_14 LongPressButton_15 <Mask>00008000 LongPressButton_16   // 3DX-600051
    {0x256f, 0xc635, O, "SpaceMouse Compact",               "3DX-700059",  2, {"MENU", "FIT"}},
    {0x256f, 0xc636, O, "SpaceMouse Module",                "",            0, {""}},   // ??
    {0x256f, 0xc652, N, "SpaceMouse Universal Receiver",    "3DX-700069",  0, {""}},
};

static const unsigned num_known_devs = sizeof(devinfo)/sizeof(devinfo[0]);

static int usbdev_init(struct spndev* dev, const unsigned type);
static void usbdev_close(struct spndev* dev);
static int usbdev_read_O(struct spndev* dev, union spndev_event* evt);
static int usbdev_read_N(struct spndev* dev, union spndev_event* evt);
static void usbdev_setled(struct spndev* dev, int led);
static int usbdev_getled(struct spndev* dev);
static inline void usbdev_parsebuttons(const struct spndev* dev, union spndev_event* evt, const unsigned char* report);
static inline void checkrange(const struct spndev* dev, const int val);

#ifdef _WIN32
#define VID_PID_FORMAT_STR "%04x:%04x"
#else
#define VID_PID_FORMAT_STR "%#0.4hx:%#0.4hx"
#endif

int spndev_usb_open(struct spndev *dev, const char *devstr, unsigned short vend, unsigned short prod)
{
    int err = -1;
    struct hid_device_info* deviceinfos;

    memset(dev, 0, sizeof(*dev));

    if (devstr) {
        /* Make a list of specific devices */
        deviceinfos = hid_enumerate(vend, prod);
        if (!deviceinfos) {
            fprintf(stderr, "USB HID device " VID_PID_FORMAT_STR " not found\n", vend, prod);
            return -1;
        }
    } else {
        /* Make a list of all HID devices */
        deviceinfos = hid_enumerate(0, 0);
        if (!deviceinfos) {
            fprintf(stderr, "No USB HID devices found\n");
            return -1;
        }
    }

    struct hid_device_info* cinfo = deviceinfos;

    unsigned devidx = 0;
    unsigned hididx = 0;
    char opened = 0;

    /* Try to open the first known device */
    while (cinfo && !opened)
    {
        char pidmatch = 0;
        char vidmatch = 0;
        /* Loop through the found HID devices */
        for (devidx = 0; devidx < num_known_devs; ++devidx)
        {
            /* Searching for a match with one of our known devices */
            if (devinfo[devidx].vid == cinfo->vendor_id) {
                vidmatch = 1;
                if (devinfo[devidx].pid == cinfo->product_id)
                {
                    pidmatch = 1;
                    hid_device* hiddev = hid_open_path(cinfo->path);
                    if (hiddev) {
                        if (!(dev->path = _strdup(cinfo->path))) {
                            fprintf(stderr, "spndev_open: Failed to allocate device path\n");
                            goto cleanup;
                        }
                        if (!(dev->name = (char*)_wcsdup(cinfo->product_string))) {
                            fprintf(stderr, "spndev_open: Failed to allocate device name\n");
                            goto cleanup;
                        }
                        if (-1 == hid_set_nonblocking(hiddev, 1)) {
                            fprintf(stderr, "spndev_open: Failed to set non-blocking HID mode.\n");
                            goto cleanup;

                        }
                        dev->internal_id = hididx;
                        dev->handle = (void*)hiddev;
                        dev->usb_vendor = cinfo->vendor_id;
                        dev->usb_product = cinfo->product_id;
                        opened = 1;
                        err = 0;    // Success
                        fwprintf(stderr, L"Opened USB device %ws " VID_PID_FORMAT_STR " %hs\n", cinfo->product_string, cinfo->vendor_id, cinfo->product_id, cinfo->path);
                        break;
                    }
                    else {
                        fwprintf(stderr, L"Could not open USB device %s " VID_PID_FORMAT_STR " %hs\n", cinfo->product_string, cinfo->vendor_id, cinfo->product_id, cinfo->path);
                    }
                }
            }
        }

        if (vidmatch && !pidmatch) {
            fwprintf(stderr, L"Found unsupported USB device %s %s " VID_PID_FORMAT_STR " %hs\n",
                     cinfo->manufacturer_string, cinfo->product_string, cinfo->vendor_id, cinfo->product_id, cinfo->path);
            fprintf(stderr, "Usage Page: %#.2hx, Usage ID: %#.2hx\n", cinfo->usage_page, cinfo->usage);
            if (1 == cinfo->usage_page && 8 == cinfo->usage) {
                fprintf(stderr, "The device seems to be a \"Multi - axis Controller\". Please report it.\n");
            }
        }

        cinfo = cinfo->next;
        hididx++;
    }


cleanup:
    hid_free_enumeration(deviceinfos);

    if (-1 != err) {
        if (-1 == (err = usbdev_init(dev, devidx))) {
            fprintf(stderr, "spndev_open: failed to initialize device structure\n");
        }
    }

    if (-1 == err) {
        usbdev_close(dev);
    }

    return err;
}

static int usbdev_init(struct spndev* dev, const unsigned type)
{
    int i;
    static const char* axnames[] = { "Tx", "Ty", "Tz", "Rx", "Ry", "Rz" };

    dev->num_axes = 6;
    dev->num_buttons = devinfo[type].nbuttons;

    if (!(dev->drvdata = calloc(1, sizeof(tspnav_hid)))) {
        fprintf(stderr, "spndev_open: failed to allocate HID buffer\n");
        return -1;
    }

    if (!(dev->aprop = malloc(dev->num_axes * sizeof * dev->aprop))) {
        return -1;
    }
    if (!(dev->bn_name = malloc(dev->num_buttons * sizeof * dev->bn_name))) {
        return -1;
    }

    for (i = 0; i < 6; i++) {
        dev->aprop[i].name = axnames[i];
        dev->aprop->maxval = axis_max;
        dev->aprop->minval = axis_min;
        dev->aprop->deadz = axis_deadz;
    }
    for (i = 0; i < dev->num_buttons; i++) {
        dev->bn_name[i] = devinfo[type].bnames[i];
    }

    dev->close = usbdev_close;
    if (O == devinfo[type].posrotreport) {
        dev->read = usbdev_read_O;
    } else if (N == devinfo[type].posrotreport) {
        dev->read = usbdev_read_N;
    }
    else {
        // WTF?
    }
    dev->getled = usbdev_getled;
    dev->setled = usbdev_setled;
    return 0;
}

static void usbdev_close(struct spndev *dev) {
    if (dev) {
        hid_close((hid_device*)dev->handle);
        free(dev->name);
        free(dev->path);
        free(dev->drvdata);
        free(dev->aprop);
        free((void *)dev->bn_name);
    }

}


// Read and parse "Old" (two reports) style positon and rotation data
static int usbdev_read_O(struct spndev *dev, union spndev_event *evt)
{
    evt->type = SPNDEV_NONE;
    unsigned char *buffer = ((tspnav_hid*)dev->drvdata)->buf;

    if (hid_read((hid_device*)dev->handle, buffer, oldposrotreportsize) > 0) {
        switch (buffer[0]) {
        case 0:
            for (size_t i = 0; i < oldposrotreportsize; ++i) {
                printf("%x", buffer[i]);
            }
            break;

        case 1:   // Translation
            evt->type = SPNDEV_MOTION;
            for (int i = 0; i < 3; ++i) {
                evt->mot.v[i] = *(int16_t*)(buffer + 1 + 2 * i);
                checkrange(dev, evt->mot.v[i]);
            }
            break;

        case 2:   // Rotation
            evt->type = SPNDEV_MOTION;
            for (int i = 0; i < 3; ++i) {
                evt->mot.v[i + 3] = *(int16_t*)(buffer + 1 + 2 * i);
                checkrange(dev, evt->mot.v[i + 3]);
            }
            break;
        case 3:  // Buttons
            usbdev_parsebuttons(dev, evt, buffer);
            break;

        default:
            for (size_t i = 0; i < newposrotreportsize; ++i) {
                printf("%x", buffer[i]);
            }
            break;
        }
    }
    return evt->type;
}

// Read and parse "New" (single report) style positon and rotation data
static int usbdev_read_N(struct spndev* dev, union spndev_event* evt)
{
    evt->type = SPNDEV_NONE;
    unsigned char* buffer = ((tspnav_hid*)dev->drvdata)->buf;

    if (hid_read((hid_device*)dev->handle, buffer, newposrotreportsize) > 0) {
        switch (buffer[0]) {
        case 0:
            for (size_t i = 0; i < newposrotreportsize; ++i) {
                printf("%x", buffer[i]);
            }
            break;

        case 1:   // Translation & Rotation
            evt->type = SPNDEV_MOTION;
            for (int i = 0; i < 6; ++i) {
                evt->mot.v[i] = *(int16_t*)(buffer + 1 + 2 * i);
                checkrange(dev, evt->mot.v[i]);
            }
            break;

        case 3:  // Buttons
            usbdev_parsebuttons(dev, evt, buffer);
            break;

        default:
            for (size_t i = 0; i < newposrotreportsize; ++i) {
                printf("%x", buffer[i]);
            }
            break;
        }
    }
    return evt->type;
}

static void usbdev_setled(struct spndev *dev, int led)
{
    const unsigned char led_report[2] = { 4, led };
    hid_write((hid_device*)dev->handle, led_report, sizeof(led_report));
    dev->led = led;
}

static int usbdev_getled(struct spndev *dev)
{
    return dev->led;
}

static inline void usbdev_parsebuttons(const struct spndev* dev, union spndev_event* evt, const unsigned char *report) {
    unsigned char* btn = ((tspnav_hid*)dev->drvdata)->btn;
    int ii = 0;
    for (int j = 0; j < 6; j++)
    {
        for (int k = 0; k < 8; k++)
        {
            if ((1 << k & (unsigned char)*(report + 1 + j)) > 0) {
                if (!btn[ii]) {
                    evt->type = SPNDEV_BUTTON;
                    evt->bn.press = 1;
                    evt->bn.num = ii;
                    btn[ii] = 1;
                }
            }
            else {
                if (btn[ii]) {
                    evt->type = SPNDEV_BUTTON;
                    evt->bn.press = 0;
                    evt->bn.num = ii;
                    btn[ii] = 0;
                }
            }
            ii++;
        }
    }
}

static inline void checkrange(const struct spndev* dev, const int val) {
    if (dev->aprop->maxval < val) {
        fwprintf(stderr, L"Too high %i\n", val);
    } else if (dev->aprop->minval > val) {
        fwprintf(stderr, L"Too low %i\n", val);
    }
}
