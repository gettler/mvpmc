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

#include "ir.h"

static ir_key_t codes[] = {
	{ KEY_IR_OLD_POWER, "Power (Old Remote)" },
	{ KEY_IR_OLD_GO, "Go (Old Remote)" },
	{ KEY_IR_OLD_ONE, "1 (Old Remote)" },
	{ KEY_IR_OLD_TWO, "2 (Old Remote)" },
	{ KEY_IR_OLD_THREE, "3 (Old Remote)" },
	{ KEY_IR_OLD_FOUR, "4 (Old Remote)" },
	{ KEY_IR_OLD_FIVE, "5 (Old Remote)" },
	{ KEY_IR_OLD_SIX, "6 (Old Remote)" },
	{ KEY_IR_OLD_SEVEN, "7 (Old Remote)" },
	{ KEY_IR_OLD_EIGHT, "8 (Old Remote)" },
	{ KEY_IR_OLD_NINE, "9 (Old Remote)" },
	{ KEY_IR_OLD_ZERO, "0 (Old Remote)" },
	{ KEY_IR_OLD_BACK, "Back/Exit (Old Remote)" },
	{ KEY_IR_OLD_MENU, "Menu (Old Remote)" },
	{ KEY_IR_OLD_RED, "Red (Old Remote)" },
	{ KEY_IR_OLD_YELLOW, "Yellow (Old Remote)" },
	{ KEY_IR_OLD_GREEN, "Green (Old Remote)" },
	{ KEY_IR_OLD_BLUE, "Blue (Old Remote)" },
	{ KEY_IR_OLD_UP, "Up/CH+ (Old Remote)" },
	{ KEY_IR_OLD_DOWN, "Down/CH- (Old Remote)" },
	{ KEY_IR_OLD_LEFT, "Left/Vol- (Old Remote)" },
	{ KEY_IR_OLD_RIGHT, "Right/Vol+ (Old Remote)" },
	{ KEY_IR_OLD_OK, "OK (Old Remote)" },
	{ KEY_IR_OLD_MUTE, "Mute (Old Remote)" },
	{ KEY_IR_OLD_BLANK, "Blank (Old Remote)" },
	{ KEY_IR_OLD_FULL, "Full (Old Remote)" },
	{ KEY_IR_OLD_REWIND, "Rewind (Old Remote)" },
	{ KEY_IR_OLD_PLAY, "Play (Old Remote)" },
	{ KEY_IR_OLD_FFWD, "Fast Forward (Old Remote)" },
	{ KEY_IR_OLD_REC, "Record (Old Remote)" },
	{ KEY_IR_OLD_STOP, "Stop (Old Remote)" },
	{ KEY_IR_OLD_PAUSE, "Pause (Old Remote)" },
	{ KEY_IR_OLD_REPLAY, "Replay (Old Remote)" },
	{ KEY_IR_OLD_SKIP, "Skip (Old Remote)" },
	{ KEY_IR_NEW_POWER, "Power (New Remote)" },
	{ KEY_IR_NEW_GO, "Go (New Remote)" },
	{ KEY_IR_NEW_TV, "TV (New Remote)" },
	{ KEY_IR_NEW_VIDEOS, "Videos (New Remote)" },	
	{ KEY_IR_NEW_MUSIC, "Music (New Remote)" },	
	{ KEY_IR_NEW_PICTURES, "Pictures (New Remote)" },	
	{ KEY_IR_NEW_GUIDE, "Guide (New Remote)" },	
	{ KEY_IR_NEW_RADIO, "Radio (New Remote)" },	
	{ KEY_IR_NEW_UP, "Up (New Remote)" },
	{ KEY_IR_NEW_DOWN, "Down (New Remote)" },
	{ KEY_IR_NEW_LEFT, "Left (New Remote)" },
	{ KEY_IR_NEW_RIGHT, "Right (New Remote)" },	
	{ KEY_IR_NEW_OK, "OK (New Remote)" },
	{ KEY_IR_NEW_BACK, "Back/Exit (New Remote)" },
	{ KEY_IR_NEW_MENU, "Menu (New Remote)" },
	{ KEY_IR_NEW_VOLUP, "Volume Up (New Remote)" },	
	{ KEY_IR_NEW_VOLDOWN, "Volume Down (New Remote)" },	
	{ KEY_IR_NEW_CHANUP, "Channel Up (New Remote)" },
	{ KEY_IR_NEW_CHANDOWN, "Channel Down (New Remote)" },
	{ KEY_IR_NEW_PREVCHAN, "Previous Channel (New Remote)" },
	{ KEY_IR_NEW_MUTE, "Mute (New Remote)" },
	{ KEY_IR_NEW_REC, "Record (New Remote)" },
	{ KEY_IR_NEW_STOP, "Stop (New Remote)" },
	{ KEY_IR_NEW_REPLAY, "Replay (New Remote)" },	
	{ KEY_IR_NEW_SKIP, "Skip (New Remote)" },
	{ KEY_IR_NEW_REWIND, "Rewind (New Remote)" },	
	{ KEY_IR_NEW_FFWD, "Fast Forward (New Remote)" },
	{ KEY_IR_NEW_PLAY, "Play (New Remote)" },
	{ KEY_IR_NEW_PAUSE, "Pause (New Remote)" },	
	{ KEY_IR_NEW_ONE, "1 (New Remote)" },
	{ KEY_IR_NEW_TWO, "2 (New Remote)" },
	{ KEY_IR_NEW_THREE, "3 (New Remote)" },
	{ KEY_IR_NEW_FOUR, "4 (New Remote)" },
	{ KEY_IR_NEW_FIVE, "5 (New Remote)" },
	{ KEY_IR_NEW_SIX, "6 (New Remote)" },
	{ KEY_IR_NEW_SEVEN, "7 (New Remote)" },
	{ KEY_IR_NEW_EIGHT, "8 (New Remote)" },
	{ KEY_IR_NEW_NINE, "9 (New Remote)" },
	{ KEY_IR_NEW_ZERO, "0 (New Remote)" },
	{ KEY_IR_NEW_TEXT, "Text (New Remote)" },
	{ KEY_IR_NEW_SUBTITLE, "Subtitle/CC (New Remote)" },	
	{ KEY_IR_NEW_RED, "Red (New Remote)" },
	{ KEY_IR_NEW_GREEN, "Green (New Remote)" },
	{ KEY_IR_NEW_YELLOW, "Yellow (New Remote)" },	
	{ KEY_IR_NEW_BLUE, "Blue (New Remote)" },
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
	unsigned int f = O_RDONLY;
	int fd;

	if ((f & INPUT_BLOCKING) == 0)
		f |= O_NONBLOCK;

	if ((fd=open("/dev/rawir", f)) < 0) {
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

	if (read(handle->fd, &key, sizeof(key)) < 0) {
		return -1;
	}

	if (raw) {
		return key;
	}

	switch (key & 0xffff0000) {
	case KEY_IR_OLD_POWER:
	case KEY_IR_NEW_POWER:
		cmd = INPUT_CMD_POWER;
		break;
	case KEY_IR_OLD_UP:
	case KEY_IR_NEW_UP:
		cmd = INPUT_CMD_UP;
		break;
	case KEY_IR_OLD_DOWN:
	case KEY_IR_NEW_DOWN:
		cmd = INPUT_CMD_DOWN;
		break;
	case KEY_IR_OLD_OK:
	case KEY_IR_NEW_OK:
		cmd = INPUT_CMD_SELECT;
		break;
	case KEY_IR_OLD_LEFT:
	case KEY_IR_NEW_LEFT:
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
		if (codes[i].code == (key&0xffff0000)) {
			return codes[i].name;
		}
		i++;
	}

	return NULL;
}
