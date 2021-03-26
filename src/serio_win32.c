/*
libspnavdev - RS-232 access with Win32 API
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

#ifdef _WIN32

#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>
#include "serio.h"

#define checkerror(funccall, msg) \
        if(!funccall) { \
            print_error(msg); \
            serclose((int)h); \
            return -1; \
        }

static void print_error(const char* context);

int seropen(char const* devstr) {
    HANDLE h;
    COMMTIMEOUTS CommTimeouts;
    DCB dcb;

    h = CreateFile(devstr /* lpFileName */, GENERIC_READ | GENERIC_WRITE /* dwDesiredAccess */,
                   0 /* dwShareMode */, NULL /* lpSecurityAttributes */,
                   OPEN_EXISTING /* dwCreationDisposition */,
                   FILE_ATTRIBUTE_NORMAL /* dwFlagsAndAttributes */, NULL /* hTemplateFile */
    );

    if(INVALID_HANDLE_VALUE == h) {
        print_error("Could not open serial device");
        return -1;
    }

    // Flush away any bytes previously read or written.
    checkerror(FlushFileBuffers(h), "Failed to flush serial port");

    // https://docs.microsoft.com/en-us/windows-hardware/drivers/serports/setting-read-and-write-timeouts-for-a-serial-device
    CommTimeouts.ReadIntervalTimeout         = 2 * 1000 * 8 / 9600; // 9600kbps * 2 for margin
    CommTimeouts.ReadTotalTimeoutMultiplier  = 1;
    CommTimeouts.ReadTotalTimeoutConstant    = 1;
    CommTimeouts.WriteTotalTimeoutMultiplier = 2 * 1000 * 8 / 9600;
    CommTimeouts.WriteTotalTimeoutConstant   = 0;
    checkerror(SetCommTimeouts(h, &CommTimeouts), "Failed to set serial timeouts");

    // https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/cc732236(v=ws.11)
    // BuildCommDCBAndTimeouts("baud=9600 parity=N data=8 stop=1 xon=on to=on", &dcb, &CommTimeouts);  // does not do what I need
    dcb.DCBlength = sizeof(DCB);
    checkerror(GetCommState(h, &dcb), "Failed to get serial state");
    BuildCommDCB("baud=9600 parity=N data=8 stop=1 xon=on dtr=on rts=on", &dcb); //     RTS_CONTROL_ENABLE
    checkerror(SetCommState(h, &dcb), "Failed to set serial state");

    return (int)h;
}

int serread(int h, void* buf, unsigned int len) {
    DWORD read;
    if(ReadFile((void*)h, buf, len, &read, NULL)) {
        return read;
    } else {
        print_error("Read failed");
        return -1;
    };
}

int serwrite(int h, void const* buf, unsigned int len) {
    DWORD written;
    if(WriteFile((void*)h, buf, len, &written, NULL)) {
        if(written != len) {
            print_error("Not all written");
        }
        return written;
    } else {
        print_error("Write failed");
        return -1;
    };
}

int serclose(int h) {
    if(CloseHandle((void*)h)) {
        return 0;
    } else {
        print_error("Close failed");
        return -1;
    }
}

static void print_error(const char* context) {
    DWORD error_code = GetLastError();
    char buffer[256];
    DWORD size =
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, error_code,
                       MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), buffer, sizeof(buffer), NULL);
    if(size == 0) {
        buffer[0] = 0;
    }
    fprintf(stderr, "%s: %s\n", context, buffer);
}

#endif