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
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <plugin.h>
#include <gw.h>

#include "myth_local.h"

volatile cmyth_conn_t control;

static int tcp_control = 4096;

char *server;

int
conn_open(char *host, int portnum)
{
	int port = 6543;

	if (portnum > 0) {
		port = portnum;
	}
	if (host == NULL) {
		host = server;
	}

	if ((control=cmyth_conn_connect_ctrl(host, port, 16*1024,
					     tcp_control)) == NULL) {
		return -1;
	}

	return 0;
}

int
conn_close(void)
{
	CHANGE_GLOBAL_REF(control, NULL);

	return 0;
}
