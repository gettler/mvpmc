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

#ifndef PI_LOCAL_H
#define PI_LOCAL_H

#include "plugin.h"
#include "mvp_atomic.h"

#define plugin_open	__piutil_plugin_open
#define plugin_start	__piutil_plugin_start
#define plugin_request	__piutil_plugin_request
#define loaded		__piutil_loaded

typedef enum {
	PLUGIN_UNUSED = 0,
	PLUGIN_LOADING,
	PLUGIN_UNLOADING,
	PLUGIN_INUSE,
} plugin_state_t;

typedef enum {
	PI_TYPE_PLUGIN = 1,
	PI_TYPE_LIB,
} pi_type_t;

typedef struct {
	char *name;
	char *path;
	void *handle;
	unsigned long *version;
	void *(*init)(void);
	int (*release)(void);
	mvp_atomic_t refcnt;
	void *reloc;
	volatile plugin_state_t state;
	pi_type_t type;
} plugin_data_t;

extern void* plugin_start(int n);
extern void* plugin_open(char *name, char *path);
extern void* plugin_request(int);

extern volatile plugin_data_t *loaded;

#endif /* PI_LOCAL_H */
