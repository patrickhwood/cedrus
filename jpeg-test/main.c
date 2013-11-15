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
#include <err.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "jpeg.h"
#include "ve.h"
#include "io.h"
#include "disp.h"

void set_quantization_tables(jpeg_t *jpeg, void *regs)
{
	int i;
	for (i = 0; i < 64; i++)
		writel(regs + 0x100 + 0x80, (uint32_t)(64 + i) << 8 | jpeg->quant[0]->coeff[i]);
	for (i = 0; i < 64; i++)
		writel(regs + 0x100 + 0x80, (uint32_t)(i) << 8 | jpeg->quant[1]->coeff[i]);
}

void set_huffman_tables(jpeg_t *jpeg, void *regs)
{
	uint32_t buffer[512];
	memset(buffer, 0, 4*512);
	int i;
	for (i = 0; i < 4; i++)
	{
		if (jpeg->huffman[i])
		{
			int j, sum, last;

			last = 0;
			sum = 0;
			for (j = 0; j < 16; j++)
			{
				((uint8_t *)buffer)[i * 64 + 32 + j] = sum;
				sum += jpeg->huffman[i]->num[j];
				if (jpeg->huffman[i]->num[j] != 0)
					last = j;
			}
			memcpy(&(buffer[256 + 64 * i]), jpeg->huffman[i]->codes, sum);
			sum = 0;
			for (j = 0; j <= last; j++)
			{
				((uint16_t *)buffer)[i * 32 + j] = sum;
				sum += jpeg->huffman[i]->num[j];
				sum *= 2;
			}
			for (j = last + 1; j < 16; j++)
			{
				((uint16_t *)buffer)[i * 32 + j] = 0xffff;
			}
		}
	}

	for (i = 0; i < 512; i++)
	{
		writel(regs + 0x100 + 0xe4, buffer[i]);
	}
}

void set_format(jpeg_t *jpeg, void *regs)
{
	uint8_t fmt = (jpeg->comp[0].samp_h << 4) | jpeg->comp[0].samp_v;

	switch (fmt)
	{
	case 0x11:
		writeb(regs + 0x100 + 0x1b, 0x1b);
		break;
	case 0x21:
		writeb(regs + 0x100 + 0x1b, 0x13);
		break;
	case 0x12:
		writeb(regs + 0x100 + 0x1b, 0x23);
		break;
	case 0x22:
		writeb(regs + 0x100 + 0x1b, 0x03);
		break;
	}
}

void set_size(jpeg_t *jpeg, void *regs)
{
	uint16_t h = (jpeg->height - 1) / (8 * jpeg->comp[0].samp_v);
	uint16_t w = (jpeg->width - 1) / (8 * jpeg->comp[0].samp_h);
	writel(regs + 0x100 + 0xb8, (uint32_t)h << 16 | w);
}

void output_ppm(FILE *file, jpeg_t *jpeg, uint8_t *luma_buffer, uint8_t *chroma_buffer)
{
	fprintf(file, "P3\n%u %u\n255\n", jpeg->width, jpeg->height);
	int x, y, i = 0;
	for (y = 0; y < jpeg->height; y++)
	{
		for (x = 0; x < jpeg->width; x++)
		{
			// reordering and colorspace conversion should be done by Display Engine Frontend (DEFE)
			int cy = y / jpeg->comp[0].samp_v;
			float Y = *((uint8_t *)(luma_buffer + (x / 32) * 1024 + (x % 32) + ((y % 32) * 32) + ((y / 32) * (((jpeg->width + 31) / 32) * 1024))));
			float Cb = *((uint8_t *)(chroma_buffer + (x / 32) * 1024 + ((x % 32) / 2 * 2) + ((cy % 32) * 32) + ((cy / 32) * (((jpeg->width + 31) / 32) * 1024)))) - 128.0;
			float Cr = *((uint8_t *)(chroma_buffer + (x / 32) * 1024 + ((x % 32) / 2 * 2 + 1) + ((cy % 32) * 32) + ((cy / 32) * (((jpeg->width + 31) / 32) * 1024)))) - 128.0;

			float R = Y + 1.402 * Cr;
			float G = Y - 0.344136 * Cb - 0.714136 * Cr;
			float B = Y + 1.772 * Cb;

			if (R > 255.0) R = 255.0;
			else if (R < 0.0) R = 0.0;

			if (G > 255.0) G = 255.0;
			else if (G < 0.0) G = 0.0;

			if (B > 255.0) B = 255.0;
			else if (B < 0.0) B = 0.0;

			fprintf(file, "%3u %3u %3u %c", (uint8_t)R, (uint8_t)G, (uint8_t)B, (++i % 5) ? ' ' : '\n');
		}
	}
	fprintf(file, "\n");
}

typedef struct {
	int layer;
	int color;
	uint8_t *luma_output;
	uint8_t *chroma_output;
	jpeg_t jpeg;
} image_layer;

void decode_jpeg(image_layer *layer)
{
	jpeg_t *jpeg = &layer->jpeg;
	long long startt, loadt, decodet;
	struct timeb tb;

	void *ve_regs = ve_get_regs();

	int input_size =(jpeg->data_len + 65535) & ~65535;
	uint8_t *input_buffer = ve_malloc(input_size);
	int output_size = ((jpeg->width + 31) & ~31) * ((jpeg->height + 31) & ~31);
	layer->luma_output = ve_malloc(output_size);
	layer->chroma_output = ve_malloc(output_size);
	memcpy(input_buffer, jpeg->data, jpeg->data_len);
	ve_flush_cache(input_buffer, jpeg->data_len);

	ftime (&tb);
	startt = (long long) tb.time * 1000 + tb.millitm;
	// activate MPEG engine
	writel(ve_regs + 0x00, 0x00130000);

	// set restart interval
	writel(ve_regs + 0x100 + 0xc0, jpeg->restart_interval);

	// set JPEG format
	set_format(jpeg, ve_regs);

	// set output buffers (Luma / Croma)
	writel(ve_regs + 0x100 + 0xcc, ve_virt2phys(layer->luma_output));
	writel(ve_regs + 0x100 + 0xd0, ve_virt2phys(layer->chroma_output));

	// set size
	set_size(jpeg, ve_regs);

	// ??
	writel(ve_regs + 0x100 + 0xd4, 0x00000000);

	// input end
	writel(ve_regs + 0x100 + 0x34, ve_virt2phys(input_buffer) + input_size - 1);

	// ??
	writel(ve_regs + 0x100 + 0x14, 0x0000007c);

	// set input offset in bits
	writel(ve_regs + 0x100 + 0x2c, 0 * 8);

	// set input length in bits
	writel(ve_regs + 0x100 + 0x30, jpeg->data_len * 8);

	// set input buffer
	writel(ve_regs + 0x100 + 0x28, ve_virt2phys(input_buffer) | 0x70000000);

	// set Quantisation Table
	set_quantization_tables(jpeg, ve_regs);

	// set Huffman Table
	writel(ve_regs + 0x100 + 0xe0, 0x00000000);
	set_huffman_tables(jpeg, ve_regs);

	ftime (&tb);
	loadt = (long long) tb.time * 1000 + tb.millitm;

	// start
	writeb(ve_regs + 0x100 + 0x18, 0x0e);

	// wait for interrupt
	ve_wait(1);

	ftime (&tb);
	decodet = (long long) tb.time * 1000 + tb.millitm;

	// clean interrupt flag (??)
	writel(ve_regs + 0x100 + 0x1c, 0x0000c00f);

	// stop MPEG engine
	writel(ve_regs + 0x0, 0x00130007);

	// output_ppm(stdout, jpeg, output, output + (output_buf_size / 2));

	fprintf (stderr, "total time: %lld msec, decode time: %lld msec\n", decodet - startt, decodet - loadt);
	fflush (stderr);

	ve_free(input_buffer);
}

void init_jpeg(image_layer *layer, const char *filename)
{
	jpeg_t *jpeg = &layer->jpeg;
	uint8_t *data;
	int in;
	struct stat s;

	if ((in = open(filename, O_RDONLY)) == -1)
		err(EXIT_FAILURE, "%s", filename);

	if (fstat(in, &s) < 0)
		err(EXIT_FAILURE, "stat %s", filename);

	if (s.st_size == 0)
		errx(EXIT_FAILURE, "%s empty", filename);

	if ((data = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, in, 0)) == MAP_FAILED)
		err(EXIT_FAILURE, "mmap %s", filename);

	memset(jpeg, 0, sizeof(jpeg_t));
	if (!parse_jpeg(jpeg, data, s.st_size))
		warnx("Can't parse JPEG");

	decode_jpeg(layer);
	munmap(data, s.st_size);
	close(in);

	layer->layer = disp_layer_open();
	if (layer->layer <= 0) {
		fprintf(stderr, "Can't open layer\n");
		disp_close();
		return;
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
		xoff, yoff, width, height);
}

void transition_layers(image_layer *layer1, image_layer *layer2)
{
	jpeg_t *jpeg1 = &layer1->jpeg;
	jpeg_t *jpeg2 = &layer2->jpeg;
	__disp_fb_create_para_t fb_para = fb_get_para(0);
	int width1, height1, width2, height2;
	int xoff1, yoff1, xoff2, yoff2;
	float sf;

	if ((float) jpeg1->width / fb_para.width > (float) jpeg1->height / fb_para.height) {
		width1 = fb_para.width;
		height1 = (float) fb_para.width / jpeg1->width * jpeg1->height;
	}
	else {
		width1 = (float) fb_para.height / jpeg1->height * jpeg1->width;
		height1 = fb_para.height;
	}

	xoff1 = (fb_para.width - width1) / 2;
	yoff1 = (fb_para.height - height1) / 2;

	if ((float) jpeg2->width / fb_para.width > (float) jpeg2->height / fb_para.height) {
		width2 = fb_para.width;
		height2 = (float) fb_para.width / jpeg2->width * jpeg2->height;
	}
	else {
		width2 = (float) fb_para.height / jpeg2->height * jpeg2->width;
		height2 = fb_para.height;
	}

	xoff2 = (fb_para.width - width2) / 2;
	yoff2 = (fb_para.height - height2) / 2;

	for (sf = .01f; sf <= 1.0f; sf += .02f) {
		disp_wait_for_vsync();
		disp_set_xoff(layer2->layer, xoff2 - width2 * (1.0f - sf));
		disp_set_xoff(layer1->layer, xoff1 + width2 * sf);
		disp_set_alpha(layer2, 0);

		usleep(2000);
	}
}

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
	sleep(1);
	show_jpeg(&layers[1]);
	transition_layers(&layers[0], &layers[1]);

	free_jpeg(&layers[0]);
	sleep(2);
	free_jpeg(&layers[1]);

	disp_close();
	ve_close();

	return EXIT_SUCCESS;
}
