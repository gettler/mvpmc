#ifndef MVPMC_PLUGIN_H
#define MVPMC_PLUGIN_H

/*
 *  Copyright (C) 2007, Jon Gettler
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

#define PLUGIN_MAJOR_VER	0
#define PLUGIN_MINOR_VER	2

#define PLUGIN_MAJOR(x)		((x >> 16) & 0xffff)
#define PLUGIN_MINOR(x)		(x & 0xffff)

#define PLUGIN_VERSION(x,y)	((x << 16) | y);

#define CURRENT_PLUGIN_VERSION	PLUGIN_VERSION(PLUGIN_MAJOR_VER, \
					       PLUGIN_MINOR_VER)

#define PLUGIN_PATH		"/usr/share/mvpmc/plugins"

#define PLUGIN_NAME_MAX		32

/*
 * Symbols from the mvpmc application
 */
extern void *plugin_load(char *name);
extern int plugin_unload(char *name);

/*
 * Symbols that must be defined by each plugin
 */
extern unsigned long plugin_version;

extern void *plugin_init(void);
extern int plugin_release(void);

#if defined(PLUGIN_SUPPORT)
#define PLUGIN_INIT(x)		void *plugin_init(void) { return x(); }
#define PLUGIN_RELEASE(x)	int plugin_release(void) { return x(); }
#else
#define PLUGIN_INIT(x)		void *plugin_##x(void) { return x(); }
#define PLUGIN_RELEASE(x)	int plugin_##x(void) { return x(); }
#endif /* !PLUGIN_SUPPORT */

#endif /* MVPMC_PLUGIN_H */
