/*
 *  Copyright (C) 2004-2007, Jon Gettler
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
#include <sys/time.h>
#include <time.h>
#include <libgen.h>
#include <string.h>

#include "mvp_osd.h"

#define INCLUDE_LINUX_LOGO_DATA
#define __initdata
#include <linux/linux_logo.h>

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

typedef struct {
	unsigned int c;
	char name[24];
} color_name_t;

#if defined(MVPMC_MG35)
static int width = 640, height = 465;
#else
static int width = 720, height = 480;
#endif

static char *test = NULL;

static int lr;

#if defined(MVPMC_MG35)
struct timeval start, end, delta;
#define timer_start()	fflush(stdout); gettimeofday(&start, NULL)
#define timer_end()	gettimeofday(&end, NULL)
#define timer_print()	timersub(&end,&start,&delta); \
			snprintf(buf, sizeof(buf), "%lu.%.2lu seconds\n", \
			         delta.tv_sec, delta.tv_usec/10000); \
			printf(buf)
#else
struct timeval start, end, delta;

#define timer_start()	fflush(stdout); gettimeofday(&start, NULL)

#define timer_end()	gettimeofday(&end, NULL)

#define timer_print()	timersub(&end,&start,&delta); \
			snprintf(buf, sizeof(buf), "%5.2f seconds\n", \
			         delta.tv_sec + (delta.tv_usec/1000000.0)); \
			printf(buf)
#endif

#define FAIL		{ \
				lr = __LINE__; \
				goto err; \
			}

static void
fb_clear(void)
{
	osd_surface_t *surface;

	surface = osd_create_surface(width, height, OSD_BLACK, OSD_FB);

#if 0
	if (surface) {
		osd_display_surface(surface);
		osd_destroy_surface(surface);
	}
#endif
}

static int
test_sanity(char *name)
{
	osd_surface_t *surface = NULL;
	unsigned long c1, c2;

	printf("testing osd sanity\t");
	timer_start();

	if ((surface=osd_create_surface(width, height, 0, OSD_GFX)) == NULL)
		FAIL;

	c1 = rand() | 0xff000000;
	c2 = rand() | 0xff000000;

	if (osd_draw_text(surface, 0, 0, "Hello World!", c1, c2, 1, NULL) < 0)
		FAIL;

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_create_surfaces(char *name)
{
	int i;
	int n = 50;
	osd_surface_t *surface = NULL;

	printf("creating %d surfaces\t", n);

	timer_start();

	for (i=0; i<n; i++) {
		if ((surface=osd_create_surface(width, height,
						0, OSD_GFX)) == NULL)
			FAIL;
		if (i == 0) {
			if (osd_display_surface(surface) < 0)
				FAIL;
			if (osd_draw_text(surface, 100, 200,
					 "Creating surfaces!", OSD_GREEN, 0, 1,
					 NULL) < 0)
				FAIL;
		} else {
			if (osd_destroy_surface(surface) < 0)
				FAIL;
		}
	}

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_text(char *name)
{
	int i;
	osd_surface_t *surface = NULL;

	printf("testing %s\t\t", name);

	timer_start();

	if ((surface=osd_create_surface(width, height,
					0, OSD_GFX)) == NULL)
		FAIL;

	for (i=0; i<height; i+=50) {
		unsigned long c1, c2;

		c1 = rand() | 0xff000000;
		c2 = rand() | 0xff000000;

		if (osd_draw_text(surface, i+100, i, "Hello World!",
				 c1, c2, 1, NULL) < 0)
			FAIL;
	}

	if (osd_display_surface(surface) < 0)
		FAIL;

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_rectangles(char *name)
{
	int i;
#if defined(MVPMC_MG35)
	int n = 50;
#else
	int n = 500;
#endif
	osd_surface_t *surface = NULL;

	printf("drawing %d rectangles\t", n);

	timer_start();

	if ((surface=osd_create_surface(width, height, 0, OSD_GFX)) == NULL)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	for (i=0; i<n; i++) {
		int x, y, w, h;
		unsigned long c;

		x = rand() % width;
		y = rand() % height;
		w = rand() % (width - x);
		h = rand() % (height - y);
		c = rand();

		if (osd_fill_rect(surface, x, y, w, h, c) < 0)
			FAIL;
	}

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_circles(char *name)
{
	int i;
	int n = 30;
	osd_surface_t *surface = NULL;

	printf("drawing %d circles\t", n);

	timer_start();

	if ((surface=osd_create_surface(width, height,
					0, OSD_GFX)) == NULL)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	for (i=0; i<n; i++) {
		int x, y, r, f;
		unsigned long c;

		x = rand() % width;
		y = rand() % height;
		r = rand() % 100;
		c = rand() | 0xff000000;
		f = rand() % 2;

		if (osd_draw_circle(surface, x, y, r, f, c) < 0)
			FAIL;
	}

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_polygons(char *name)
{
	int i;
	int n = 100;
	osd_surface_t *surface = NULL;

	printf("drawing %d polygons\t", n);

	timer_start();

	if ((surface=osd_create_surface(width, height,
					0, OSD_GFX)) == NULL)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	for (i=0; i<n; i++) {
		int x[12], y[12];
		int n, i;
		unsigned long c;

		n = (rand() % 8) + 3;

		for (i=0; i<n; i++) {
			x[i] = rand() % width;
			y[i] = rand() % height;
		}

		c = rand() | 0xff000000;

		if (osd_draw_polygon(surface, x, y, n, c) < 0)
			FAIL;
	}

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_lines(char *name)
{
	int i, j;
	int p = 10, n = 100;
	osd_surface_t *surface = NULL;

	printf("drawing %d lines\t", p*n);

	timer_start();

	if ((surface=osd_create_surface(width, height,
					0, OSD_GFX)) == NULL)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	for (i=0; i<p; i++) {
		int x1, y1;
		unsigned long c;

		x1 = rand() % width;
		y1 = rand() % height;
		c = rand() | 0xff000000;

		for (j=0; j<n; j++) {
			int x2, y2;

			x2 = rand() % width;
			y2 = rand() % height;

			if (osd_draw_line(surface, x1, y1, x2, y2, c) < 0)
				FAIL;
		}
	}

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_display_control(char *name)
{
	osd_surface_t *surface = NULL;

	printf("testing display control\t");

	timer_start();

	if ((surface=osd_create_surface(width, height,
					OSD_BLUE, OSD_GFX)) == NULL)
		FAIL;

	if (osd_set_engine_mode(surface, 0) < 0)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	if (osd_get_display_control(surface, 2) < 0)
		FAIL;
	if (osd_get_display_control(surface, 3) < 0)
		FAIL;
	if (osd_get_display_control(surface, 4) < 0)
		FAIL;
	if (osd_get_display_control(surface, 5) < 0)
		FAIL;
	if (osd_get_display_control(surface, 7) < 0)
		FAIL;
	if (osd_get_display_options(surface) < 0)
		FAIL;

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_blit(char *name)
{
	osd_surface_t *surface = NULL, *hidden = NULL;
	int i, j, k;
	int h, w;
	int mx, my;
	int x1, y1, x2, y2;
	int c;

	printf("testing blit\t\t");

	timer_start();

	if ((surface=osd_create_surface(width, height,
					0, OSD_GFX)) == NULL)
		FAIL;
	if ((hidden=osd_create_surface(width, height, 0, OSD_GFX)) == NULL)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	w = width / 4;
	h = height / 4;

	for (i=0; i<4; i++) {
		x1 = i * w;
		x2 = (i+1) * w - 1;
		for (j=0; j<4; j++) {
			y1 = j * h;
			y2 = (j+1) * h - 1;
			mx = x1 + (w/2);
			my = y1 + (h/2);
			c = rand() | 0xff000000;
			for (k=0; k<300; k++) {
				int ex = x1 + (rand() % w);
				int ey = y1 + (rand() % h);
				if (osd_draw_line(hidden,
						  mx, my, ex, ey, c) < 0)
					FAIL;
			}
			if (osd_blit(surface, x1, y1, hidden, x1, y1, w, h) < 0)
				FAIL;
		}
	}

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_cursor(char *name)
{
	osd_surface_t *surface = NULL, *cursor = NULL;
	int i;
	int x, y;

	printf("testing cursor\t\t");

	timer_start();

	if ((surface=osd_create_surface(width, height,
					0, OSD_GFX)) == NULL)
		FAIL;

	if ((cursor=osd_create_surface(64, 76, OSD_WHITE, OSD_CURSOR)) == NULL)
		FAIL;

	if (osd_palette_add_color(cursor, OSD_BLACK) < 0)
		FAIL;
	if (osd_palette_add_color(cursor, OSD_BLUE) < 0)
		FAIL;
	if (osd_palette_add_color(cursor, OSD_GREEN) < 0)
		FAIL;
	if (osd_palette_add_color(cursor, OSD_RED) < 0)
		FAIL;
	if (osd_palette_add_color(cursor, 0) < 0)
		FAIL;

	if (osd_fill_rect(cursor, 0, 0, 64, 16, OSD_RED) < 0)
		FAIL;
	if (osd_fill_rect(cursor, 0, 16, 64, 16, 0) < 0)
		FAIL;
	if (osd_fill_rect(cursor, 0, 32, 64, 16, OSD_BLUE) < 0)
		FAIL;
	if (osd_fill_rect(cursor, 0, 48, 64, 16, OSD_GREEN) < 0)
		FAIL;

	if (osd_fill_rect(surface, 64, 64, 64, 64, OSD_RED) < 0)
		FAIL;
	if (osd_fill_rect(surface, 128, 64, 64, 64, OSD_GREEN) < 0)
		FAIL;
	if (osd_fill_rect(surface, 196, 64, 64, 64, OSD_BLUE) < 0)
		FAIL;

	for (y=150; y<300; y+=30) {
		if (osd_fill_rect(surface, 50, y, 400, 10, OSD_WHITE) < 0)
			FAIL;
	}

	if (osd_display_surface(surface) < 0)
		FAIL;

	if (osd_display_surface(cursor) < 0)
		FAIL;

	x = y = 0;
	for (i=0; i<100; i++) {
		x += 4;
		y += 4;
		if (osd_move(cursor, x, y) < 0)
			FAIL;
		usleep(10000);
	}

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_fb(char *name)
{
#if defined(LINUX_LOGO_COLORS)
	osd_surface_t *surface = NULL;
	osd_indexed_image_t image;
	int x, y;

	printf("testing framebuffer\t");

	timer_start();

	if ((surface=osd_create_surface(width, height, OSD_BLACK, OSD_FB)) == NULL)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	image.colors = LINUX_LOGO_COLORS;
	image.width = 80;
	image.height = 80;
	image.red = linux_logo_red;
	image.green = linux_logo_green;
	image.blue = linux_logo_blue;
	image.image = linux_logo;

	x = (width - image.width) / 2;
	y = (height - image.height) / 2;

	if (osd_draw_indexed_image(surface, &image, x, y) < 0)
		FAIL;

	if (osd_palette_add_color(surface, OSD_GREEN) < 0)
		FAIL;

	timer_end();

	return 0;

 err:
#endif /* LINUX_LOGO_COLORS */
	return -1;
}

static int
test_blit2(char *name)
{
#if defined(LINUX_LOGO_COLORS)
	osd_surface_t *fb = NULL;
	osd_surface_t *osd = NULL;
	osd_indexed_image_t image;
	int x, y;

	printf("testing blit2\t\t");

	timer_start();

	if ((fb=osd_create_surface(width, height, OSD_BLACK, OSD_FB)) == NULL)
		FAIL;

	if ((osd=osd_create_surface(width, height, OSD_RED, OSD_GFX)) == NULL)
		FAIL;

	if (osd_display_surface(osd) < 0)
		FAIL;

	image.colors = LINUX_LOGO_COLORS;
	image.width = 80;
	image.height = 80;
	image.red = linux_logo_red;
	image.green = linux_logo_green;
	image.blue = linux_logo_blue;
	image.image = linux_logo;

	x = (width - image.width) / 2;
	y = (height - image.height) / 2;

	if (osd_draw_indexed_image(fb, &image, x, y) < 0)
		FAIL;

	if (osd_blit(osd, x-80, y, fb, x, y, 80, 80) < 0)
		FAIL;
	if (osd_blit(osd, x+80, y, fb, x, y, 80, 80) < 0)
		FAIL;
	if (osd_blit(osd, x, y-80, fb, x, y, 80, 80) < 0)
		FAIL;
	if (osd_blit(osd, x, y+80, fb, x, y, 80, 80) < 0)
		FAIL;

	fb_clear();

	timer_end();

	return 0;

 err:
#endif /* LINUX_LOGO_COLORS */
	return -1;
}

static int
test_color(char *name)
{
	int i, y = 20;
	osd_surface_t *surface = NULL;
	color_name_t colors[] = {
		{ OSD_WHITE, "white" },
		{ OSD_BLACK, "black" },
		{ OSD_RED, "red" },
		{ OSD_BLUE, "blue" },
		{ OSD_GREEN, "green" },
		{ OSD_YELLOW, "yellow" },
		{ OSD_ORANGE, "orange" },
		{ OSD_PURPLE, "purple" },
		{ OSD_BROWN, "brown" },
		{ OSD_CYAN, "cyan" },
	};
	int ncolors = sizeof(colors)/sizeof(colors[0]);

	printf("testing %s\t\t", name);

	timer_start();

	if ((surface=osd_create_surface(width, height, 0, OSD_GFX)) == NULL)
		FAIL;

	for (i=0; i<ncolors; i++) {
		unsigned long c;

		c = colors[i].c;

		if (osd_draw_text(surface, 300, y, colors[i].name,
				  c, 0, 1, NULL) < 0)
			FAIL;
		if (osd_fill_rect(surface, 400, y, 100, 20, c) < 0)
			FAIL;
		y += 45;
	}

	if (osd_display_surface(surface) < 0)
		FAIL;

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_font(char *name)
{
	osd_surface_t *surface = NULL;
	int w, h;
	char *str = "Test: ABC xyz 012 pqg @#$";
	char buf[128];

	printf("testing %s\t\t", name);

	timer_start();

	if ((surface=osd_create_surface(width, height, OSD_BLACK, OSD_GFX)) == NULL)
		FAIL;

	h = osd_font_height(NULL);
	w = osd_font_width(NULL, str);

	if ((w <= 0) || (h <= 0))
		FAIL;

	if (osd_fill_rect(surface, 300, 100+h, w, h, OSD_RED) < 0)
		FAIL;
	if (osd_fill_rect(surface, 300+w, 100, 30, h, OSD_RED) < 0)
		FAIL;

	if (osd_draw_text(surface, 300, 100, str,
			  OSD_WHITE, OSD_BLACK, 0, NULL) < 0)
		FAIL;

	snprintf(buf, sizeof(buf), "font width is %d, height is %d", w, h);
	if (osd_draw_text(surface, 300, 200, buf,
			  OSD_WHITE, OSD_BLACK, 1, NULL) < 0)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_clip(char *name, osd_type_t type)
{
	osd_surface_t *surface = NULL;
	osd_surface_t *cs = NULL;
	osd_clip_t clip;
	osd_clip_region_t reg[4];
	char *msg = "Hello World!";

	printf("testing %s\t\t", name);

	timer_start();

	if ((surface=osd_create_surface(width, height,
					OSD_BLACK, type)) == NULL)
		FAIL;

	if (type == OSD_FB) {
		if (osd_palette_add_color(surface, OSD_GREEN) < 0)
			FAIL;
		if (osd_palette_add_color(surface, OSD_RED) < 0)
			FAIL;
		if (osd_palette_add_color(surface, OSD_WHITE) < 0)
			FAIL;
	}

	clip.n = 4;
	clip.regs = reg;

	reg[0].x = 300;
	reg[0].y = 100;
	reg[0].h = 50;
	reg[0].w = 300;

	reg[1].x = 300;
	reg[1].y = 100;
	reg[1].h = 200;
	reg[1].w = 50;

	reg[2].x = 600;
	reg[2].y = 100;
	reg[2].h = 200;
	reg[2].w = 50;

	reg[3].x = 300;
	reg[3].y = 300;
	reg[3].h = 50;
	reg[3].w = 350;

	if ((cs=osd_clip_set(surface, &clip)) == NULL)
		FAIL;

	if (osd_fill_rect(cs, 0, 0, width, height, OSD_RED) < 0)
		FAIL;

	if (osd_draw_text(cs, 300, 100, msg, OSD_WHITE, 0, 0, NULL) < 0)
		FAIL;
	if (osd_draw_text(cs, 300, 200, msg, OSD_WHITE, 0, 0, NULL) < 0)
		FAIL;
	if (osd_draw_text(cs, 300, 300, msg,
			  OSD_WHITE, OSD_GREEN, 1, NULL) < 0)
		FAIL;

	if (osd_display_surface(surface) < 0)
		FAIL;

	timer_end();

	return 0;

 err:
	return -1;
}

static int
test_clip_gfx(char *name)
{
	return test_clip(name, OSD_GFX);
}

static int
test_clip_fb(char *name)
{
	return test_clip(name, OSD_FB);
}

typedef struct {
	char *name;
	int sleep;
	int (*func)(char*);
} tester_t;

static tester_t tests[] = {
	{ "sanity",		0,	test_sanity },
	{ "surfaces",		0,	test_create_surfaces },
	{ "text",		2,	test_text },
	{ "font",		2,	test_font },
	{ "rectangles",		2,	test_rectangles },
	{ "circles",		2,	test_circles },
	{ "lines",		2,	test_lines },
	{ "polygons",		2,	test_polygons },
	{ "display",		2,	test_display_control },
	{ "blit",		2,	test_blit },
	{ "cursor",		2,	test_cursor },
	{ "framebuffer",	2,	test_fb },
	{ "blit2",		2,	test_blit2 },
	{ "color",		2,	test_color },
	{ "clip",		2,	test_clip_gfx },
	{ "clip2",		2,	test_clip_fb },
	{ NULL, 0, NULL },
};

void
print_help(char *prog)
{
	printf("Usage: %s [-hl] [-t test]\n", basename(prog));
	printf("\t-h        print help\n");
	printf("\t-l        list tests\n");
	printf("\t-t test   perform individual test\n");
}

void
print_tests(void)
{
	int i = 0;

	while (tests[i].name != NULL) {
		printf("\t%s\n", tests[i++].name);
	}
}

int
main(int argc, char **argv)
{
	int i = 0;
	int c;
	char buf[256];
	osd_surface_t *surface;
	int ret = 0;

	while ((c=getopt(argc, argv, "hlt:")) != -1) {
		switch (c) {
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		case 'l':
			print_tests();
			exit(0);
			break;
		case 't':
			test = strdup(optarg);
			break;
		default:
			print_help(argv[0]);
			exit(1);
			break;
		}
	}

	srand(getpid());

	fb_clear();

	while (tests[i].name) {
		if (test && (strcmp(test, tests[i].name) != 0)) {
			i++;
			continue;
		}
		if (tests[i].func(tests[i].name) == 0) {
			timer_print();
			if (tests[i].sleep) {
				surface = osd_get_visible_surface();
				osd_draw_text(surface, 100, 200, buf,
					     OSD_GREEN, 0, 1,
					     NULL);
				osd_draw_text(surface, 100, 80, tests[i].name,
					     OSD_GREEN, 0, 1,
					     NULL);
				sleep(tests[i].sleep);
			}
		} else {
			printf("failed at line %d\n", lr);
			ret = -1;
		}
		osd_destroy_all_surfaces();
		i++;
	}

	fb_clear();

	return ret;
}
