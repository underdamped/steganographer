/* * * * * * * * * * * * * * * *
 * steganographer, helpers.c
 *
 * auxiliary functions
 *
 * Javier Lombillo
 * October 2015
 */

#include "steganographer.h"
#include <unistd.h>  // for getopt()

/*
 * this tries to detect file type by looking for "magic bytes" at the beginning
 * of the file
 */
int find_type(const char *name)
{
    unsigned char b[12]; // 12 is the minimum number of bytes required to find the WAVE tag
    int type = -1;
    FILE *f = fopen( name, "rb" ); // 'b' in case we're compiled on windoze

    if ( !f )
    {
        fprintf( stderr, "[ERROR] could not open %s: %s\nAborting.\n", name, strerror(errno) );
        exit( EXIT_FAILURE );
    }

    printf( "reading %s.... ", name );

    fread( b, sizeof(b), 1, f );
    fclose( f );

    // bitmap: look for "BM" magic bytes
    // wavfile: look for "RIFF" and "WAVE" magic bytes
    if ( (b[0] == 'B') && (b[1] == 'M') )
    {
        type = bitmap;
        printf( "detected bitmap." );
    }
    else if ( (b[0] == 'R') && (b[1] == 'I') && (b[2] == 'F') && (b[3] == 'F')
           && (b[8] == 'W') && (b[9] == 'A') && (b[10] == 'V') && (b[11] == 'E') )
    {
        type = wavfile;
        printf( "detected PCM WAV file." );
    }

    puts("");

    return type;
}

/*
 * parse command-line arguments
 */
void parse_args(int argc, char **argv, struct user_input *u)
{
    int opt;

    short mode_set = 0;
    short payload_set = 0;
    short basefile_set = 0;
    short outputfile_set = 0;
    short size_set = 0;

	if ( argc == 1 )
	{
        show_usage();
		exit( EXIT_FAILURE );
	}

	while ( (opt = getopt(argc, argv, "hHRp:b:o:s:")) != -1 )
	{
		switch (opt)
		{
		    case 'H':
			    mode = hide;
                mode_set = 1;
			    break;
		    case 'R':
			    mode = recover;
                mode_set = 1;
			    break;
		    case 'p':
                if ( strlen(optarg) > MAX_FILENAME_LENGTH )
                {
                    printf( "[ERROR] filename must be less than %d characters, aborting.\n", MAX_FILENAME_LENGTH );
                    exit( EXIT_FAILURE );
                }
                else
                {
                    strncpy( u->hidefile, optarg, MAX_FILENAME_LENGTH );
                    payload_set = 1;
                }
			    break;
		    case 'b':
                if ( strlen(optarg) > MAX_FILENAME_LENGTH )
                {
                    printf( "[ERROR] filename must be less than %d characters, aborting.\n", MAX_FILENAME_LENGTH );
                    exit( EXIT_FAILURE );
                }
                else
                {
                    strncpy( u->basefile, optarg, MAX_FILENAME_LENGTH );
                    basefile_set = 1;
                }
			    break;
		    case 'o':
                if ( strlen(optarg) > MAX_FILENAME_LENGTH )
                {
                    printf( "[ERROR] filename must be less than %d characters, aborting.\n", MAX_FILENAME_LENGTH );
                    exit( EXIT_FAILURE );
                }
                else
                {
                    strncpy( u->outputfile, optarg, MAX_FILENAME_LENGTH );
                    outputfile_set = 1;
                }
			    break;
		    case 's':
			    u->payload_size = atoi( optarg );
                size_set = 1;
			    break;
            case 'h':
		    case '?':
		    default:
                show_usage();
			    exit( EXIT_FAILURE );
		}
	}

    // make sure we have everything we need from the user
    if ( !mode_set )
    {
        fprintf( stderr, "[ERROR] missing mode flag (-H or -R). Use -h for help.\n" );
        exit( EXIT_FAILURE );
    }

    if ( (mode == hide) && (!basefile_set || !outputfile_set || !payload_set) )
    {
        fprintf( stderr, "[ERROR] missing arguments: hide mode requires -b, -p, and "
                         "-o parameters.\nUse -h for help.\n" );
        exit( EXIT_FAILURE );
    }
    else if ( (mode == recover) && (!basefile_set || !outputfile_set || !size_set) )
    {
        fprintf( stderr, "[ERROR] missing arguments: recover mode requires -b, -s, "
                         "and -o parameters.\nUse -h for help.\n" );
        exit( EXIT_FAILURE );
    }
}

void show_status(struct user_input *u)
{
    if ( mode == hide )
    {
        printf( "attempting to hide %s in %s; output will be saved as %s\n\n", u->hidefile, u->basefile, u->outputfile );
    }
    else if ( mode == recover )
    {
        printf( "attempting to recover %d bytes from %s into %s...\n\n", u->payload_size, u->basefile, u->outputfile );
    }
    else
    {
        printf( "math has stopped working, seek immediate shelter!\n" );
        exit( EXIT_FAILURE );
    }

    return;
}

void show_usage(void)
{
    fprintf( stderr,
            "steganographer v%0.1f -- a tool that hides data using LSB steganography\n"
            "Copyleft October 2015, Javier Lombillo, Miami-Dade College School of Engineering & Technology\n\n"
            "steganographer has two modes of operation, one for hiding data and another for recovering\n"
            "previously hidden data.  HIDE mode is enabled with the -H flag; RECOVER mode with the -R flag.\n\n"
            "The following arguments are required in HIDE mode:\n"
            "\t-b <base filename>\t\tthe camouflage data, so to speak\n"
            "\t-p <payload filename>\t\tthe data you want to hide\n"
            "\t-o <output filename>\t\twhere you want to store this stuff\n\n"
            "The following arguments are required in RECOVER mode:\n"
            "\t-b <base filename>\t\tthe file that contains the hidden data\n"
            "\t-s <size of payload>\t\tthe size in bytes of the hidden data\n"
            "\t-o <output filename>\t\twhere to write the hidden data\n\n"
            "Example:\n\n"
            "To hide main.c in the pixels of america.bmp, saving output as america2.bmp, run\n"
            "\tsteganographer -H -b america.bmp -p main.c -o america2.bmp\n\n"
            "NB: The camouflage data must be at least 8 times as large as the payload.\n"
            "Currently supported camouflage: 24-bit bitmaps and WAV files.\n", VERSION );
}
