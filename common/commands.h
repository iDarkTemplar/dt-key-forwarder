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

#ifndef DTKEY_COMMANDS_H
#define DTKEY_COMMANDS_H

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>

#ifdef KEYDATA_HAS_LENGTH
#include <stddef.h>
#endif /* KEYDATA_HAS_LENGTH */

#define dtkey_command_max_length 4096

#define dtkey_command_key_press "key_press"
#define dtkey_command_key_release "key_release"

typedef struct KeyData
{
	KeySym keysym;
	char *string;
#ifdef KEYDATA_HAS_LENGTH
	size_t string_length;
#endif /* KEYDATA_HAS_LENGTH */
} KeyData_t;

#ifdef KEYDATA_HAS_LENGTH
#define KEYDATA_TAIL_INIT , 0
#else /* KEYDATA_HAS_LENGTH */
#define KEYDATA_TAIL_INIT
#endif /* KEYDATA_HAS_LENGTH */

static KeyData_t grab_keys[] = {
	{ XF86XK_AudioPlay, NULL KEYDATA_TAIL_INIT },
	{ XF86XK_AudioStop, NULL KEYDATA_TAIL_INIT },
	{ XF86XK_AudioPause, NULL KEYDATA_TAIL_INIT },
	{ XF86XK_AudioNext, NULL KEYDATA_TAIL_INIT },
	{ XF86XK_AudioPrev, NULL KEYDATA_TAIL_INIT },
	{ XF86XK_AudioRaiseVolume, NULL KEYDATA_TAIL_INIT },
	{ XF86XK_AudioLowerVolume, NULL KEYDATA_TAIL_INIT },
	{ XF86XK_AudioMute, NULL KEYDATA_TAIL_INIT },
};

#endif /* DTKEY_COMMANDS_H */
