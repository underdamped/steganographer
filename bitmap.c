/* * * * * * * * * * * * * * * *
 * steganographer, bitmap.c
 *
 * bitmap-specific functions
 *
 * Javier Lombillo
 * October 2015
 */

#include "steganographer.h"
#include <math.h>  // for floor() in calculate_padding()

/*
 * load header data from a bitmap file
 */
void get_bitmap_info(struct container *c)
{
    unsigned char magic_bytes[2];

    fseek( c->fp, OFF_MAGIC_BYTES, SEEK_SET );
    fread( magic_bytes, sizeof(magic_bytes), 1, c->fp );

    if ( (magic_bytes[0] != 'B') || (magic_bytes[1] != 'M') )
    {
        fprintf( stderr, "unrecognized file, exiting.\n" );
        exit( EXIT_FAILURE );
    }

    fread( &c->filesize, sizeof(c->filesize), 1, c->fp );

    fseek( c->fp, OFF_PIXEL_START, SEEK_SET );
    fread( &c->b->data_offset, sizeof(c->b->data_offset), 1, c->fp );

    fseek( c->fp, OFF_BITMAP_WIDTH, SEEK_SET );
    fread( &c->b->width, sizeof(c->b->width), 1, c->fp );
    fread( &c->b->height, sizeof(c->b->height), 1, c->fp );

    fseek( c->fp, OFF_BITMAP_DEPTH, SEEK_SET );
    fread( &c->b->depth, sizeof(c->b->depth), 1, c->fp );

    return;
}

/*
 * load the pixel matrix from a bitmap file
 */
int get_bitmap(struct container *c)
{
    int i, j, r = 0;

    fseek( c->fp, c->b->start, SEEK_SET );

    for ( i = 0; i < c->b->height; i++ )
    {
        for ( j = 0; j < c->b->rowlen; j++ )
        {
            c->b->pixel[i][j] = fgetc( c->fp );
            r++;
        }
    }

    return r;
}

/*
 * write pixel matrix from memory to a file
 */
int write_bitmap(FILE *out, struct container *c)
{
    int i, j, w = 0;

    fseek( out, c->b->start, SEEK_SET );

    for ( i = 0; i < c->b->height; i++ )
        for ( j = 0; j < c->b->rowlen; j++ )
            w += fwrite( &c->b->pixel[i][j], 1, 1, out );

    return w;
}

/*
 * wrapper function to block-copy the basefile's bitmap header to "target"
 */
int write_bitmap_header(FILE *target, struct container *c)
{
    return block_copy( c->fp, target, c->b->data_offset );
}

/*
 * ensure we're using a 24-bit bitmap, and that the base file is at least 8
 * times larger than the payload
 */
void validate_bitmap(struct container *c, struct payload *p)
{
    int bitmap_size;

    if ( (c->b->depth != 24) )
    {
        fprintf( stderr,
                "[ERROR] unsupported color-depth detected; bitmap format must be 24-bit color.\n"
                 "%s: %d-bit\n", c->filename, c->b->depth );

        exit( EXIT_FAILURE );
    }

    bitmap_size = c->b->width * c->b->height;

    if ( (bitmap_size / p->size) < 8 )
    {
        fprintf( stderr,
                "[ERROR] ratio of pixels in %s to bytes in %s must be greater than 8.\n\n"
                "%s: %dx%d = %d pixels\n"
                "%s: %d bytes\n\n"
                "Ratio: %d / %d = %0.2f\n",
                c->filename, p->filename, c->filename,
                c->b->width, c->b->height, bitmap_size, p->filename, p->size,
                bitmap_size, p->size, (float)bitmap_size / p->size );

        exit( EXIT_FAILURE );
    }
}


/*
 * the bitmap spec says the length (in bytes) of a row of pixels must be a
 * multiple of 4.  the "natural" row length is the product of the bitmap width
 * and the size of a pixel; this function calculates the number of pad bytes
 * necessary to achieve an appropriate row length
 *
 * (nearest multiple of 4 algo from https://en.wikipedia.org/wiki/BMP_file_format)
 */
int calculate_padding(int width, int colordepth)
{
    int natural_length_of_row = width * colordepth / 8;
    int nearest_multiple_of_four = 4 * floor( (colordepth * width + 31) / 32 );

    return (nearest_multiple_of_four - natural_length_of_row);
}

/*
 * pretty-print some info for the user
 */
void show_bitmap_info(struct container *c, struct payload *p)
{
    printf( "--[base file]------------------\n"
            "file name  : %s\n"
            "file size  : %d bytes\n"
            "data offset: %d bytes\n"
            "BMP width  : %d pixels\n"
            "BMP height : %d pixels\n"
            "color depth: %d bits\n",
            c->filename, c->filesize, c->b->data_offset,
            c->b->width, c->b->height, c->b->depth );

    printf( "\n%d byte%s of padding required per row.\n", c->b->pad, (c->b->pad == 1) ? "" : "s" );
    printf( "row length (+ padding): %d bytes\n\n", c->b->rowlen );

    printf( "--[hide file]------------------\n"
            "file name: %s\n"
            "file size: %d bytes (IMPORTANT: this number is required to recover the file)\n", p->filename, p->size );

    puts( "-------------------------------\n" );

    return;
}

