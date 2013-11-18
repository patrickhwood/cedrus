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

int main(const int argc, const char **argv)
{
	image_layer layers[2];

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s infile1.jpg infile2.jpg\n", argv[0]);
		exit(EXIT_FAILURE);
	}


	if (!ve_open())
		err(EXIT_FAILURE, "Can't open VE");

	if (!disp_open())
		err(EXIT_FAILURE, "Can't open /dev/disp or /dev/fb0\n");

	init_jpeg(&layers[0], argv[1]);
	init_jpeg(&layers[1], argv[2]);

	show_jpeg(&layers[0]);
	show_jpeg(&layers[1]);
	disp_set_layer_top(layers[0].layer);
	disp_set_alpha(layers[0].layer, 255);
	while (1) {
		sleep(5);
		transition_layers(&layers[0], &layers[1]);
		sleep(5);
		transition_layers(&layers[1], &layers[0]);
	}
	free_jpeg(&layers[0]);
	free_jpeg(&layers[1]);

	disp_close();
	ve_close();

	return EXIT_SUCCESS;
}
