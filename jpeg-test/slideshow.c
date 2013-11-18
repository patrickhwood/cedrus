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

int set_slide(void *vshow, int n)
{
	slideshow *show = (slideshow *) vshow;
	image_layer newlayer;

	char filename[PATH_MAX];

	sprintf(filename, "%s/%d.jpg", show->dir, n);
	if (init_jpeg(&newlayer, filename) < 0) {
		return -1;
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
