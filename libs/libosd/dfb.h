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

#ifndef OSD_DFB_H
#define OSD_DFB_H

#include <directfb.h>

#define DFBCHECK(x...)                                         \
  {                                                            \
    DFBResult err = x;                                         \
                                                               \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        dl_DirectFBErrorFatal( #x, err );                         \
      }                                                        \
  }

#define osd_layer	__dfb_osd_layer

#define PCH_DATADIR "/home/firmware/whsaw/SMP8634/2.7.176/dcchd/directfb/share/directfb-examples/fonts/"

#define ISTAR_DATADIR "/home/firmware/rolandhii/release/8634/dcchd/directfb/share/directfb-examples/fonts/"

extern void dfb_deinit();

#endif /* OSD_DFB_H */
