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
#include "input.h"

int
gw_menu_title_set(gw_t *widget, char *title)
{
	gw_menu_t *data;
	char *p;

	if (widget->type != GW_TYPE_MENU) {
		return -1;
	}

	data = widget->data.menu;

	if ((p=ref_strdup(title)) == NULL) {
		return -1;
	}

	if (data->title) {
		ref_release(data->title);
	}

	data->title = p;

	return 0;
}

char*
gw_menu_title_get(gw_t *widget)
{
	gw_menu_t *data;

	if (widget->type != GW_TYPE_MENU) {
		return NULL;
	}

	data = widget->data.menu;

	return data->title;
}

int
gw_menu_item_add(gw_t *widget, char *text, void *key,
		 gw_select_t select, gw_hilite_t hilite)
{
	gw_menu_t *data;
	gw_menu_item_t **items = NULL, *item = NULL;

	if (widget->type != GW_TYPE_MENU) {
		return -1;
	}

	data = widget->data.menu;

	item = (gw_menu_item_t*)ref_alloc(sizeof(*item));

	if (item == NULL) {
		return -1;
	}

	item->text = ref_strdup(text);
	item->selectable = true;
	item->checked = false;
	item->hilited = false;
	item->select = select;
	item->hilite = hilite;
	item->key = key;

	if (data->n == 0) {
		items = (gw_menu_item_t**)ref_alloc(sizeof(*items));
		if (items == NULL) {
			goto err;
		}
		item->hilited = true;
		items[0] = item;
		data->n = 1;
		data->items = items;
	} else {
		items = (gw_menu_item_t**)ref_realloc(data->items,
						      sizeof(*items)*
						      (data->n+1));
		items[data->n++] = item;
		data->items = items;
	}

	return 0;

 err:
	if (items) {
		ref_release(items);
	}
	if (item) {
		ref_release(item);
	}

	return -1;
}

static int
menu_select(gw_t *widget)
{
	gw_menu_t *data;
	void *key;
	char *text;
	int i = 0;

	data = widget->data.menu;

	while (i < data->n) {
		if (data->items[i]->hilited) {
			printf("Select '%s'\n", data->items[i]->text);
			key = data->items[i]->key;
			text = data->items[i]->text;
			if (data->items[i]->select) {
				data->items[i]->select(widget, text, key);
				return 0;
			}
			return -1;
		}
		i++;
	}

	return -1;
}

int
gw_menu_input(gw_t *widget, int c)
{
	gw_menu_t *data;
	int i = 0, cur = -1;
	int delta;

	data = widget->data.menu;

	if (data->n < 0) {
		return -1;
	}

	printf("Input command: %d\n", c);

	switch (c) {
	case INPUT_CMD_UP:
		delta = -1;
		break;
	case INPUT_CMD_DOWN:
		delta = 1;
		break;
	case INPUT_CMD_SELECT:
		return menu_select(widget);
	case INPUT_CMD_ERROR:
		return -1;
	default:
		if (focus_input) {
			focus_input(widget, c);
		}
		return -1;
	}

	while (i < data->n) {
		if (data->items[i]->hilited) {
			cur = i;
			break;
		}
		i++;
	}

	if (cur >= 0) {
		data->items[cur]->hilited = false;
		i = (cur+data->n+delta) % data->n;
		data->items[i]->hilited = true;
		printf("Change hilite: %d --> %d\n", cur, i);
	}

	return 0;
}

int
gw_menu_clear(gw_t *widget)
{
	gw_menu_t *data;
	int i = 0;

	data = widget->data.menu;

	while (i < data->n) {
		gw_menu_item_t *item = data->items[i];
		ref_release(item->text);
		i++;
	}

	ref_release(data->items);
	data->items = NULL;
	data->n = 0;

	return 0;
}
