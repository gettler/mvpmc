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

#include <mvp_string.h>
#include <plugin.h>
#include <plugin/av.h>

#include "av_local.h"

#if defined(PLUGIN_SUPPORT)
unsigned long plugin_version = CURRENT_PLUGIN_VERSION;
#endif

av_info_t info;

static int
av_play_file(char *path)
{
	if (access(path, R_OK) < 0) {
		return -1;
	}

	if (info.playing) {
		do_stop();
	}

	info.playing = 1;

	if (do_play_file(path) < 0) {
		info.playing = 0;
		return -1;
	}

	return 0;
}

static int
av_play_dvd(char *path)
{
	char *dvd_path;

	if ((dvd_path=malloc(strlen(path)+32)) != NULL) {
		sprintf(dvd_path, "%s/VIDEO_TS/VIDEO_TS.IFO", path);
		if (access(dvd_path, R_OK) == 0) {
			free(dvd_path);
			printf("playing dvd...\n");
			do_play_dvd(path);
			return 0;
		}
		sprintf(dvd_path, "%s/video_ts/video_ts.ifo", path);
		if (access(dvd_path, R_OK) == 0) {
			free(dvd_path);
			printf("playing dvd...\n");
			do_play_dvd(path);
			return 0;
		}
		free(dvd_path);
	}

	return -1;
}

static int
av_stop(void)
{
	info.playing = 0;

	return do_stop();
}

static plugin_av_t av = {
	.play_file = av_play_file,
	.play_dvd = av_play_dvd,
	.stop = av_stop,
};

static void*
init_av(void)
{
	if (arch_init() < 0) {
		return NULL;
	}

	printf("Audio/Video plug-in registered!\n");

	return (void*)&av;
}

static int
release_av(void)
{
	return 0;
}

PLUGIN_INIT(init_av);
PLUGIN_RELEASE(release_av);