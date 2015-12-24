/* * * * * * * * * * * * * * * *
 * steganographer, file_io.c
 *
 * general file input/output
 *
 * Javier Lombillo
 * October 2015
 */

#include "steganographer.h"

/*
 * get read-only file pointer and store file name
 */
FILE *open_file(const char *name, char (*ptr)[256])
{
    FILE *f = fopen( name, "rb" ); // 'b' in case we're compiled on windoze

    if ( !f )
    {
        fprintf( stderr, "[ERROR] could not open %s: %s\nAborting.\n", name, strerror(errno) );
        exit( EXIT_FAILURE );
    }

    strncpy( *ptr, name, MAX_FILENAME_LENGTH );

    return f;
}

/*
 * load entire payload file
 */
int get_payload(struct payload *p)
{
    rewind( p->fp );

    return fread( p->bytes, 1, p->size, p->fp );
}


/*
 * block-copy 'size' bytes from beginning of one file to another
 */
int block_copy(FILE *in, FILE *out, int size)
{
    int w;
    unsigned char *buf;

    buf = malloc( size );

    rewind( in );
    rewind( out );

    fread( buf, 1, size, in );
    w = fwrite( buf, 1, size, out );

    free( buf );

    return w;
}


/*
 * write entire payload from memory to a file
 */
int write_payload(FILE *out, struct payload *p)
{
    int i, w = 0;

    for ( i = 0; i < p->size; i++ )
        w += fwrite( &p->bytes[i], 1, 1, out );

    return w;
}
