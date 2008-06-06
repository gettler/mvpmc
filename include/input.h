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

#ifndef INPUT_H
#define INPUT_H

typedef struct input_s input_t;

typedef enum {
	INPUT_KEYBOARD=1,
	INPUT_MOUSE,
} input_type_t;

typedef enum {
	INPUT_CMD_ERROR=0,
	INPUT_CMD_POWER,
	INPUT_CMD_UP,
	INPUT_CMD_DOWN,
	INPUT_CMD_LEFT,
	INPUT_CMD_RIGHT,
	INPUT_CMD_SELECT,
	INPUT_CMD_PLAY,
	INPUT_CMD_STOP,
	INPUT_CMD_PAUSE,
} input_cmd_t;

#define INPUT_BLOCKING	0x1

extern int input_init(void);
extern int input_release(void);
extern input_t* input_open(input_type_t type, int flags);
extern int input_read(input_t *handle);
extern int input_read_raw(input_t *handle);
extern int input_test(input_t *handle);
extern const char *input_key_name(int key);
extern int input_fd(input_t*);

#endif /* INPUT_H */
