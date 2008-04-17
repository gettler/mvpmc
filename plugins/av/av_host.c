/*
 *  Copyright (C) 2008, Jon Gettler
 *  http://www.mvpmc.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>   
#include <sys/socket.h>   
#include <sys/signal.h>   
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>

#include <mvp_string.h>
#include <plugin.h>
#include <plugin/av.h>

#include "av_local.h"

int
arch_init(void)
{
	return 0;
}

int
do_play_file(char *path)
{
	pid_t pid;

	if ((pid=fork()) == 0) {
		int i, max = getdtablesize();

		for (i=0; i<max; i++) {
			close(i);
		}
		open("/dev/null", O_RDONLY);
		open("/dev/null", O_WRONLY);
		open("/dev/null", O_WRONLY);

		execlp("mplayer", "mplayer", path, NULL);
		exit(1);
	}

	info.child = pid;

	return 0;
}

int
do_play_dvd(char *path)
{
	return -1;
}

int
do_play_url(char *path)
{
	return -1;
}

int
do_stop(void)
{
	if (info.playing) {
		info.playing = 0;
		kill(info.child, SIGKILL);
		waitpid(info.child, NULL, 0);
	}

	return 0;
}
