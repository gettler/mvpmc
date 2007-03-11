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

static int
gw_create_container(gw_t *widget)
{
	widget->data.container =
		(gw_container_t*)ref_alloc(sizeof(gw_container_t*));

	if (widget->data.container == NULL) {
		return -1;
	}

	widget->data.container->child = NULL;

	return 0;
}

static int
gw_create_menu(gw_t *widget)
{
	widget->data.menu = (gw_menu_t*)ref_alloc(sizeof(gw_menu_t*));

	if (widget->data.menu == NULL) {
		return -1;
	}

	memset(widget->data.menu, 0, sizeof(gw_menu_t));

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
gw_realize(gw_t *widget)
{
	if (widget == NULL) {
		return -1;
	}

	widget->realized = 1;

	return 0;
}
