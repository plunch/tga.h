#ifndef TGA_TARGA_H
#define TGA_TARGA_H

#include <inttypes.h>

#define TGA_HEADER_LEN 18

/* No image data */
#define TGA_THA_IMGT_NONE 0
/* Uncompressed, color-mapped */
#define TGA_IMGT_CMAP 1
/* Uncompressed, RGB */
#define TGA_IMGT_RGB 2
/* Uncompressed black and white */
#define TGA_IMGT_BW 3
/* Uncompressed runlength encoded color-mapped */
#define TGA_IMGT_CMAP_RLE 9
/* Uncompressed runlength encoded RGB */
#define TGA_IMGT_RGB_RLE 10
/* Compressed black and white */
#define TGA_IMGT_BW_C 11
/* Compressed color-mapped, using Huffman, Delta and runlength encoding*/
#define TGA_IMGT_CMAP_C_HD_RLE 32
/* Compressed color-mapped, using Huffman, Delta and runlength encoding
 * 4-pass quadtree type process */
#define TGA_IMGT_CMAP_C_HD_RLE_4 33

typedef struct {
	/* F1 - Image ID length */
	uint8_t id_length;

	/* F2  - Color map type */
	uint8_t color_map_type;

	/* F3 - Image type */
	uint8_t image_type;
	/*
	0  -  No image data included.
	    1  -  Uncompressed, color-mapped images.
	    2  -  Uncompressed, RGB images.
	    3  -  Uncompressed, black and white images.
	    9  -  Runlength encoded color-mapped images.
	   10  -  Runlength encoded RGB images.
	   11  -  Compressed, black and white images.
	   32  -  Compressed color-mapped data, using Huffman, Delta, and
	                runlength encoding.
	   33  -  Compressed color-mapped data, using Huffman, Delta, and
	                runlength encoding.  4-pass quadtree-type process.
	 */

	/* F4 - Color map specification */
	uint16_t color_map_origin;
	uint16_t color_map_length;
	uint8_t color_map_entry_size;

	/* F5 - Image specification */
	uint16_t x_origin;
	uint16_t y_origin;
	uint16_t width;
	uint16_t height;
	uint8_t depth; /* Bit depth. 8, 15, 16, 24 or 32 */
	uint8_t image_descriptor;
	/* Bitfield
	  	3-0	Number of attribute bits associated with each pixel
	  		for targa16 - 0 or 1
	  		for targa24 - 0
	  		for targa32 - 8

	 	4	reserved. must be 0

	  	5	Screen origin. 0 = lower left, 1 upper left. Must be 0 for truevision

	  	7-6	Data storage interleaving
	  		00 = non-interleaved
	  		01 = two-way (even/odd) interleaving.
	  		10 = four way interleaving.
	  		11 = reserved.
	 */
} targa_header;

typedef struct {
	targa_header head;

	uint8_t* image_identification_field;
	uint8_t* color_map_data;
	uint8_t* image_data;
} targa_file;

targa_header* extract_targa_header ( void* file_start );
targa_file* load_targa_file ( const char* filename );
int write_targa_file ( targa_file*, int fd);

#endif /* TGA_TARGA_H */
