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
#include <sys/un.h>
#include <sys/signal.h>   
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>
#include <libgen.h>

#include <plugin.h>
#include <plugin/av.h>

#include "av_local.h"

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t thread;

static pid_t player;

static volatile char *pathname;

static volatile int pending;
static volatile int playlist;

#define PL_HTML	"/tmp/playlist.htm"

static int
create_playlist(char **files)
{
	FILE *f;
	char *c;
	int i = 0;

	if ((f=fopen(PL_HTML, "w")) == NULL) {
		return -1;
	}

	c = files[i];
	while (c) {
		if (i != 0) {
			fprintf(f, "|");
		}
		fprintf(f, "%s|0|0|file://%s", basename(c), c);
		c = files[++i];
	}

	fclose(f);

	return 0;
}

static int
monitor(pid_t pid)
{
	int i;
	char buf[128];
	struct sockaddr_un addr;
	int fd;

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, "/dev/lircd");

	if ((fd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("connect");
		close(fd);
		return -1;
	}

	while (waitpid(pid, NULL, WNOHANG) == 0) {
		fd_set fds;
		struct timeval to;
		int key;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		to.tv_sec = 0;
		to.tv_usec = 20000;

		if (select(fd+1, &fds, NULL, NULL, &to) > 0) {
			i=read(fd, buf, sizeof(buf));
			if (i==-1) {
				perror("read");
				break;
			};
			if (!i) {
				break;
			}
			buf[i] = '\0';
			key = atoi(buf);
			printf("keypress: %d\n", key);
		}
	}

	close(fd);

	return 0;
}

static int
is_audio(char *file)
{
	char *wc[] = { ".mp3", ".flac", NULL };
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
is_dvd(char *path)
{
	char *dvd_path;

	if ((dvd_path=malloc(strlen(path)+32)) != NULL) {
		sprintf(dvd_path, "%s/VIDEO_TS/VIDEO_TS.IFO", path);
		if (access(dvd_path, R_OK) == 0) {
			free(dvd_path);
			return 1;
		}
		sprintf(dvd_path, "%s/video_ts/video_ts.ifo", path);
		if (access(dvd_path, R_OK) == 0) {
			free(dvd_path);
			return 1;
		}
		free(dvd_path);
	}

	return 0;
}

static int
start_player(void)
{
	char *p = (char*)pathname;
	char *argv[16];
	pid_t child;
	char path[strlen(p)+1];

	memset(argv, 0, sizeof(argv));

	if (is_audio(p)) {
		argv[0] = "/bin/mono";
		argv[1] = "-audio";
		argv[2] = "-prebuf";
		argv[3] = "100";
		argv[4] = "-bgimg";
		if (playlist) {
			char pl[512];
			snprintf(pl, sizeof(pl),
				 "file://%s?start_url=file:/%s",
				 PL_HTML, p);
			argv[5] = "-playlist";
			argv[6] = pl;
		} else {
			argv[5] = "-single";
			argv[6] = p;
		}
		argv[7] = "-dram";
		argv[8] = "1";
	} else if (is_video(p)) {
		argv[0] = "/bin/mono";
		argv[1] = "-single";
		argv[2] = "-nogui";
		argv[3] = p;
		argv[4] = "-dram";
		argv[5] = "1";
	} else if (is_dvd(p)) {
		sprintf(path, "%s/", p);
		argv[0] = "/bin/amp_test";
		argv[1] = path;
		argv[2] = "--dfb:quiet";
		argv[3] = "--osd32";
		argv[4] = "--bgnd:/bin/logo.jpg";
	} else {
		return -1;
	}

	printf("Starting player: %s\n", argv[0]);
	switch (child=fork()) {
	case 0:
		execv(argv[0], argv);
		exit(1);
	case -1:
		return -1;
		break;
	}

	while (waitpid(child, NULL, 0) != child)
		;

	return 0;
}

static void*
av_monitor(void *arg)
{
	pid_t child;

	pthread_mutex_lock(&mutex);

	while (1) {
		if (!pending) {
			pthread_cond_wait(&cond, &mutex);
		}

		pending = 0;
		if ((child=fork()) == 0) {
			_exit(start_player());
		} else {
			player = child;
			monitor(child);
		}
		playlist = 0;
		player = 0;
		if (!pending) {
			free((char*)pathname);
			pathname = NULL;

			do_stop();

			gw_device_add(GW_DEV_OSD);
			gw_output();
		}

	}
}

int
arch_init(void)
{
	pthread_attr_t attr;

	player = 0;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024*64);

	pthread_create(&thread, &attr, av_monitor, NULL);

	return 0;
}

int
do_play_file(char *path)
{
	pending = 1;

	gw_device_remove(GW_DEV_OSD);

	if (pathname) {
		free((char*)pathname);
	}
	pathname = strdup(path);
	pthread_cond_broadcast(&cond);

	return 0;
}

int
do_play_dvd(char *path)
{
	return do_play_file(path);
}

int
do_play_url(char *path)
{
	return do_play_file(path);
}

int
do_play_list(char **list)
{
	if (create_playlist(list) != 0) {
		return -1;
	}

	playlist = 1;

	return do_play_file(*list);
}

int
do_stop(void)
{
	if (player) {
		system("echo 212 > /tmp/irkey");
		player = 0;
	}

	return 0;
}
