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
#include <sys/select.h>

#include <plugin.h>
#include "gw_local.h"

#include <plugin/http.h>
#include <plugin/osd.h>
#include <plugin/screensaver.h>

#define MAX_CONSOLES	16

gw_t *root = NULL;
gw_t *commands = NULL;

plugin_osd_t *osd = NULL;
plugin_http_t *http = NULL;
plugin_screensaver_t *ss = NULL;

static volatile unsigned int current = 0;

int pipefds[2];

static gw_console_t consoles[MAX_CONSOLES];

gw_t*
gw_root(char *name)
{
	int i;

	if (name == NULL) {
		name = ROOT_CONSOLE;
	}

	for (i=0; i<MAX_CONSOLES; i++) {
		if (consoles[i].name && (strcmp(consoles[i].name, name) == 0)) {
			return consoles[i].widget;
		}
	}

	return NULL;
}

static gw_t*
console_create(char *name, int which)
{
	gw_t *root;

	if ((root=(gw_t*)ref_alloc(sizeof(*root))) == NULL) {
		return NULL;
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
		return NULL;
	}

	root->data.container->child = NULL;

	root->console = &consoles[which];

	gw_name_set(root, name);

	return root;
}

gw_t*
gw_create_console(char *name)
{
	int i;

	for (i=0; i<MAX_CONSOLES; i++) {
		if (consoles[i].name == NULL) {
			consoles[i].widget = console_create(name, i);
			consoles[i].name = strdup(name);
			return consoles[i].widget;
		}
	}

	return NULL;
}

int
gw_set_console(char *name)
{
	int i;

	printf("%s(): set console to %s\n", __FUNCTION__, name);

	for (i=0; i<MAX_CONSOLES; i++) {
		if (strcmp(consoles[i].name, name) == 0) {
			if (root != consoles[i].widget) {
				root = consoles[i].widget;
				gw_output();
			}
			return 0;
		}
	}

	fprintf(stderr, "%s(): failed to set console!\n", __FUNCTION__);

	return -1;
}

char*
gw_get_console(void)
{
	int i;

	if (root) {
		for (i=0; i<MAX_CONSOLES; i++) {
			if (root == consoles[i].widget) {
				return consoles[i].name;
			}
		}
	}

	return NULL;
}

static int
gw_load_plugins(unsigned int dev)
{
	unsigned int loaded = 0;

	if ((dev & GW_DEV_HTTP) && ((http=plugin_load("http")) == NULL)) {
		goto err;
	}
	loaded |= GW_DEV_HTTP;

	if ((dev & GW_DEV_OSD) && ((osd=plugin_load("osd")) == NULL)) {
		goto err;
	}
	if ((dev & GW_DEV_OSD) && ((ss=plugin_load("screensaver")) == NULL)) {
		goto err;
	}
	loaded |= GW_DEV_OSD;

	if ((dev & GW_DEV_OSD) && ss) {
		ss->enable();
		ss->feed(SS_TIMEOUT);
	}
	if ((dev & GW_DEV_HTTP) && http) {
		http->startd(0);
	}

	return 0;

 err:
	if (loaded & GW_DEV_OSD) {
		plugin_unload("osd");
		plugin_unload("screensaver");
		osd = NULL;
		ss = NULL;
	}

	if (loaded & GW_DEV_HTTP) {
		if (http)
			http->stopd();
		plugin_unload("http");
		http = NULL;
	}

	return -1;
}

static int
gw_unload_plugins(unsigned int dev)
{
	if (dev & GW_DEV_OSD) {
		plugin_unload("screensaver");
		plugin_unload("osd");
		osd = NULL;
		ss = NULL;
	}
	if (dev & GW_DEV_HTTP) {
		plugin_unload("http");
		http = NULL;
	}

	return 0;
}

int
gw_device_add(unsigned int dev)
{
	if ((dev & ~(GW_DEV_OSD|GW_DEV_HTTP)) != 0) {
		return -1;
	}

	dev &= ~current;

	if (gw_load_plugins(dev) < 0) {
		return -1;
	}

	current |= dev;

	if (dev & GW_DEV_OSD) {
		gw_t *widget = root;

		while (widget) {
			if (widget->realized) {
				osd->update_widget(widget);
			}
			widget = widget->next;
		}
	}

	write(pipefds[1], " ", 1);

	return 0;
}

int
gw_device_remove(unsigned int dev)
{
	if ((dev & ~(GW_DEV_OSD|GW_DEV_HTTP)) != 0) {
		return -1;
	}

	dev &= current;

	if (gw_unload_plugins(dev) < 0) {
		return -1;
	}

	current &= ~dev;

	write(pipefds[1], " ", 1);

	return 0;
}

int
gw_init(void)
{
	pipe(pipefds);

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
	if (current & GW_DEV_HTTP) {
		http->generate();
	}

	if (current & GW_DEV_OSD) {
		osd->generate(root);
	}

	return 0;
}

int
gw_ss_enable(void)
{
	if (ss) {
		return ss->enable();
	}

	return -1;
}

int
gw_ss_disable(void)
{
	if (ss) {
		return ss->disable();
	}

	return -1;
}
