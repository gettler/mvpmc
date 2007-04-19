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

extern FILE *yyin;
extern int yyparse(void);

int
main(int argc, char **argv)
{
	int ret;
	char *path = argv[1];

	printf("CSS: parse file '%s'\n", path);

	if ((yyin=fopen(path, "r")) == NULL) {
		perror(path);
		return -1;
	}

	ret = yyparse();

	fclose(yyin);

	printf("CSS: parser returned %d\n", ret);

	return 0;
}
