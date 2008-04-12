/*
 *  Copyright (C) 2008, Jon Gettler
 *  http://www.mvpmc.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "plugin.h"
#include "piutil.h"

#if !defined(PLUGIN_SUPPORT)
extern void *plugin_init_html(void);
extern int plugin_release_html(void);
extern void *plugin_init_http(void);
extern int plugin_release_http(void);
extern void *plugin_init_osd(void);
extern int plugin_release_osd(void);
extern void *plugin_init_av(void);
extern int plugin_release_av(void);
#endif /* !PLUGIN_SUPPORT */

static builtin_t builtins[] = {
#if !defined(PLUGIN_SUPPORT)
	{ "html", plugin_init_html, plugin_release_html },
	{ "http", plugin_init_http, plugin_release_http },
	{ "osd", plugin_init_osd, plugin_release_osd },
	{ "av", plugin_init_av, plugin_release_av },
#endif /* !PLUGIN_SUPPORT */
	{ NULL, NULL, NULL }
};

int
do_plugin_setup(void)
{
	extern int plugin_setup(builtin_t*, int);

	return plugin_setup(builtins,
			    (int)(sizeof(builtins)/sizeof(builtins[0]))-1);
}
