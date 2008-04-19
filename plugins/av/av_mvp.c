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
#include <errno.h>

#include <mvp_string.h>
#include <mvp_av.h>
#include <plugin.h>
#include <plugin/av.h>

#include "av_local.h"

#define BSIZE		(1024*32)

static pthread_cond_t cond_audio = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_audio = PTHREAD_MUTEX_INITIALIZER;

static pthread_t thread_audio, thread_video;

static char *pathname;
static volatile int stop_request;

static int
mp3_play(int fd)
{
	static char buf[BSIZE];
	int n = 0, w = 0;
	int t;
	int afd;

	afd = av_get_audio_fd();

	printf("%s(): fd %d afd %d\n", __FUNCTION__, fd, afd);

	av_set_audio_output(AV_AUDIO_MPEG);
	av_set_audio_type(0);

	av_play();

	while (!stop_request) {
		if (n == 0) {
			if ((n=read(fd, buf, BSIZE)) == 0) {
				usleep(1000);
			}
			if (n <= 0) {
				break;
			}
		}

		if (n > 0) {
			if ((t=write(afd, buf+w, n-w)) <= 0) {
				usleep(1000);
			}
			if (t < 0) {
				break;
			}
			w += t;

			if (w == n) {
				n = w = 0;
			}
		}
	}

	printf("%s(): finished\n", __FUNCTION__);

	return 0;
}

static void*
loop_audio(void *arg)
{
	int fd;

	pthread_mutex_lock(&mutex_audio);

	while (1) {
		pthread_cond_wait(&cond_audio, &mutex_audio);
		printf("Start playback: %s\n", pathname);
		if ((fd=open(pathname, O_RDONLY|O_LARGEFILE|O_NDELAY)) >= 0) {
			mp3_play(fd);
			close(fd);
		}
	}

	return NULL;
}

static void*
loop_video(void *arg)
{
	while (1) {
		pause();
	}

	return NULL;
}

int
arch_init(void)
{
	pthread_attr_t attr;

	if (av_init() == AV_DEMUX_ERROR) {
		return -1;
	}

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024*64);

	pthread_create(&thread_audio, &attr, loop_audio, NULL);
	pthread_create(&thread_video, &attr, loop_video, NULL);

	return 0;
}

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

int
do_play_file(char *path)
{
	if (is_audio(path)) {
		if (pathname)
			free(pathname);
		pathname = strdup(path);
		printf("%s(): kick playback thread\n", __FUNCTION__);
		pthread_cond_broadcast(&cond_audio);
		return 0;
	}

	return -1;
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
	stop_request = 1;

	pthread_mutex_lock(&mutex_audio);
	stop_request = 0;
	pthread_mutex_unlock(&mutex_audio);

	return 0;
}
