typedef struct {
	unsigned char *bitmap;
	int width, height;
	int xoff, yoff;
	void *next;
} bitmap;

void *init_slideshow(const char *dir);
int free_slideshow(void *vshow);
int end_slideshow(void *vshow);
int set_slide(void *vshow, int n, bitmap *bitmap);
int cur_slide(void *vshow);
int merge_bitmap(void *vshow, const unsigned char *bitmap, int width, int height, int xoff, int yoff);
