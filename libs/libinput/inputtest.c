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

#include "input.h"

int
main(int argc, char **argv)
{
	unsigned int key;
	input_t *handle;

	if (input_init() < 0) {
		fprintf(stderr, "input init failed!\n");
		exit(1);
	}

	if ((handle=input_open(INPUT_KEYBOARD, INPUT_BLOCKING)) == NULL) {
		fprintf(stderr, "input open failed!\n");
		exit(1);
	}

	printf("Press any key (stop to quit):\n");

	while (1) {
		const char *name;
		key = input_read_raw(handle);
		name = input_key_name(key);
		if (name == NULL) {
			fprintf(stderr, "Unknown key 0x%.8x!\n", key);
			exit(1);
		}
		printf("\t0x%.8x\t%s\n", key, name);

		if (strstr(name, "Stop") != NULL) {
			break;
		}
	}

	return 0;
}
