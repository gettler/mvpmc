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

#include "plugin.h"

static plugin_dl_t *shmaddr = NULL;
static int shmid;

static void
plugin_teardown(void)
{
	if (shmaddr) {
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

	size = PLUGIN_MAX_LOAD * sizeof(plugin_dl_t);

	if ((shmid=shmget(0x42e51023, size, IPC_CREAT|0600)) < 0) {
		if (errno != EEXIST) {
			return -1;
		}
	}

	if ((shmaddr=(plugin_dl_t*)shmat(shmid, NULL, 0)) == (void*)-1) {
		shmaddr = NULL;
		return -1;
	}

	if (reset) {
		printf("piutil: reset shmaddr!!!\n");
		memset(shmaddr, 0, size);

		atexit(plugin_teardown);
	}

	return 0;
}

int
pi_init(int reset)
{
	if (plugin_shmem(reset) < 0) {
		return -1;
	}

	return 0;
}

int
pi_deinit(void)
{
	return 0;
}

static void*
pi_find(char *name)
{
	int i;

	if (shmaddr == NULL) {
		return NULL;
	}

	for (i=0; i<PLUGIN_MAX_LOAD; i++) {
		if ((shmaddr[i].handle != NULL) &&
		    (strcmp(shmaddr[i].name, name) == 0)) {
			mvp_atomic_inc(&(shmaddr[i].ref));
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
		if (shmaddr[i].handle == NULL) {
			if ((handle=dlopen(name, RTLD_LAZY)) == NULL) {
				return NULL;
			}
			strncpy(shmaddr[i].name, name,
				sizeof(shmaddr[i].name));
			shmaddr[i].handle = handle;
			mvp_atomic_set(&(shmaddr[i].ref), 1);

			printf("piutil: register %s at %p\n", name, handle);

			return handle;
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
			if ((ref=mvp_atomic_dec(&(shmaddr[i].ref))) == 0) {
				printf("piutil: calling dlclose()...\n");
				dlclose(shmaddr[i].handle);
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
