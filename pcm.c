/* * * * * * * * * * * * * * * *
 * steganographer, pcm.c
 *
 * wavfile-specific functions
 *
 * Javier Lombillo
 * October 2015
 */

#include "steganographer.h"

/*
 * load header data from a WAV file
 */
void get_pcm_info(struct container *c)
{
    fread( &c->w->chunkID, 4, 1, c->fp );
    fread( &c->w->chunkSize, 4, 1, c->fp );

    // chunkSize is the size of the file minus the 8-byte RIFF header
    c->filesize = c->w->chunkSize + 8;

    // find "WAVE" string
    pcm_find_string( "WAVE", c->fp );

    // find "fmt" string
    pcm_find_string( "fmt ", c->fp );

    // skip past SubChunk1Size (4 bytes)
    fseek( c->fp, 4, SEEK_CUR );

    fread( &c->w->audioformat, 2, 1, c->fp );
    fread( &c->w->channels, 2, 1, c->fp );
    fread( &c->w->rate, 4, 1, c->fp );
    fread( &c->w->bytes_per_second, 4, 1, c->fp );
    fread( &c->w->block_align, 2, 1, c->fp );
    fread( &c->w->depth, 2, 1, c->fp );

    if ( c->w->audioformat != 1 )
    {
        fprintf( stderr, "[ERROR] not a PCM file, aborting." );
        exit( EXIT_FAILURE );
    }

    c->w->sample_size = c->w->depth / 8;    // size in bytes of one sample

    pcm_find_string( "data", c->fp );
    fread( &c->w->subchunk2size, 4, 1, c->fp );

    c->w->data_offset = c->filesize - c->w->subchunk2size;

    // nFrames = subchunk2size / block_align
    // nSamples = nFrames * nChannels
    c->w->total_samples = (c->w->subchunk2size / c->w->block_align) * c->w->channels;

}

/*
 * load the sample data
 */
int get_samples(struct container *c)
{
    int i;

    fseek( c->fp, c->w->data_offset, SEEK_SET );

    for ( i = 0; i < c->w->subchunk2size; i++ )
        c->w->samples[i] = fgetc( c->fp );

    return i;
}

/*
 * write in-memory sample data to a file
 */
int write_samples(FILE *out, struct container *c)
{
    int i;

    fseek( out, c->w->data_offset, SEEK_SET );

    for ( i = 0; i < c->w->subchunk2size; i++ )
        fwrite( &c->w->samples[i], 1, 1, out );

    return i;
}

/*
 * wrapper function to block-copy the basefile's WAV header to "target"
 */
int write_pcm_header(FILE *target, struct container *c)
{
    return block_copy( c->fp, target, c->w->data_offset );
}

/*
 * we require a minimum 16-bit WAV file, and that it is at least 8 times larger
 * than the payload
 */
void validate_wavfile(struct container *c, struct payload *p)
{
    if ( (c->w->depth < 16) )
    {
        fprintf( stderr,
                "[ERROR] Unsupported wordlength detected; 8-bit samples not supported at this time.\n"
                 "%s: %d-bit\n", c->filename, c->w->depth );

        exit( EXIT_FAILURE );
    }

    // ensure we have enough sample data for LSB stego
    if ( (c->w->total_samples / p->size) < 8 )
    {
        fprintf( stderr,
                "[ERROR] Ratio of samples in %s to bytes in %s must be greater than 8.\n\n"
                "%s: %ld samples\n"
                "%s: %d bytes\n\n"
                "Ratio: %ld / %d = %0.2f\n",
                c->filename, p->filename, c->filename,
                c->w->total_samples, p->filename, p->size,
                c->w->total_samples, p->size, (float)c->w->total_samples / p->size );

        exit( EXIT_FAILURE );
    }
}

/*
 * the WAV format is a sub-format of RIFF (resource interchange file format),
 * which organizes data in tagged "chunks". the spec is pretty loose in terms
 * of chunk organization, so this function finds tags, leaving the file
 * position indicator in a known spot for the calling function
 */
int pcm_find_string(char *id, FILE *fp)
{
    char buf[4];
    int i = 0, n;

    while ( 1 )
    {
        n = fread( &buf, 4, 1, fp );

        if ( strncmp(buf, id, 4) == 0 )
            return n;
        else if ( n == 0 )
        {
            printf( "ERROR: could not find ID (%s)\n", id );
            exit(1);
        }
        else
            fseek( fp, i++, SEEK_SET );
    }
    return n;
}

/*
 * pretty-print some info for the user
 */
void show_pcm_info(struct container *c, struct payload *p)
{
    printf( "--[base file]------------------\n"
            "file name  : %s\n"
            "file size  : %d bytes\n"
            "data offset: %d bytes\n"
            "wordlength : %d bits\n"
            "sample rate: %d Hz\n"
            "total      : %ld samples\n\n",
            c->filename, c->filesize, c->w->data_offset,
            c->w->depth, c->w->rate, c->w->total_samples );

    printf( "--[hide file]------------------\n"
            "file name: %s\n"
            "file size: %d bytes (IMPORTANT: this number is required to recover the file)\n", p->filename, p->size );

    puts( "-------------------------------\n" );
}
