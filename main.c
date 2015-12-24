/* * * * * * * * * * * * * * * *
 * steganographer, main.c
 *
 * steganographer is a tool that hides data in other data, or
 * recovers data previously hidden
 *
 * Javier Lombillo
 * October 2015
 */

#include "steganographer.h"

// function pointers for the callbacks
void (*get_info)(struct container *);
int  (*get_data)(struct container *);
void (*show_info)(struct container *, struct payload *);
int  (*write_data)(FILE *, struct container *);
int  (*write_header)(FILE *, struct container *);
void (*init_data_storage)(struct container *);
void (*validate_data)(struct container *, struct payload *);

// points to the appropriate cover() or uncover() function
int (*mode_action)(struct container *, struct payload *);

int main(int argc, char **argv)
{
    int  result;            // for various function return values
    FILE *outfile;          // where we write what we've hidden or recovered

    struct payload pload;   // the thing we want to hide
    struct container data;  // the "camouflage"
    struct user_input user; // command-line args

    // handle command-line arguments, store in the 'user' struct
    parse_args( argc, argv, &user );

    // figure out what kind of file we're using as camouflage
    data.type = find_type( user.basefile );

    // now that we have a data type (e.g., bitmap or PCM wav), we can
    // instantiate the appropriate data "object" by allocating memory
    // for it and associating the appropriate callbacks
    switch (data.type)
    {
        case bitmap:

            data.b = malloc( sizeof(*data.b) );

            get_data          = &get_bitmap;
            get_info          = &get_bitmap_info;
            show_info         = &show_bitmap_info;
            write_data        = &write_bitmap;
            write_header      = &write_bitmap_header;
            validate_data     = &validate_bitmap;
            init_data_storage = &init_pixel_matrix;

            mode_action = (mode == hide ) ? &bitmap_cover
                                          : &bitmap_uncover;
            break;

        case wavfile:

            data.w = malloc( sizeof(*data.w) );

            get_data          = &get_samples;
            get_info          = &get_pcm_info;
            show_info         = &show_pcm_info;
            write_data        = &write_samples;
            write_header      = &write_pcm_header;
            validate_data     = &validate_wavfile;
            init_data_storage = &init_sample_storage;

            mode_action = (mode == hide ) ? &pcm_cover
                                          : &pcm_uncover;

            break;

        default:

            fprintf( stderr, "[ERROR] unknown data type, aborting.\n" );
            exit( EXIT_FAILURE );
    }

    show_status( &user );

    // open the camouflage file and load its header
    data.fp = open_file( user.basefile, &data.filename );
    get_info( &data );

    // this next block represents payload management
    //
    // in hide mode, the user supplies a payload filename, so we'll
    // associate the user's selection with the payload "object"
    //
    // in recover mode, the user supplies a payload size
    if ( mode == hide )
    {
        pload.fp = open_file( user.hidefile, &(pload.filename) );

        // get payload size
        fseek( pload.fp, 0, SEEK_END );
        pload.size = ftell( pload.fp );
        rewind( pload.fp );

        // make sure everything is copacetic
        validate_data( &data, &pload );
    }
    else // just need the size in recover mode
    {
        pload.size = user.payload_size;
    }

    // pre-production
    init_data_storage( &data );
    init_payload_storage( &pload );

    // grab the data bytes
    result = get_data( &data );

    if ( mode == hide )
    {
        show_info( &data, &pload );
        printf( "%s: read %d bytes of data.\n", data.filename, result );

        result = get_payload( &pload );
        printf ( "%s: read %d bytes.\n\n", pload.filename, result );

        fclose( pload.fp ); // we're done with the payload file
    }

    outfile = fopen( user.outputfile, "wb" );

    if ( !outfile )
    {
        fprintf( stderr, "Error opening %s for writing: %s\n", user.outputfile, strerror(errno) );
        exit( EXIT_FAILURE );
    }

    // hide or recover data, as appropriate
    mode_action( &data, &pload );

    // we're finished; write to the output file and let the user know what happened
    if ( mode == hide )
    {
        result = write_header( outfile, &data );
        result += write_data( outfile, &data );

        printf( "[COMPLETE] wrote %d bytes to %s.\n", result, user.outputfile );
    }
    else
    {
        result = write_payload( outfile, &pload );
        printf( "[COMPLETE] recovered %d bytes to %s\n", result, user.outputfile );
    }

    fclose( data.fp );
    fclose( outfile );

    clean_up( &data, &pload );

    return 0;
}
