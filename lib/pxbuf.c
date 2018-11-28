/*
 * pxbuf.c - Library to handle array with combined RGB
 * Compare with pxrgb.c.
 */
#include "fractal_common.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

enum {
        /* This is all we support - boring old 24-bit RGB. */
        RGB_NCHAN = 3,
        PXBUF_BPS = 8 * RGB_NCHAN,
};

struct pxbuf_t {
        unsigned int heightpx;
        unsigned int widthpx;
        unsigned int depth;
        unsigned int bytewidth; /* Saved widthpx * RGB_NCHAN */
        unsigned long bufsize;
        unsigned char *buf;
};

/* Because rows are backwards in .bmp files */
static inline unsigned int
true_row(Pxbuf *pxbuf, unsigned int row)
{
        return pxbuf->heightpx - row - 1;
}

/* Fast version of pxbuf_*ptr() - Don't check boundaries */
static unsigned char *
pxbuf_rowptr_safe(Pxbuf *pxbuf, unsigned int row)
{
        return &pxbuf->buf[true_row(pxbuf, row) * pxbuf->bytewidth];
}

static unsigned char *
pxbuf_colptr_safe(Pxbuf *pxbuf, unsigned int row, unsigned int col)
{
        unsigned int idx = true_row(pxbuf, row) * pxbuf->bytewidth
                           + RGB_NCHAN * col;
        assert(idx < pxbuf->bufsize);
        return &pxbuf->buf[idx];
}

static unsigned char *
pxbuf_fill_pixel_safe(unsigned char *pixel, unsigned int color)
{
        /*
         * Bitmap files are arranged in BGR order.  If TO_RGB() was used
         * to create @color, then so will be @color.
         */
        *pixel++ = color & 0xffu;
        color >>= 8;
        *pixel++ = color & 0xffu;
        color >>= 8;
        *pixel++ = color & 0xffu;
        return pixel;
}

/*
 * Normalize @array to max RGB value (0 to 255).
 * If @safety_only, then do not normalize if every value is less than
 * or equal to 255.
 */
static void
normalize(unsigned int *array, size_t alen, bool safety_only)
{
        int i;
        unsigned int max = 0;
        for (i = 0; i < alen; i++) {
                if (max < array[i])
                        max = array[i];
        }

        /* Can't normalize */
        if (max == 0)
                return;

        /* Already normalized */
        if (max == 255)
                return;

        if (safety_only && max <= 255)
                return;

        for (i = 0; i < alen; i++) {
                unsigned int v = array[i];
                /*
                 * XXX REVISIT: Arbitrary division!
                 * Is it faster to convert to double and multiply
                 * by initialized 1.0/(double)max?
                 */
                v = (v * 256) / max;
                if (v > 255)
                        v = 255;
                array[i] = v;
        }
}

/* Return pointer to start of @row, or NULL if @row is out of frame */
unsigned char *
pxbuf_rowptr(Pxbuf *pxbuf, unsigned int row)
{
        /*
         * Bitmaps are weird.
         * Their rows are backwards, but their columns are not.
         * pxbuf has no pad byptes, so don't count them here.
         */
        if (row >= pxbuf->heightpx)
                return NULL;
        return pxbuf_rowptr_safe(pxbuf, row);
}

unsigned char *
pxbuf_colptr(Pxbuf *pxbuf, unsigned int row, unsigned int col)
{
        /* Ditto about bitmaps being weird. */
        if (col >= pxbuf->widthpx || row >= pxbuf->heightpx)
                return NULL;
        return pxbuf_colptr_safe(pxbuf, row, col);
}

/**
 * pxbuf_fill_pixel - Set a pixel to a certain color
 * @pxbuf: Bitmap
 * @row: Pixel row
 * @col: Pixel column
 * @color: Color to set.  Use either a COLOR_* enum or TO_RGB()
 */
void
pxbuf_fill_pixel(Pxbuf *pxbuf, unsigned int row,
                 unsigned int col, unsigned int color)
{
        if (col >= pxbuf->widthpx || row >= pxbuf->heightpx)
                return;
        pxbuf_fill_pixel_safe(pxbuf_colptr_safe(pxbuf, row, col), color);
}

static unsigned int
pxbuf_get_pixel(Pxbuf *pxbuf, unsigned int row,
                unsigned int col)
{
        /* TODO: If made public, add safety check */
        unsigned char *colptr = pxbuf_colptr_safe(pxbuf, row, col);
        return (unsigned int)colptr[0]
                | ((unsigned int)colptr[1] << 8)
                | ((unsigned int)colptr[2] << 16);
}

/*
 * FIXME: These filters are one-dimensional on a 2-D plane.
 * Revisit my old DSP class notes w/r/t 2-D convolution.
 */

/**
 * pxbuf_filter_row - Filter a row of a bitmap
 * @pxbuf: Bitmap
 * @row: Row to filter
 * @filter: Array of filter
 * @filterlen: Length of filter
 */
void
pxbuf_filter_row(Pxbuf *pxbuf, int row,
                 const unsigned int *filter, int filterlen)
{
        unsigned char *rowptr;
        unsigned int *tbuf, *tcolor;
        unsigned int *colorp[RGB_NCHAN];
        int rgbchan;

        if (row >= pxbuf->heightpx)
                return;

        tbuf = malloc((pxbuf->widthpx + filterlen) * sizeof(*tbuf));
        if (!tbuf)
                goto e_tbuf;

        tcolor = malloc(pxbuf->widthpx * RGB_NCHAN * sizeof(*tcolor));
        if (!tcolor)
                goto e_tcolor;

        colorp[0] = tcolor;
        colorp[1] = &tcolor[pxbuf->widthpx];
        colorp[2] = &tcolor[pxbuf->widthpx * 2];

        rowptr = pxbuf_rowptr_safe(pxbuf, row);
        /* XXX REVISIT: Cross-over definition of RGB_NCHAN and RGB_NCHAN */
        for (rgbchan = 0; rgbchan < RGB_NCHAN; rgbchan++) {
                int i;
                for (i = 0; i < pxbuf->widthpx; i++)
                        colorp[rgbchan][i] = rowptr[i * RGB_NCHAN + rgbchan];

                convolve(tbuf, colorp[rgbchan], filter, pxbuf->widthpx, filterlen);
                memcpy(colorp[rgbchan], &tbuf[filterlen/2], pxbuf->widthpx * sizeof(*tcolor));
        }

        /*
         * Normalize all three channels together,
         * or else they will not be scaled in proportion with
         * each other.
         */
        normalize(tcolor, pxbuf->widthpx * RGB_NCHAN, true);

        for (rgbchan = 0; rgbchan < RGB_NCHAN; rgbchan++) {
                int i;
                /* XXX REVISIT: Do I want to adjust for phase lag? */
                for (i = 0; i < pxbuf->widthpx; i++)
                        rowptr[i * RGB_NCHAN + rgbchan] = colorp[rgbchan][i];
        }
        free(tcolor);
        free(tbuf);
        return;

e_tcolor:
        free(tbuf);
e_tbuf:
        fprintf(stderr, "OOM!\n");
        exit(EXIT_FAILURE);
}

/* Rotate right 90 degrees */
int
pxbuf_rotate(Pxbuf *pxbuf)
{
        Pxbuf tmp;
        int row, col;

        tmp.heightpx = pxbuf->widthpx;
        tmp.widthpx = pxbuf->heightpx;
        tmp.depth = pxbuf->depth;
        tmp.bytewidth = tmp.widthpx * RGB_NCHAN;
        tmp.bufsize = pxbuf->bufsize;
        /* XXX REVISIT: Surely there's a faster swap-in-place method */
        tmp.buf = malloc(tmp.bufsize);
        if (!tmp.buf)
                return -1;

        for (row = 0; row < pxbuf->heightpx; row++) {
                for (col = 0; col < pxbuf->widthpx; col++) {
                        unsigned int color = pxbuf_get_pixel(pxbuf, row, col);
                        /* row is now col and col is now row */
                        pxbuf_fill_pixel(&tmp, col, row, color);
                }
        }

        /* Don't zombify our old buffer */
        free(pxbuf->buf);

        memcpy(pxbuf, &tmp, sizeof(*pxbuf));
        return 0;
}

/**
 * pxbuf_filter_column - Filter a column of a bitmap
 * @pxbuf: Bitmap
 * @col: Column to filter
 * @filter: Array of filter
 * @filterlen: Length of filter
 */
void
pxbuf_filter_column(Pxbuf *pxbuf, int col,
                    const unsigned int *filter, int filterlen)
{
        unsigned int *tbuf, *tcolor;
        unsigned int rgbchan;
        unsigned char *start;
        unsigned int *colorp[RGB_NCHAN];

        if (col >= pxbuf->widthpx)
                return;

        tbuf = malloc((pxbuf->heightpx + filterlen) * sizeof(*tbuf));
        if (!tbuf)
                goto e_tbuf;

        tcolor = malloc(pxbuf->heightpx * sizeof(*tcolor) * RGB_NCHAN);
        if (!tcolor)
                goto e_tcolor;

        colorp[0] = tcolor;
        colorp[1] = &tcolor[pxbuf->heightpx];
        colorp[2] = &tcolor[pxbuf->heightpx * 2];

        start = &pxbuf->buf[RGB_NCHAN * col];
        for (rgbchan = 0; rgbchan < RGB_NCHAN; rgbchan++, start++) {
                int row;
                unsigned char *p = start;
                for (row = 0; row < pxbuf->heightpx; row++) {
                        colorp[rgbchan][row] = *p;
                        p += pxbuf->bytewidth;
                }
                convolve(tbuf, colorp[rgbchan], filter, pxbuf->heightpx, filterlen);
                memcpy(colorp[rgbchan], &tbuf[filterlen/2],
                       pxbuf->heightpx * sizeof(*tcolor));
        }

        /*
         * Normalize all three channels together,
         * or else they will not be scaled in proportion with
         * each other.
         */
        normalize(tcolor, pxbuf->heightpx * RGB_NCHAN, true);

        start = &pxbuf->buf[RGB_NCHAN * col];
        for (rgbchan = 0; rgbchan < RGB_NCHAN; rgbchan++, start++) {
                int row;
                unsigned char *p = start;
                for (row = 0; row < pxbuf->heightpx; row++) {
                        *p = colorp[rgbchan][row];
                        p += pxbuf->bytewidth;
                }
        }
        free(tcolor);
        free(tbuf);
        return;

e_tcolor:
        free(tbuf);
e_tbuf:
        fprintf(stderr, "OOM!\n");
        exit(EXIT_FAILURE);
}

/**
 * pxbuf_create - Get a new bitmap buffer
 *
 * Free this only with pxbuf_free()
 */
Pxbuf *
pxbuf_create(int width, int height, unsigned int color)
{
        Pxbuf *ret = malloc(sizeof(*ret));
        if (!ret)
                goto epx;

        ret->heightpx = height;
        ret->widthpx = width;
        ret->bytewidth = ret->widthpx * RGB_NCHAN;
        ret->bufsize = ret->bytewidth * ret->heightpx;
        ret->buf = malloc(ret->bufsize);
        if (!ret->buf)
                goto ebuf;

        if (color == COLOR_WHITE || color == COLOR_BLACK) {
                /* The fast path */
                memset(ret->buf, color == COLOR_WHITE ? -1 : 0, ret->bufsize);
        } else {
                /* The slow path */
                /*
                 * XXX REVISIT:  We can make this faster by making a
                 * small array of long integers and copying them without
                 * the bit-shifting that occurs in
                 * pxbuf_fill_pixel_safe().
                 */
                unsigned char *bufptr = ret->buf;
                unsigned long npx = ret->heightpx * ret->widthpx;
                int i;
                for (i = 0; i < npx; i++)
                        bufptr = pxbuf_fill_pixel_safe(bufptr, color);
        }
        return ret;

ebuf:
        free(ret);
epx:
        return NULL;
}

void
pxbuf_free(Pxbuf *pxbuf)
{
        free(pxbuf->buf);
        free(pxbuf);
}

void
pxbuf_print(Pxbuf *pxbuf, FILE *fp)
{
        bmp_print(fp, pxbuf->buf, pxbuf->widthpx, pxbuf->heightpx, PXBUF_BPS);
}

