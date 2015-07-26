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
#include <X11/XF86keysym.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/XKBlib.h>

#include <stdlib.h>
#include <signal.h>

#include <stdio.h>

static volatile int run = 1;

static int xi_opcode;

static KeySym grab_keys[] = {
	XF86XK_AudioPlay,
	XF86XK_AudioStop,
	XF86XK_AudioPause,
	XF86XK_AudioNext,
	XF86XK_AudioPrev,
	XF86XK_AudioRaiseVolume,
	XF86XK_AudioLowerVolume,
	XF86XK_AudioMute
};

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
	int screen;
	int event, error;
	XIEventMask event_mask;

	if (setup_sig_handlers() < 0)
	{
		return 0;
	}

	dpy = XOpenDisplay(getenv("DISPLAY"));
	if (dpy == NULL)
	{
		//PostErrorLog("createGLWindow(): open display error");
		//goto createGLWindow_error_1;
		return 0;
	}

	if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error))
	{
		//printf("X Input extension not available.\n");
		return 0;
	}

	screen = DefaultScreen(dpy);

	event_mask.deviceid = XIAllDevices;
	event_mask.mask_len = XIMaskLen(XI_LASTEVENT);
	event_mask.mask = (unsigned char*) calloc(event_mask.mask_len, sizeof(char));
	if (event_mask.mask == NULL)
	{
		return 0;
	}

	XISetMask(event_mask.mask, XI_KeyPress);
	XISetMask(event_mask.mask, XI_KeyRelease);

	XISelectEvents(dpy, RootWindow(dpy, screen), &event_mask, 1);
	XSync(dpy, False);

	free(event_mask.mask);

	while (run)
	{
		XEvent ev;
		XGenericEventCookie *cookie = (XGenericEventCookie*)&ev.xcookie;
		XNextEvent(dpy, (XEvent*)&ev);

		if ((XGetEventData(dpy, cookie))
		    && (cookie->type == GenericEvent)
		    && (cookie->extension == xi_opcode))
		{
			switch (cookie->evtype)
			{
			case XI_KeyPress:
			case XI_KeyRelease:
				{
					XIDeviceEvent* event = (XIDeviceEvent*) cookie->data;
					KeySym keysym = XkbKeycodeToKeysym(dpy, event->detail, event->group.effective, event->mods.effective);

					int found = 0;

					for (int i = 0; i < sizeof(grab_keys)/sizeof(grab_keys[0]); ++i)
					{
						if (keysym == grab_keys[i])
						{
							found = 1;
							break;
						}
					}

					if (found)
					{
						printf("%d: key %d: %s\n", cookie->evtype, event->detail, XKeysymToString(keysym));
					}
				}
				break;

			default:
				break;
			}
		}

		XFreeEventData(dpy, cookie);
	}

	XCloseDisplay(dpy);

	return 0;
}
