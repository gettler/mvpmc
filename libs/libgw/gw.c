/*
 *  Copyright (C) 2007, Jon Gettler
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

#include "gw_local.h"

static mvp_atomic_t events;

static gw_t *focus;

static gw_select_t focus_input;

static int
gw_create_container(gw_t *widget)
{
	widget->data.container =
		(gw_container_t*)ref_alloc(sizeof(gw_container_t));

	if (widget->data.container == NULL) {
		return -1;
	}

	widget->data.container->child = NULL;

	gw_name_set(widget, "container");

	return 0;
}

static int
gw_create_menu(gw_t *widget)
{
	widget->data.menu = (gw_menu_t*)ref_alloc(sizeof(gw_menu_t));

	if (widget->data.menu == NULL) {
		return -1;
	}

	memset(widget->data.menu, 0, sizeof(gw_menu_t));

	gw_name_set(widget, "menu");

	return 0;
}

static int
gw_create_text(gw_t *widget)
{
	widget->data.text = (gw_text_t*)ref_alloc(sizeof(gw_text_t));

	if (widget->data.text == NULL) {
		return -1;
	}

	memset(widget->data.text, 0, sizeof(gw_text_t));

	gw_name_set(widget, "text");

	return 0;
}

static int
add_child(gw_t *parent, gw_t *child)
{
	gw_t *cur;
	gw_container_t *c;

	if (parent->type != GW_TYPE_CONTAINER) {
		return -1;
	}

	c = parent->data.container;

	cur = c->child;

	if (cur) {
		do {
			child->above = cur;
			cur = cur->below;
		} while (cur);
	} else {
		child->above = NULL;
		c->child = child;
	}

	child->below = NULL;

	return 0;
}

gw_t*
gw_create(gw_type_t type, gw_t *parent)
{
	gw_t *gw = NULL;

	if (parent == NULL) {
		parent = root;
	}

	if ((gw=(gw_t*)ref_alloc(sizeof(gw_t))) == NULL) {
		return NULL;
	}
	memset(gw, 0, sizeof(*gw));

	switch (type) {
	case GW_TYPE_CONTAINER:
		gw_create_container(gw);
		break;
	case GW_TYPE_MENU:
		gw_create_menu(gw);
		break;
	case GW_TYPE_TEXT:
		gw_create_text(gw);
		break;
	default:
		break;
	}

	if (gw) {
		gw->type = type;
		gw->realized = false;
		gw->event_mask = 0;
		gw->parent = parent;

		if (add_child(parent, gw) < 0) {
			ref_release(gw);
			gw = NULL;
		}

		update(parent);
		update(gw);
	}

	return gw;
}

int
gw_name_set(gw_t *widget, char *name)
{
	char *p;

	if ((p=ref_strdup(name)) == NULL) {
		return -1;
	}

	ref_release(widget->name);

	widget->name = p;

	return 0;
}

int
gw_map(gw_t *widget)
{
	if (widget == NULL) {
		return -1;
	}

	if (widget->realized) {
		return -1;
	}

	widget->realized = true;

	update(widget);

	return 0;
}

int
gw_unmap(gw_t *widget)
{
	if (widget == NULL) {
		return -1;
	}

	if (!widget->realized) {
		return -1;
	}

	widget->realized = false;

	update(widget);

	return 0;
}

static void
handle_events(void)
{
	/*
	 * Inform each plug-in that they should update their state
	 */

	gw_output();
}

static void
input(int c)
{
	int ret = -1;

	if (focus == NULL)
		return;

	switch (focus->type) {
	case GW_TYPE_MENU:
		printf("Add input to menu...\n");
		ret = gw_menu_input(focus, c);
		break;
	case GW_TYPE_TEXT:
		if (focus_input) {
			ret = focus_input(focus);
		}
		break;
	default:
		return;
	}

	if (ret == 0) {
		mvp_atomic_inc(&events);
	}
}

int
gw_loop(struct timeval *to)
{
	int c;
	int fd;
	fd_set fds;

	fd = osd->input_fd();

	printf("input fd %d\n", fd);

	while (1) {
		/*
		 * Handle outstanding events.
		 */
		if (mvp_atomic_val(&events) > 0) {
			mvp_atomic_set(&events, 0);
			handle_events();
			continue;
		}

		/*
		 * Block on all file descriptors for new events.
		 */
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		if (select(fd+1, &fds, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(fd, &fds)) {
				c = osd->input_read();
				printf("key pressed: 0x%x!\n", c);
				input(c);
			}
		}
	}

	return 0;
}

int
gw_text_set(gw_t *widget, char *text)
{
	char *next;

	if (widget->type != GW_TYPE_TEXT) {
		return -1;
	}

	if (widget->data.text == NULL) {
		widget->data.text = (gw_text_t*)ref_alloc(sizeof(gw_text_t));
		if (widget->data.text == NULL) {
			return -1;
		}
		memset(widget->data.text, 0, sizeof(gw_text_t));
	}

	if ((next=ref_strdup(text)) == NULL) {
		return -1;
	}

	if (widget->data.text->text != NULL) {
		ref_release(widget->data.text->text);
	}

	widget->data.text->text = next;

	return 0;
}

int
update(gw_t *widget)
{
	mvp_atomic_inc(&events);

	/*
	 * Inform each plug-in that cares of a change in this widget
	 */

	if (osd && osd->update_widget) {
		osd->update_widget(widget);
	}
	if (html && html->update_widget) {
		html->update_widget(widget);
	}

	return 0;
}

int
gw_focus_set(gw_t *widget)
{
	if ((widget != NULL) && (!widget->realized)) {
		return -1;
	}

	focus = widget;

	return 0;
}

int
gw_focus_cb_set(gw_select_t input)
{
	focus_input = input;

	return 0;
}
