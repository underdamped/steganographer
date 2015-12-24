/* * * * * * * * * * * * * * * *
 * steganographer, stego.c
 *
 * functions that hide or recover data
 *
 * Javier Lombillo
 * October 2015
 */

#include "steganographer.h"

/*
 * steganography comes from the Greek 'steganos', meaning 'covered'.
 *
 * this function iterates through each non-pad byte in the base file's pixel
 * matrix, while simultaneously iterating through every byte in the payload,
 * bit by bit (starting at the msb).  the lsb of the current pixel byte is set
 * to the current bit from the current payload byte.  in this way, all n bits
 * in the payload are distributed throughout the first n bytes of the base
 * file, achieving lsb steganography.
 */
int bitmap_cover(struct container *c, struct payload *p)
{
    int i, j, bitcount, bytecount, lastbyte;

    printf( "mixing bits from %s into image from %s...\n", p->filename, c->filename );

    bitcount  = 0;
    bytecount = 0;
    lastbyte  = c->b->rowlen - c->b->pad; // marks the last non-pad byte

    for ( i = 0; i < c->b->height; i++ )
    {
        for ( j = 0; j < c->b->rowlen; j++ )
        {
            if ( j == lastbyte )
                break;

            // set the lsb of the pixel byte to the value of the current payload bit (see NOTE at end of this file)
            c->b->pixel[i][j] = (c->b->pixel[i][j] & ~1) | (1 & (p->bytes[bytecount] >> (7 - (bitcount % 8))));

            bitcount++;

            if ( (bitcount % 8) == 0 )
                bytecount++;

            // done, bail out of the loops with the mighty GOTO
            if ( bytecount == p->size )
                goto GALOIS_FIELD_2;
        }
    }

GALOIS_FIELD_2:
    return 0;
}

/*
 * iterate through each byte in the pixel matrix (stepping over the pad bytes),
 * and store each lsb in the payload structure until we've recovered the
 * complete file
 */
int bitmap_uncover(struct container *c, struct payload *p)
{
    int i, j, bitcount, bytecount, lastbyte;
    int bits[8]; // temp storage

    bitcount  = 0;
    bytecount = 0;
    lastbyte  = c->b->rowlen - c->b->pad; // marks the last non-pad byte

    for ( i = 0; i < c->b->height; i++ )
    {
        for ( j = 0; j < c->b->rowlen; j++ )
        {
            if ( j == lastbyte )
                break;

            // the bit assignment can be accomplished in one line:
            //
            //   p->bytes[bytecount] |= (b->pixel[i][j] & 1) << (7 - (bitcount % 8));
            //
            // but i'm spelling it all out to make it more clear

            bits[bitcount % 8] = c->b->pixel[i][j] & 1; // get lsb from each byte

            bitcount++;

            if ( (bitcount % 8) == 0 )
            {
                p->bytes[bytecount] = (bits[0] << 7) | (bits[1] << 6) | (bits[2] << 5) |
                                      (bits[3] << 4) | (bits[4] << 3) | (bits[5] << 2) |
                                      (bits[6] << 1) | (bits[7] << 0);

                bytecount++;
            }

            if ( bytecount == p->size )
                goto RIEMANNIAN_MANIFOLD;
        }
    }

RIEMANNIAN_MANIFOLD:
    return 0;
}

/*
 * pcm equivalent of bitmap_cover()
 */
int pcm_cover(struct container *c, struct payload *p)
{
    int i, bitcount, bytecount;

    printf( "mixing bits from %s into sample data from %s...\n", p->filename, c->filename );

    bitcount  = 0;
    bytecount = 0;

    // iterate through each sample (not each byte)
    for ( i = 0; i < c->w->subchunk2size; i += c->w->sample_size )
    {
        // set the lsb of each sample to the value of the current payload bit (see NOTE at bottom)
        c->w->samples[i] = (c->w->samples[i] & ~1) | (1 & (p->bytes[bytecount] >> (7 - (bitcount % 8))));

        bitcount++;

        if ( (bitcount % 8) == 0 )
            bytecount++;

        // done, bail out of the loop
        if ( bytecount == p->size )
            break;
    }

    return 0;
}

/*
 * pcm equivalent of bitmap_uncover()
 */
int pcm_uncover(struct container *c, struct payload *p)
{
    int i, bitcount = 0, bytecount = 0;

    // iterate through each sample
    for ( i = 0; i < c->w->subchunk2size; i += c->w->sample_size )
    {
        // build each byte one LSB at a time
        p->bytes[bytecount] |= (c->w->samples[i] & 1) << (7 - (bitcount % 8));

        bitcount++;

        if ( (bitcount % 8) == 0 )
            bytecount++;

        if ( bytecount == p->size )
            break;
    }

    return 0;
}

/**** NOTE ****

The hairy expression in the bitmap_cover() function is

    b->pixel[i][j] = (b->pixel[i][j] & ~1) | (1 & (p->bytes[bytecount] >> (7 - (bitcount % 8))));

At a high level, this overwrites the LSB of a pixel byte with the value of a
specific bit in the payload.  The expression runs in a loop, iterating over
every bit in the payload, writing each to the LSB of a different pixel byte.
To understand how this works, let's first consider the expression on the right
of the OR:

  1 & (byte >> (7 - (bitcount % 8)))

Working our way out from the nested parentheses, 'bitcount % 8' iterates
through the set {0,1,2,3,4,5,6,7} as the loop chugs on.  Therefore, the
expression '7 - (bitcount % 8)' runs through {7,6,5,4,3,2,1,0}. This provides
the mechanism to cycle through each bit in a payload byte.

A notion of "current bit" is given by right-shifting: the value of each bit --
starting with the MSB -- is right-shifted to the LSB.  Thus the LSB in the
value of

  byte >> (7 - (bitcount % 8))

always stores the value of the "current bit".  Therefore, the expression

  1 & (byte >> (7 - (bitcount % 8)))

tests the value of the current bit: if the LSB is a 1, the above expression
evaluates to 1, because 1 & 1 = 1.  Likewise, if the LSB is a 0, it results in
a 0 (1 & 0 = 0).  So logically the full hairy expression is equivalent to

  pixel = (pixel & ~1) | currentbitvalue;

On the left of the OR, inside the parentheses, we're masking all bits except
the LSB of the pixel byte; for example, if the value of the pixel byte is
currently 0xFF (all bits on), then 'pixel & ~1' is

  1111 1111
& 1111 1110
  ---------
  1111 1110 = 0xFE

We then OR this value with the current bitvalue in the payload; let's say it's 0:

  1111 1110
| 0000 0000
  ---------
  1111 1110 = 0xFE

So the value of the pixel byte -- which had been 0xFF -- is now 0xFE. Had the
current bitvalue been 1, then we'd have

  1111 1110
| 0000 0001
  ---------
  1111 1111 = 0xFF

In other words, we've set the LSB of the byte to the value of the current
payload bit. This is the heavy-lifting done in the expression; the rest of the
stuff is just scaffolding.

Here's a walk-through and break-down of the complete expression for one
iteration.  To give us state,

  let b->pixel[0][0] = 0b0000 0100 (0x04)
  let p->bytes[0] = 0b1010 1010 (0xAA)
  let i = j = bytecount = bitcount = 0

then

  b->pixel[0][0] = (b->pixel[0][0] & ~1) | (1 & (p->bytes[0] >> (7 - (0 % 8))));
  b->pixel[0][0] = (b->pixel[0][0] & ~1) | (1 & (p->bytes[0] >> 7));
  b->pixel[0][0] = (b->pixel[0][0] & ~1) | (1 & 1; // because bit7 is on
  b->pixel[0][0] = (0b00000100 & 0b11111110) | 0b00000001;
  b->pixel[0][0] = 0b00000101; // 0x05

and so on

****/
