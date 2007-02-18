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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include "mvp_osd.h"

#include "osd.h"
#include "mg35.h"

static int fd = -1;

static rm_overlay_t *overlay;
static rm_info_t info;

static int
overlay_add_color(osd_surface_t *surface, unsigned int c)
{
	if (surface->data.overlay.colors == 256) {
		return -1;
	}

	surface->data.overlay.palette[surface->data.overlay.colors++] = c;

	return surface->data.overlay.colors;
}

static int
find_color(osd_surface_t *surface, unsigned int c)
{
	int i;

	for (i=0; i<surface->data.overlay.colors; i++) {
		if (c == surface->data.overlay.palette[i])
			return i;
	}

	return -1;
}

static inline void
draw_pixel(osd_surface_t *surface, int x, int y, unsigned char pixel)
{
	surface->data.overlay.data[(y*surface->width)+x] = pixel;
}

static int
overlay_draw_pixel(osd_surface_t *surface, int x, int y, unsigned int c)
{
	int pixel;

	if ((pixel=find_color(surface, c)) == -1) {
		if ((pixel=overlay_add_color(surface, c)) < 0) {
			return -1;
		}
	}

	draw_pixel(surface, x, y, (unsigned char)pixel);

	return 0;
}

static int
overlay_display_surface(osd_surface_t *surface)
{
	if (surface == visible) {
		return 0;
	}

	if (visible) {
		return -1;
	}

	memcpy(overlay->palette, surface->data.overlay.palette,
	       sizeof(unsigned long)*256);
	memset(overlay->data, 0, overlay->width * overlay->height);

	free(surface->data.overlay.palette);
	free(surface->data.overlay.data);

	surface->data.overlay.palette = overlay->palette;
	surface->data.overlay.data = overlay->data;

	visible = surface;

	return 0;
}

static int
overlay_undisplay_surface(osd_surface_t *surface)
{
	if (surface == visible) {
		visible = NULL;
	}

	return 0;
}

static int
overlay_destroy_surface(osd_surface_t *surface)
{
	if (surface == visible) {
		visible = NULL;
	} else {
		if (surface->data.overlay.palette)
			free(surface->data.overlay.palette);
		if (surface->data.overlay.data)
			free(surface->data.overlay.data);
	}

	return 0;
}

static osd_func_t fp = {
	.draw_pixel = overlay_draw_pixel,
	.display = overlay_display_surface,
	.undisplay = overlay_undisplay_surface,
	.destroy = overlay_destroy_surface,
};

osd_surface_t*
overlay_create(int w, int h, unsigned long color)
{
	osd_surface_t *surface;
	int i;

	if (fd < 0) {
		if (overlay_init() < 0)
			return NULL;
	}

	if ((w <= 0) || (w > info.width))
		return NULL;
	if ((h <= 0) || (h > info.height))
		return NULL;

	if ((surface=(osd_surface_t*)malloc(sizeof(*surface))) == NULL) {
		return NULL;
	}
	memset(surface, 0, sizeof(*surface));

	surface->type = OSD_FB;
	surface->fp = &fp;

	surface->fd = fd;
	surface->width = w;
	surface->height = h;

#if 0
	if ((surface->data.overlay.palette=
	     malloc(sizeof(unsigned long)*256)) == NULL) {
		return NULL;
	}
	if ((surface->data.overlay.data= malloc(w*h)) == NULL) {
		return NULL;
	}
#else
	surface->data.overlay.palette = overlay->palette;
	surface->data.overlay.data = overlay->data;
	visible = surface;
#endif

	memset(surface->data.overlay.palette, 0, sizeof(unsigned long)*256);
	memset(surface->data.overlay.data, 0, w*h);

	i = 0;
	while ((all[i] != NULL) && (i < OSD_MAX_SURFACES))
		i++;
	if (i < OSD_MAX_SURFACES)
		all[i] = surface;

	osd_fill_rect(surface, 0, 0, w, h, 0);

	return surface;
}

int
overlay_init(void)
{
	if ((fd=open("/dev/realmagichwl0", 0, 0xade7)) < 0) {
		return -1;
	}

	if (ioctl(fd, IOCTL_RM_INIT, &info) < 0) {
		goto err;
	}

	overlay = (rm_overlay_t*)info.addr;

	return 0;

 err:
	close(fd);
	fd = -1;

	return -1;
}
