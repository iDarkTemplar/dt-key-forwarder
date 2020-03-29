/*
 * Copyright (C) 2016-2020 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT Key Forwarder.
 *
 * DT Key Forwarder is free software: you can redistribute it and/or modify
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

#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include <stdio.h>

#include <dt-command.h>

#include "common/commands.h"

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

static void print_help(FILE *stream, const char *name)
{
	fprintf(stream,
	"USAGE: %s OPTIONS\n"
	"\twhere options are one of following:\n"
	"\t--socket [-s] -- path to create a socket at\n"
	"\t--lock [-l] -- path to create and hold lock at\n"
	"\t--help [-h] -- show this help message\n",
	name);
}

struct client
{
	int clientfd;
	size_t buf_used;
	char buf[dtkey_command_max_length + 1];
};

static struct client **clients = NULL;
static size_t clients_count = 0;

static int add_client(int client)
{
	size_t i;
	struct client *cur_client;
	struct client **tmp;

	for (i = 0; i < clients_count; ++i)
	{
		if (clients[i]->clientfd == client)
		{
			return 0;
		}
	}

	cur_client = (struct client*) malloc(sizeof(struct client));
	if (cur_client == NULL)
	{
		fprintf(stderr, "Memory allocation failure\n");
		goto add_client_error_1;
	}

	cur_client->clientfd = client;
	cur_client->buf_used = 0;

	tmp = (struct client**) realloc(clients, sizeof(struct client*)*(clients_count+1));
	if (tmp == NULL)
	{
		fprintf(stderr, "Memory allocation failure\n");
		goto add_client_error_2;
	}

	++clients_count;
	clients = tmp;
	clients[clients_count-1] = cur_client;

	return 1;

add_client_error_2:
	free(cur_client);

add_client_error_1:
	shutdown(client, SHUT_RDWR);
	close(client);
	return -1;
}

static int remove_client(int client)
{
	size_t i;
	struct client **tmp;
	struct client *del;

	for (i = 0; i < clients_count; ++i)
	{
		if (clients[i]->clientfd == client)
		{
			del = clients[i];
			--clients_count;

			if (clients_count > 0)
			{
				clients[i] = clients[clients_count];
				clients[clients_count] = del;

				// NOTE: consider non-fatal error on shrinking realloc failure
				tmp = (struct client**) realloc(clients, sizeof(struct client*)*clients_count);
				if (tmp != NULL)
				{
					clients = tmp;
				}
			}
			else
			{
				free(clients);
				clients = NULL;
			}

			shutdown(del->clientfd, SHUT_RDWR);
			close(del->clientfd);
			free(del);

			return 1;
		}
	}

	return 0;
}

static void remove_all_clients(void)
{
	size_t i;

	if (clients != NULL)
	{
		for (i = 0; i < clients_count; ++i)
		{
			shutdown(clients[i]->clientfd, SHUT_RDWR);
			close(clients[i]->clientfd);
			free(clients[i]);
		}

		free(clients);

		clients_count = 0;
		clients = NULL;
	}
}

int main(int argc, char **argv)
{
	Display *dpy;
	int event, error, major, minor;

	int result = 0;
	size_t i, j;
	int index;
	int show_help = 0;
	int show_error = 0;
	char *server_socket_path = NULL;
	char *lock_path = NULL;

	const int backlog = 4;
	int rc;
	struct sockaddr_un sockaddr;
	char buffer[12];
	int lockfd;
	int socket_fd;

#define pollfds_count_default 1
	size_t pollfds_count = pollfds_count_default;
	struct pollfd *pollfds = NULL;
	void *tmp;
	char *tmp_str;
	dt_command_t *cmd;
	size_t key_index;
	Bool key_action;

	for (index = 1; index < argc; ++index)
	{
		if (((strcmp(argv[index], "--socket") == 0)
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
		else if (((strcmp(argv[index], "--lock") == 0)
			|| (strcmp(argv[index], "-l") == 0))
			&& (index + 1 < argc))
		{
			if (lock_path == NULL)
			{
				lock_path = argv[++index];
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
		fprintf(stderr, "Socket path is not specified\n");
		print_help(stderr, argv[0]);
		result = -1;
		goto error_1;
	}
	else if (lock_path == NULL)
	{
		fprintf(stderr, "Lock path is not specified\n");
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

	umask(0);

	socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		fprintf(stderr, "Error opening socket\n");
		result = -1;
		goto error_2;
	}

	sockaddr.sun_family = AF_LOCAL;
	memset(sockaddr.sun_path, 0, sizeof(sockaddr.sun_path));
	strncat(sockaddr.sun_path, server_socket_path, sizeof(sockaddr.sun_path) - 1);

	lockfd = open(lock_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (lockfd == -1)
	{
		fprintf(stderr, "Error obtaining lock file\n");
		result = -1;
		goto error_3;
	}

	rc = flock(lockfd, LOCK_EX | LOCK_NB);
	if (rc < 0)
	{
		fprintf(stderr, "Error locking lock file\n");
		result = -1;
		goto error_4;
	}

	unlink(sockaddr.sun_path);
	rc = bind(socket_fd, (struct sockaddr*) &sockaddr, sizeof(struct sockaddr_un));
	if (rc < 0)
	{
		fprintf(stderr, "Error binding socket\n");
		result = -1;
		goto error_4;
	}

	snprintf(buffer, sizeof(buffer), "%10d\n", getpid());
	if (write(lockfd, buffer, sizeof(buffer) - 1) != sizeof(buffer) - 1)
	{
		fprintf(stderr, "Error writing process pid to lockfile\n");
		result = -1;
		goto error_5;
	}

#if (defined _POSIX_SYNCHRONIZED_IO) && (_POSIX_SYNCHRONIZED_IO > 0)
	if (fdatasync(lockfd) != 0)
#else
	if (fsync(lockfd) != 0)
#endif
	{
		fprintf(stderr, "Error synchronizing lockfile\n");
		result = -1;
		goto error_5;
	}

	if (listen(socket_fd, backlog) < 0)
	{
		fprintf(stderr, "Error listening socket\n");
		result = -1;
		goto error_5;
	}

	pollfds = (struct pollfd*) malloc(sizeof(struct pollfd)*pollfds_count);
	if (pollfds == NULL)
	{
		fprintf(stderr, "Memory allocation failure\n");
		result = -1;
		goto error_5;
	}

	while (run)
	{
		pollfds[0].fd = socket_fd;
		pollfds[0].events = POLLIN;
		pollfds[0].revents = 0;

		for (i = 0; i < clients_count; ++i)
		{
			pollfds[i + pollfds_count_default].fd      = clients[i]->clientfd;
			pollfds[i + pollfds_count_default].events  = POLLIN;
			pollfds[i + pollfds_count_default].revents = 0;
		}

		rc = poll(pollfds, pollfds_count, -1);
		if (rc == -1)
		{
			if (errno != EINTR)
			{
				fprintf(stderr, "Poll failed, errno %d\n", errno);
				result = -1;
				goto error_6;
			}

			continue;
		}

		if ((pollfds[0].revents & POLLHUP) || (pollfds[0].revents & POLLERR) || (pollfds[0].revents & POLLNVAL))
		{
			fprintf(stderr, "Invalid poll result on client socket\n");
			result = -1;
			goto error_6;
		}
		else if (pollfds[0].revents & POLLIN)
		{
			rc = accept(socket_fd, NULL, NULL);
			if (rc < 0)
			{
				fprintf(stderr, "Accepting client failed, errno %d\n", errno);
				result = -1;
				goto error_6;
			}

			rc = add_client(rc);
			if (rc < 0)
			{
				result = -1;
				goto error_6;
			}
		}

		for (i = pollfds_count_default; i < pollfds_count; ++i)
		{
			if ((pollfds[i].revents & POLLHUP) || (pollfds[i].revents & POLLERR) || (pollfds[i].revents & POLLNVAL))
			{
				remove_client(pollfds[i].fd);
			}
			else if (pollfds[i].revents & POLLIN)
			{
				for (j = 0; j < clients_count; ++j)
				{
					if (clients[j]->clientfd == pollfds[i].fd)
					{
						break;
					}
				}

				if (j == clients_count)
				{
					fprintf(stderr, "BUG: could not find non-existent client\n");
					result = -1;
					goto error_6;
				}

				rc = read(clients[j]->clientfd, &(clients[j]->buf[clients[j]->buf_used]), dtkey_command_max_length - clients[j]->buf_used);
				if ((rc <= 0) && (errno != EINTR))
				{
					remove_client(pollfds[i].fd);
					continue;
				}

				clients[j]->buf_used += rc;
				clients[j]->buf[clients[j]->buf_used] = 0;

				while ((tmp_str = strchr(clients[j]->buf, '\n')) != NULL)
				{
					rc = dt_validate_command(clients[j]->buf);
					if (!rc)
					{
						goto exit_remove_client;
					}

					cmd = dt_parse_command(clients[j]->buf);
					if (cmd == NULL)
					{
						fprintf(stderr, "Memory allocation failure");
						result = -1;
						goto error_6;
					}

					if (strcmp(cmd->cmd, dtkey_command_key_press) == 0)
					{
						key_action = True;
					}
					else if (strcmp(cmd->cmd, dtkey_command_key_release) == 0)
					{
						key_action = False;
					}
					else
					{
						goto finish_processing_command;
					}

					if ((cmd->args_count == 1) && (cmd->args[0] != NULL))
					{
						for (key_index = 0; key_index < sizeof(grab_keys)/sizeof(grab_keys[0]); ++key_index)
						{
							if (strcmp(grab_keys[key_index].string, cmd->args[0]) == 0)
							{
								break;
							}
						}

						if (key_index < sizeof(grab_keys)/sizeof(grab_keys[0]))
						{
							// Send a fake key event to the window.
							XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, grab_keys[key_index].keysym), key_action, CurrentTime);
							XSync(dpy, False);
						}
					}

finish_processing_command:
					dt_free_command(cmd);

					clients[j]->buf_used -= (tmp_str + 1 - clients[j]->buf);
					memmove(clients[j]->buf, tmp_str+1, clients[j]->buf_used + 1);
				}

				if (clients[j]->buf_used == dtkey_command_max_length)
				{
exit_remove_client:
					remove_client(pollfds[i].fd);
				}
			}
		}

		if (pollfds_count != pollfds_count_default + clients_count)
		{
			pollfds_count = pollfds_count_default + clients_count;
			tmp = realloc(pollfds, sizeof(struct pollfd)*pollfds_count);
			if (tmp == NULL)
			{
				fprintf(stderr, "Memory allocation failure\n");
				result = -1;
				goto error_6;
			}

			pollfds = (struct pollfd*) tmp;
		}
	}

error_6:
	remove_all_clients();
	free(pollfds);

error_5:
	unlink(sockaddr.sun_path);
	close(socket_fd);

	unlink(lock_path);
	close(lockfd);
	goto error_2;

error_4:
	unlink(lock_path);
	close(lockfd);

error_3:
	close(socket_fd);

error_2:
	XCloseDisplay(dpy);

error_1:
	return result;
}
