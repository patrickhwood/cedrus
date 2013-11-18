#ifndef __show_h
#define __show_h
#include "decode.h"
int init_jpeg(image_layer *layer, const char *filename);
void free_jpeg(image_layer *layer);
void show_jpeg(image_layer *layer);
void transition_layers(image_layer *layer1, image_layer *layer2);
#endif // __show_h
