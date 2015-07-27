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

#include "common/commands.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XTest.h>

#include <stdlib.h>
#include <signal.h>

#include <stdio.h>

#define KEYCODE XF86XK_AudioPlay

static volatile int run = 1;

static void signal_handler(int signum)
{
	switch (signum)
	{
	case SIGTERM:
	case SIGINT:
		run = 0;
		break;
	}
}

static int setup_sig_handlers(void)
{
	struct sigaction action;

	action.sa_handler = signal_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	if (sigaction(SIGTERM, &action, NULL) < 0)
	{
		return -1;
	}

	if (sigaction(SIGINT, &action, NULL) < 0)
	{
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	Display *dpy;
	int event, error, major, minor;

	int result = 0;
	size_t i;

	if (setup_sig_handlers() < 0)
	{
		fprintf(stderr, "Failed to setup sighandlers\n");
		result = -1;
		goto error_1;
	}

	for (i = 0; i < sizeof(grab_keys)/sizeof(grab_keys[0]); ++i)
	{
		grab_keys[i].string = XKeysymToString(grab_keys[i].keysym);
		if (grab_keys[i].string == NULL)
		{
			fprintf(stderr, "Failed to convert keysym %lux to string\n", grab_keys[i].keysym);
			result = -1;
			goto error_1;
		}
	}

	// Obtain the X11 display.
	dpy = XOpenDisplay(getenv("DISPLAY"));
	if (dpy == NULL)
	{
		fprintf(stderr, "Failed to open display\n");
		result = -1;
		goto error_1;
	}

	if (!XTestQueryExtension(dpy, &event, &error, &major, &minor))
	{
		fprintf(stderr, "X Test extension not available\n");
		result = -1;
		goto error_2;
	}

	// Send a fake key press event to the window.
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, KEYCODE), True, CurrentTime);
	XSync(dpy, False);

	// Send a fake key release event to the window.
	XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, KEYCODE), False, CurrentTime);
	XSync(dpy, False);

error_2:
	XCloseDisplay(dpy);

error_1:
	return result;
}
