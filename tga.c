/*
Copyright (c) 2014 Per Lundh

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#define _DEFAULT_SOURCE
#include "tga.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define lendian2host16(x, o) (x[o]<<0)|(x[o + 1]<<8)

uint8_t depth2bytes ( uint8_t d )
{
	uint8_t bpp = 4;
	switch ( d ) {
	case 8:
		bpp = 1;
		break;
	case 16:
		bpp = 2;
		break;
	case 24:
		bpp = 3;
		break;
	case 32:
		bpp = 4;
		break;
	}
	return bpp;
}

targa_header* extract_targa_header ( void* file_buf )
{
	char* file_start = ( char* ) file_buf;
	targa_header* h = ( targa_header* ) calloc ( 1, sizeof ( targa_header ) );

	h->id_length 		= file_start[0];

	h->color_map_type 	= file_start[1];

	h->image_type 		= file_start[2];

	h->color_map_origin 	= lendian2host16 ( file_start, 3 );
	h->color_map_length 	= lendian2host16 ( file_start, 5 );
	h->color_map_entry_size = file_start[7];

	h->x_origin 		= lendian2host16 ( file_start, 8 );
	h->y_origin 		= lendian2host16 ( file_start, 10 );
	h->width 		= lendian2host16 ( file_start, 12 );
	h->height 		= lendian2host16 ( file_start, 14 );
	h->depth 		= file_start[16];
	h->image_descriptor 	= file_start[17];

	return h;
}

uint8_t* serialize_targa_header ( const targa_header* h )
{
	uint8_t* ret = ( uint8_t* ) malloc ( TGA_HEADER_LEN );
	ret[0]  =  h->id_length;
	ret[1]  =  h->color_map_type;
	ret[2]  =  h->image_type;

	ret[3]  = ( h->color_map_origin & 0x00FF );
	ret[4]  = ( h->color_map_origin & 0xFF00 ) / 256;
	ret[5]  = ( h->color_map_length & 0x00FF );
	ret[6]  = ( h->color_map_length & 0xFF00 ) / 256;
	ret[7]  =  h->color_map_entry_size;

	ret[8]  = ( h->x_origin & 0x00FF );
	ret[9]  = ( h->x_origin & 0xFF00 ) / 256;
	ret[10] = ( h->y_origin & 0x00FF );
	ret[11] = ( h->y_origin & 0xFF00 ) / 256;
	ret[12] = ( h->width & 0x00FF );
	ret[13] = ( h->width & 0xFF00 ) / 256;
	ret[14] = ( h->height & 0x00FF );
	ret[15] = ( h->height & 0xFF00 ) / 256;
	ret[16] =  h->depth;
	ret[17] =  h->image_descriptor;
	return ret;
}

targa_file* tga_readfile ( int f_desc )
{
	/* Read the file header into the struct */
	char* file_header = ( char* ) malloc ( TGA_HEADER_LEN );

	ssize_t header_read_result;
	do {
		header_read_result = read ( f_desc, file_header, TGA_HEADER_LEN );
		if(header_read_result == -1){
			int e = errno;
			switch(e){
				case EAGAIN:
					continue;
				default:
					free(file_header);
					errno = e;
					return NULL;
			}
		}
	} while(header_read_result == -1);

	/* Assert that we have at least a header to read */
	if(header_read_result != TGA_HEADER_LEN){
		errno = EINVAL;
		return NULL;
	}

	targa_header* ht = extract_targa_header ( file_header );
	targa_header h = *ht;
	free ( ht );
	free ( file_header );

	/* Get the file size reported by the heder */
	size_t alleged_length = TGA_HEADER_LEN 
		+ h.id_length
		+ h.color_map_length * depth2bytes(h.color_map_entry_size)
		+ h.width * h.height * depth2bytes(h.depth);

	char* file_data = ( char* ) malloc ( alleged_length - TGA_HEADER_LEN );
	
	ssize_t read_result = 0;
	do {
		read_result = read ( f_desc, file_data, alleged_length - TGA_HEADER_LEN );
		if(read_result == -1){
			int e = errno;
			switch(e){
				case EAGAIN:
					continue;
				default:
					free(file_data);
					errno = e;
					return NULL;
			}
		}
	} while(read_result == -1);

	/* We did not get the whole file, something is wrong. */
	/* Exit with error instead of risking a memory error */
	if(read_result != alleged_length - TGA_HEADER_LEN){
		free(file_data);
		errno = EADDRNOTAVAIL;
		return NULL;
	}

	/* Get pointers to the different parts of the file */
	uint8_t* color_map_pointer;
	uint8_t* id_buffer_pointer;
	uint8_t* image_buffer_pointer;

	if ( h.color_map_type != 0 ) {
		color_map_pointer = ( uint8_t* ) file_data + h.id_length;
	} else {
		color_map_pointer = NULL;
	}

	if ( h.id_length == 0 ) {
		id_buffer_pointer = NULL;
	} else {
		id_buffer_pointer = ( uint8_t* ) file_data;
	}

	if ( h.image_type != 0 ) {
		image_buffer_pointer = ( uint8_t* ) file_data + h.id_length + h.color_map_length;
	} else {
		image_buffer_pointer = NULL;
	}

	targa_file* ret = ( targa_file* ) calloc ( 1, sizeof ( targa_file ) );

	ret->head = h;
	ret->image_identification_field = id_buffer_pointer;
	ret->color_map_data = color_map_pointer;
	ret->image_data = image_buffer_pointer;

	return ret;
}

int tga_writefile ( targa_file* f, int fd )
{
	uint32_t idlen = f->head.id_length;
	uint32_t cmlen = f->head.color_map_length * depth2bytes (
	                     f->head.color_map_entry_size );
	uint32_t img_len = f->head.width * f->head.height * depth2bytes ( f->head.depth );
	uint8_t* header = serialize_targa_header ( &f->head );

	/* Write header */
	if ( write ( fd, header, TGA_HEADER_LEN ) == -1 ) {
		int res = errno;
		free ( header );
		errno = res;
		return -1;
	}
	free ( header );
	/* Write image identification field */
	if ( write ( fd, f->image_identification_field, idlen ) == -1 ) {
		return -1;
	}
	/* Write color map data */
	if ( cmlen > 0 ) {
		if ( write ( fd, f->color_map_data, cmlen ) == -1 ) {
			return -1;
		}
	}
	/* Write image data */
	if ( img_len > 0 ) {
		if ( write ( fd, f->image_data, img_len ) == -1 ) {
			return -1;
		}
	}

	return 0;
}

size_t tga_undo_rle( targa_file* from, uint8_t** output_buffer )
{
	size_t output_len = 0;

	targa_header* fh = &(from->head);
	size_t b = 0;
	uint8_t header = 0;
	size_t img_data_len = fh->width * fh->height * depth2bytes(fh->depth);

	while(b < img_data_len){
		header = from->image_data[b++];
		size_t len = ((header &!UINT8_MAX) + 1) * depth2bytes(fh->depth);
		output_len += len;
		*output_buffer = (uint8_t*)realloc(*output_buffer,
				sizeof(uint8_t) * output_len);

		/* Check if the high-order bit is 1 or 0 */
		if(header & UINT8_MAX > 0){
			uint8_t val = from->image_data[b++];
			while (len > 0) {
				*output_buffer[output_len - (len--)] = val;
			}
		} else {
			while(len > 0) {
				*output_buffer[output_len - (len--)] = from->image_data[b++];
			}
		}
	}

	return output_len;
}
