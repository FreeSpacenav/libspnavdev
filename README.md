spnavdev library
================
You're probably looking for https://github.com/FreeSpacenav/libspnav, the client
library for receiving 6dof input from spacenavd or the proprietary 3dxsrv
spacemouse driver.

libspnavdev a different thing. I meant to extract all the low level device
handling code from spacenavd, and provide an API for both spacenavd and 3rd
party applications who would rather talk to 6dof devices directly to use for
device access. This is still a good future plan, but since I haven't managed to
find the time to finish this transition in 4 years, I decided to drop this
project for the time being, to avoid confusing users who stumble upon it.

It will most likely be resurrected at some point in the future as time allows.
