/*
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

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "disp.h"
#include "sunxi_disp_ioctl.h"

#define DRAM_OFFSET (0x40000000)

static int fd = -1;
static int fbdev = -1;
static int last_id = -1;

int disp_open(void)
{
	fd = open("/dev/disp", O_RDWR);
	if (fd == -1)
		return 0;

	fbdev = open("/dev/fb0", O_RDWR);
	if (fbdev == -1) {
		close (fd);
		return 0;
	}

	return 1;
}

int disp_layer_open(void)
{
	int layer;

	uint32_t args[4];

	args[0] = 0;
	args[1] = DISP_LAYER_WORK_MODE_SCALER;
	args[2] = 0;
	args[3] = 0;
	layer = ioctl(fd, DISP_CMD_LAYER_REQUEST, args);

	return layer;
}

__disp_fb_create_para_t fb_get_para(const int fb)
{
	uint32_t args[4];
	__disp_fb_create_para_t fb_info;
	memset(&fb_info, 0, sizeof(fb_info));

	args[0] = fb;
	args[1] = (unsigned long)(&fb_info);
	args[2] = 0;
	args[3] = 0;
	ioctl(fd, DISP_CMD_FB_GET_PARA, args);
	return fb_info;
}

void disp_wait_for_vsync()
{
	uint32_t args[4];
	ioctl(fbdev, FBIO_WAITFORVSYNC, args);
}

int disp_set_para(const int layer, const uint32_t luma_buffer, const uint32_t chroma_buffer,
			const int color_format, const int width, const int height,
			const int out_x, const int out_y, const int out_width, const int out_height, const int alpha)
{
	uint32_t args[4];
	__disp_layer_info_t layer_info;
	memset(&layer_info, 0, sizeof(layer_info));
	layer_info.pipe = 1;
	layer_info.mode = DISP_LAYER_WORK_MODE_SCALER;
	layer_info.fb.mode = DISP_MOD_MB_UV_COMBINED;
	switch (color_format)
	{
	case COLOR_YUV420:
		layer_info.fb.format = DISP_FORMAT_YUV420;
		break;
	case COLOR_YUV422:
		layer_info.fb.format = DISP_FORMAT_YUV422;
		break;
	default:
		return 0;
		break;
	}
	layer_info.fb.seq = DISP_SEQ_UVUV;
	layer_info.fb.br_swap = 0;
	layer_info.fb.addr[0] = luma_buffer + DRAM_OFFSET;
	layer_info.fb.addr[1] = chroma_buffer + DRAM_OFFSET;

	layer_info.alpha_en = 1;
	layer_info.alpha_val = alpha;

	layer_info.fb.cs_mode = DISP_BT601;
	layer_info.fb.size.width = width;
	layer_info.fb.size.height = height;
	layer_info.src_win.x = 0;
	layer_info.src_win.y = 0;
	layer_info.src_win.width = width;
	layer_info.src_win.height = height;
	layer_info.scn_win.x = out_x;
	layer_info.scn_win.y = out_y;
	layer_info.scn_win.width = out_width;
	layer_info.scn_win.height = out_height;

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&layer_info);
	args[3] = 0;

	disp_wait_for_vsync();
	ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
	ioctl(fd, DISP_CMD_LAYER_TOP, args);
	ioctl(fd, DISP_CMD_LAYER_OPEN, args);

	return 1;
}

int disp_set_layer_top(const int layer)
{
	uint32_t args[4];

	args[0] = 0;
	args[1] = layer;
	args[2] = 0;
	args[3] = 0;
	return ioctl(fd, DISP_CMD_LAYER_TOP, args);
}

int disp_set_layer_bottom(const int layer)
{
	uint32_t args[4];

	args[0] = 0;
	args[1] = layer;
	args[2] = 0;
	args[3] = 0;
	return ioctl(fd, DISP_CMD_LAYER_BOTTOM, args);
}

int disp_set_scn_window(const int layer, const __disp_rect_t *scn_win)
{
	uint32_t args[4];

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long) scn_win;
	args[3] = 0;
	return ioctl(fd, DISP_CMD_LAYER_SET_SCN_WINDOW, args);
}

int disp_set_xoff(const int layer, const int xoff)
{
	uint32_t args[4];
	__disp_layer_info_t layer_info;

	memset(&layer_info, 0, sizeof(layer_info));

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&layer_info);
	args[3] = 0;
	ioctl(fd, DISP_CMD_LAYER_GET_PARA, args);

	layer_info.scn_win.x = xoff;

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long) &layer_info.scn_win;
	args[3] = 0;
	return ioctl(fd, DISP_CMD_LAYER_SET_SCN_WINDOW, args);
}

int disp_set_yoff(const int layer, const int yoff)
{
	uint32_t args[4];
	__disp_layer_info_t layer_info;

	memset(&layer_info, 0, sizeof(layer_info));

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&layer_info);
	args[3] = 0;
	ioctl(fd, DISP_CMD_LAYER_GET_PARA, args);

	layer_info.scn_win.y = yoff;

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long) &layer_info.scn_win;
	args[3] = 0;
	return ioctl(fd, DISP_CMD_LAYER_SET_SCN_WINDOW, args);
}

int disp_set_alpha(const int layer, const int alpha)
{
	uint32_t args[4];

	args[0] = 0;
	args[1] = layer;
	args[2] = alpha;
	args[3] = 0;
	return ioctl(fd, DISP_CMD_LAYER_SET_ALPHA_VALUE, args);
}

int disp_new_frame(const int layer, const uint32_t luma_buffer, const uint32_t chroma_buffer,
			const int id, const int frame_rate)
{
	uint32_t args[4], i = 0;

	__disp_video_fb_t video;
	memset(&video, 0, sizeof(__disp_video_fb_t));
	video.id = id;
	video.frame_rate = frame_rate;
	video.addr[0] = luma_buffer + DRAM_OFFSET;
	video.addr[1] = chroma_buffer + DRAM_OFFSET;

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&video);
	args[3] = 0;
	int tmp;
	while ((tmp = ioctl(fd, DISP_CMD_VIDEO_GET_FRAME_ID, args)) != last_id)
	{
		usleep(1000);
		if (i++ > 10)
			return 0;
	}

	ioctl(fd, DISP_CMD_VIDEO_SET_FB, args);
	last_id = id;

	return 1;
}

void disp_layer_close(const int layer)
{
	uint32_t args[4];

	args[0] = 0;
	args[1] = layer;
	args[2] = 0;
	args[3] = 0;

	disp_wait_for_vsync();
	ioctl(fd, DISP_CMD_LAYER_CLOSE, args);
	disp_wait_for_vsync();
	ioctl(fd, DISP_CMD_LAYER_RELEASE, args);
}

void disp_close()
{
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	if (fbdev >= 0) {
		close(fbdev);
		fbdev = -1;
	}
}
