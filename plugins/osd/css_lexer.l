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

static int linenum = 1;
%}

%%

\#		{ return HASH; }
\.		{ return DOT; }
\,		{ return COMMA; }
\{		{ return LBRAK; }
\}		{ return RBRAK; }
\;		{ return SEMI; }

[^ \t\n\{\};:]+	{ return STR; }

%%

#include <stdio.h>

int
yywrap(void)
{
	return 1;
}

int
yyerror(char *str)
{
	fprintf(stderr, "parse error in line %d, '%s'\n", linenum, str);

	if (str == NULL) {
		yyunput(0,0);
	}

	return -1;
}
