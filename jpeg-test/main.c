/*
 * Proof of Concept JPEG decoder using Allwinners CedarX
 *
 * WARNING: Don't use on "production systems". This was made by reverse
 * engineering and might crash your system or destroy data!
 * It was made only for my personal use of testing the reverse engineered
 * things, so don't expect good code quality. It's far from complete and
 * might crash if the JPEG doesn't fit it's requirements!
 *
 *
 *
 * Copyright (c) 2013 Jens Kuske <jenskuske@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include "disp.h"
#include "ve.h"
#include "show.h"

#include "slideshow.h"

void slideshow (const char *dir)
{
	void *show = init_slideshow(dir);
	int ret = set_slide (show, 1, NULL);
	if (ret < 0)
		fprintf (stderr, "set_slide returned -1\n");

	int c = 'N';
	while (1) {
		// c = getchar();
		if (c < 0)
			break;
		if (c >= 'a' && c <= 'z') {
			if (cur_slide (show) == c - 'a' + 1)
				// skip current slide
				continue;

			ret = set_slide (show, c - 'a' + 1, NULL);
			if (ret < 0)
				fprintf (stderr, "set_slide returned -1\n");
		}
		else if (c == 'N') {
			int i = cur_slide (show);
			
			ret = set_slide (show, i + 1, NULL);
			if (ret < 0) {
				fprintf (stderr, "set_slide returned -1\n");
				c = 'P';
			}
		}
		else if (c == 'P') {
			int i = cur_slide (show);
			
			ret = set_slide (show, i - 1, NULL);
			if (ret < 0) {
				fprintf (stderr, "set_slide returned -1\n");
				c = 'N';
			}
		}
	}
	end_slideshow(show);
	free_slideshow(show);
	return;
}

int main(const int argc, const char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s slideshow-dir\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!ve_open())
		err(EXIT_FAILURE, "Can't open VE");

	if (!disp_open())
		err(EXIT_FAILURE, "Can't open /dev/disp or /dev/fb0\n");

	slideshow(argv[1]);

	disp_close();
	ve_close();

	return 0;
}
