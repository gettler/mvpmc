/*
 *  Copyright (C) 2004-2007, Jon Gettler
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

/** \file mvp_osd.h
 * MediaMVP On-Screen-Display interface library.  This library is used to
 * draw graphics on the TV screen.
 *
 * The hardware provides a drawing surface concept, which allows the
 * programmer to create on one or more surfaces at a time.  However, only
 * one surface can be visible at a time.
 *
 * Drawing something complicated, such as an image, can be slow.  So the
 * hardware allows very fast copying of data between surfaces.  This allows
 * you to draw on hidden surfaces, and copy the results to the visible
 * surface to avoid the appearance of slow drawing.
 */

#ifndef MVP_OSD_H
#define MVP_OSD_H

typedef struct osd_surface_s osd_surface_t;
typedef struct osd_font_s osd_font_t;

typedef enum {
	OSD_CURSOR=0,
	OSD_GFX=1,
	OSD_FB=2,
} osd_type_t;

typedef struct {
	int colors;
	int width;
	int height;
	unsigned char *red;
	unsigned char *green;
	unsigned char *blue;
	unsigned char *image;
} osd_indexed_image_t;

typedef struct {
	int x;
	int y;
	int w;
	int h;
} osd_clip_region_t;

typedef struct {
	osd_clip_region_t *regs;
	int n;
} osd_clip_t;

extern int osd_open(void);
extern int osd_close(void);

extern osd_surface_t *osd_clip_set(osd_surface_t *surface, osd_clip_t *clip);

extern int osd_clip(osd_surface_t *surface,
		    int left, int top, int right, int bottom);

/**
 * Create a new drawing surface
 * \param w surface width (-1 for full width)
 * \param h surface height (-1 for full height)
 * \param color background color
 * \return handle to the new surface
 */
extern osd_surface_t *osd_create_surface(int w, int h, unsigned long color,
					 osd_type_t type);

/**
 * Destroy a drawing surface.
 * \param surface handle to a drawing surface
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_destroy_surface(osd_surface_t *surface);

/**
 * Destroy all existing drawing surfaces.
 */
extern void osd_destroy_all_surfaces(void);

/**
 * Display a drawing surface.
 * \param surface drawing surface to display
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_display_surface(osd_surface_t *surface);

extern void osd_undisplay_surface(osd_surface_t *surface);

/**
 * Return the size of the drawing surface
 * \param surface handle to a drawing surface
 * \param[out] w surface width
 * \param[out] h surface height
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_get_surface_size(osd_surface_t *surface, int *w, int *h);

/**
 * Set the full size of the screen.
 * \param w screen width
 * \param h screen height
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_set_screen_size(int w, int h);

/**
 * Shut down access to the hardware OSD device.
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_close(void);

/**
 * Draw a single pixel on a drawing surface.
 * \param surface handle to a drawing surface
 * \param x horizontal coordinate
 * \param y vertical coordinate
 * \param c color
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_draw_pixel(osd_surface_t *surface, int x, int y,
			  unsigned int c);

/**
 * Draw a single pixel on a drawing surface.
 * \param surface handle to a drawing surface
 * \param x horizontal coordinate
 * \param y vertical coordinate
 * \param a alpha channel
 * \param Y y channel
 * \param U u channel
 * \param V v channel
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_draw_pixel_ayuv(osd_surface_t *surface, int x, int y,
			       unsigned char a, unsigned char Y,
			       unsigned char U, unsigned char V);

/**
 * Draw a horizontal line on a drawing surface.
 * \param surface handle to a drawing surface
 * \param x1 horizontal coordinate of start of line
 * \param x2 horizontal coordinate of end of line
 * \param y vertical coordinate
 * \param c color
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_draw_horz_line(osd_surface_t *surface, int x1, int x2, int y,
			      unsigned int c);

/**
 * Draw a vertical line on a drawing surface.
 * \param surface handle to a drawing surface
 * \param x horizontal coordinate
 * \param y1 vertical coordinate of start of line
 * \param y2 vertical coordinate of end of line
 * \param c color
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_draw_vert_line(osd_surface_t *surface, int x, int y1, int y2,
			      unsigned int c);

/**
 * Draw a line on a drawing surface.
 * \param surface handle to a drawing surface
 * \param x1 horizontal coordinate of start of line
 * \param y1 vertical coordinate of start of line
 * \param x2 horizontal coordinate of end of line
 * \param y2 vertical coordinate of end of line
 * \param c color
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_draw_line(osd_surface_t *surface,
			 int x1, int y1, int x2, int y2, unsigned int c);

/**
 * Fill a rectangle on a drawing surface.
 * \param surface handle to a drawing surface
 * \param x horizontal coordinate
 * \param y vertical coordinate
 * \param w width of rectangle
 * \param h height of rectangle
 * \param c color
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_fill_rect(osd_surface_t *surface, int x, int y, int w, int h,
			 unsigned int c);

/**
 * Draw a text string on a drawing surface.
 * \param surface handle to a drawing surface
 * \param x horizontal coordinate
 * \param y vertical coordinate
 * \param str text string to draw
 * \param fg foreground text color
 * \param bg background text color
 * \param background 0 if background should not be drawn, 1 if it should
 * \param FONT font to use
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_draw_text(osd_surface_t *surface, int x, int y, const char *str,
			 unsigned int fg, unsigned int bg, 
			 int background, osd_font_t *font);

/**
 * Bit blast a rectangle from one drawing surface to another.
 * \param dstsfc handle to the destination drawing surface
 * \param dstx destination horizontal coordinate
 * \param dsty destination vertical coordinate
 * \param srcsfc handle to the source drawing surface
 * \param srcx source horizontal coordinate
 * \param srcy source vertical coordinate
 * \param w width of rectangle
 * \param h height of rectangle
 * \retval 0 success
 * \retval -1 error
 */
extern int osd_blit(osd_surface_t *dstsfc, int dstx, int dsty,
		    osd_surface_t *srcsfc, int srcx, int srcy, int w, int h);

/**
 * Return the color of a specified pixel.
 * \param surface handle to a drawing surface
 * \param x horizontal coordinate
 * \param y vertical coordinate
 * \return pixel color
 */
extern unsigned int osd_read_pixel(osd_surface_t *surface, int x, int y);

extern int osd_get_display_control(osd_surface_t *surface, int type);
extern int osd_set_display_control(osd_surface_t *surface, int type, int value);
extern int osd_set_display_options(osd_surface_t *surface, unsigned char option);
extern int osd_get_display_options(osd_surface_t *surface);
extern int osd_set_engine_mode(osd_surface_t *surface, int mode);
extern osd_surface_t* osd_get_visible_surface(void);
extern int osd_move(osd_surface_t *surface, int x, int y);
extern int osd_draw_circle(osd_surface_t *surface, int xc, int yc, int radius,
			   int filled, unsigned long c);
extern int osd_draw_polygon(osd_surface_t *surface, int *x, int *y, int n,
			    unsigned long c);
extern int osd_draw_pixel_list(osd_surface_t *surface, int *x, int *y, int n,
			       unsigned int c);

/**
 * Convert RGBA into a pixel color.
 * \param r red
 * \param g green
 * \param b blue
 * \param a alpha channel
 * \return pixel color
 */
static inline unsigned long
osd_rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (a<<24) | (r<<16) | (g<<8) | b;
}

extern int osd_draw_indexed_image(osd_surface_t *surface,
				  osd_indexed_image_t *image, int x, int y);
extern int osd_palette_add_color(osd_surface_t *surface, unsigned int c);
extern int osd_palette_init(osd_surface_t *surface);

extern osd_font_t* osd_load_font(char *file);
extern int osd_destroy_font(osd_font_t *font);

extern int osd_font_height(osd_font_t *font);
extern int osd_font_width(osd_font_t *font, char *str);

extern void rgb2yuv(unsigned char r, unsigned char g, unsigned char b,
		    unsigned char *y, unsigned char *u, unsigned char *v);
extern void yuv2rgb(unsigned char y, unsigned char u, unsigned char v,
		    unsigned char *r, unsigned char *g, unsigned char *b);

static inline unsigned int
rgba2yuva(unsigned int c)
{
	unsigned char a, r, g, b, Y, U, V;

	a = (c >> 24) & 0xff;
	r = (c >> 16) & 0xff;
	g = (c >> 8) & 0xff;
	b = c & 0xff;
	rgb2yuv(r, g, b, &Y, &U, &V);

	return ((V<<24) | (U<<16) | (Y<<8) | a);
}

static inline unsigned int
yuva2rgba(unsigned int c)
{
	unsigned char Y, U, V, r, g, b, a;

	Y = (c >> 8) & 0xff;
	U = (c >> 16) & 0xff;
	V = (c >> 24) & 0xff;
	a = c & 0xff;

	yuv2rgb(Y, U, V, &r, &g, &b);

	return (a<<24) | (r<<16) | (g<<8) | b;
}

#define OSD_COLOR(r,g,b,a)	((a<<24) | (r<<16) | (g<<8) | b)

#define OSD_COLOR_WHITE		OSD_COLOR(255,255,255,255)
#define OSD_COLOR_RED		OSD_COLOR(255,0,0,255)
#define OSD_COLOR_BLUE		OSD_COLOR(0,0,255,255)
#define OSD_COLOR_GREEN		OSD_COLOR(0,255,0,255)
#define OSD_COLOR_BLACK		OSD_COLOR(0,0,0,255)
#define OSD_COLOR_YELLOW	OSD_COLOR(255,255,0,255)
#define OSD_COLOR_ORANGE	OSD_COLOR(255,180,0,255)
#define OSD_COLOR_PURPLE	OSD_COLOR(255,0,234,255)
#define OSD_COLOR_BROWN		OSD_COLOR(118,92,0,255)
#define OSD_COLOR_CYAN		OSD_COLOR(0,255,234,255)

#endif /* MVP_OSD_H */

#if 0
#ifndef FONT_H
#define FONT_H

/*
 * This structure is for compatibility with bogl fonts.  Use bdftobogl to
 * create new .c font files from X11 BDF fonts.
 */
typedef struct bogl_font {
	char *name;			/* Font name. */
	int height;			/* Height in pixels. */
	unsigned long *content;		/* 32-bit right-padded bitmap array. */
	short *offset;			/* 256 offsets into content. */
	unsigned char *width;		/* 256 character widths. */
} osd_font_t;

#endif /* FONT_H */
#endif
