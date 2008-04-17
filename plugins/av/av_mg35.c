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

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pid_t player;

static pthread_t thread;

static char *pathname;

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

static int
play_audio_file(char *path)
{
	printf("play MP3: '%s'\n", path);

	if ((player=vfork()) == 0) {
		execl("/fileplayer.bin", "/fileplayer.bin", "MP3", path, NULL);
		_exit(-1);
	}

	return 0;
}

static int
play_video_file(char *path)
{
	printf("play MPEG: '%s'\n", path);

	if ((player=vfork()) == 0) {
		execl("/mpegplayer.bin", "/mpegplayer.bin", path, NULL);
		_exit(-1);
	}

	return 0;
}

static void*
av_player(void *arg)
{
	pid_t child = -1;

	pthread_mutex_lock(&mutex);

	while (1) {
		pthread_cond_wait(&cond, &mutex);
		if (child > 0)
			waitpid(child, NULL, WNOHANG);
		if (is_audio(pathname)) {
			play_audio_file(pathname);
		} else if (is_video(pathname)) {
			system("/bin/cls");
			play_video_file(pathname);
		}
		child = player;
	}

	return NULL;
}

int
do_play_file(char *path)
{
	if (pathname) {
		free(pathname);
	}
	pathname = strdup(path);

	pthread_cond_broadcast(&cond);

	return 0;
}

int
arch_init(void)
{
	pthread_attr_t attr;

	player = 0;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024*64);

	pthread_create(&thread, &attr, av_player, NULL);

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
	if (player > 0) {
		kill(player, SIGTERM);
		player = -1;
	}
	if (pathname) {
		free(pathname);
		pathname = NULL;
	}

	return 0;
}
