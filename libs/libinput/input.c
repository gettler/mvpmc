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

#include "input.h"
#include "input_local.h"

int
input_init(void)
{
	return input_init_local();
}

int
input_release(void)
{
	return input_release_local();
}

input_t*
input_open(input_type_t type, int flags)
{
	switch (type) {
	case INPUT_KEYBOARD:
		return input_open_kbd(flags);
		break;
	case INPUT_MOUSE:
		break;
	}

	return NULL;
}

int
input_read(input_t *handle)
{
	switch (handle->type) {
	case INPUT_KEYBOARD:
		return input_read_kbd(handle, 0);
		break;
	case INPUT_MOUSE:
		break;
	}

	return -1;
}

int
input_read_raw(input_t *handle)
{
	switch (handle->type) {
	case INPUT_KEYBOARD:
		return input_read_kbd(handle, 1);
		break;
	case INPUT_MOUSE:
		break;
	}

	return -1;
}

int
input_test(input_t *handle)
{
	return 0;
}

int
input_fd(input_t *handle)
{
	if (handle == NULL)
		return -1;

	return handle->fd;
}
