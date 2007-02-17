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
#include <dlfcn.h>

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

static FT_Error (*dl_FT_Init_FreeType)(FT_Library*) = NULL;
static FT_Error (*dl_FT_New_Face)(FT_Library, const char*, FT_Long, FT_Face*) = NULL;
static FT_Error (*dl_FT_Done_Face)(FT_Face) = NULL;
static FT_UInt (*dl_FT_Get_Char_Index)(FT_Face, FT_ULong) = NULL;
static FT_Error (*dl_FT_Load_Glyph)(FT_Face, FT_UInt, FT_Int32) = NULL;
static FT_Error (*dl_FT_Get_Glyph)(FT_GlyphSlot, FT_Glyph*) = NULL;
static FT_Error (*dl_FT_Glyph_To_Bitmap)(FT_Glyph*, FT_Render_Mode, FT_Vector*, FT_Bool) = NULL;

static void *handle = NULL;

static int
lib_open(void)
{
	if (handle == NULL) {
		if ((handle=dlopen("libfreetype.so", RTLD_LAZY)) == NULL) {
			return -1;
		}

		dl_FT_Init_FreeType = dlsym(handle, "FT_Init_FreeType");
		dl_FT_New_Face = dlsym(handle, "FT_New_Face");
		dl_FT_Done_Face = dlsym(handle, "FT_Done_Face");
		dl_FT_Get_Char_Index = dlsym(handle, "FT_Get_Char_Index");
		dl_FT_Load_Glyph = dlsym(handle, "FT_Load_Glyph");
		dl_FT_Get_Glyph = dlsym(handle, "FT_Get_Glyph");
		dl_FT_Glyph_To_Bitmap = dlsym(handle, "FT_Glyph_To_Bitmap");
	}

	return 0;
}

#if 0
static int
lib_close(void)
{
	if (handle) {
		dlclose(handle);
		handle = NULL;
		return 0;
	}

	return -1;
}
#endif

osd_font_t*
osd_load_font(char *file)
{
	FT_Error error;
	osd_font_t *font;

	if (lib_open() < 0)
		return NULL;

	if ((font=(osd_font_t*)malloc(sizeof(*font))) == NULL)
		return NULL;

	memset(font, 0, sizeof(*font));
	font->file = strdup(file);

	error = dl_FT_Init_FreeType(&font->library);

	if (error)
		return NULL;

	error = dl_FT_New_Face(font->library, file, 0, &font->face);

	if (error)
		return NULL;

	PRINTF("font sizes: %d\n", font->face->num_fixed_sizes);

	if (FT_HAS_KERNING(font->face)) {
		PRINTF("Font has kerning!\n");
	}

	font->height = -1;
	font->ascent = 0;
	font->descent = 0;

	return font;
}

int
osd_destroy_font(osd_font_t *font)
{
	dl_FT_Done_Face(font->face);

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

	PRINTF("y %d  y_top %d  height %d %d\n",
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
		index = dl_FT_Get_Char_Index (face, (FT_ULong)text[i]);
		error = dl_FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
		if (error) {
			printf("%s(): FT_Load_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		error = dl_FT_Get_Glyph(face->glyph, glyph);
		g->advance[i] = (int)(face->glyph->advance.x >> 6);
		if (error) {
			printf("%s(): FT_Get_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		if (((FT_GlyphRec*)glyph)->format != ft_glyph_format_bitmap)
		{
			error = dl_FT_Glyph_To_Bitmap(glyph,
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

static osd_font_t*
get_default_font(void)
{
	if (default_font == NULL) {
		char *font = getenv("OSDFONT");
		if (font == NULL) {
			default_font = osd_load_font("/etc/helvB18.pcf");
		} else {
			default_font = osd_load_font(font);
		}
		if (default_font == NULL)
			return NULL;
		PRINTF("Default font loaded\n");
	}

	return default_font;
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
		if ((font=get_default_font()) == NULL)
			return -1;
	}

	if (font->height == -1)
		osd_font_height(font);

	slot = font->face->glyph;

	if ((g=render_glyphs(font, text)) == NULL)
		return -1;

	if (background) {
		int w =  0;

		for (i=0; i<strlen(text); i++) {
			w += g->advance[i];
		}

		osd_fill_rect(surface, x, y, w, font->height, bg);
	}

	y += font->height - font->descent;

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

static int
calculate_height(osd_font_t *font)
{
	glyph_str_t *g;
	int i;
	FT_Glyph *glyph;
	FT_Face face;
	FT_Error error;
	int maxtop = 0, maxbottom = 0;

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	if (font == NULL)
		return -1;

	if ((g=(glyph_str_t*)malloc(sizeof(*g))) == NULL)
		return -1;

	if ((g->glyphs=(FT_Glyph*)malloc(sizeof(FT_Glyph))) == NULL)
		goto err;
	if ((g->advance=(int*)malloc(sizeof(int))) == NULL)
		goto err;

	glyph = g->glyphs;
	face = font->face;

	for (i=0; i<256; i++) {
		FT_UInt index;
		int top, bottom;
		index = dl_FT_Get_Char_Index (face, (FT_ULong)i);
		error = dl_FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
		if (error) {
			printf("%s(): FT_Load_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		error = dl_FT_Get_Glyph(face->glyph, glyph);
		g->advance[i] = (int)(face->glyph->advance.x >> 6);
		top = (int)(face->glyph->metrics.horiBearingY >> 6);
		bottom = (int)(face->glyph->metrics.height >> 6) - top;
		if (error) {
			printf("%s(): FT_Get_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		if (((FT_GlyphRec*)glyph)->format != ft_glyph_format_bitmap)
		{
			error = dl_FT_Glyph_To_Bitmap(glyph,
						      ft_render_mode_mono,
						      0, 1);
			if (error) {
				printf("%s(): FT_Glyph_To_Bitmap() failed\n",
				       __FUNCTION__);
				goto err;
			}
		}

		if (top > maxtop)
			maxtop = top;
		if (bottom > maxbottom)
			maxbottom = bottom;
	}

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	font->ascent = maxtop;
	font->descent = maxbottom;

	return maxtop + maxbottom;

 err:
	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	if (g) {
		if (g->glyphs)
			free(g->glyphs);
		free(g);
	}

	return -1;
}

static int
calculate_width(osd_font_t *font, char *text)
{
	glyph_str_t *g;
	int i;
	FT_Glyph *glyph;
	FT_Face face;
	FT_Error error;
	int width = 0;

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	if ((font == NULL) || (text == NULL))
		return -1;

	if ((g=(glyph_str_t*)malloc(sizeof(*g))) == NULL)
		return -1;

	if ((g->glyphs=(FT_Glyph*)malloc(sizeof(FT_Glyph)*strlen(text))) == NULL)
		goto err;
	if ((g->advance=(int*)malloc(sizeof(int)*strlen(text))) == NULL)
		goto err;

	g->len = strlen(text);

	glyph = g->glyphs;
	face = font->face;

	for (i=0; i<g->len; i++) {
		FT_UInt index;
		index = dl_FT_Get_Char_Index (face, (FT_ULong)text[i]);
		error = dl_FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
		if (error) {
			printf("%s(): FT_Load_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		error = dl_FT_Get_Glyph(face->glyph, glyph);
		g->advance[i] = (int)(face->glyph->advance.x >> 6);
		if (error) {
			printf("%s(): FT_Get_Glyph() failed i = %d\n",
			       __FUNCTION__, i);
			continue;
		}
		if (((FT_GlyphRec*)glyph)->format != ft_glyph_format_bitmap)
		{
			error = dl_FT_Glyph_To_Bitmap(glyph,
						      ft_render_mode_mono,
						      0, 1);
			if (error) {
				printf("%s(): FT_Glyph_To_Bitmap() failed\n",
				       __FUNCTION__);
				goto err;
			}
		}

		width += g->advance[i];
	}

	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	return width;

 err:
	PRINTF("%s(): %d\n", __FUNCTION__, __LINE__);

	if (g) {
		if (g->glyphs)
			free(g->glyphs);
		free(g);
	}

	return -1;
}

int
osd_font_height(osd_font_t *font)
{
	if (font == NULL) {
		if ((font=get_default_font()) == NULL)
			return -1;
	}

	/*
	 * XXX: rewrite to use render_glyphs() instead (256 character string)
	 */

	if (font->height == -1) {
		font->height = calculate_height(font);
	}

	/*
	 * XXX: height is wrong!  it is the height above the baseline
	 */

	return font->height;
}

int
osd_font_width(osd_font_t *font, char *text)
{
	if (text == NULL)
		return -1;

	if (font == NULL) {
		if ((font=get_default_font()) == NULL)
			return -1;
	}

	/*
	 * XXX: rewrite to use render_glyphs() instead
	 */

	return calculate_width(font, text);
}
