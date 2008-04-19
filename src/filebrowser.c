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
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>
#include <libgen.h>

#include <mvp_av.h>
#include <mvp_demux.h>
#include <mvp_refmem.h>
#include <gw.h>
#include <plugin/av.h>

#if defined(_LARGEFILE_SOURCE)
#define STAT stat64
#define FSTAT fstat64
#else
#define STAT stat
#define FSTAT fstat
#endif

static gw_t *fb;

#if defined(MVPMC_MG35)
static char cwd[1024] = "/cdrom/";
#elif defined(MVPMC_NMT)
static char cwd[1024] = "/opt/sybhttpd/localhost.drives/";
#else
static char cwd[1024] = "/";
#endif

static long dir_count = 0;
static int file_count = 0;

static void fb_exit(void);

extern plugin_av_t *av;

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
	char *dvd_path;
	char *dir = ref_hold(text);
	extern void fb_display(void);

	printf("Select dir: '%s'\n", dir);

	if ((dvd_path=malloc(strlen(cwd)+strlen(dir)+32)) != NULL) {
		sprintf(dvd_path, "%s/%s", cwd, dir);
		if (av->play_dvd(dvd_path) == 0) {
			free(dvd_path);
			return 0;
		}
		free(dvd_path);
	}

	gw_menu_clear(fb);

	if (strcmp(dir, "../") == 0) {
		buf = strdup(dirname(cwd));
		snprintf(cwd, sizeof(cwd), "%s/", buf);
		free(buf);
	} else {
		buf = alloca(strlen(cwd) + strlen(dir) + 1);
		sprintf(buf, "%s%s", cwd, dir);

		strcpy(cwd, buf);
	}

	if (strcmp(cwd, "//") == 0)
		cwd[1] = '\0';

	printf("Menu title: '%s'\n", cwd);

	gw_menu_title_set(fb, cwd);

	ref_release(dir);

	fb_display();

	return 0;
}

static int
select_file(gw_t *widget, char *text, void *key)
{
	char path[1024];

	printf("Select file: '%s'\n", text);

	snprintf(path, sizeof(path), "%s/%s", cwd, text);
	av->play_file(path);

	return 0;
}

static void
add_dirs(void)
{
	char buf[256];
	struct STAT sb;
	struct dirent *d;
	struct dirent **nl;
	int n, i;

	dir_count = 1;

	n = scandir(cwd, &nl, NULL, alphasort);

	for (i=0; i<n; i++) {
		d = nl[i];
		if (strcmp(d->d_name, ".") == 0) {
			goto cont;
		}
		if ((strcmp(cwd, "/") == 0) &&
		    (strcmp(d->d_name, "..") == 0)) {
			goto cont;
		}
		snprintf(buf, sizeof(buf), "%s%s", cwd, d->d_name);
		STAT(buf, &sb);
		if (S_ISDIR(sb.st_mode) && (access(buf, X_OK|R_OK) == 0)) {
			sprintf(buf, d->d_name);
			strcat(buf, "/");
			gw_menu_item_add(fb, buf,
					 (void*)dir_count++,
					 select_dir, NULL);
		}
	cont:
		free(d);
	}
}

static int
do_glob(char *wc[])
{
	char buf[256];
	struct STAT sb;
	int w, count = 0;

	w = 0;
	while (wc[w] != NULL) {
		struct dirent *d;
		struct dirent **nl;
		int n, i;

		n = scandir(cwd, &nl, NULL, alphasort);

		for (i=0; i<n; i++) {
			d = nl[i];
			snprintf(buf, sizeof(buf), "%s%s", cwd, d->d_name);
			STAT(buf, &sb);
			if (!S_ISREG(sb.st_mode)) {
				goto cont;
			}
			if ((fnmatch(wc[w], d->d_name, 0) == 0) &&
			    (access(buf, R_OK) == 0)) {
				gw_menu_item_add(fb, d->d_name,
						 (void*)dir_count++,
						 select_file, NULL);
				count++;
			}
		cont:
			free(d);
		}

		w++;
	}

	return count;
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
