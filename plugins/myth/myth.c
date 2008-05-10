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
#include <signal.h>
#include <pthread.h>

#include <plugin.h>
#include <plugin/app.h>

#include "mvp_refmem.h"
#include "input.h"

#if defined(PLUGIN_SUPPORT)
unsigned long plugin_version = CURRENT_PLUGIN_VERSION;
#endif

static gw_t *menu;
static int level = 0;
static void (*return_func)(void) = NULL;
static int top(gw_t *widget, char *text, void *key);

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

	return 0;
}

static int
myth_exit(void)
{
	return 0;
}

static plugin_app_t reloc = {
	.name = "MythTV",
	.enter = myth_enter,
	.exit = myth_exit,
};

static int
top(gw_t *widget, char *text, void *key)
{
	int item = (int)key;

	level++;
	gw_menu_clear(widget);

	switch (item) {
	case 0:
		gw_menu_title_set(widget, "Watch Recordings");
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

	root = gw_root();

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
