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

#include "mvp_refmem.h"
#include "mvp_osd.h"

#include <plugin.h>
#include <plugin/screensaver.h>

#if defined(PLUGIN_SUPPORT)
unsigned long plugin_version = CURRENT_PLUGIN_VERSION;
#endif

static pthread_t thread;

static int width = -1, height = -1;

static volatile time_t timeout = 0;

static volatile int running = 0;

static int
start(void)
{
	timeout = time(NULL);
	pthread_kill(thread, SIGURG);

	return 0;
}

static int
stop(void)
{
	timeout = 0;
	pthread_kill(thread, SIGURG);

	return 0;
}

static int
feed(int seconds)
{
	timeout = time(NULL) + seconds;
	pthread_kill(thread, SIGURG);

	return 0;
}

static void
draw(osd_surface_t *surface)
{
	time_t clear = time(NULL);
	unsigned long c[64];
	int i;

	for (i=0; i< 64; i++) {
		unsigned char r, g, b, a;

		r = rand();
		g = rand();
		b = rand();
		a = 0xff;

		c[i] = osd_rgba(r, g, b, a);
	}

	while (timeout && (timeout <= time(NULL))) {
		int x, y, w, h;

		x = rand() % width;
		y = rand() % height;
		w = rand() % (width - x);
		h = rand() % (height - y);
		i = rand() % 64;

		osd_fill_rect(surface, x, y, w, h, c[i]);

		usleep(30000);

		if ((time(NULL) - clear) > 30) {
			i = rand() % 64;
			osd_fill_rect(surface, 0, 0, width, height, c[i]);
			clear = time(NULL);
		}

		pthread_testcancel();
	}
}

static int
ss(void)
{
	unsigned int bg = OSD_COLOR_GREEN;
	osd_surface_t *s, *v;

	v = osd_get_visible_surface();

	if ((s=osd_create_surface(width, height, bg, OSD_GFX)) == NULL) {
		return -1;
	}

	if (osd_display_surface(s) < 0) {
		osd_destroy_surface(s);
		return -1;
	}

	draw(s);

	osd_undisplay_surface(s);

	if (v) {
		osd_display_surface(v);
	}

	osd_destroy_surface(s);

	return 0;
}

static void
sig_handler(int sig)
{
	printf("screensaver kicked!\n");
}

static void*
ss_thread(void *arg)
{
	signal(SIGURG, sig_handler);

	while (1) {
		time_t delta, to;

		pthread_testcancel();

		if ((to=timeout) == 0) {
#if defined(MVPMC_MG35)
			sleep(2);
#else
			pause();
#endif
			continue;
		}

		delta = to - time(NULL);

		if (delta > 0) {
			sleep(delta);
			continue;
		}

		printf("start screensaver\n");
		running = 1;
		if (ss() < 0) {
			printf("screensaver failed\n");
			sleep(10);
		} else {
			printf("stop screensaver\n");
		}
		running = 0;
	}

	return NULL;
}

static int
is_running(void)
{
	return running;
}

static plugin_screensaver_t reloc = {
	.start = start,
	.stop = stop,
	.feed = feed,
	.is_running = is_running,
};

static void*
init_screensaver(void)
{
	pthread_attr_t attr;

#if !defined(MVPMC_MG35)
	if (osd_open() < 0) {
		return NULL;
	}
#endif

	osd_get_screen_size(&width, &height);

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024*64);

	pthread_create(&thread, &attr, ss_thread, NULL);

	printf("Screensaver plug-in registered!\n");

	return &reloc;
}

static int
release_screensaver(void)
{
	pthread_cancel(thread);

#if 0
	osd_close();
#endif

	printf("Screensaver plug-in deregistered!\n");

	return 0;
}

PLUGIN_INIT(init_screensaver);
PLUGIN_RELEASE(release_screensaver);
