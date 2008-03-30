/*
 *  Copyright (C) 2007-2008, Jon Gettler
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
#include <dirent.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>

#include <mvp_av.h>
#include <mvp_demux.h>
#include <mvp_refmem.h>
#include <gw.h>

#if defined(_LARGEFILE_SOURCE)
#define STAT stat64
#define FSTAT fstat64
#else
#define STAT stat
#define FSTAT fstat
#endif

static gw_t *fb;

static char cwd[1024] = "/";

static long dir_count = 0;
static int file_count = 0;

static void fb_exit(void);

int
fb_init(gw_t *root)
{
	if ((fb=gw_create(GW_TYPE_MENU, root)) == NULL)
		goto err;

	return 0;

 err:
	return -1;
}

static int
select_dir(gw_t *widget, char *text, void *key)
{
	char *buf;
	char *dir = ref_hold(text);
	extern void fb_display(void);

	printf("Select dir: '%s'\n", text);

	gw_menu_clear(fb);

	buf = alloca(strlen(cwd) + strlen(text) + 1);
	sprintf(buf, "%s%s", cwd, text);

	strcpy(cwd, buf);

	printf("Menu title: '%s' '%s'\n", buf, cwd);

	gw_menu_title_set(fb, cwd);

	ref_release(dir);

	fb_display();

	return 0;
}

static int
select_file(gw_t *widget, char *text, void *key)
{
	printf("Select file: '%s'\n", text);

#if defined(MVPMC_NMT)
	extern int play_file(char*, char*);
	play_file(cwd, text);
#endif

	return 0;
}

static void
add_dirs(void)
{
	glob_t gb;
	char pattern[1024], buf[1024], *ptr;
	int i;
	struct STAT sb;

	printf("%s(): cwd '%s'\n", __FUNCTION__, cwd);

	memset(&gb, 0, sizeof(gb));
	snprintf(pattern, sizeof(pattern), "%s/*", cwd);

	dir_count = 1;
	if (glob(pattern, GLOB_ONLYDIR, NULL, &gb) == 0) {
		i = 0;
		while (gb.gl_pathv[i]) {
			STAT(gb.gl_pathv[i], &sb);
			if (S_ISDIR(sb.st_mode)) {
				if ((ptr=strrchr(gb.gl_pathv[i], '/')) == NULL)
					ptr = gb.gl_pathv[i];
				else
					ptr++;
				sprintf(buf, ptr);
				strcat(buf, "/");
				printf("DIR: '%s'\n", buf);
				gw_menu_item_add(fb, buf,
						 (void*)dir_count++,
						 select_dir, NULL);
			}
			i++;
		}
	}
	globfree(&gb);
}

static int
do_glob(char *wc[])
{
	int w, i = 0;
	struct STAT sb;
	glob_t gb;
	char pattern[1024], *ptr;

	w = 0;
	while (wc[w] != NULL) {
		memset(&gb, 0, sizeof(gb));
		snprintf(pattern, sizeof(pattern), "%s/%s", cwd, wc[w]);
		if (glob(pattern, 0, NULL, &gb) == 0) {
			i = 0;
			while (gb.gl_pathv[i]) {
				STAT(gb.gl_pathv[i], &sb);
				if ((ptr=strrchr(gb.gl_pathv[i], '/')) == NULL)
					ptr = gb.gl_pathv[i];
				else
					ptr++;
				gw_menu_item_add(fb, ptr,
						 (void*)dir_count++,
						 select_file, NULL);
				i++;
			}
		}
		globfree(&gb);
		w++;
	}

	return i;
}

static void
add_files(void)
{
	char *wc[] = { "*.mpg", "*.mpeg", "*.mp3", "*.nuv", "*.vob", "*.gif",
			"*.bmp", "*.m3u", "*.pls", "*.jpg", "*.jpeg", "*.png", "*.wav",
			"*.ac3", "*.ogg", "*.ts", "*.flac", NULL };
	char *WC[] = { "*.MPG", "*.MPEG", "*.MP3", "*.NUV", "*.VOB", "*.GIF",
			"*.BMP", "*.M3U", "*.JPG", "*.JPEG", "*.PNG", "*.WAV",
			"*.AC3", "*.OGG", "*.TS", "*.FLAC", NULL };


	file_count = 0;
	do_glob(wc);
	do_glob(WC);
}

static int
do_key(gw_t *widget, int key)
{
	char *p;
	char *slash;

	printf("filebrowser key: cwd '%s'\n", cwd);

	if (strcmp(cwd, "/") == 0) {
		fb_exit();
		return 0;
	}

	if ((p=strrchr(cwd, '/')) == NULL) {
		return 0;
	}

	if (p == cwd) {
		return 0;
	}

	*p = '\0';

	if ((p=strrchr(cwd, '/')) == NULL) {
		return 0;
	}

	*p = '\0';

	printf("change dir: '%s'\n", cwd);

	slash = ref_strdup("/");
	select_dir(widget, slash, NULL);
	ref_release(slash);

	return 0;
}

void
fb_display(void)
{
	gw_menu_title_set(fb, cwd);

	add_dirs();
	add_files();

	gw_map(fb);
	gw_focus_set(fb);
	gw_focus_cb_set(do_key);

	gw_output();
}

void
fb_redisplay(void)
{
	gw_unmap(fb);
	gw_menu_clear(fb);
	fb_display();
}

static void
fb_exit(void)
{
	extern void main_display(void);

	gw_unmap(fb);

	main_display();
}
