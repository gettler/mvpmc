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
input_read_kbd(input_t *handle)
{
	unsigned int key;
	fd_set fds;

	if (handle->flags & INPUT_BLOCKING) {
		FD_ZERO(&fds);
		FD_SET(handle->fd, &fds);

		select(handle->fd+1, &fds, NULL, NULL, NULL);
	}

	if (read(handle->fd, &key, sizeof(key)) < 0) {
		return -1;
	}

	return key;
}
