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
#include <sys/stat.h>

#include "mvp_osd.h"

#include "osd.h"
#include "dfb.h"
#include "piutil.h"

#define DBG fprintf(stdout,"%s: %s():%d\n",__FILE__,__FUNCTION__,__LINE__)


IDirectFBDisplayLayer   *osd_layer;

static IDirectFB               *dfb = NULL;
static IDirectFBSurface        *root;
static IDirectFBInputDevice    *remote;
static IDirectFBEventBuffer    *input_buffer;

static DFBDisplayLayerConfig layer_config;

static DFBSurfaceDescription dsc;
static DFBFontDescription font_dsc;
static IDirectFBFont *font = NULL;

#if defined(USE_LIBDL)
static DFBResult (*dl_DirectFBCreate)(IDirectFB **) = NULL;
static DFBResult (*dl_DirectFBInit)(int*, char *(*argv[])) = NULL;
static DFBResult (*dl_DirectFBSetOption)(const char*, const char*) = NULL;
static DFBResult (*dl_DirectFBErrorFatal)(const char*, DFBResult) = NULL;

static void *handle = NULL;
#else
#define dl_DirectFBCreate	DirectFBCreate
#define dl_DirectFBInit		DirectFBInit
#define dl_DirectFBSetOption	DirectFBSetOption
#define dl_DirectFBErrorFatal	DirectFBErrorFatal
#endif /* USE_LIBDL */

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
		if (root->Lock(root, DSLF_WRITE,
			       (void**) (void*)&dst, &pitch) ==DFB_OK) {
			offset = y * pitch + x*4;
			dst[offset++]=b;
			dst[offset++]=g;
			dst[offset++]=r;
			dst[offset]=a;
			root->Unlock (root);
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
		root->SetColor(root,r,g,b,a);
		root->DrawLine(root,x1,y1,x2,y2);
	}

	return 0;
}

static int
dfb_draw_indexed_image(osd_surface_t *surface, osd_indexed_image_t *image,
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
		DFBCHECK (root->SetColor(root,r,g,b,a));
		DFBCHECK (root->FillRectangle (root,x,y,w,h));
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
		root->Blit(root, src, &rect, dstx, dsty);
	}

	return 0;
}

static int
dfb_display_surface(osd_surface_t *surface)
{
	DFBRectangle rect;

	if (visible != surface) {
		visible = surface;
		DFBCHECK (root->Clear (root, 0x0, 0x0, 0x0, 0xFF));
		rect.x = 0;
		rect.y = 0;
		rect.w = surface->width;
		rect.h = surface->height;
		root->Blit(root, surface->data.primary,&rect, 0, 0);
	}

	return 0;
}

static int
dfb_undisplay_surface(osd_surface_t *surface)
{
	if (visible) {
		DFBCHECK (root->Clear (root, 0x0, 0x0, 0x0, 0xFF));
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
	int h, w;

	DFBCHECK(font->GetHeight(font, &h));

	if (background) {
		c2rgba(bg,&r,&g,&b,&a);
		s->SetColor(s,r,g,b,a);
		DFBCHECK(font->GetStringWidth(font, text, -1, &w));
		osd_fill_rect(surface, x, y, w, h, bg);
	}

	y += h;
	c2rgba(fg,&r,&g,&b,&a);
	s->SetColor(s,r,g,b,a);
	DFBCHECK (s->SetFont (s, font));
	DFBCHECK(s->DrawString(s, text, -1, x, y, DSTF_LEFT));

	if (surface == visible) {
		root->SetColor(root,r,g,b,a);
		DFBCHECK (root->SetFont (root, font));
		DFBCHECK (root->DrawString(root, text, -1, x, y, DSTF_LEFT));
	}

	return 0;
}

static int
dfb_draw_image(osd_surface_t *surface, char *path)
{
	struct stat sb;
	IDirectFBSurface *s = surface->data.primary;
	IDirectFBImageProvider *provider;

	if (access(path, R_OK) != 0) {
		return -1;
	}

	if (stat(path, &sb) != 0) {
		return -1;
	}

	/*
	 * This method of drawing images takes a lot of memory, so for now
	 * cap the image size at 2mb.
	 */
	if (sb.st_size > (1024*1024*2)) {
		return -1;
	}

	if (dfb->CreateImageProvider(dfb, path, &provider) != DFB_OK) {
		return -1;
	}

	DFBCHECK(provider->GetSurfaceDescription(provider, &dsc));
	DFBCHECK(provider->RenderTo(provider, s, NULL));

	if (surface == visible) {
		root->Blit(root, s, NULL, 0, 0);
	}

	provider->Release(provider);

	return -1;
}

static osd_func_t fp_dfb = {
	.draw_pixel = dfb_draw_pixel,
	.read_pixel = dfb_read_pixel,
	.draw_line = dfb_draw_line,
	.draw_indexed_image = dfb_draw_indexed_image,
	.fill_rect = dfb_fill_rect,
	.display = dfb_display_surface,
	.undisplay = dfb_undisplay_surface,
	.destroy = dfb_destroy_surface,
	.palette_add_color = dfb_add_color,
	.blit = dfb_blit,
	.draw_text = dfb_draw_text,
	.draw_image = dfb_draw_image,
};

osd_surface_t*
dfb_create(int w, int h, unsigned long color)
{
	int i;
	osd_surface_t *surface;
	unsigned char r,g,b,a;

	if (dfb == NULL)
		return NULL;

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

static int
get_video_mode(void)
{
	FILE *f;
	char line[256];
	int mode = -1;

	/*
	 * TV Modes (/tmp/setting.txt video_output):
	 *
	 *    1   - NTSC 480i
	 *    2   - PAL 576i
	 *    6   - 720p50
	 *    8   - 1080i50
	 *    7   - 1080p50
	 *    10  - 720p60
	 *    12  - 1080i60
	 *    11  - 1080p60
	 */

	if ((f=fopen("/tmp/setting.txt", "r")) == NULL) {
		return -1;
	}

	while (fgets(line, sizeof(line), f) != NULL) {
		char *p;

		if ((p=strchr(line, '=')) != NULL) {
			*(p++) = '\0';
			if (strcmp(line, "video_output") == 0) {
				mode = atoi(p);
				break;
			}
		}
	}

	fclose(f);

	printf("DFB: tv mode %d\n", mode);

	return mode;
}

static int
get_tv_name(void)
{
	FILE *f;
	char line[256];
	int ret = 0;

	/*
	 * Values seen in /tmp/tvname
	 *
	 *    HDMI_1080p60
	 *    Component NTSC
	 */

	if ((f=fopen("/tmp/tvname", "r")) == NULL) {
		return 0;
	}

	do {
		char *res, *mode;

		if (fgets(line, sizeof(line), f) == NULL) {
			break;
		}

		if ((res=strchr(line, '_')) != NULL) {
			*(res++) = '\0';
		}
		if ((mode=strchr(line, ' ')) != NULL) {
			*(mode++) = '\0';
		}

		if (mode && strcmp(mode, "NTSC") == 0) {
			ret = 1;
			break;
		}
		if (mode && strcmp(mode, "PAL") == 0) {
			ret = 2;
			break;
		}

		if (res && strcmp(res, "720p60") == 0) {
			ret = 10;
			break;
		}
		if (res && strcmp(res, "1080p60") == 0) {
			ret = 11;
			break;
		}
		if (res && strcmp(res, "1080i60") == 0) {
			ret = 12;
			break;
		}
		if (res && strcmp(res, "720p50") == 0) {
			ret = 6;
			break;
		}
		if (res && strcmp(res, "1080p50") == 0) {
			ret = 7;
			break;
		}
		if (res && strcmp(res, "1080i50") == 0) {
			ret = 8;
			break;
		}
	} while (0);

	printf("DFB: tv name %d\n", ret);

	fclose(f);

	return ret;
}

int
dfb_init(void)
{
	int mode, name;
	char dfb_mode[32];
	char *dfb_signal = NULL;
	char *dfb_tv = NULL;
	char *dfb_connector = NULL;
	char *dfb_analog_mode = NULL;

	if (dfb)
		return 0;

#if defined(USE_LIBDL)
	if (handle == NULL) {
		if ((handle=pi_register("libdirectfb-1.0.so.0")) == NULL) {
			return -1;
		}

		dl_DirectFBCreate = dlsym(handle, "DirectFBCreate");
		dl_DirectFBInit = dlsym(handle, "DirectFBInit");
		dl_DirectFBSetOption = dlsym(handle, "DirectFBSetOption");
		dl_DirectFBErrorFatal = dlsym(handle, "DirectFBErrorFatal");
	}
#endif /* USE_LIBDL */

	mode = get_video_mode();
	name = get_tv_name();

	/*
	 * If mode is 0 (Auto TV Mode), then use the mode from get_tv_name()
	 */
	if (mode == 0) {
		mode = name;
	}

	DFBCHECK(dl_DirectFBInit( NULL, NULL));

	switch (mode) {
	case 1:
		full_width = 720;
		full_height = 480;
		dfb_connector = "ycrcb";
		dfb_analog_mode = "ntsc";
		break;
	case 2:
		full_width = 720;
		full_height = 576;
		dfb_connector = "ycrcb";
		dfb_analog_mode = "pal";
		break;
	case 6:
		full_width = 1280;
		full_height = 720;
		dfb_signal = "720p";
		dfb_tv = "hdtv50";
		dfb_connector = "ycrcb";
		dfb_analog_mode = "pal";
		break;
	case 7:
		full_width = 1920;
		full_height = 1080;
		dfb_signal = "1080p";
		dfb_tv = "hdtv50";
		dfb_connector = "ycrcb";
		dfb_analog_mode = "pal";
		break;
	case 8:
		full_width = 1920;
		full_height = 1080;
		dfb_signal = "1080i";
		dfb_tv = "hdtv50";
		dfb_connector = "ycrcb";
		dfb_analog_mode = "pal";
		break;
	case 10:
		full_width = 1280;
		full_height = 720;
		dfb_signal = "720p";
		dfb_tv = "hdtv60";
		dfb_connector = "ycrcb";
		dfb_analog_mode = "ntsc";
		break;
	case 11:
		full_width = 1920;
		full_height = 1080;
		dfb_signal = "1080p";
		dfb_tv = "hdtv60";
		dfb_connector = "ycrcb";
		dfb_analog_mode = "ntsc";
		break;
	case 12:
		full_width = 1920;
		full_height = 1080;
		dfb_signal = "1080i";
		dfb_tv = "hdtv60";
		dfb_connector = "ycrcb";
		dfb_analog_mode = "ntsc";
		break;
	}

	snprintf(dfb_mode, sizeof(dfb_mode), "%dx%d", full_width, full_height);
	DFBCHECK(dl_DirectFBSetOption ("mode", dfb_mode));
	if (dfb_signal) {
		DFBCHECK(dl_DirectFBSetOption ("dtv-signal", dfb_signal));
		DFBCHECK(dl_DirectFBSetOption ("component-signal", dfb_signal));
	}
	if (dfb_tv) {
		DFBCHECK(dl_DirectFBSetOption ("dtv-tv-standard", dfb_tv));
		DFBCHECK(dl_DirectFBSetOption ("component-tv-standard", dfb_tv));
	}
	if (dfb_connector)
		DFBCHECK(dl_DirectFBSetOption ("component-connector",
					       dfb_connector));
	if (dfb_analog_mode)
		DFBCHECK(dl_DirectFBSetOption ("analog-tv-standard",
					       dfb_analog_mode));

	/* create the super interface */
	DFBCHECK(dl_DirectFBCreate( &dfb ));

	dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN);

	DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &osd_layer ));
	osd_layer->SetCooperativeLevel(osd_layer, DLSCL_EXCLUSIVE);
	osd_layer->GetConfiguration (osd_layer, &layer_config);
	DFBCHECK(osd_layer->GetSurface(osd_layer, &root ));
	
	DFBCHECK(root->Clear (root, 0x0, 0x0, 0x0, 0xFF));


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
	if (root)
		root->Release( root );
	if (osd_layer)
		osd_layer->Release( osd_layer );
	if (dfb)
		dfb->Release( dfb ); 

	font = NULL;
	input_buffer = NULL;
	remote = NULL;
	osd_layer = NULL;
	root = NULL;
	dfb = NULL;

#if defined(USE_LIBDL)
	if (handle)
		pi_deregister(handle);
	handle = NULL;
#endif /* USE_LIBDL */
}

int
font_height(osd_font_t *osd_font)
{
	int h;

	DFBCHECK(font->GetHeight(font, &h));

	return h;
}

int
font_width(osd_font_t *osd_font, char *text)
{
	int w;

	DFBCHECK(font->GetStringWidth(font, text, -1, &w));

	return w;
}
