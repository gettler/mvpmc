%{
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
#include <string.h>

#include "mvp_refmem.h"
#include "css.h"

extern int yyerror(char*);
extern int yylex(void);

extern char *yytext;

static int css_declaration(char *property, char *value);
static int css_selector(char *selector, char *modifier);
%}

%union {
	char *lexeme;
	void *p;
}

%token STR ANY
%token HASH DOT COMMA
%token LBRAK RBRAK SEMI COLON ASTERISK GT
%token COMMENT_START COMMENT_END

%type <lexeme> str astr selector
%type <p> list statement comment rule slist
%type <p> declist declaration

%%

start : list	{ return 0; }

list : list statement { }
list : statement { }

statement : comment { }
statement : rule { }

comment : COMMENT_START any COMMENT_END { printf("COMMENT\n"); }

rule : slist LBRAK declist RBRAK { /*printf("\n");*/ }

declist : declist declaration { }
declist : declaration { }

declaration : str COLON str SEMI { css_declaration($1, $3); }

slist : slist selector { }
slist : selector { }

selector : str { css_selector($1, NULL); }
selector : HASH str { css_selector($2, "#"); }
selector : DOT str { css_selector($2, "."); }
selector : COMMA str { css_selector($2, ","); }
selector : GT str { css_selector($2, ">"); }

any : any astr { }
any : any ASTERISK { }
any : astr { }

str : STR { $$ = ref_strdup(yytext); }
astr : ANY { $$ = ref_strdup(yytext); }

%%

#include "css_lexer.c"

typedef struct {
	css_prop_t type;
	char *value;
} css_property_t;

static css_property_t plist[] = {
	{ CSS_PROP_BACKGROUND, "background" },
	{ CSS_PROP_BORDER, "border" },
	{ CSS_PROP_COLOR, "color" },
	{ CSS_PROP_DISPLAY, "display" },
	{ CSS_PROP_FONT, "font" },
	{ CSS_PROP_FONT_WEIGHT, "font-weight" },
	{ CSS_PROP_FONT_STYLE, "font-style" },
	{ CSS_PROP_FONT_SIZE, "font-size" },
	{ CSS_PROP_HEIGHT, "height" },
	{ CSS_PROP_MARGIN, "margin" },
	{ CSS_PROP_MARGIN_LEFT, "margin-left" },
	{ CSS_PROP_MARGIN_RIGHT, "margin-right" },
	{ CSS_PROP_MARGIN_TOP, "margin-top" },
	{ CSS_PROP_MARGIN_BOTTOM, "margin-bottom" },
	{ CSS_PROP_PADDING, "padding" },
	{ CSS_PROP_POSITION, "position" },
	{ CSS_PROP_OVERFLOW, "overflow" },
	{ CSS_PROP_TEXT_ALIGN, "text-align" },
	{ CSS_PROP_TEXT_DECOR, "text-decoration" },
	{ CSS_PROP_WIDTH, "width" },
	{ CSS_PROP_Z_INDEX, "z-index" },
	{ 0, NULL },
};

#if 0
static css_rule_t currule;
#endif

static int
css_declaration(char *property, char *value)
{
	int i = 0;
	css_prop_t prop = 0;

	while (plist[i].value) {
		if (strcasecmp(property, plist[i].value) == 0) {
			prop = plist[i].type;
			break;
		}
		i++;
	}

	//printf("    '%s' (%d): '%s'\n", property, prop, value);

	return 0;
}

static int
css_selector(char *selector, char *modifier)
{
	//printf(" %s '%s'", modifier, selector);

	/*
	 * selector modifiers:
	 *   ''		decendant
	 *   '#'	ID
	 *   '.'	class
	 *   '>'	child
	 *   ','	and
	 */

	return 0;
}
