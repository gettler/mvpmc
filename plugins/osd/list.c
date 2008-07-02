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
#include <errno.h>
#include <signal.h>

#include "osd_local.h"

osd_widget_t *wlist = NULL;		/* visible widget list */
osd_widget_t *hlist = NULL;		/* hidden widget list */

int
list_add(osd_widget_t **head, gw_t *gw, osd_widget_t *widget)
{
	osd_widget_t *old;
	osd_widget_t *cur;

	/*
	 * XXX: are the lists ordered?
	 */

	if (widget && (widget->gw != gw)) {
		return -1;
	}

	cur = *head;
	while (cur) {
		if ((cur == widget) || (cur->gw == gw)) {
			return 0;
		}
		cur = cur->next;
	}

	if (widget == NULL) {
		if ((widget=ref_alloc(sizeof(*widget))) == NULL) {
			return -1;
		}

		widget->gw = ref_hold(gw);
		mvp_atomic_set(&widget->generation,
			       mvp_atomic_val(&gw->generation));
		widget->dirty = false;
		widget->visible = gw->realized;
	}

	old = *head;
	*head = widget;
	widget->next = old;

	return 0;
}

osd_widget_t*
list_remove(osd_widget_t **head, gw_t *gw)
{
	osd_widget_t *cur, *prev;
	osd_widget_t *ret = NULL;

	cur = *head;
	prev = NULL;

	while (cur) {
		if (cur->gw == gw) {
			if (prev) {
				prev->next = cur->next;
			} else {
				*head = cur->next;
			}
			ret = cur;
			ref_release(gw);
			break;
		}

		prev = cur;
		cur = cur->next;
	}

	return ret;
}
