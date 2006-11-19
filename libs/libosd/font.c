/*
 *  Copyright (C) 2006, Jon Gettler
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
#include "font.h"

#if 0
#define PRINTF(x...) printf(x)
#else
#define PRINTF(x...)
#endif

typedef struct {
	FT_UInt index;
	FT_Vector pos;
	FT_Glyph image;
} Glyph;

typedef struct {
	FT_Glyph *glyphs;
	int *advance;
	int len;
	osd_font_t *font;
} glyph_str_t;

static osd_font_t *default_font = NULL;

#define MAX_NUM_GLYPHS 256

osd_font_t*
osd_load_font(char *file)
{
	FT_Error error;
	osd_font_t *font;

	if ((font=(osd_font_t*)malloc(sizeof(*font))) == NULL)
		return NULL;

	memset(font, 0, sizeof(*font));
	font->file = strdup(file);

	error = FT_Init_FreeType(&font->library);

	if (error)
		return NULL;

	error = FT_New_Face(font->library, file, 0, &font->face);

	if (error)
		return NULL;

	PRINTF("font sizes: %d\n", font->face->num_fixed_sizes);

	if (FT_HAS_KERNING(font->face)) {
		PRINTF("Font has kerning!\n");
	}
	return font;
}

int
osd_destroy_font(osd_font_t *font)
{
	FT_Done_Face(font->face);

	free(font->file);
	free(font);

	return 0;
}

static int
draw_glyph(osd_surface_t *surface, FT_Glyph *glyph, int x, int y, int advance,
	   unsigned int fg, unsigned int bg, int background)
{
	FT_BitmapGlyph bgl = (FT_BitmapGlyph)(*glyph);
	FT_Bitmap *bitmap = &bgl->bitmap;
	FT_Int  i, j;
	int pixels = bitmap->width * bitmap->rows;
	int fg_x[pixels], fg_y[pixels];
	int bg_x[pixels], bg_y[pixels];
	int fg_n = 0, bg_n = 0;

	PRINTF("%s(): (%d,%d) size %d x %d\n",
	       __FUNCTION__, x, y, bitmap->width, bitmap->rows);

	FT_Bitmap *source = bitmap;
	int y_top = y - bgl->top;
	int pitch = bitmap->pitch << 3;
	int height = source->rows;
	int buffer_height = surface->height;
	int buffer_width = surface->width;
	int x_top = bgl->top + x;
	unsigned char *src = source->buffer;
	int width = source->width;

	printf("y %d  y_top %d  height %d %d\n",
	       y, y_top, height, bitmap->rows);

	/*
	 * XXX: what is the font height?
	 */

	x_top = x + (advance - source->width)/2;

	for (i = 0, y = y_top; i < pitch * height; i += pitch, y++)
        {
		if (y < 0 || y >= buffer_height)
			continue;
		
		for (j = 0, x = x_top; j < width; j++, x++)
		{
			if (x < 0 || x >= buffer_width)
			{
				continue;
			}

			if (src[(i + j) >> 3] & (1 << (7 - ((i + j) % 8)))) {
				fg_x[fg_n] = x;
				fg_y[fg_n] = y;
				fg_n++;
#if 0
			} else if (background) {
				bg_x[bg_n] = x;
				bg_y[bg_n] = y;
				bg_n++;
#endif
			}
		}
        }

	if (fg_n > 0)
		osd_draw_pixel_list(surface, fg_x, fg_y, fg_n, fg);
	if (bg_n > 0)
		osd_draw_pixel_list(surface, bg_x, bg_y, bg_n, bg);

	return 0;
}

static int
draw_glyphs(osd_surface_t *surface, glyph_str_t *g,
	    int x, int y, unsigned int fg, unsigned int bg, int background)
{
	int i;
	int advance;
	FT_GlyphRec *gr;

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	for (i=0; i<g->len; i++) {
		gr = (FT_GlyphRec*)g->glyphs+i;
		advance = g->advance[i];
		draw_glyph(surface, &(g->glyphs[i]), x, y, advance,
				fg, bg, background);
		x += advance;
	}

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	return 0;
}

static glyph_str_t*
render_glyphs(osd_font_t *font, const char *text)
{
	glyph_str_t *g;
	int i;
	FT_Glyph *glyph;
	FT_Face face;
	FT_Error error;

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	if ((font == NULL) || (text == NULL))
		return NULL;

	if ((g=(glyph_str_t*)malloc(sizeof(*g))) == NULL)
		return NULL;

	if ((g->glyphs=(FT_Glyph*)malloc(sizeof(FT_Glyph)*strlen(text))) == NULL)
		goto err;
	if ((g->advance=(int*)malloc(sizeof(int)*strlen(text))) == NULL)
		goto err;

	g->len = strlen(text);

	glyph = g->glyphs;
	face = font->face;

	for (i=0; i<g->len; i++) {
		FT_UInt index;
		index = FT_Get_Char_Index (face, (FT_ULong)text[i]);
		error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
		if (error) {
			printf("%s(): FT_Load_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		error = FT_Get_Glyph(face->glyph, glyph);
		g->advance[i] = (int)(face->glyph->advance.x >> 6);
		if (error) {
			printf("%s(): FT_Get_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		if (((FT_GlyphRec*)glyph)->format != ft_glyph_format_bitmap)
		{
			error = FT_Glyph_To_Bitmap(glyph,
						   ft_render_mode_mono,
						   0, 1);
			if (error) {
				printf("%s(): FT_Glyph_To_Bitmap() failed\n",
				       __FUNCTION__);
				goto err;
			}
		}
		glyph++;
	}

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	return g;

 err:
	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	if (g) {
		if (g->glyphs)
			free(g->glyphs);
		free(g);
	}

	return NULL;
}

int
osd_draw_text(osd_surface_t *surface, int x, int y, const char *text,
	      unsigned int fg, unsigned int bg, int background,
	      osd_font_t *font)
{
	FT_GlyphSlot slot;
	glyph_str_t *g;
	int i;

	if (font == NULL) {
		if (default_font == NULL) {
			char *font = getenv("OSDFONT");
			if (font == NULL) {
				default_font = osd_load_font("/etc/helvB18.pcf");
			} else {
				default_font = osd_load_font(font);
			}
			if (default_font == NULL)
				return -1;
			PRINTF("Default font loaded\n");
		}
		font = default_font;
	}

	slot = font->face->glyph;

	if ((g=render_glyphs(font, text)) == NULL)
		return -1;

	if (background) {
		int w =  0;
		int h = 14;

		for (i=0; i<strlen(text); i++) {
			w += g->advance[i];
		}

		/*
		 * XXX: the font is in the wrong place!
		 */
		osd_fill_rect(surface, x, y-h, w, h, bg);
	}

	draw_glyphs(surface, g, x, y, fg, bg, background);

	if (g) {
#if 0
		for (i=0; g->len; i++) {
			FT_Done_Glyph(g->glyphs[i]);
		}
#endif
		free(g->glyphs);
		free(g);
	}

	return 0;
}
