/*
 *  Copyright (C) 2008-2010, Jon Gettler
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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <plugin.h>
#include <plugin/app.h>
#include <mvp_refmem.h>
#include <input.h>

#include "myth_local.h"

#if defined(PLUGIN_SUPPORT)
unsigned long plugin_version = CURRENT_PLUGIN_VERSION;
#endif

static gw_t *menu;
static int level = 0;
static void (*return_func)(void) = NULL;
static int top(gw_t *widget, char *text, void *key);

char *recdir;

static int
set_top(void)
{
	gw_menu_clear(menu);
	gw_menu_title_set(menu, "MythTV");
	gw_menu_item_add(menu, "Watch Recordings", (void*)0, top, NULL);
	gw_menu_item_add(menu, "Upcoming Recordings", (void*)1, top, NULL);

	gw_output();

	return 0;
}

static int
do_key(gw_t *widget, int key)
{
	if ((level == 0) && (key == INPUT_CMD_LEFT)) {
		gw_unmap(menu);
		if (return_func) {
			return_func();
		}

		return 0;
	}

	level--;

	if (level == 0) {
		set_top();
	}

	return 0;
}

static int
myth_enter(void (*cb)(void))
{
	printf("map myth menu\n");

	return_func = cb;

	gw_map(menu);
	gw_focus_set(menu);
	gw_focus_cb_set(do_key);

#if defined(MVPMC_NMT)
	if (recdir == NULL) {
		char buf[512];
		int status;

		system("insmod $ROOT/lib/modules/2.6.15-sigma/fuse.ko");
		mkdir("/tmp/mvpmc_mythtv", 0755);
		status = system("mythfuse /tmp/mvpmc_mythtv");

		snprintf(buf, sizeof(buf),
			 "/tmp/mvpmc_mythtv/%s/files", server);
		recdir = strdup(buf);
	}
#endif /* MVPMC_NMT */

	return 0;
}

static int
myth_exit(void)
{
	gw_unmap(menu);

	return 0;
}

static int
myth_option(char *option, void *val)
{
	if (strcmp(option, "server") == 0) {
		server = strdup((char*)val);
	} else if (strcmp(option, "recdir") == 0) {
		recdir = strdup((char*)val);
	}

	return 0;
}

static plugin_app_t reloc = {
	.name = "MythTV",
	.enter = myth_enter,
	.exit = myth_exit,
	.option = myth_option,
};

static int
top(gw_t *widget, char *text, void *key)
{
	intptr_t item = (intptr_t)key;

	level++;
	gw_menu_clear(widget);

	switch (item) {
	case 0:
		gw_menu_title_set(widget, "Watch Recordings");
		rec_list(widget);
		break;
	case 1:
		gw_menu_title_set(widget, "Upcoming Recordings");
		break;
	}

	return 0;
}

static void*
init_myth(void)
{
	gw_t *root;

	root = gw_root(ROOT_CONSOLE);

	if ((menu=gw_create(GW_TYPE_MENU, root)) == NULL)
		goto err;

	set_top();

	return &reloc;

err:
	return NULL;
}

static int
release_myth(void)
{
	return 0;
}

PLUGIN_INIT(init_myth);
PLUGIN_RELEASE(release_myth);
