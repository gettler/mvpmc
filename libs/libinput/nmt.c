/*
 *  Copyright (C) 2008, Jon Gettler
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
#include <dlfcn.h>
#include <directfb.h>

#include "input.h"
#include "input_local.h"
#include "piutil.h"

static IDirectFB               *dfb = NULL;
static IDirectFBInputDevice    *remote;
static IDirectFBEventBuffer    *input_buffer;

static void *handle = NULL;

static DFBResult (*dl_DirectFBCreate)(IDirectFB **) = NULL;
static DFBResult (*dl_DirectFBInit)(int*, char *(*argv[])) = NULL;
static DFBResult (*dl_DirectFBSetOption)(const char*, const char*) = NULL;
static DFBResult (*dl_DirectFBErrorFatal)(const char*, DFBResult) = NULL;

#define DFBCHECK(x...)                                         \
  {                                                            \
    DFBResult err = x;                                         \
                                                               \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        dl_DirectFBErrorFatal( #x, err );                         \
      }                                                        \
  }

int
input_init_local(void)
{
	if (handle == NULL) {
		if ((handle=pi_register("libdirectfb-1.0.so.0")) == NULL) {
			return -1;
		}

		dl_DirectFBCreate = dlsym(handle, "DirectFBCreate");
		dl_DirectFBInit = dlsym(handle, "DirectFBInit");
		dl_DirectFBSetOption = dlsym(handle, "DirectFBSetOption");
		dl_DirectFBErrorFatal = dlsym(handle, "DirectFBErrorFatal");
	}

	DFBCHECK(dl_DirectFBCreate( &dfb ));
	DFBCHECK(dfb->GetInputDevice(dfb, DIDID_REMOTE, &remote ));
	DFBCHECK(dfb->CreateInputEventBuffer(dfb, DICAPS_ALL, DFB_FALSE, &input_buffer));

	return 0;
}

int
input_release_local(void)
{
	if (input_buffer)
		input_buffer->Release(input_buffer);
	if (remote)
		remote->Release( remote );
	if (dfb)
		dfb->Release( dfb ); 

	input_buffer = NULL;
	remote = NULL;
	dfb = NULL;

	if (handle)
		pi_deregister(handle);

	handle = NULL;

	return 0;
}

input_t*
input_open_kbd(int flags)
{
	input_t *input;
	int fd;

	DFBCHECK(input_buffer->CreateFileDescriptor(input_buffer, &fd));

	if ((input=(input_t*)malloc(sizeof(*input))) == NULL) {
		return NULL;
	}
	memset(input, 0, sizeof(*input));

	input->type = INPUT_KEYBOARD;
	input->fd = fd;

	return input;
}

int
input_read_kbd(input_t *handle, int raw)
{
	DFBEvent event;

	read(handle->fd, &event, sizeof(event));

	if (event.input.type == DIET_KEYPRESS) {
		int cmd;

		switch (event.input.key_symbol) {
		case DIKS_CURSOR_UP:
			cmd = INPUT_CMD_UP;
			break;
		case DIKS_CURSOR_DOWN:
			cmd = INPUT_CMD_DOWN;
			break;
		case 0xd: /* return */
		case DIKS_CURSOR_LEFT:
			cmd = INPUT_CMD_LEFT;
			break;
		case DIKS_CURSOR_RIGHT:
			cmd = INPUT_CMD_RIGHT;
			break;
		case DIKS_SELECT:
			cmd = INPUT_CMD_SELECT;
			break;
		case DIKS_PLAY:
			cmd = INPUT_CMD_PLAY;
			break;
		case DIKS_STOP:
			cmd = INPUT_CMD_STOP;
			break;
		default:
			fprintf(stderr, "Unknown key: %x\n",
				event.input.key_symbol);
			cmd = INPUT_CMD_ERROR;
			break;
		}

		return cmd;
	}

	return INPUT_CMD_ERROR;
}

const char*
input_key_name(int key)
{
	return NULL;
}
