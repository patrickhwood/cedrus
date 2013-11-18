void *init_slideshow(const char *dir);
int free_slideshow(void *vshow);
int end_slideshow(void *vshow);
int set_slide(void *vshow, int n);
int cur_slide(void *vshow);
