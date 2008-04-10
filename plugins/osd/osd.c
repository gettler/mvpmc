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
#include <signal.h>

#include "mvp_refmem.h"
#include "mvp_osd.h"

#include <plugin.h>
#include <plugin/osd.h>

#include "input.h"

#include "osd_local.h"

#if defined(PLUGIN_SUPPORT)
unsigned long plugin_version = CURRENT_PLUGIN_VERSION;
#endif

static int fd = -1;
static input_t *input;

static osd_widget_t *list[128];

int screen_width = -1, screen_height = -1;

static int
osd_input_fd(void)
{
	return fd;
}

static int
osd_input_read(void)
{
	return input_read(input);
}

static int
widget_draw(gw_t *widget)
{
	osd_widget_t *cur;

	/*
	 * Draw the widgets in wlist
	 */

	cur = lookup(widget);

	while (cur) {
		osd_widget_t *list;

		draw(cur);

		list = cur->child;
		while (list) {
			widget_draw(list->gw);
#if 0
			list = list->next;
#else
			list = NULL;
#endif
		}

		cur = cur->next;
	}

	return 0;
}

static int
widget_update(gw_t *root)
{
	/*
	 * Make sure widget lists is up-to-date
	 */

	return 0;
}

static int
osd_generate(gw_t *root)
{
	int count;

	if (root->type != GW_TYPE_CONTAINER) {
		return -1;
	}

	if (widget_update(root) < 0) {
		return -1;
	}

	count = osd_css_arrange(root);

	if (count < 0) {
		return -1;
	}
	if (count == 0) {
		return 0;
	}

	if (widget_draw(root) < 0) {
		return -1;
	}

	return 0;
}

osd_widget_t*
lookup(gw_t *widget)
{
	osd_widget_t *new;
	int i;

	if (widget == NULL) {
		return NULL;
	}

	for (i=0; i<128; i++) {
		if (list[i] && (list[i]->gw == widget)) {
			return list[i];
		}
	}

	new = ref_alloc(sizeof(*new));

	memset(new, 0, sizeof(*new));

	new->gw = widget;
	if (widget->realized) {
		new->visible = true;
	}

	for (i=0; i<128; i++) {
		if (list[i] == NULL) {
			list[i] = new;
			break;
		}
	}

	return new;
}

static void
update_list(gw_t *widget, bool map, osd_widget_t **list)
{
	osd_widget_t *w;

	if ((widget == NULL) || (widget->updating))
		return;

	ref_hold(widget);
	widget->updating = true;

	w = lookup(widget);
	w->visible = map;

	printf("%s(): %p '%s'\n", __FUNCTION__, w, widget->name);

	if (widget->type == GW_TYPE_CONTAINER) {
		gw_t *child = widget->data.container->child;

		while (child) {
			printf("%s(): add child %p '%s'\n",
			       __FUNCTION__, child, child->name);
			list_add(&w->child, child, NULL);
			update_list(child, map, &w->child);
#if 0
			child = child->next;
#else
			child = NULL;
#endif
		}
	}

	list_add(list, widget, w);

	widget->updating = false;
	ref_release(widget);
}

static bool
mapped(gw_t *widget)
{
	if (widget->parent == NULL)
		return widget->realized;

	if (widget->realized == false)
		return false;

	return mapped(widget->parent);
}

static int
osd_update(gw_t *widget)
{
	osd_widget_t *w;

	bool map = mapped(widget);

	ref_hold(widget);

	w = lookup(widget->parent);

	if (w) {
		update_list(widget, map, &w->child);
	} else {
		update_list(widget, map, &wlist);
	}

	ref_release(widget);

	return 0;
}

static plugin_osd_t reloc = {
	.input_fd = osd_input_fd,
	.input_read = osd_input_read,
	.css_load = osd_css_load,
	.generate = osd_generate,
	.update_widget = osd_update,
};

static void*
init_osd(void)
{
	if (osd_open() < 0) {
		return NULL;
	}

	osd_get_screen_size(&screen_width, &screen_height);

	osd_css_load(CSS_DEFAULT);

	if (input_init() < 0) {
		return NULL;
	}

	if ((input=input_open(INPUT_KEYBOARD, 0)) == NULL) {
		return NULL;
	}

	if ((fd=input_fd(input)) < 0) {
		return NULL;
	}

	if (draw_init() < 0) {
		return NULL;
	}

	if (css_init() < 0) {
		return NULL;
	}

	printf("OSD plug-in registered!\n");

	return &reloc;
}

static int
release_osd(void)
{
	osd_close();
	input_release();

	fd = -1;

	printf("OSD plug-in deregistered!\n");

	return 0;
}

PLUGIN_INIT(init_osd);
PLUGIN_RELEASE(release_osd);
