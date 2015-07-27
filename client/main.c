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

#if (defined OS_FreeBSD)
#define _WITH_DPRINTF
#endif /* (defined OS_FreeBSD) */

#include "common/commands.h"

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/XKBlib.h>

#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>

#include <stdio.h>

static volatile int run = 1;

static int xi_opcode;

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

static void print_help(FILE *stream, const char *name)
{
	fprintf(stream,
	"USAGE: %s OPTIONS\n"
	"\twhere options are one of following:\n"
	"\t--server [-s] -- path to server socket\n"
	"\t--help [-h] -- show this help message\n",
	name);
}

int main(int argc, char **argv)
{
	Display *dpy;
	int screen;
	int event, error;
	XIEventMask event_mask;

	int result = 0;
	size_t i;
	int index;
	int show_help = 0;
	int show_error = 0;
	char *server_socket_path = NULL;

	XEvent ev;
	XGenericEventCookie *cookie;
	XIDeviceEvent *device_event;
	KeySym keysym;

	struct sockaddr_un sockaddr;
	int socket_fd;
	struct pollfd pollfds;

	for (index = 1; index < argc; ++index)
	{
		if (((strcmp(argv[index], "--server") == 0)
			|| (strcmp(argv[index], "-s") == 0))
			&& (index + 1 < argc))
		{
			if (server_socket_path == NULL)
			{
				server_socket_path = argv[++index];
			}
			else
			{
				show_error = 1;
			}
		}
		else if ((strcmp(argv[index], "--help") == 0)
			|| (strcmp(argv[index], "-h") == 0))
		{
			if (show_help == 0)
			{
				show_help = 1;
			}
			else
			{
				show_error = 1;
			}
		}
		else
		{
			show_error = 1;
		}
	}

	if (show_error)
	{
		fprintf(stderr, "Failed to parse options\n");
		print_help(stderr, argv[0]);
		result = -1;
		goto error_1;
	}
	else if (show_help)
	{
		print_help(stdout, argv[0]);
		goto error_1;
	}
	else if (server_socket_path == NULL)
	{
		fprintf(stderr, "Server socket path is not specified\n");
		print_help(stderr, argv[0]);
		result = -1;
		goto error_1;
	}

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

	dpy = XOpenDisplay(getenv("DISPLAY"));
	if (dpy == NULL)
	{
		fprintf(stderr, "Failed to open display\n");
		result = -1;
		goto error_1;
	}

	if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error))
	{
		fprintf(stderr, "X Input extension not available\n");
		result = -1;
		goto error_2;
	}

	screen = DefaultScreen(dpy);

	event_mask.deviceid = XIAllDevices;
	event_mask.mask_len = XIMaskLen(XI_LASTEVENT);
	event_mask.mask = (unsigned char*) calloc(event_mask.mask_len, sizeof(char));
	if (event_mask.mask == NULL)
	{
		fprintf(stderr, "Memory allocation error\n");
		result = -1;
		goto error_2;
	}

	XISetMask(event_mask.mask, XI_KeyPress);
	XISetMask(event_mask.mask, XI_KeyRelease);

	XISelectEvents(dpy, RootWindow(dpy, screen), &event_mask, 1);
	XSync(dpy, False);

	free(event_mask.mask);

	socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		fprintf(stderr, "Failed to open socket\n");
		result = -1;
		goto error_2;
	}

	sockaddr.sun_family = AF_LOCAL;
	memset(sockaddr.sun_path, 0, sizeof(sockaddr.sun_path));
	strncpy(sockaddr.sun_path, server_socket_path, sizeof(sockaddr.sun_path) - 1);

	if (connect(socket_fd, (struct sockaddr*) &sockaddr, sizeof(struct sockaddr_un)) < 0)
	{
		fprintf(stderr, "Failed to connect to server\n");
		result = -1;
		goto error_3;
	}

	cookie = (XGenericEventCookie*)&ev.xcookie;

	while (run)
	{
		if (XPending(dpy) > 0)
		{
			XNextEvent(dpy, (XEvent*)&ev);

			if ((XGetEventData(dpy, cookie))
				&& (cookie->type == GenericEvent)
				&& (cookie->extension == xi_opcode))
			{
				switch (cookie->evtype)
				{
				case XI_KeyPress:
				case XI_KeyRelease:
					device_event = (XIDeviceEvent*) cookie->data;
					keysym = XkbKeycodeToKeysym(dpy, device_event->detail, 0, 0);

					for (i = 0; i < sizeof(grab_keys)/sizeof(grab_keys[0]); ++i)
					{
						if (keysym == grab_keys[i].keysym)
						{
							break;
						}
					}

					if (i < sizeof(grab_keys)/sizeof(grab_keys[0]))
					{
						dprintf(socket_fd, "%s(\"%s\")\n", (cookie->evtype == XI_KeyPress) ? dtkey_command_key_press : dtkey_command_key_release , grab_keys[i].string);
					}
					break;

				default:
					break;
				}
			}

			XFreeEventData(dpy, cookie);
		}
		else
		{
			pollfds.fd = socket_fd;
			pollfds.events = POLLIN;
			pollfds.revents = 0;

			// sleep 10ms and check if server quit
			index = poll(&pollfds, 1, 10);

			if ((pollfds.revents & POLLHUP) || (pollfds.revents & POLLERR) || (pollfds.revents & POLLNVAL))
			{
				run = 0;
			}
		}
	}

error_3:
	close(socket_fd);

error_2:
	XCloseDisplay(dpy);

error_1:
	return result;
}
