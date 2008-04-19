/*
 *  Copyright (C) 2006-2008, Jon Gettler
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
#include <pthread.h>

#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <gdk/gdkx.h>

#include "mvp_osd.h"

#include "font.h"

#include "osd.h"

#if 0
#define PRINTF(x...) printf(x)
#else
#define PRINTF(x...)
#endif

#define MAX_SURFACE	128

static GtkWidget *mainwindow;
static GtkWidget *drawing_area;
static GdkColormap *cmap;
static GdkGC *gc;
static GdkDrawable *drawable;
static GdkDrawable *blank;
static GdkColor black = { 0, 0x00, 0x00, 0x00 };

static osd_surface_t *slist[MAX_SURFACE];

osd_font_t font_CaslonRoman_1_25;

static int gtk_init_done = 0;

int gtk_open(void);

static gint
expose_callback (GtkWidget * widget, guint * datum)
{
	if (drawable) {
		gdk_draw_drawable(drawing_area->window,
				  gc, drawable, 0, 0, 0, 0,
				  full_width, full_height);
	} else {
		printf("bad expose!\n");
	}

	return TRUE;
}

static gint
quit_program (GtkWidget * widget, gpointer datum)
{
	printf("Bye!\n");
	exit(0);
	return (TRUE);
}

static int
gtk_add_color(osd_surface_t *surface, unsigned int c)
{
	return 0;
}

static int
gtk_draw_pixel(osd_surface_t *surface, int x, int y, unsigned int c)
{
	GdkColor color;

	gdk_threads_enter();

	color.pixel = 0;
	color.red = ((c >> 16) & 0xff) << 8;
	color.green = ((c >> 8) & 0xff) << 8;
	color.blue = (c & 0xff) << 8;
	gdk_color_alloc (cmap, &color);

	gdk_gc_set_foreground(gc, &color);
	gdk_gc_set_background(gc, &color);

	gdk_draw_point (surface->data.drawable, gc, x, y);

	if (drawable == surface->data.drawable) {
		gdk_draw_point (drawing_area->window, gc, x, y);
	}

	gdk_threads_leave();

	return 0;
}

int
gtk_draw_line(osd_surface_t *surface, int x1, int y1, int x2, int y2,
	      unsigned int c)
{
	GdkColor color;

	gdk_threads_enter();

	color.pixel = 0;
	color.red = ((c >> 16) & 0xff) << 8;
	color.green = ((c >> 8) & 0xff) << 8;
	color.blue = (c & 0xff) << 8;
	gdk_color_alloc (cmap, &color);

	gdk_gc_set_foreground(gc, &color);
	gdk_gc_set_background(gc, &color);

	gdk_draw_line (surface->data.drawable, gc, x1, y1, x2, y2);

	if (drawable == surface->data.drawable) {
		gdk_draw_line (drawing_area->window, gc, x1, y1, x2, y2);
	}

	gdk_threads_leave();

	return 0;
}

int
gtk_draw_horz_line(osd_surface_t *surface, int x1, int x2, int y,
		   unsigned int c)
{
	return osd_draw_line(surface, x1, y, x2, y, c);
}

int
gtk_draw_vert_line(osd_surface_t *surface, int x, int y1, int y2,
		   unsigned int c)
{
	return osd_draw_line(surface, x, y1, x, y2, c);
}

static int
gtk_draw_image(osd_surface_t *surface, osd_indexed_image_t *image,
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
			gtk_draw_pixel(surface, x+X, y+Y, c);
		}
	}

	return 0;
}

static int
gtk_fill_rect(osd_surface_t *surface, int x, int y, int w, int h,
	      unsigned int c)
{
	GdkRectangle update_rect;
	GdkColor color;

	gdk_threads_enter();

	color.pixel = 0;
	color.red = ((c >> 16) & 0xff) << 8;
	color.green = ((c >> 8) & 0xff) << 8;
	color.blue = (c & 0xff) << 8;
	gdk_color_alloc (cmap, &color);

	gdk_gc_set_foreground(gc, &color);
	gdk_gc_set_background(gc, &color);

	update_rect.x = x;
	update_rect.y = y;
	update_rect.width = w;
	update_rect.height = h;

	gdk_draw_rectangle (surface->data.drawable, gc, TRUE,
			    update_rect.x, update_rect.y,
			    update_rect.width, update_rect.height);

	if (drawable == surface->data.drawable) {
		gdk_draw_rectangle (drawing_area->window, gc, TRUE,
				    update_rect.x, update_rect.y,
				    update_rect.width, update_rect.height);
	}

	gdk_threads_leave();

	return 0;
}

int
gtk_blit(osd_surface_t *dstsfc, int dstx, int dsty,
	 osd_surface_t *srcsfc, int srcx, int srcy, int w, int h)
{
	gdk_draw_drawable(dstsfc->data.drawable,
			gc, srcsfc->data.drawable,
			srcx, srcy, dstx, dsty, w, h);

	if (dstsfc->data.drawable == drawable) {
		expose_callback(NULL, NULL);
	}

	return 0;
}

int
gtk_fillblt(osd_surface_t *surface, int x, int y, int width, int height, 
	    unsigned int c)
{
	return osd_fill_rect(surface, x, y, width, height, c);
}

static int
gtk_destroy_surface(osd_surface_t *surface)
{
	int i;

#if 0
	gdk_window_destroy(surface->data.drawable);
#endif

	for (i=0; i<MAX_SURFACE; i++) {
		if (slist[i] == surface) {
			slist[i] = NULL;
			break;
		}
	}

	if (drawable == surface->data.drawable) {
		drawable = blank;
		expose_callback(NULL, NULL);
	}

	free(surface);

	return 0;
}

void
gtk_destroy_all_surfaces(void)
{
	int i;

	for (i=0; i<MAX_SURFACE; i++) {
		if (slist[i]) {
			osd_destroy_surface(slist[i]);
		}
	}
}

static int
gtk_display_surface(osd_surface_t *surface)
{
	if (drawable != surface->data.drawable) {
		drawable = surface->data.drawable;
		expose_callback(NULL, NULL);
	}

	visible = surface;

	return 0;
}

static int
gtk_undisplay_surface(osd_surface_t *surface)
{
	return -1;
}

osd_surface_t*
gtk_get_visible_surface(void)
{
	int i;

	for (i=0; i<MAX_SURFACE; i++) {
		if (slist[i]->data.drawable == drawable) {
			return slist[i];
		}
	}

	return NULL;
}

static osd_func_t fp = {
	.draw_pixel = gtk_draw_pixel,
	.blit = gtk_blit,
	.fill_rect = gtk_fill_rect,
	.display = gtk_display_surface,
	.undisplay = gtk_undisplay_surface,
	.destroy = gtk_destroy_surface,
	.draw_indexed_image = gtk_draw_image,
	.palette_add_color = gtk_add_color,
};

osd_surface_t*
gtk_create(int w, int h, unsigned long color)
{
	osd_surface_t *surface;
	int i;

	if (!gtk_init_done) {
		if (gtk_open() < 0)
			return NULL;
	}

	if ((surface=(osd_surface_t*)malloc(sizeof(*surface))) == NULL)
		return NULL;

	memset(surface, 0, sizeof(*surface));

	surface->data.drawable = gdk_pixmap_new(drawing_area->window, w, h, -1);

	for (i=0; i<MAX_SURFACE; i++) {
		if (slist[i] == NULL) {
			slist[i] = surface;
			break;
		}
	}

	surface->type = OSD_GFX;
	surface->fp = &fp;

	surface->width = w;
	surface->height = h;

	return surface;
}

static void*
thread(void *arg)
{
	while (1) {
		gdk_threads_enter();
		while (gtk_events_pending()) {
			gtk_main_iteration_do(FALSE);
		}
		gdk_threads_leave();
		usleep(10000);
	}

	return 0;
}

int
gtk_open(void)
{
	int w = full_width, h = full_height;
	GdkRectangle rectangle = {
		0, 0, w, h
	};
	pthread_t t;

	g_thread_init(NULL);
	gdk_threads_init();
    
    	gtk_init(0, NULL);
    
	/*
	 * Create the main window
	 */
	mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (mainwindow), "mvpmc");
	gtk_container_set_border_width (GTK_CONTAINER (mainwindow), 0);
	gtk_widget_set_size_request(GTK_WIDGET(mainwindow), w, h);
	g_signal_connect (G_OBJECT (mainwindow), "delete_event",
			  G_CALLBACK (quit_program), NULL);
	gtk_widget_show(mainwindow);

	PRINTF("mainwindow wid: %lu\n",
	       GDK_WINDOW_XWINDOW(mainwindow->window));

	/*
	 * Create a drawing window
	 */
	drawing_area = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER (mainwindow), drawing_area);
	g_signal_connect (G_OBJECT (drawing_area), "expose_event",
			  G_CALLBACK (expose_callback), NULL);
	gtk_widget_show(drawing_area);

	PRINTF("drawing area wid: %lu\n",
	       GDK_WINDOW_XWINDOW(drawing_area->window));

	/*
	 * Create a graphics context
	 */
	gc = gdk_gc_new(mainwindow->window);
	gdk_gc_set_clip_origin (gc, 0, 0);
	rectangle.width = w;
	rectangle.height = h;
	gdk_gc_set_clip_rectangle (gc, &rectangle);
	gdk_gc_set_clip_mask (gc, NULL);
	cmap = gdk_colormap_get_system ();
	gdk_color_alloc (cmap, &black);
	gdk_gc_set_foreground(gc, &black);
	gdk_gc_set_background(gc, &black);

	blank = gdk_pixmap_new(drawing_area->window, w, h, -1);
	drawable = blank;

	gdk_draw_rectangle (blank, gc, TRUE, 0, 0, w, h);

	pthread_create(&t, NULL, thread, NULL);

	gtk_init_done = 1;

	return 0;
}

int
gtk_close(void)
{
	return 0;
}
