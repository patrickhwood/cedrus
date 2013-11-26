#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "slideshow.h"
#include "show.h"

typedef struct {
	int slide;
	char *dir;
	image_layer layer;
} slideshow;

void *init_slideshow(const char *dir)
{
	slideshow *show = (slideshow *) malloc(sizeof(slideshow));
	if (!show)
		return (void *) 0;

	memset(show, 0, sizeof(slideshow));
	show->slide = 0;
	show->dir = (char *) malloc(strlen(dir) + 1);
	if (!show->dir) {
		free(show);
		return (void *) 0;
	}

	strcpy(show->dir, dir);

	return (void *) show;
}

int free_slideshow(void *vshow)
{
	slideshow *show = (slideshow *) vshow;
	free_jpeg(&show->layer);
	free(show->dir);
	free(show);
	return 0;
}

int end_slideshow(void *vshow)
{
	slideshow *show = (slideshow *) vshow;
	image_layer newlayer;

	memset(&newlayer, 0, sizeof(image_layer));
	transition_layers(&show->layer, &newlayer);
	return 0;
}

static void merge_bitmaps(image_layer *layer, bitmap *bitmap)
{
	int xoff = bitmap->xoff;
	int yoff = bitmap->yoff;
	int width = bitmap->width;
	int height = bitmap->height;
	unsigned char *bitmap_in = bitmap->bitmap;
	int jwidth = layer->jpeg.width;
	int jheight = layer->jpeg.width;
	int x, y;
	uint8_t *luma = layer->luma_output;
	uint8_t *chroma = layer->chroma_output;

	// bitmap->color should be in YUV

	for (y = yoff; y < yoff + height && y < jheight; y++) {
		int cy = y / layer->jpeg.comp[0].samp_v;
		for (x = xoff; x < xoff + width && x < jwidth; x++, bitmap_in++) {
			// only set black pixels
			if (!*bitmap_in) {
				*(luma + (x / 32) * 1024 + (x % 32) + ((y % 32) * 32) + ((y / 32) * (((jwidth + 31) / 32) * 1024))) = bitmap->color[0];
				*(chroma + (x / 32) * 1024 + ((x % 32) / 2 * 2) + ((cy % 32) * 32) + ((cy / 32) * (((jwidth + 31) / 32) * 1024))) = bitmap->color[1] + 128;
				*(chroma + (x / 32) * 1024 + ((x % 32) / 2 * 2 + 1) + ((cy % 32) * 32) + ((cy / 32) * (((jwidth + 31) / 32) * 1024))) = bitmap->color[2] + 128;
			}
		}
	}
}

int set_slide(void *vshow, int n, bitmap *bitmap)
{
	slideshow *show = (slideshow *) vshow;
	image_layer newlayer;

	char filename[PATH_MAX];

	sprintf(filename, "%s/%d.jpg", show->dir, n);
	if (init_jpeg(&newlayer, filename) < 0) {
		return -1;
	}

	// do this before showing the layer!
	while (bitmap) {
		merge_bitmaps(&newlayer, bitmap);
		bitmap = bitmap->next;
	}

	show_jpeg(&newlayer);

	transition_layers(&show->layer, &newlayer);
	free_jpeg(&show->layer);

	show->slide = n;
	show->layer = newlayer;
	return 0;
}

int cur_slide(void *vshow)
{
	slideshow *show = (slideshow *) vshow;
	return show->slide;
}
