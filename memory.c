/* * * * * * * * * * * * * * * *
 * steganographer, memory.c
 *
 * functions that manage dynamic memory
 *
 * Javier Lombillo
 * October 2015
 */

#include "steganographer.h"

/* to handle arbitrary bitmap sizes, the pixel matrix is created dynamically
 * at run-time. because the number of bytes in bitmap rows are required to be
 * a multiple of 4, the byte -- and not the pixel -- is the primitive unit.
 *
 * the data structure itself is an array of pointers, each of which
 * points to an array of unsigned chars. thus pixel[i][j] points to the
 * jth byte in the ith row.
 */
void init_pixel_matrix(struct container *c)
{
    int i;

    // derive a few essential values; see 'bitmap' declaration in steganographer.h
    c->b->pad    = calculate_padding( c->b->width, c->b->depth );
    c->b->size   = c->b->depth / 8;
    c->b->start  = c->b->data_offset;
    c->b->rowlen = c->b->size * c->b->width + c->b->pad;

    // allocate storage for each row of pointers
    c->b->pixel = malloc( c->b->height * sizeof(unsigned char *) );

    if ( c->b->pixel == NULL )
    {
        fprintf( stderr, "[ERROR] init_pixel_matrix: memory allocation failed, aborting.\n" );
        exit( EXIT_FAILURE );
    }

    // allocate storage for the elements in each row
    // (this treats each pad byte as pixel-sized, which may
    // over-allocate some memory)
    for ( i = 0; i < c->b->height; i++ )
    {
        c->b->pixel[i] = malloc( c->b->rowlen * sizeof(unsigned char *) );

        if ( c->b->pixel[i] == NULL )
        {
            fprintf( stderr, "[ERROR] init_pixel_matrix: memory allocation failed, aborting.\n" );
            exit( EXIT_FAILURE );
        }
    }

    return;
}

/*
 * the size (in bytes) of the sample stream is contained in the
 * 'subchunk2size' header field
 */
void init_sample_storage(struct container *c)
{
    c->w->samples = malloc( c->w->subchunk2size * sizeof(unsigned char *) );

    if ( c->w->samples == NULL )
    {
        fprintf( stderr, "[ERROR] init_sample_storage: memory allocation failed, aborting.\n" );
        exit( EXIT_FAILURE );
    }

    return;
}

void init_payload_storage(struct payload *p)
{
    p->bytes = malloc( p->size );

    if ( p->bytes == NULL )
    {
        fprintf( stderr, "[ERROR] init_payload_storage: memory allocation failed, aborting.\n" );
        exit( EXIT_FAILURE );
    }

    return;
}

/*
 * if you love something, set it free . . .
 */
void clean_up(struct container *c, struct payload *p)
{
    if ( c->type == bitmap )
    {
        int i;

        for ( i = 0; i < c->b->height; i++ )
            free( c->b->pixel[i] );

        free( c->b->pixel );
        free( c->b );
    }
    else if ( c->type == wavfile )
    {
        free( c->w->samples );
        free( c->w );
    }

    free( p->bytes );
}
