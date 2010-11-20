/*
 *  Copyright (C) 2007-2010, Jon Gettler
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

#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

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

static gint
snoop(GtkWidget *grab_widget, GdkEventKey *event, gpointer func_data)
{
	gint rc = 0;
	input_t *input;

	input = (input_t*)func_data;

	if (event->type == GDK_KEY_PRESS) {
		input->down = event->keyval;
	} else if (event->type == GDK_KEY_RELEASE) {
		if (input->down == event->keyval) {
			int n;
			n = write(input->fd_write, &event->keyval,
				  sizeof(event->keyval));
			if (n != sizeof(event->keyval)) {
				rc = -1;
			}
		}
		input->down = 0;
	}

	return rc;
}

input_t*
input_open_kbd(int flags)
{
	input_t *input;
	int fds[2];

	if ((input=(input_t*)malloc(sizeof(*input))) == NULL) {
		return NULL;
	}
	memset(input, 0, sizeof(*input));

	if (pipe(fds) != 0) {
		return NULL;
	}

	input->type = INPUT_KEYBOARD;
	input->fd = fds[0];
	input->fd_write = fds[1];

	gtk_key_snooper_install(snoop, (void*)input);

	return input;
}

int
input_read_kbd(input_t *handle, int raw)
{
	guint c;
	int cmd;

	if (read(handle->fd, &c, sizeof(guint)) != sizeof(guint)) {
		return INPUT_CMD_ERROR;
	}

	switch (c) {
	case GDK_Return:
		cmd = INPUT_CMD_SELECT;
		break;
	case GDK_Up:
		cmd = INPUT_CMD_UP;
		break;
	case GDK_Down:
		cmd = INPUT_CMD_DOWN;
		break;
	case GDK_Left:
		cmd = INPUT_CMD_LEFT;
		break;
	case GDK_Right:
		cmd = INPUT_CMD_RIGHT;
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
	return NULL;
}
