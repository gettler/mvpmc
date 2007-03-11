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

int
gw_menu_item_add(gw_t *widget, char *text)
{
	gw_menu_t *data;
	gw_menu_item_t **items = NULL, *item = NULL;

	if (widget->type != GW_TYPE_MENU) {
		return -1;
	}

	data = widget->data.menu;

	item = (gw_menu_item_t*)ref_alloc(sizeof(*item));

	item->text = ref_strdup(text);
	item->selectable = true;
	item->checked = false;
	item->hilite = false;

	if (item == NULL) {
		return -1;
	}

	if (data->n == 0) {
		items = (gw_menu_item_t**)ref_alloc(sizeof(*items));
		if (items == NULL) {
			goto err;
		}
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
