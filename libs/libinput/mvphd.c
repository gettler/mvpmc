/*
 *  Copyright (C) 2010, Jon Gettler
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

#include "input.h"
#include "input_local.h"

int
input_init_local(void)
{
	return 0;
}

int
input_release_local(void)
{
	return 0;
}

input_t*
input_open_kbd(int flags)
{
	return NULL;
}

int
input_read_kbd(input_t *handle, int raw)
{
	return INPUT_CMD_ERROR;
}

const char*
input_key_name(int key)
{
	return NULL;
}
