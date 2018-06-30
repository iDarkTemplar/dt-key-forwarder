/*
 * Copyright (C) 2016-2018 i.Dark_Templar <darktemplar@dark-templar-archives.net>
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

#ifndef DTKEY_COMMANDS_H
#define DTKEY_COMMANDS_H

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>

#define dtkey_command_max_length 4096

#define dtkey_command_key_press "key_press"
#define dtkey_command_key_release "key_release"

typedef struct KeyData
{
	KeySym keysym;
	char *string;
} KeyData_t;

static KeyData_t grab_keys[] = {
	{ XF86XK_AudioPlay, NULL },
	{ XF86XK_AudioStop, NULL },
	{ XF86XK_AudioPause, NULL },
	{ XF86XK_AudioNext, NULL },
	{ XF86XK_AudioPrev, NULL },
	{ XF86XK_AudioRaiseVolume, NULL },
	{ XF86XK_AudioLowerVolume, NULL },
	{ XF86XK_AudioMute, NULL },
};

#endif /* DTKEY_COMMANDS_H */
