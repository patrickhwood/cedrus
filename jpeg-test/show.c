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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "jpeg.h"
#include "ve.h"
#include "io.h"
#include "disp.h"
#include "decode.h"
#include "show.h"

int init_jpeg(image_layer *layer, const char *filename)
{
	jpeg_t *jpeg = &layer->jpeg;
	uint8_t *data;
	int in;
	struct stat s;

	if ((in = open(filename, O_RDONLY)) == -1)
		return -1;

	if (fstat(in, &s) < 0)
		return -2;

	if (s.st_size == 0)
		return -3;

	if ((data = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, in, 0)) == MAP_FAILED)
		return -4;

	memset(jpeg, 0, sizeof(jpeg_t));
	if (!parse_jpeg(jpeg, data, s.st_size))
		return -5;

	decode_jpeg(layer);
	munmap(data, s.st_size);
	close(in);

	layer->layer = disp_layer_open();
	if (layer->layer <= 0) {
		fprintf(stderr, "Can't open layer\n");
		disp_close();
		return -6;
	}

	switch ((jpeg->comp[0].samp_h << 4) | jpeg->comp[0].samp_v)
	{
	case 0x11:
	case 0x21:
		layer->color = COLOR_YUV422;
		break;
	case 0x12:
	case 0x22:
	default:
		layer->color = COLOR_YUV420;
		break;
	}
	return 0;
}

void free_jpeg(image_layer *layer)
{
	disp_layer_close(layer->layer);

	ve_free(layer->luma_output);
	ve_free(layer->chroma_output);
}

void show_jpeg(image_layer *layer)
{
	jpeg_t *jpeg = &layer->jpeg;
	__disp_fb_create_para_t fb_para = fb_get_para(0);
	int width, height;
	int xoff, yoff;

	if ((float) jpeg->width / fb_para.width > (float) jpeg->height / fb_para.height) {
		width = fb_para.width;
		height = (float) fb_para.width / jpeg->width * jpeg->height;
	}
	else {
		width = (float) fb_para.height / jpeg->height * jpeg->width;
		height = fb_para.height;
	}

	xoff = (fb_para.width - width) / 2;
	yoff = (fb_para.height - height) / 2;

	disp_set_para(layer->layer, ve_virt2phys(layer->luma_output), ve_virt2phys(layer->chroma_output),
		layer->color, jpeg->width, jpeg->height,
		xoff, yoff, width, height, 0);
}

void transition_layers(image_layer *layer1, image_layer *layer2)
{
	int alpha;

	if (layer1->layer > 0) {
		disp_set_layer_top(layer1->layer);
		for (alpha = 255; alpha >= 0; alpha -= 5) {
			disp_wait_for_vsync();
			disp_set_alpha(layer1->layer, alpha);
		}
	}
	if (layer2->layer > 0) {
		disp_set_layer_top(layer2->layer);
		for (alpha = 0; alpha <= 255; alpha += 5) {
			disp_wait_for_vsync();
			disp_set_alpha(layer2->layer, alpha);
		}
	}
}
