/*
 * Copyright (C) 2016 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT Key Forwarder.
 *
 * DT Kernel Cleaner is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DT Key Forwarder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DT Key Forwarder.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XTest.h>

#include <stdlib.h>

#define KEYCODE XF86XK_AudioPlay

int main(int argc, char **argv)
{
	// Obtain the X11 display.
	Display *dpy = XOpenDisplay(getenv("DISPLAY"));
	if(dpy == NULL)
	{
		return -1;
	}

	int event, error, major, minor;
	if (!XTestQueryExtension(dpy, &event, &error, &major, &minor))
	{
		return -1;
	}

	// Send a fake key press event to the window.
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, KEYCODE), True, CurrentTime);
	XSync(dpy, False);

	// Send a fake key release event to the window.
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, KEYCODE), False, CurrentTime);
	XSync(dpy, False);

	XCloseDisplay(dpy);
	return 0;
}
