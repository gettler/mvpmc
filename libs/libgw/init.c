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
#include <sys/select.h>

#include "plugin.h"
#include "gw_local.h"

#include "plugin_html.h"
#include "plugin_osd.h"

gw_t *root = NULL;
gw_t *commands = NULL;

plugin_osd_t *osd = NULL;
plugin_html_t *html = NULL;

gw_t*
gw_root(void)
{
	return root;
}

static int
gw_load_plugins(unsigned int dev)
{
	unsigned int loaded = 0;

	if ((dev & GW_DEV_HTML) && ((html=plugin_load("html")) == NULL)) {
		goto err;
	}
	loaded |= GW_DEV_HTML;

	if ((dev & GW_DEV_OSD) && ((osd=plugin_load("osd")) == NULL)) {
		goto err;
	}
	loaded |= GW_DEV_OSD;

	return 0;

 err:
	if (loaded & GW_DEV_OSD) {
		plugin_unload("osd");
		osd = NULL;
	}

	if (loaded & GW_DEV_HTML) {
		plugin_unload("html");
		html = NULL;
	}

	return -1;
}

int
gw_device_add(unsigned int dev)
{
	return -1;
}

int
gw_device_remove(unsigned int dev)
{
	return -1;
}

int
gw_init(unsigned int dev)
{
	if (root != NULL) {
		return -1;
	}

	if (dev == 0) {
		return -1;
	}

	if (gw_load_plugins(dev) < 0) {
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

int
gw_output(void)
{
#if 0
	plugin_html_t *h = (plugin_html_t*)html;

	h->generate(fileno(stdout));
#endif

	osd->generate(root);

	return 0;
}