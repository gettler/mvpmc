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
#include <plugin/av.h>
#include <gw.h>

#include "myth_local.h"

static plugin_av_t *av;

static int
av_load(void)
{
	if ((av=plugin_load("av")) == NULL) {
		return -1;
	}

	return 0;
}

int
rec_select(gw_t *widget, char *text, void *key)
{
	cmyth_proginfo_t prog = (cmyth_proginfo_t)key;
	char *path;
	char buf[512];

	path = cmyth_proginfo_pathname(prog);

	snprintf(buf, sizeof(buf), "%s%s", recdir, path);

	ref_release(path);

	if (!av) {
		if (av_load() < 0) {
			return -1;
		}
	}

	av->play_file(buf);

	return 0;
}

int
rec_list(gw_t *widget)
{
	cmyth_conn_t ctrl;
	cmyth_proglist_t ep_list;
	int i, count;

	if (conn_open(NULL, -1) < 0) {
		fprintf(stderr, "MythTV connect failed!\n");
		return -1;
	}

	printf("MythTV connection established\n");

	ctrl = ref_hold(control);
	ep_list = cmyth_proglist_get_all_recorded(ctrl);
	count = cmyth_proglist_get_count(ep_list);

	printf("MythTV: found %d recordings\n", count);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		char *t, *s;
		char buf[256];

		prog = cmyth_proglist_get_item(ep_list, i);
		t = cmyth_proginfo_title(prog);
		s = cmyth_proginfo_subtitle(prog);

		snprintf(buf, sizeof(buf), "%s - %s", t, s);
		gw_menu_item_add(widget, buf, (void*)prog, rec_select, 0);

		ref_release(t);
		ref_release(s);
	}

	ref_release(ep_list);
	ref_release(ctrl);

	conn_close();

	return 0;
}
