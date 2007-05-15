/*
 *  Copyright (C) 2007, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

#include "input.h"
#include "input_local.h"

#include "fip.h"

static fip_key_t codes[] = {
	{ KEY_FIP_UP, "FIP Up" },
	{ KEY_FIP_DOWN, "FIP Down" },
	{ KEY_FIP_LEFT, "FIP Left" },
	{ KEY_FIP_RIGHT, "FIP Right" },
	{ KEY_FIP_ENTER, "FIP Enter" },
	{ KEY_FIP_POWER, "FIP Power" },
	{ KEY_FIP_PLAY, "FIP Play/Pause" },
	{ KEY_FIP_STOP, "FIP Stop/Init" },
	{ KEY_IR_POWER, "IR Power" },
	{ KEY_IR_MENU, "IR Menu" },
	{ KEY_IR_TITLE, "IR Title" },
	{ KEY_IR_SETUP, "IR Setup" },
	{ KEY_IR_ANGLE, "IR Angle" },
	{ KEY_IR_REPEAT, "IR Repeat" },
	{ KEY_IR_ABREPEAT, "IR A-B Repeat" },
	{ KEY_IR_SLOW, "IR Slow" },
	{ KEY_IR_FILEINFO, "IR File Info" },
	{ KEY_IR_TIMESRCH, "IR Time Search" },
	{ KEY_IR_BCONT, "IR Brt. Cont" },
	{ KEY_IR_MEDIA, "IR Media Type" },
	{ KEY_IR_UP, "IR Up" },
	{ KEY_IR_DOWN, "IR Down" },
	{ KEY_IR_LEFT, "IR Left" },
	{ KEY_IR_RIGHT, "IR Right" },
	{ KEY_IR_ENTER, "IR Enter" },
	{ KEY_IR_MAINPAGE, "IR Main Page" },
	{ KEY_IR_PLAY, "IR Play/Pause" },
	{ KEY_IR_STOP, "IR Stop" },
	{ KEY_IR_REWIND, "IR Rewind" },
	{ KEY_IR_FF, "IR Fast Forward" },
	{ KEY_IR_PREV, "IR Prev" },
	{ KEY_IR_NEXT, "IR Next" },
	{ KEY_IR_VIDEO, "IR Video" },
	{ KEY_IR_AUDIO, "IR Audio" },
	{ KEY_IR_SUBTITLE, "IR Subtitle" },
	{ KEY_IR_SCRSIZE, "IR Screen Size" },
	{ KEY_IR_ONE, "IR 1" },
	{ KEY_IR_TWO, "IR 2" },
	{ KEY_IR_THREE, "IR 3" },
	{ KEY_IR_FOUR, "IR 4" },
	{ KEY_IR_FIVE, "IR 5" },
	{ KEY_IR_SIX, "IR 6" },
	{ KEY_IR_SEVEN, "IR 7" },
	{ KEY_IR_EIGHT, "IR 8" },
	{ KEY_IR_NINE, "IR 9" },
	{ KEY_IR_ZERO, "IR 0" },
	{ KEY_IR_VOLUP, "IR Volume Up" },
	{ KEY_IR_VOLDOWN, "IR Volume Down" },
	{ KEY_IR_MUTE, "IR Mute" },
	{ KEY_IR_CANCEL, "IR Cancel" },
	{ 0, NULL },
};

int
input_init_local(void)
{
	return 0;
}

input_t*
input_open_kbd(int flags)
{
	input_t *input;
	int fd;

	if ((fd=open("/dev/fip", O_RDONLY)) < 0) {
		return NULL;
	}

	if (ioctl(fd, 0x00450003, 0) < 0) {
		return NULL;
	}

	if ((input=(input_t*)malloc(sizeof(*input))) == NULL) {
		return NULL;
	}

	input->type = INPUT_KEYBOARD;
	input->flags = flags;
	input->fd = fd;

	return input;
}

int
input_read_kbd(input_t *handle, int raw)
{
	unsigned int key;
	int cmd;
	fd_set fds;

	if (handle->flags & INPUT_BLOCKING) {
		FD_ZERO(&fds);
		FD_SET(handle->fd, &fds);

		select(handle->fd+1, &fds, NULL, NULL, NULL);
	}

	if (ioctl(handle->fd, 0x00450001, &key) < 0)
		return -1;

	if (raw) {
		return key;
	}

	switch (key) {
	case KEY_FIP_POWER:
	case KEY_IR_POWER:
		cmd = INPUT_CMD_POWER;
		break;
	case KEY_FIP_UP:
	case KEY_IR_UP:
		cmd = INPUT_CMD_UP;
		break;
	case KEY_FIP_DOWN:
	case KEY_IR_DOWN:
		cmd = INPUT_CMD_DOWN;
		break;
	case KEY_FIP_ENTER:
	case KEY_IR_ENTER:
		cmd = INPUT_CMD_SELECT;
		break;
	case KEY_FIP_LEFT:
	case KEY_IR_LEFT:
		cmd = INPUT_CMD_LEFT;
		break;
	default:
		cmd = INPUT_CMD_ERROR;
		break;
	}

	return cmd;
}

const char*
input_key_name(int key)
{
	int i = 0;

	while (codes[i].name != NULL) {
		if (codes[i].code == key) {
			return codes[i].name;
		}
		i++;
	}

	return NULL;
}
