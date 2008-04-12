/*
 *  Copyright (C) 2007-2008, Jon Gettler
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
#include <sys/ipc.h>
#include <sys/shm.h>

#include "plugin.h"
#include "piutil.h"
#include "mvp_atomic.h"
#include "pi_local.h"

#if defined(PLUGIN_SUPPORT)
#include <dlfcn.h>
#endif

static unsigned long version = PLUGIN_VERSION(PLUGIN_MAJOR_VER,PLUGIN_MINOR_VER);

#if defined(PLUGIN_SUPPORT)
static int enabled = 1;
#else
static int enabled = 0;
#endif

static builtin_t *builtins;

#if defined(PLUGIN_SUPPORT)
void*
plugin_start(int n)
{
	void *reloc;

	if ((loaded[n].init == NULL) || (loaded[n].release == NULL)) {
		return NULL;
	}

	if ((loaded[n].version == NULL) || (*(loaded[n].version) != version)) {
		return NULL;
	}

	if ((reloc=loaded[n].init()) == NULL) {
		dlclose(loaded[n].handle);
		free(loaded[n].name);
		memset((void*)(loaded+n), 0, sizeof(loaded[0]));
		return NULL;
	}

	return reloc;
}

static int
plugin_stop(int n)
{
	int rc;

	if ((rc=loaded[n].release()) == 0) {
		dlclose(loaded[n].handle);
		free(loaded[n].name);
		memset((void*)(loaded+n), 0, sizeof(loaded[0]));
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
		sprintf(path, "%s/%s", search, name);
		if (access(path, R_OK|X_OK) == 0) {
			printf("PLUGIN: found %s\n", path);
			goto found;
		}
		search = colon;
	}

	free(path);
	path = NULL;

 found:
 err:
	if (search)
		free(search);

	return path;
}

static void*
plugin_load_file(char *name)
{
	char filename[PLUGIN_NAME_MAX+128];
	char *path;
	int i;
	void *reloc = NULL;

	if ((name == NULL) || (strlen(name) >= PLUGIN_NAME_MAX)) {
		return NULL;
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

	snprintf(filename, sizeof(filename), "libmvpmc_%s.plugin", name);
	if ((path=plugin_find(filename)) != NULL) {
		goto found;
	}

	return NULL;

 found:
	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if (loaded[i].state == PLUGIN_UNUSED) {
			loaded[i].state = PLUGIN_LOADING;
			loaded[i].type = PI_TYPE_PLUGIN;
			loaded[i].name = strdup(name);
			loaded[i].path = strdup(path);
			reloc = plugin_request(i);
			break;
		}
	}

	free(path);

	return reloc;
}
#else
void*
plugin_start(int n)
{
	void *reloc;

	if ((reloc=loaded[n].init()) == NULL) {
		return NULL;
	}

	return reloc;
}
#endif /* PLUGIN_SUPPORT */

static void*
plugin_load_builtin(int n)
{
	void *rc = NULL;
	int i;

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if (loaded[i].name == NULL) {
			loaded[i].state = PLUGIN_LOADING;
			loaded[i].name = strdup(builtins[n].name);
			loaded[i].init = builtins[n].init;
			loaded[i].release = builtins[n].release;
			loaded[i].version = &version;
			/* Set refcnt to 2 to prevent unloading */
			mvp_atomic_set(&(loaded[i].refcnt), 2);

			rc = plugin_start(i);
			break;
		}
	}

	printf("%s(): %d\n", __FUNCTION__, __LINE__);

	return rc;
}

void*
plugin_load(char *name)
{
	void *reloc = NULL;
	int i;

	if (name == NULL) {
		return NULL;
	}

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if (loaded[i].name && (strcmp(loaded[i].name, name) == 0)) {
			reloc = loaded[i].reloc;
			mvp_atomic_inc(&(loaded[i].refcnt));
			break;
		}
	}

	if (reloc == NULL) {
		i = 0;
		while (builtins[i].name) {
			if (strcmp(builtins[i].name, name) == 0) {
				reloc = plugin_load_builtin(i);
				break;
			}
			i++;
		}
	}

#if defined(PLUGIN_SUPPORT)
	if (reloc == NULL) {
		reloc = plugin_load_file(name);
	}
#endif /* PLUGIN_SUPPORT */

	return reloc;
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

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
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
plugin_setup(builtin_t *bi, int n)
{
	builtins = bi;

	printf("Plug-in setup: version %lu.%lu, %d builtins\n",
	       PLUGIN_MAJOR(version), PLUGIN_MINOR(version), n);

	if (pi_init(1) < 0) {
		return -1;
	}

	if (!enabled) {
		printf("Loadable plug-in support disabled\n");
		return 0;
	}

	return 0;
}

