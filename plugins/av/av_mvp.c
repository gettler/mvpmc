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
#include <mvp_demux.h>
#include <plugin.h>
#include <plugin/av.h>

#include "av_local.h"

#define BSIZE		(1024*32)

static gw_t *video;

static pthread_cond_t cond_audio = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_audio = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond_video = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_video = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond_status = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_status = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond_list = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_list = PTHREAD_MUTEX_INITIALIZER;

static pthread_t thread_audio, thread_video, thread_status, thread_list;

static char *pathname;
static volatile int stop_request;
static char **playlist;

static demux_handle_t *handle;

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
		pthread_cond_broadcast(&cond_list);
	}

	return NULL;
}

static int
mpg_play(int fd)
{
	int fda, fdv;
	int len = 0, n = 0, ret;
	static char buf[BSIZE];

	if (handle) {
		fprintf(stderr, "demuxer in use!\n");
		return -1;
	}

	if ((handle=demux_init(1024*1024*2.5)) == NULL) {
		fprintf(stderr, "failed to initialize demuxer\n");
		return -1;
	}

	fda = av_get_audio_fd();
	fdv = av_get_video_fd();

	av_stop();
	av_video_blank();
	av_reset();
	av_reset_stc();

	av_set_audio_output(AV_AUDIO_MPEG);
	av_play();

	while (!stop_request) {
		if (n == len) {
			len = read(fd, buf, sizeof(buf));
			n = 0;
		}
		if (len < 0) {
			fprintf(stderr, "%s(): error line %d\n", __FUNCTION__, __LINE__);
			break;
		}
		if (len > 0) {
			ret = demux_put(handle, buf+n, len-n);
			if (ret < 0) {
				fprintf(stderr, "%s(): error line %d\n", __FUNCTION__, __LINE__);
				break;
			}
			n += ret;
		}
		if (len == 0) {
			break;
		}
		demux_write_video(handle, fdv);
		demux_write_audio(handle, fda);
	}

	printf("%s(): finished\n", __FUNCTION__);

	if (handle) {
		demux_destroy(handle);
		handle = NULL;
	}

	return 0;
}

static void
video_clear(void)
{
	av_stop();
	av_video_blank();
	av_reset();
	av_reset_stc();
}

static void*
loop_video(void *arg)
{
	int fd;

	pthread_mutex_lock(&mutex_video);

	while (1) {
		pthread_cond_wait(&cond_video, &mutex_video);
		printf("Start playback: %s\n", pathname);
		if ((fd=open(pathname, O_RDONLY|O_LARGEFILE|O_NDELAY)) >= 0) {
			mpg_play(fd);
			video_clear();
			gw_set_console(ROOT_CONSOLE);
			gw_ss_enable();
			close(fd);
		}
		pthread_cond_broadcast(&cond_list);
	}

	return NULL;
}

static void*
loop_status(void *arg)
{
	pthread_mutex_lock(&mutex_status);

	while (1) {
		pthread_cond_wait(&cond_status, &mutex_status);
	}

	return NULL;
}

static void*
loop_list(void *arg)
{
	int i;

	pthread_mutex_lock(&mutex_list);

	while (1) {
		pthread_cond_wait(&cond_list, &mutex_list);
		i = 0;
		while (playlist && (playlist[i] != NULL)) {
			do_play_file(playlist[i++]);
			pthread_cond_wait(&cond_list, &mutex_list);
		}
		if (playlist) {
			playlist = NULL;
		}
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

	video = gw_create_console(VIDEO_CONSOLE);
	gw_map(video);

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024*64);

	pthread_create(&thread_audio, &attr, loop_audio, NULL);
	pthread_create(&thread_video, &attr, loop_video, NULL);
	pthread_create(&thread_list, &attr, loop_list, NULL);
	pthread_create(&thread_status, &attr, loop_status, NULL);

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

static int
is_video(char *file)
{
	char *wc[] = { ".mpg", ".mpeg", ".nuv", NULL };
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
	int ret = -1;

	if (is_audio(path)) {
		if (pathname)
			free(pathname);
		pathname = strdup(path);
		printf("%s(): kick playback thread\n", __FUNCTION__);
		pthread_cond_broadcast(&cond_audio);
		ret = 0;
	} else if (is_video(path)) {
		if (pathname)
			free(pathname);
		pathname = strdup(path);
		gw_set_console(VIDEO_CONSOLE);
		gw_ss_disable();
		printf("%s(): kick playback thread\n", __FUNCTION__);
		pthread_cond_broadcast(&cond_video);
		ret = 0;
	}

	return ret;
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
do_play_list(char **list)
{
	int i;

	if ((playlist=malloc(sizeof(char*)*128)) == NULL) {
		return -1;
	}

	for (i=0; i<127; i++) {
		if (list[i] == NULL) {
			break;
		}
		playlist[i] = strdup(list[i]);
	}
	playlist[i] = NULL;

	pthread_cond_broadcast(&cond_list);

	return 0;
}

int
do_stop(void)
{
	stop_request = 1;

	pthread_mutex_lock(&mutex_audio);
	pthread_mutex_lock(&mutex_video);
	stop_request = 0;
	pthread_mutex_unlock(&mutex_video);
	pthread_mutex_unlock(&mutex_audio);

	return 0;
}
