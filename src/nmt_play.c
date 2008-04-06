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
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <gw.h>

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
play_audio_file(char *cwd, char *file)
{
	extern void fb_redisplay(void);
	char cmd[1024];
	pid_t child;

	gw_device_remove(GW_DEV_OSD);

	snprintf(cmd,sizeof(cmd),
		 "/bin/mono -audio -prebuf 100 -bgimg -single '%s/%s' -dram 1",
		 cwd, file);

	if ((child=fork()) == 0) {
		system(cmd);
		_exit(0);
	} else {
		monitor(child);
	}

	gw_device_add(GW_DEV_OSD);

	fb_redisplay();

	return 0;
}

static int
play_video_file(char *cwd, char *file)
{
	extern void fb_redisplay(void);
	char cmd[1024];
	pid_t child;

	gw_device_remove(GW_DEV_OSD);

	snprintf(cmd,sizeof(cmd),
		 "/bin/mono -single -nogui '%s/%s' -dram 1",
		 cwd, file);

	if ((child=fork()) == 0) {
		system(cmd);
		_exit(0);
	} else {
		monitor(child);
	}

	gw_device_add(GW_DEV_OSD);

	fb_redisplay();

	return 0;
}

int
play_file(char *cwd, char *file)
{
	if (is_audio(file)) {
		play_audio_file(cwd, file);
	} else if (is_video(file)) {
		play_video_file(cwd, file);
	} else {
		return -1;
	}

	return 0;
}

int
play_dvd(char *cwd, char *file)
{
	extern void fb_redisplay(void);
	char cmd[1024];
	pid_t child;

	gw_device_remove(GW_DEV_OSD);

	snprintf(cmd,sizeof(cmd),
		 "/bin/amp_test %s/%s/ --dfb:quiet -osd32 -bgnd:/bin/logo.jpg",
		 cwd, file);

	if ((child=fork()) == 0) {
		system(cmd);
		_exit(0);
	} else {
		waitpid(child, NULL, 0);
	}

	gw_device_add(GW_DEV_OSD);

	fb_redisplay();

	return 0;
}
