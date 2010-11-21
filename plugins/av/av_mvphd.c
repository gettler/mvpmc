/*
 *  Copyright (C) 2010, Jon Gettler
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

static int
monitor(pid_t pid)
{
	pid_t child;

	while (waitpid(pid, NULL, WNOHANG) == 0) {
		sleep(1);
		if (player == 0) {
			printf("killing media player\n");
			switch (child=fork()) {
			case 0:
				unsetenv("LD_LIBRARY_PATH");
				unsetenv("PLUGIN_PATH");
				system("killall test_rmfp");
				exit(0);
				break;
			}
			while (waitpid(child, NULL, 0) != child)
				;
		}
	}

	return 0;
}

static int
start_player(void)
{
	char *p = (char*)pathname;
	char *argv[16];
	pid_t child;

	memset(argv, 0, sizeof(argv));

	argv[0] = "/usr/local/bin/test_rmfp";
	argv[1] = p;

	printf("Starting media player: %s\n", p);

	player = 1;

	switch (child=fork()) {
	case 0:
		unsetenv("LD_LIBRARY_PATH");
		unsetenv("PLUGIN_PATH");
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
		player = 0;
		if (!pending) {
			free((char*)pathname);
			pathname = NULL;

			do_stop();
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
	return 0;
}

int
do_play_url(char *path)
{
	return 0;
}

int
do_play_list(char **list)
{
	return 0;
}

int
do_stop(void)
{
	if (player) {
		player = 0;
	}

	return 0;
}
