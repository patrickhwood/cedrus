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
#include "jpeg.h"
#include "decode.h"
#include "ve.h"
#include "io.h"

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
