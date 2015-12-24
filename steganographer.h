/* * * * * * * * * * * * * * * * * *
 * steganographer, steganographer.h
 *
 * Javier Lombillo
 * October 2015
 */

#ifndef STEGO_HEADER
#define STEGO_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define VERSION 0.8

#define MAX_FILENAME_LENGTH 255

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * BMP specs from https://en.wikipedia.org/wiki/BMP_file_format  *
 *                                                               *
 * Offset (hex)    Size (bytes)    Description                   *
 * ------------    ------------    ----------------------------- *
 * 00              2               magic bytes ("BM")            *
 * 02              4               size of file (bytes)          *
 * 06              4               reserved                      *
 * 0A              4               pixel array offset (bytes)    *
 * 0E              4               size of BMP header (bytes)    *
 * 12              4               bitmap width (pixels)         *
 * 16              4               bitmap height (pixels)        *
 * 1C              2               color depth (bits)            *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// useful byte offsets for file navigation in bitmaps
#define OFF_MAGIC_BYTES     0x00
#define OFF_FILE_SIZE       0x02
#define OFF_RESERVED        0x06
#define OFF_PIXEL_START     0x0A
#define OFF_BITMAP_WIDTH    0x12
#define OFF_BITMAP_HEIGHT   0x16
#define OFF_BITMAP_DEPTH    0x1C

// operational state
enum MODE { hide, recover } mode;

// command-line args get stored here
struct user_input
{
    int  payload_size;
    char basefile[MAX_FILENAME_LENGTH + 1];
    char hidefile[MAX_FILENAME_LENGTH + 1];
    char outputfile[MAX_FILENAME_LENGTH + 1];
};

// everything we need to know about the payload
struct payload
{
    char filename[MAX_FILENAME_LENGTH + 1];
    FILE *fp;
    int  size;                // size of the file in bytes
    unsigned char *bytes;     // payload data
};

// everything we need to know about a bitmap
struct bitmap
{
    // header data
    int32_t data_offset;      // byte location of the pixel matrix
    int32_t width;            // bitmap width in pixels
    int32_t height;           // bitmap height in pixels
    int16_t depth;            // color depth in bits

    // derived data
    int32_t start;            // byte location of first pixel
    int32_t size;             // size of each pixel in bytes
    int32_t pad;              // number of pad bytes required per row
    int32_t rowlen;           // length of a row (with padding) in bytes

    // the pixel byte matrix
    unsigned char **pixel;
};

// everything we need to know about a WAV file
struct pcm
{
    // RIFF chunk
    int32_t chunkID;          // "RIFF"
    int32_t chunkSize;        // (wav header + sample data) - 8
    int32_t format;           // "WAVE"

    // WAVE, fmt subchunk
    int32_t subchunk1ID;      // "fmt "
    int32_t subchunk1size;    // size of this chunk (should be 16 for PCM)
    int16_t audioformat;      // 1 for PCM format
    int16_t channels;         // mono = 1, stereo = 2
    int32_t rate;             // sample rate in Hz
    int32_t bytes_per_second; // rate * channels * (depth / 8)
    int16_t block_align;      // frame size in bytes; channels * (depth / 8)
    int16_t depth;            // word length in bits

    // WAVE, data subchunk
    int32_t subchunk2ID;      // "data"
    int32_t subchunk2size;    // size of sample data in bytes

    unsigned char *samples;  // the sample stream

    // derived data
    int32_t data_offset;      // byte location of the sample data
    int16_t sample_size;      // size in bytes of one sample
    int64_t total_samples;    // total number of samples in file
};

// generic data container, an abstraction for the various data "classes"
struct container
{
    FILE *fp;
    char filename[MAX_FILENAME_LENGTH + 1];
    int32_t filesize;

    enum { bitmap, wavfile } type;

    struct bitmap *b;
    struct pcm *w;
};

// stego.c -- the hide and recover routines
int bitmap_cover(struct container *, struct payload *);
int bitmap_uncover(struct container *, struct payload *);
int pcm_cover(struct container *, struct payload *);
int pcm_uncover(struct container *, struct payload *);

// memory.c -- heap managament
void init_pixel_matrix(struct container *);
void init_sample_storage(struct container *);
void init_payload_storage(struct payload *);
void clean_up(struct container *, struct payload *);

// file_io.c -- reading and writing bytes
int  get_payload(struct payload *);
int  write_payload(FILE *, struct payload *);
int  block_copy(FILE *, FILE *, int);
FILE *open_file(const char *, char (*)[MAX_FILENAME_LENGTH + 1]);

// helpers.c -- aux routines
int  find_type(const char *);
void parse_args(int, char **, struct user_input *);
void show_status(struct user_input *);
void show_usage(void);

// bitmap.c -- bitmap-specific functions
int  get_bitmap(struct container *);
void get_bitmap_info(struct container *);
int  write_bitmap(FILE *, struct container *);
int  write_bitmap_header(FILE *, struct container *);
int  calculate_padding(int, int);
void validate_bitmap(struct container *, struct payload *);
void show_bitmap_info(struct container *, struct payload *);

// pcm.c -- wav file functions
void get_pcm_info(struct container *);
int  get_samples(struct container *);
int  write_samples(FILE *out, struct container *);
int  pcm_find_string(char *, FILE *);
int  write_pcm_header(FILE *, struct container *);
void show_pcm_info(struct container *, struct payload *);
void validate_wavfile(struct container *, struct payload *);

#endif
