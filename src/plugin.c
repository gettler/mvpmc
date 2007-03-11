/*
 *  Copyright (C) 2007, Jon Gettler
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

#include "plugin.h"
#include "mvp_atomic.h"

#if defined(PLUGIN_SUPPORT)
#include <dlfcn.h>
#endif

static unsigned long version = PLUGIN_VERSION(PLUGIN_MAJOR_VER,PLUGIN_MINOR_VER);

#if defined(PLUGIN_SUPPORT)
static int enabled = 1;
#else
static int enabled = 0;
#endif

#define MAX_LOAD	128

typedef struct {
	char *name;
	void *handle;
	unsigned long *version;
	int (*init)(void);
	int (*release)(void);
	mvp_atomic_t refcnt;
} plugin_data_t;

static plugin_data_t loaded[MAX_LOAD];
static int nload = 0;

typedef struct {
	char *name;
	int (*init)(void);
	int (*release)(void);
} builtin_t;

static builtin_t builtins[] = {
#if !defined(PLUGIN_SUPPORT)
	{ "html", plugin_init_html, plugin_release_html },
	{ "osd", plugin_init_osd, plugin_release_osd },
#endif /* !PLUGIN_SUPPORT */
	{ NULL, NULL, NULL }
};

#if defined(PLUGIN_SUPPORT)
static int
plugin_start(int n)
{
	if ((loaded[n].init == NULL) || (loaded[n].release == NULL)) {
		return -1;
	}

	if ((loaded[n].version == NULL) || (*(loaded[n].version) != version)) {
		return -1;
	}

	if (loaded[n].init() < 0) {
		dlclose(loaded[n].handle);
		free(loaded[n].name);
		memset(loaded+n, 0, sizeof(loaded[0]));
		nload--;
		return -1;
	}

	return 0;
}

static int
plugin_stop(int n)
{
	int rc;

	if ((rc=loaded[n].release()) == 0) {
		dlclose(loaded[n].handle);
		free(loaded[n].name);
		memset(loaded+n, 0, sizeof(loaded[0]));
		nload--;
	}

	return rc;
}

static char*
plugin_find(char *name)
{
	char *colon;
	char *search = NULL, *path = NULL;

	if ((search=getenv("PLUGIN_PATH")) == NULL) {
		search = PLUGIN_PATH;
	}

	if ((search=strdup(search)) == NULL) {
		goto err;
	}
	if ((path=malloc(strlen(search)+strlen(name)+2)) == NULL) {
		goto err;
	}

	/*
	 * XXX: is ; an acceptable seperator for pathnames and URLs?
	 */
	while (search) {
		if ((colon=strchr(search, ';')) != NULL) {
			*(colon++) = '\0';
		}
		sprintf(path, "%s/%s", search, path);
		if (access(path, R_OK|X_OK) == 0) {
			break;
		}
		search = colon;
	}

	free(path);
	path = NULL;

 err:
	if (search)
		free(search);

	return path;
}

static int
plugin_load_file(char *name)
{
	char filename[PLUGIN_NAME_MAX+128];
	char *path;
	int i, rc = -1;

	if ((name == NULL) || (strlen(name) >= PLUGIN_NAME_MAX)) {
		return -1;
	}

	snprintf(filename, sizeof(filename), "mvpmc_%s.plugin.%lu.%lu",
		 name, PLUGIN_MAJOR(version), PLUGIN_MINOR(version));
	if ((path=plugin_find(filename)) != NULL) {
		goto found;
	}

	snprintf(filename, sizeof(filename), "mvpmc_%s.plugin.%lu",
		 name, PLUGIN_MAJOR(version));
	if ((path=plugin_find(filename)) != NULL) {
		goto found;
	}

	snprintf(filename, sizeof(filename), "mvpmc_%s.plugin", name);
	if ((path=plugin_find(filename)) != NULL) {
		goto found;
	}

	return -1;

 found:
	for (i=0; i<MAX_LOAD; i++) {
		if (loaded[i].name == NULL) {
			void *handle;
			int (*dl_init)(void);
			int (*dl_release)(void);
			unsigned long *dl_ver;

			if ((handle=dlopen(path, RTLD_LAZY)) != NULL) {
				dl_init = dlsym(handle, "plugin_init");
				dl_release = dlsym(handle, "plugin_release");
				dl_ver = dlsym(handle, "plugin_version");

				if ((dl_init == NULL) ||
				    (dl_release == NULL) ||
				    (dl_ver == NULL)) {
					break;
				}

				loaded[i].name = strdup(name);
				loaded[i].handle = handle;
				loaded[i].init = dl_init;
				loaded[i].release = dl_release;
				loaded[i].version = dl_ver;
				mvp_atomic_set(&(loaded[i].refcnt), 1);

				nload++;

				rc = plugin_start(i);
			}
			break;
		}
	}

	free(path);

	return rc;
}
#endif /* PLUGIN_SUPPORT */

static int
plugin_load_builtin(int n)
{
	int rc = -1;
	int i;

	for (i=0; i<MAX_LOAD; i++) {
		if (loaded[i].name == NULL) {
			loaded[i].name = strdup(builtins[n].name);
			loaded[i].init = builtins[n].init;
			loaded[i].release = builtins[n].release;
			loaded[i].version = &version;
			/* Set refcnt to 2 to prevent unloading */
			mvp_atomic_set(&(loaded[i].refcnt), 2);

			nload++;

			rc = plugin_start(i);
		}
	}

	return rc;
}

int
plugin_load(char *name)
{
	int rc = -1;
	int i;

	if (name == NULL) {
		return -1;
	}

	for (i=0; i<MAX_LOAD; i++) {
		if (loaded[i].name && (strcmp(loaded[i].name, name) == 0)) {
			rc = 0;
			mvp_atomic_inc(&(loaded[i].refcnt));
			break;
		}
	}

	if (rc < 0) {
		i = 0;
		while (builtins[i].name) {
			if (strcmp(builtins[i].name, name) == 0) {
				rc = plugin_load_builtin(i);
				break;
			}
			i++;
		}
	}

#if defined(PLUGIN_SUPPORT)
	if ((rc < 0) && (nload < MAX_LOAD)) {
		rc = plugin_load_file(name);
	}
#endif /* PLUGIN_SUPPORT */

	return rc;
}

int
plugin_unload(char *name)
{
	int rc = -1;

#if defined(PLUGIN_SUPPORT)
	int i;

	if (name == NULL) {
		return -1;
	}

	for (i=0; i<MAX_LOAD; i++) {
		if (loaded[i].name && (strcmp(loaded[i].name, name) == 0)) {
			int unload;
			unload = mvp_atomic_dec_and_test(&(loaded[i].refcnt));
			if (unload) {
				rc = plugin_stop(i);
			} else {
				rc = 1;
			}
			break;
		}
	}
#endif /* PLUGIN_SUPPORT */

	return rc;
}

int
plugin_setup(void)
{
	printf("Plug-in setup: version %lu.%lu, %d builtins\n",
	       PLUGIN_MAJOR(version), PLUGIN_MINOR(version),
	       (sizeof(builtins)/sizeof(builtins[0]))-1);

	if (!enabled) {
		printf("Loadable plug-in support disabled\n");
	}

	return 0;
}

