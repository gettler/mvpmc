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

#ifndef CSS_H
#define CSS_H

typedef enum {
	CSS_TYPE_ID = 1,
	CSS_TYPE_CLASS,
	CSS_TYPE_CHILD,
	CSS_TYPE_TAG,
} css_type_t;

typedef enum {
	CSS_PROP_BACKGROUND,
	CSS_PROP_BORDER,
	CSS_PROP_COLOR,
	CSS_PROP_DISPLAY,
	CSS_PROP_FONT,
	CSS_PROP_FONT_WEIGHT,
	CSS_PROP_FONT_STYLE,
	CSS_PROP_FONT_SIZE,
	CSS_PROP_HEIGHT,
	CSS_PROP_MARGIN,
	CSS_PROP_MARGIN_LEFT,
	CSS_PROP_MARGIN_RIGHT,
	CSS_PROP_MARGIN_TOP,
	CSS_PROP_MARGIN_BOTTOM,
	CSS_PROP_PADDING,
	CSS_PROP_POSITION,
	CSS_PROP_OVERFLOW,
	CSS_PROP_TEXT_ALIGN,
	CSS_PROP_TEXT_DECOR,
	CSS_PROP_WIDTH,
	CSS_PROP_Z_INDEX,
} css_prop_t;

typedef enum {
	CSS_MOD_RIGHT,
	CSS_MOD_LEFT,
	CSS_MOD_TOP,
	CSS_MOD_BOTTOM,
	CSS_MOD_NO_REPEAT,
} css_mod_t;

typedef struct {
	css_type_t type;
	char *value;
} css_id_t;

typedef struct {
	css_prop_t property;
	char *value;
} css_dec_t;

typedef struct {
	css_id_t **selectors;
	css_dec_t **declarations;
	int nsel;
	int ndec;
} css_rule_t;

#endif /* CSS_H */
