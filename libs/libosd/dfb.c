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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <wait.h>

#include "mvp_osd.h"

#include "osd.h"
#include "dfb.h"

#define DBG fprintf(stdout,"%s: %s():%d\n",__FILE__,__FUNCTION__,__LINE__)

static IDirectFB               *dfb = NULL;
static IDirectFBSurface        *dfb_root;
static IDirectFBDisplayLayer   *osd_layer;
static IDirectFBInputDevice    *remote;
static IDirectFBEventBuffer    *input_buffer;

static DFBDisplayLayerConfig layer_config;

static DFBSurfaceDescription dsc;

static int
dfb_draw_pixel(osd_surface_t *surface, int x, int y, unsigned int c)
{
	char *dst=NULL;
	int pitch;	   /* number of bytes per row */
	int offset=0;
	unsigned char r,g,b,a;

	c2rgba(c,&r,&g,&b,&a);

	if (surface->data.primary->Lock(surface->data.primary, DSLF_WRITE,
					(void **) (void*)&dst, &pitch) ==DFB_OK) {
		offset = y * pitch + x*4;
		dst[offset++]=b;
		dst[offset++]=g;
		dst[offset++]=r;
		dst[offset]=a;
		surface->data.primary->Unlock (surface->data.primary);
	}

	if (surface == visible) {
		if (dfb_root->Lock(dfb_root, DSLF_WRITE,
				   (void**) (void*)&dst, &pitch) ==DFB_OK) {
			dst[offset++]=b;
			dst[offset++]=g;
			dst[offset++]=r;
			dst[offset]=a;
			dfb_root->Unlock (dfb_root);
		}	
	}

	return 0;
}

static unsigned int
dfb_read_pixel(osd_surface_t *surface, int x, int y)
{
	return 0;
}

static int
dfb_draw_line(osd_surface_t *surface, int x1, int y1, int x2, int y2,
	      unsigned int c)
{
	unsigned char r,g,b,a;

	c2rgba(c,&r,&g,&b,&a);

	surface->data.primary->SetColor(surface->data.primary,r,g,b,a);

	surface->data.primary->DrawLine(surface->data.primary,x1,y1,x2,y2);

	if (surface == visible ) {
		DFBRectangle rect;
		rect.x = x1;
		rect.y = y1;
		rect.h = x2-x1;
		rect.w = y2-y1;
		dfb_root->Blit(dfb_root,surface->data.primary,&rect,x1,y1);
	}

	return 0;
}

static int
dfb_draw_image(osd_surface_t *surface, osd_indexed_image_t *image,
		   int x, int y)
{
	return 0;
}

static int
dfb_fill_rect(osd_surface_t *surface, int x, int y, int w, int h,
		  unsigned int c)
{
	return 0;
}

static int
dfb_display_surface(osd_surface_t *surface)
{
	DBG;
	return 0;
}

static int
dfb_undisplay_surface(osd_surface_t *surface)
{
	DBG;
	return 0;
}

static int
dfb_destroy_surface(osd_surface_t *surface)
{
	DBG;
	return 0;
}

static osd_func_t fp_dfb = {
	.draw_pixel = dfb_draw_pixel,
	.read_pixel = dfb_read_pixel,
	.draw_line = dfb_draw_line,
	.draw_indexed_image = dfb_draw_image,
	.fill_rect = dfb_fill_rect,
	.display = dfb_display_surface,
	.undisplay = dfb_undisplay_surface,
	.destroy = dfb_destroy_surface,
};

osd_surface_t*
dfb_create(int w, int h, unsigned long color)
{
	int i;
	osd_surface_t *surface;
	unsigned char r,g,b,a;

	DBG;
	if ((surface=malloc(sizeof(*surface))) == NULL)
		return NULL;
	memset(surface, 0, sizeof(*surface));

	DBG;
	memset( &dsc, 0, sizeof(DFBSurfaceDescription) );
	dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT;

	dsc.width  = w;
	dsc.height = h;

	dsc.caps = DSCAPS_NONE;

	DBG;
	DFBCHECK(dfb->CreateSurface(dfb, &dsc, &surface->data.primary ));

	c2rgba(color,&r,&g,&b,&a);

	DBG;
	DFBCHECK(surface->data.primary->Clear(surface->data.primary,r,g,b,a));
	DFBCHECK(surface->data.primary->SetBlittingFlags (surface->data.primary, DSBLIT_NOFX));

	DBG;
	i = 0;
	while ((all[i] != NULL) && (i < OSD_MAX_SURFACES))
		i++;
	if (i < OSD_MAX_SURFACES)
		all[i] = surface;

	DBG;
	surface->type = OSD_GFX;
	surface->fp = &fp_dfb;

	surface->width = w;
	surface->height = h;

	return surface;
}

int
dfb_init(void)
{
	static int numopt = 0;
	char modebuf[50];
	char *options[5];

	DBG;
	if (numopt!=0) {
		return 1;
	}

	DBG;
	snprintf(modebuf,50,"--dfb:mode=%dx%d",720,480);
	options[0] = "libosd";
	options[1] = "--dfb:mode=720x576";
	options[2] = NULL;
	numopt = 2;

	printf("av %d %s\n",numopt,options[2]);

	DBG;
	DFBCHECK(DirectFBInit( NULL, NULL));

	DBG;
	DFBCHECK(DirectFBSetOption ("mode", "640x576"));
	/* create the super interface */
	DBG;
	DFBCHECK(DirectFBCreate( &dfb ));

	DBG;
	dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN);

	DBG;
	DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &osd_layer ));
	osd_layer->SetCooperativeLevel(osd_layer, DLSCL_EXCLUSIVE);
	DBG;
	osd_layer->GetConfiguration (osd_layer, &layer_config);
	DBG;
	DFBCHECK(osd_layer->GetSurface(osd_layer, &dfb_root ));
	DBG;
	
	DFBCHECK(dfb_root->Clear (dfb_root, 0x0, 0x0, 0x0, 0xFF));
	DBG;


	DFBCHECK(dfb->GetInputDevice(dfb, DIDID_REMOTE, &remote ));
	DBG;
	DFBCHECK(dfb->CreateInputEventBuffer(dfb, DICAPS_ALL, DFB_FALSE, &input_buffer));

	DBG;

	return 0;
}
