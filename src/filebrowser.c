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
#include <sys/wait.h>

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
static gw_t *image;

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
static int do_key_image(gw_t *widget, int key);

extern plugin_av_t *av;

typedef struct {
	char *path;
	char *opt;
} servlink_t;

static servlink_t sl[16];

#if defined(MVPMC_NMT)
static char*
get_servlink_path(char *cmd)
{
	char *c;
	char *root = "/opt/sybhttpd/localhost.drives/";
	char *proto = NULL;
	char *host = NULL;
	char *path = NULL;
	char *type = NULL;
	char buf[256];

	if ((c=strchr(cmd, '&')) != NULL) {
		*c = '\0';
	}

	if ((c=strchr(cmd, ':')) != NULL) {
		*(c++) = '\0';
		if ((*c == '/') && (*(c+1) == '/')) {
			host = c + 2;
		}
		proto = cmd;
		cmd = host;
	} else {
		return NULL;
	}

	if ((c=strchr(cmd, '/')) != NULL) {
		*(c++) = '\0';
		path = c;
	}

	if (strcmp(proto, "nfs") == 0) {
		type = "NFS";
	} else if (strcmp(proto, "smb") == 0) {
		type = "SMB";
	} else {
		return NULL;
	}

	if ((proto == NULL) || (host == NULL) || (path == NULL)) {
		return NULL;
	}

	while ((c=strchr(path, '/')) != NULL) {
		*c = ':';
	}

	snprintf(buf, sizeof(buf), "%s[%s] %s:%s/", root, type, host, path);

	return strdup(buf);
}
#endif /* MVPMC_NMT */

static int
get_servlink(void)
{
	int n = 0;
#if defined(MVPMC_NMT)
	FILE *f;
	char line[256];

	if ((f=fopen("/tmp/setting.txt", "r")) == NULL) {
		return -1;
	}

	while (fgets(line, sizeof(line), f) != NULL) {
		char *p;

		if ((p=strchr(line, '=')) != NULL) {
			*(p++) = '\0';
			if (strncmp(line, "servlink", 8) == 0) {
				sl[n].opt = strdup(p);
				if ((sl[n].path=get_servlink_path(p)) != NULL) {
					n++;
				}
			}
		}
	}

	fclose(f);
#endif /* MVPMC_NMT */

	return n;
}

int
fb_init(gw_t *root)
{
	int i;

	if ((fb=gw_create(GW_TYPE_MENU, root)) == NULL)
		goto err;
	if ((image=gw_create(GW_TYPE_IMAGE, root)) == NULL)
		goto err;
	gw_name_set(image, "image_viewer");

	if (get_servlink() < 0) {
		fprintf(stderr, "servlink error!\n");
	} else {
		i = 0;
		while (sl[i].path) {
			printf("servlink: '%s'\n", sl[i].path);
			i++;
		}
	}

	return 0;

 err:
	return -1;
}

static void
mount_share(char *dir)
{
#if defined(MVPMC_NMT)
	int i;

	if ((strcmp(dir, "/") == 0) || (strcmp(dir, "../") == 0)) {
		return;
	}

	i = 0;
	while (sl[i].path) {
		char buf[256];

		snprintf(buf, sizeof(buf), "%s%s", cwd, dir);

		if (strcmp(buf, sl[i].path) == 0) {
			pid_t child;

			printf("Mounting directory %s\n", dir);

			if ((child=fork()) == 0) {
				char *argv[3];

				snprintf(buf, sizeof(buf),
					 "smb.cmd=mount&smb.opt=%s",
					 sl[i].opt);

				argv[0] = "/opt/sybhttpd/default/smbclient.cgi";
				argv[1] = buf;
				argv[2] = NULL;
				execv(argv[0], argv);
				exit(1);
			}

			while (waitpid(child, NULL, 0) != child)
				;
			break;

		}
		i++;
	}
#endif /* MVPMC_NMT */
}

static int
select_dir(gw_t *widget, char *text, void *key)
{
	char *buf;
	char *dvd_path;
	char *dir = ref_hold(text);
	extern void fb_display(void);

	printf("Select dir: '%s'\n", dir);

	mount_share(dir);

	if ((dvd_path=malloc(strlen(cwd)+strlen(dir)+32)) != NULL) {
		sprintf(dvd_path, "%s/%s", cwd, dir);
		if (av->play_dvd(dvd_path, NULL) == 0) {
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
is_image(char *item)
{
	char *wc[] = { ".bmp", ".gif", ".jpg", ".jpeg", ".png", NULL };
	int i = 0;

	while (wc[i] != NULL) {
		if ((strlen(item) >= strlen(wc[i])) &&
		    (strcasecmp(item+strlen(item)-strlen(wc[i]), wc[i]) == 0))
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

static int
is_playlist(char *item)
{
	char *wc[] = { ".m3u", NULL };
	int i = 0;

	while (wc[i] != NULL) {
		if ((strlen(item) >= strlen(wc[i])) &&
		    (strcasecmp(item+strlen(item)-strlen(wc[i]), wc[i]) == 0))
			return 1;
		i++;
	}

	return 0;
}

static int
play_playlist(char *path)
{
	FILE *f;
	char *list[128];
	char buf[512], line[512];
	int i;

	if ((f=fopen(path, "r")) == NULL) {
		return -1;
	}

	i = 0;
	while ((fgets(line, sizeof(line), f) != NULL) && (i < 127)) {
		if (line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = '\0';
		if (line[0] == '/') {
			list[i] = strdup(line);
		} else {
			snprintf(buf, sizeof(buf), "%s%s", cwd, line);
			list[i] = strdup(buf);
		}
		i++;
	}
	list[i] = NULL;

	fclose(f);

	av->play_list(list, NULL);

	return 0;
}

static int
select_file(gw_t *widget, char *text, void *key)
{
	char path[1024];

	printf("Select file: '%s'\n", text);

	snprintf(path, sizeof(path), "%s/%s", cwd, text);

	if (is_image(text)) {
		gw_image_set(image, path);
		gw_map(image);
		gw_focus_set(image);
		gw_unmap(widget);
		gw_focus_cb_set(do_key_image);
	} else if (is_playlist(text)) {
		play_playlist(path);
	} else if (is_video(text)) {
		av->play_file(path, NULL);
	} else {
		av->play_file(path, NULL);
	}

	return 0;
}

#if defined(MVPMC_MG35)
#define alphasort	NULL

static void
sort_dirent(struct dirent **nl, int n)
{
	int sorted;

	do {
		int i;

		sorted = 1;
		for (i=0; i<n-1; i++) {
			if (strcmp(nl[i]->d_name, nl[i+1]->d_name) > 0) {
				struct dirent *d;
				d = nl[i];
				nl[i] = nl[i+1];
				nl[i+1] = d;
				sorted = 0;
			}
		}
	} while (!sorted);
}
#endif /* MVPMC_MG35 */

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

#if defined(MVPMC_MG35)
	/* alphasort() doesn't seem to get called from scandir()... */
	sort_dirent(nl, n);
#endif

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

#if defined(MVPMC_MG35)
		/* alphasort() doesn't seem to get called from scandir()... */
		sort_dirent(nl, n);
#endif

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

static int
do_key_image(gw_t *widget, int key)
{
	gw_unmap(widget);
	gw_map(fb);
	gw_focus_set(fb);
	gw_focus_cb_set(do_key);

	return 0;
}

void
fb_display(void)
{
	gw_menu_clear(fb);

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
