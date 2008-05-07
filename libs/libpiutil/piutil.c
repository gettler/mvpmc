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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dlfcn.h>
#include <pthread.h>

#include "plugin.h"
#include "pi_local.h"

static plugin_data_t *shmaddr = NULL;
static int shmid;

#if defined(PLUGIN_SUPPORT)
static pthread_t thread;

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif /* PLUGIN_SUPPORT */

volatile plugin_data_t *loaded;

static volatile int loader_started = 0;

static void
plugin_teardown(void)
{
	if (shmaddr) {
		loaded = NULL;
		if (shmdt(shmaddr) == 0) {
			shmctl(shmid, IPC_RMID, NULL);
		}
		shmaddr = NULL;
	}
}

static int
plugin_shmem(int reset)
{
	int size;

	size = PLUGIN_MAX_LOAD * sizeof(plugin_data_t);

#if defined(MVPMC_MG35)
	if ((shmaddr=(plugin_data_t*)malloc(size)) == NULL) {
		return -1;
	}
	memset(shmaddr, 0, size);
#else
	if ((shmid=shmget(0x42e51023, size, IPC_CREAT|0600)) < 0) {
		if (errno != EEXIST) {
			return -1;
		}
	}

	if ((shmaddr=(plugin_data_t*)shmat(shmid, NULL, 0)) == (void*)-1) {
		shmaddr = NULL;
		return -1;
	}
#endif

	loaded = shmaddr;

	if (reset) {
		printf("piutil: reset shmaddr!!!\n");
		memset(shmaddr, 0, size);

		atexit(plugin_teardown);
	}

	return 0;
}

#if defined(PLUGIN_SUPPORT)
static void*
load_thread(void *arg)
{
	int i;

	pthread_mutex_lock(&mutex);

	loader_started = 1;
	pthread_cond_broadcast(&cond);

	while (1) {
		pthread_cond_wait(&cond, &mutex);
		for (i=0; i<PLUGIN_MAX_LOAD; i++) {
			if (loaded[i].state == PLUGIN_LOADING) {
				plugin_open(loaded[i].name, loaded[i].path);
			}
		}
		pthread_cond_broadcast(&cond);
	}

	return NULL;
}

void*
plugin_request(int i)
{
	if (pthread_self() == thread) {
		return plugin_open(loaded[i].name, loaded[i].path);
	}

	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&cond);

	while (loaded[i].state == PLUGIN_LOADING) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);

	return loaded[i].reloc;
}

void*
plugin_open(char *name, char *path)
{
	void *reloc = NULL;
	int i;

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if ((loaded[i].state == PLUGIN_LOADING) &&
		    (strcmp(loaded[i].name, name) == 0)) {
			void *handle;
			void *(*dl_init)(void) = NULL;
			int (*dl_release)(void) = NULL;
			unsigned long *dl_ver = NULL;

			if ((handle=dlopen(path, RTLD_LAZY)) != NULL) {
				if (loaded[i].type == PI_TYPE_PLUGIN) {
					dl_init = dlsym(handle,
							"plugin_init");
					dl_release = dlsym(handle,
							   "plugin_release");
					dl_ver = dlsym(handle,
						       "plugin_version");

					if ((dl_init == NULL) ||
					    (dl_release == NULL) ||
					    (dl_ver == NULL)) {
						break;
					}
				}

				loaded[i].name = strdup(name);
				loaded[i].handle = handle;
				loaded[i].init = dl_init;
				loaded[i].release = dl_release;
				loaded[i].version = dl_ver;
				mvp_atomic_set(&(loaded[i].refcnt), 1);

				if (loaded[i].type == PI_TYPE_PLUGIN) {
					reloc = plugin_start(i);
				}

				loaded[i].reloc = reloc;

				loaded[i].state = PLUGIN_INUSE;
			}
			break;
		}
	}

	return reloc;
}
#endif /* PLUGIN_SUPPORT */

int
pi_init(int reset)
{
	if (plugin_shmem(reset) < 0) {
		return -1;
	}

#if defined(PLUGIN_SUPPORT)
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024*64);

	pthread_mutex_lock(&mutex);

	pthread_create(&thread, &attr, load_thread, NULL);

	while (!loader_started) {
		pthread_cond_wait(&cond, &mutex);
	}

	pthread_mutex_unlock(&mutex);
#endif /* PLUGIN_SUPPORT */

	printf("piutil initialized!\n");

	return 0;
}

int
pi_deinit(void)
{
	return 0;
}

#if defined(PLUGIN_SUPPORT)
static void*
pi_find(char *name)
{
	int i;

	if (shmaddr == NULL) {
		return NULL;
	}

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if ((loaded[i].state == PLUGIN_INUSE) &&
		    (strcmp(shmaddr[i].name, name) == 0)) {
			mvp_atomic_inc(&(shmaddr[i].refcnt));
			return shmaddr[i].handle;
		}
	}

	return NULL;
}

void*
pi_register(char *name)
{
	int i;
	void *handle;

	if (shmaddr == NULL) {
		if (plugin_shmem(0) < 0) {
			printf("piutil: shmem creation failed!\n");
			return NULL;
		}
	}

	if ((handle=pi_find(name)) != NULL) {
		printf("piutil: found in use %s\n", name);
		return handle;
	}

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if (loaded[i].state == PLUGIN_UNUSED) {
			loaded[i].state = PLUGIN_LOADING;
			loaded[i].type = PI_TYPE_LIB;
			loaded[i].name = strdup(name);
			loaded[i].path = strdup(name);
			plugin_request(i);
			return loaded[i].handle;
		}
	}

	printf("piutil: registration failed for %s!\n", name);

	return NULL;
}

int
pi_deregister(void *handle)
{
	int i;

	printf("piutil: deregister %p\n", handle);

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if (shmaddr[i].handle == handle) {
			int ref;
			printf("piutil: deregister %s at %p\n",
			       shmaddr[i].name, handle);
			if ((ref=mvp_atomic_dec(&(shmaddr[i].refcnt))) == 0) {
				printf("piutil: calling dlclose()...\n");
				if (dlclose(shmaddr[i].handle) != 0) {
					fprintf(stderr, "dlclose(%p) failed\n",
						shmaddr[i].handle);
				}
				shmaddr[i].handle = NULL;
				return 0;
			} else {
				printf("piutil: lib in use...%d\n", ref);
				return -1;
			}
		}
	}

	printf("piutil: handle %p not found!\n", handle);

	return -1;
}
#endif /* PLUGIN_SUPPORT */
