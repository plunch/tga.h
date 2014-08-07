#define _BSD_SOURCE
#include "tga.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define tga_ext16(x, o) (x[o]<<0)|(x[o + 1]<<8)

targa_header* extract_targa_header ( void* file_buf )
{
	char* file_start = ( char* ) file_buf;
	targa_header* h = ( targa_header* ) calloc ( 1, sizeof ( targa_header ) );

	h->id_length 		= file_start[0];

	h->color_map_type 	= file_start[1];

	h->image_type 		= file_start[2];

	h->color_map_origin 	= tga_ext16 ( file_start, 3 );
	h->color_map_length 	= tga_ext16 ( file_start, 5 );
	h->color_map_entry_size = file_start[7];

	h->x_origin 		= tga_ext16 ( file_start, 8 );
	h->y_origin 		= tga_ext16 ( file_start, 10 );
	h->width 		= tga_ext16 ( file_start, 12 );
	h->height 		= tga_ext16 ( file_start, 14 );
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

targa_file* load_targa_file ( const char* filename )
{
	struct stat st;
	stat ( filename, &st );
	size_t filesize = st.st_size;

	if ( filesize < TGA_HEADER_LEN ) {
		return NULL;
	}

	int f_desc = open ( filename, O_RDONLY );

	char* file_header = ( char* ) malloc ( TGA_HEADER_LEN );
	read ( f_desc, file_header, TGA_HEADER_LEN );
	targa_header* ht = extract_targa_header ( file_header );
	targa_header h = *ht;
	free ( ht );
	free ( file_header );

	char* file_data = ( char* ) malloc ( filesize - TGA_HEADER_LEN );
	read ( f_desc, file_data, filesize - TGA_HEADER_LEN );

	close ( f_desc );

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

int write_targa_file ( targa_file* f, int fd )
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
		return res;
	}
	free ( header );
	/* Write image identification field */
	if ( write ( fd, f->image_identification_field, idlen ) == -1 ) {
		return errno;
	}
	/* Write color map data */
	if ( cmlen > 0 ) {
		if ( write ( fd, f->color_map_data, cmlen ) == -1 ) {
			return errno;
		}
	}
	/* Write image data */
	if ( img_len > 0 ) {
		if ( write ( fd, f->image_data, img_len ) == -1 ) {
			return errno;
		}
	}

	return 0;
}
