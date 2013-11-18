#ifndef __decode_h
#define __decode_h
#include "jpeg.h"

typedef struct {
	int layer;
	int color;
	uint8_t *luma_output;
	uint8_t *chroma_output;
	jpeg_t jpeg;
} image_layer;

void decode_jpeg(image_layer *layer);
#endif // __decode_h
