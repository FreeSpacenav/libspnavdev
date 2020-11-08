spnavdev library
================
This is a direct 6dof device handling library, which will eventually replace the
device handling code in spacenavd v2, and will also be usable as a standalone
library to interface with 6dof devices without relying on spacenavd in cases
where that is preferable (embedded applications, robot controllers, non-UNIX
systems etc).

The goal of spnavdev is to handle all USB and serial 6dof devices, across
multiple platforms such as: all UNIX systems, windows, DOS, and possibly even
bare metal on certain systems.

Currently this is work in progress. Not done, and therefore no releases yet.

License
-------
Copyright (C) 2020 John Tsiombikas <nuclear@member.fsf.org>

This library is free software, feel free to use, modify and/or redistribute it
under the terms of the GNU General Public License v3, or at your option any
later version published by the Free Software Foundation. See COPYING for
details.

Releasing this under the GPL is deliberate. At the moment I don't have any
immediate plans on allowing use in proprietary software. If you want to get 6dof
input in your proprietary program, first of all please reconsider your life
choices, and if you insist, use libspnav instead which relies on spacenavd, and
is available under the 3-clause BSD license.

If you want to use this in a free software program with a GPL-incompatible
license, please contact me and we'll figure out a way to facilitate that,
possibly adding a license-specific exception, or adding a dual-license.
