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

#define INCLUDE_LINUX_LOGO_DATA

#undef __initdata
#define __initdata
#include "../../src/splash.h"

#define OSD_COLOR(r,g,b,a)	((a<<24) | (r<<16) | (g<<8) | b)

#define OSD_WHITE	OSD_COLOR(255,255,255,255)
#define OSD_RED		OSD_COLOR(255,0,0,255)
#define OSD_BLUE	OSD_COLOR(0,0,255,255)
#define OSD_GREEN	OSD_COLOR(0,255,0,255)
#define OSD_BLACK	OSD_COLOR(0,0,0,255)
#define OSD_YELLOW	OSD_COLOR(255,255,0,255)
#define OSD_ORANGE	OSD_COLOR(255,180,0,255)
#define OSD_PURPLE	OSD_COLOR(255,0,234,255)
#define OSD_BROWN	OSD_COLOR(118,92,0,255)
#define OSD_CYAN	OSD_COLOR(0,255,234,255)

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

static int
draw_logo(osd_surface_t *surface)
{
	osd_indexed_image_t image;
	int x, y;
	unsigned int bg = OSD_BLACK;
	int ret = -1;
	osd_surface_t *s;

	image.colors = LINUX_LOGO_COLORS;
	image.width = LOGO_W;
	image.height = LOGO_H;
	image.red = linux_logo_red;
	image.green = linux_logo_green;
	image.blue = linux_logo_blue;
	image.image = linux_logo;

	if ((s=osd_create_surface(image.width, image.height,
				  bg, OSD_GFX)) == NULL) {
		printf("%s(): %d\n", __FUNCTION__, __LINE__);
		goto err;
	}

	if (osd_draw_indexed_image(s, &image, 0, 0) < 0) {
		printf("%s(): %d\n", __FUNCTION__, __LINE__);
		goto err;
	}

	while (timeout && (timeout <= time(NULL))) {
		x = rand() % (width - image.width);
		y = rand() % (height - image.height);

		osd_blit(surface, x, y, s, 0, 0, image.width, image.height);

		sleep(2);

		osd_fill_rect(surface, x, y, image.width, image.height, bg);

		pthread_testcancel();
	}

	ret = 0;

 err:
	osd_destroy_surface(s);

	return ret;
}

static int
ss(void)
{
	unsigned int bg = OSD_COLOR_BLACK;
	osd_surface_t *s, *v;

	v = osd_get_visible_surface();

	if ((s=osd_create_surface(width, height, bg, OSD_GFX)) == NULL) {
		return -1;
	}

	if (osd_display_surface(s) < 0) {
		osd_destroy_surface(s);
		return -1;
	}

	draw_logo(s);

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
