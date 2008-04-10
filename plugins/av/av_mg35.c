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

static int
is_audio(char *file)
{
	char *wc[] = { ".mp3", NULL };
	int i = 0;

	while (wc[i] != NULL) {
		if ((strlen(file) >= strlen(wc[i])) &&
		    (strcasecmp(file+strlen(file)-strlen(wc[i]), wc[i]) == 0))
			return 1;
		i++;
	}

	return 0;
}

static int
is_video(char *file)
{
	char *wc[] = { ".mpg", ".mpeg", NULL };
	int i = 0;

	while (wc[i] != NULL) {
		if ((strlen(file) >= strlen(wc[i])) &&
		    (strcasecmp(file+strlen(file)-strlen(wc[i]), wc[i]) == 0))
			return 1;
		i++;
	}

	return 0;
}

int
arch_init(void)
{
	return 0;
}

static int
play_audio_file(char *path)
{
	extern void fb_redisplay(void);
	char cmd[256];

	snprintf(cmd,sizeof(cmd), "/fileplayer.bin MP3 '%s'", path);

	printf("play MP3: '%s'\n", cmd);

	if (vfork() == 0) {
		system(cmd);
		_exit(0);
	} else {
		wait(NULL);
	}

	fb_redisplay();

	return 0;
}

static int
play_video_file(char *path)
{
	extern void fb_redisplay(void);
	char cmd[256];

	snprintf(cmd,sizeof(cmd), "/mpegplayer.bin '%s'", path);

	printf("play MPEG: '%s'\n", cmd);

	if (vfork() == 0) {
		system(cmd);
		_exit(0);
	} else {
		wait(NULL);
	}

	fb_redisplay();

	return 0;
}

int
do_play_file(char *path)
{
	printf("play file...\n");

	if (is_audio(path)) {
		play_audio_file(path);
	} else if (is_video(path)) {
		play_video_file(path);
	} else {
		return -1;
	}

	return 0;
}

int
do_play_dvd(char *path)
{
	return -1;
}

int
do_stop(void)
{
	return 0;
}
