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

gw_t *root = NULL;
gw_t *commands = NULL;

gw_t*
gw_root(void)
{
	return root;
}

int
gw_init(void)
{
	if (root != NULL) {
		return -1;
	}

	if ((root=(gw_t*)ref_alloc(sizeof(*root))) == NULL) {
		return -1;
	}

	memset(root, 0, sizeof(*root));

	root->type = GW_TYPE_CONTAINER;
	root->realized = false;
	root->above = NULL;
	root->below = NULL;

	root->event_mask = 0;

	root->data.container =
		(gw_container_t*)ref_alloc(sizeof(gw_container_t*));

	if (root->data.container == NULL) {
		ref_release(root);
		root = NULL;
		return -1;
	}

	root->data.container->child = NULL;

	gw_name_set(root, "root");

	return 0;
}

int
gw_shutdown(void)
{
	return 0;
}
