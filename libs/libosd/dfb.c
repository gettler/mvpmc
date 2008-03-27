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

#define DATADIR "/home/firmware/whsaw/SMP8634/2.7.176/dcchd/directfb/share/directfb-examples/fonts/"

static IDirectFB               *dfb = NULL;
static IDirectFBSurface        *dfb_root;
IDirectFBDisplayLayer   *osd_layer;
static IDirectFBInputDevice    *remote;
static IDirectFBEventBuffer    *input_buffer;

static DFBDisplayLayerConfig layer_config;

static DFBSurfaceDescription dsc;
static DFBFontDescription font_dsc;
static IDirectFBFont *font = NULL;

static int
dfb_add_color(osd_surface_t *surface, unsigned int c)
{
	return 0;
}

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
			offset = y * pitch + x*4;
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
	char *dst=NULL;
	int pitch;	   /* number of bytes per row */
	int offset=0;
	unsigned char r,g,b,a;

	if (surface->data.primary->Lock(surface->data.primary, DSLF_READ,
					(void **) (void*)&dst, &pitch) ==DFB_OK) {
		offset = y * pitch + x*4;
		b = dst[offset++];
		g = dst[offset++];
		r = dst[offset++];
		a = dst[offset];
		surface->data.primary->Unlock (surface->data.primary);

		return rgba2c(r, g, b, a);
	}

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
		dfb_root->SetColor(dfb_root,r,g,b,a);
		dfb_root->DrawLine(dfb_root,x1,y1,x2,y2);
	}

	return 0;
}

static int
dfb_draw_image(osd_surface_t *surface, osd_indexed_image_t *image,
		   int x, int y)
{
	int i, p, X, Y;
	unsigned char r, g, b;
	unsigned int c;

	i = 0;
	for (X=0; X<image->width; X++) {
		for (Y=0; Y<image->height; Y++) {
			i = (Y*image->width) + X;
			p = image->image[i] - 32;
			if ((p < 0) || (p >= image->colors)) {
				return -1;
			}
			r = image->red[p];
			g = image->green[p];
			b = image->blue[p];
			c = rgba2c(r, g, b, 0xff);
			dfb_draw_pixel(surface, x+X, y+Y, c);
		}
	}

	return 0;
}

static int
dfb_fill_rect(osd_surface_t *surface, int x, int y, int w, int h,
		  unsigned int c)
{
	unsigned char r,g,b,a;

	if (x > surface->width)
		x = surface->width - 1;
	if (y > surface->height)
		y = surface->height - 1;

	if ((x+w) >= surface->width)
		w = surface->width - x;
	if ((y+h) >= surface->height)
		h = surface->height - y;

	if ((x < 0) || (y < 0) || (h <= 0) || (w <= 0)) {
		return 0;
	}

	c2rgba(c,&r,&g,&b,&a);

	DFBCHECK (surface->data.primary->SetColor(surface->data.primary,
						  r,g,b,a));
	DFBCHECK (surface->data.primary->FillRectangle(surface->data.primary,
						       x,y,w,h));

	if (surface == visible ) {
		DFBCHECK (dfb_root->SetColor(dfb_root,r,g,b,a));
		DFBCHECK (dfb_root->FillRectangle (dfb_root,x,y,w,h));
	}

	return 0;
}

static int
dfb_blit(osd_surface_t *dstsfc, int dstx, int dsty,
	 osd_surface_t *srcsfc, int srcx, int srcy, int w, int h)
{
	DFBRectangle rect;
	IDirectFBSurface *dst, *src;

	dst = dstsfc->data.primary;
	src = srcsfc->data.primary;

	rect.x = srcx;
	rect.y = srcy;
	rect.w = w;
	rect.h = h;
	dst->Blit(dst, src, &rect, dstx, dsty);

	if (dstsfc == visible) {
		dfb_root->Blit(dfb_root, src, &rect, dstx, dsty);
	}

	return 0;
}

static int
dfb_display_surface(osd_surface_t *surface)
{
	DFBRectangle rect;

	if (visible != surface) {
		visible = surface;
		DFBCHECK (dfb_root->Clear (dfb_root, 0x0, 0x0, 0x0, 0xFF));
		rect.x = 0;
		rect.y = 0;
		rect.w = surface->width;
		rect.h = surface->height;
		dfb_root->Blit(dfb_root, surface->data.primary,&rect, 0, 0);
	}

	return 0;
}

static int
dfb_undisplay_surface(osd_surface_t *surface)
{
	if (visible) {
		DFBCHECK (dfb_root->Clear (dfb_root, 0x0, 0x0, 0x0, 0xFF));
	}

	visible = NULL;
	return 0;
}

static int
dfb_destroy_surface(osd_surface_t *surface)
{
	if (visible == surface) {
		dfb_undisplay_surface(surface);
	}

	surface->data.primary->Release( surface->data.primary );

	return 0;
}

static int
dfb_draw_text(osd_surface_t *surface, int x, int y, const char *text,
	      unsigned int fg, unsigned int bg, int background,
	      osd_font_t *osd_font)
{
	IDirectFBSurface *s = surface->data.primary;
	unsigned char r,g,b,a;
	int h;

	c2rgba(fg,&r,&g,&b,&a);

	DFBCHECK(font->GetHeight(font, &h));
	y += h;

	s->SetColor(s,r,g,b,a);
	DFBCHECK (s->SetFont (s, font));
	DFBCHECK(s->DrawString(s, text, -1, x, y, DSTF_LEFT));

	if (surface == visible) {
		dfb_root->SetColor(dfb_root,r,g,b,a);
		DFBCHECK (dfb_root->SetFont (dfb_root, font));
		DFBCHECK (dfb_root->DrawString(dfb_root, text, -1, x, y,
					       DSTF_LEFT));
	}

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
	.palette_add_color = dfb_add_color,
	.blit = dfb_blit,
	.draw_text = dfb_draw_text,
};

osd_surface_t*
dfb_create(int w, int h, unsigned long color)
{
	int i;
	osd_surface_t *surface;
	unsigned char r,g,b,a;

	if ((surface=malloc(sizeof(*surface))) == NULL)
		return NULL;
	memset(surface, 0, sizeof(*surface));

	memset( &dsc, 0, sizeof(DFBSurfaceDescription) );
	dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT;

	dsc.width  = w;
	dsc.height = h;

	dsc.caps = DSCAPS_NONE;

	DFBCHECK(dfb->CreateSurface(dfb, &dsc, &surface->data.primary ));

	c2rgba(color,&r,&g,&b,&a);

	DFBCHECK(surface->data.primary->Clear(surface->data.primary,r,g,b,a));
	DFBCHECK(surface->data.primary->SetBlittingFlags (surface->data.primary, DSBLIT_NOFX));

	i = 0;
	while ((all[i] != NULL) && (i < OSD_MAX_SURFACES))
		i++;
	if (i < OSD_MAX_SURFACES)
		all[i] = surface;

	surface->type = OSD_GFX;
	surface->fp = &fp_dfb;

	surface->width = w;
	surface->height = h;

	return surface;
}

int
dfb_init(void)
{
	if (dfb)
		return 0;

	DFBCHECK(DirectFBInit( NULL, NULL));

	DFBCHECK(DirectFBSetOption ("mode", "640x576"));
	/* create the super interface */
	DFBCHECK(DirectFBCreate( &dfb ));

	dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN);

	DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &osd_layer ));
	osd_layer->SetCooperativeLevel(osd_layer, DLSCL_EXCLUSIVE);
	osd_layer->GetConfiguration (osd_layer, &layer_config);
	DFBCHECK(osd_layer->GetSurface(osd_layer, &dfb_root ));
	
	DFBCHECK(dfb_root->Clear (dfb_root, 0x0, 0x0, 0x0, 0xFF));


	DFBCHECK(dfb->GetInputDevice(dfb, DIDID_REMOTE, &remote ));
	DFBCHECK(dfb->CreateInputEventBuffer(dfb, DICAPS_ALL, DFB_FALSE, &input_buffer));

	font_dsc.flags = DFDESC_HEIGHT;
	font_dsc.height = 26;
	DFBCHECK(dfb->CreateFont(dfb, DATADIR"/decker.ttf", &font_dsc, &font));

	return 0;
}

void 
dfb_deinit()
{
	if (font)
		font->Release(font);
	if (input_buffer)
		input_buffer->Release(input_buffer);
	if (remote)
		remote->Release( remote );
	if (dfb_root)
		dfb_root->Release( dfb_root );
	if (osd_layer)
		osd_layer->Release( osd_layer );
	if (dfb)
		dfb->Release( dfb ); 

	font = NULL;
	input_buffer = NULL;
	remote = NULL;
	osd_layer = NULL;
	dfb_root = NULL;
	dfb = NULL;
}
