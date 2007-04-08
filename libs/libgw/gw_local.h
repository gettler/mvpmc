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

#ifndef GW_LOCAL_H
#define GW_LOCAL_H

#include "plugin_gw.h"
#include "plugin_osd.h"
#include "plugin_html.h"

#define root		__gw_root
#define commands	__gw_commands
#define osd		__gw_osd
#define html		__gw_html
#define update		__gw_update

extern gw_t *root;
extern plugin_osd_t *osd;
extern plugin_html_t *html;

extern int update(gw_t *widget);

#endif /* GW_LOCAL_H */
