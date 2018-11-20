#include "fractal_common.h"
#include <string.h>
#include <stdlib.h>

struct pxbuf_t {
        unsigned int heightpx;
        unsigned int widthpx;
        unsigned int depth;
        unsigned int bps;
        unsigned int bytewidth; /* Saved widthpx * depth */
        unsigned long bufsize;
        unsigned char *buf;
};

/* Fast version of pxbuf_*ptr() - Don't check boundaries */
static unsigned char *
pxbuf_rowptr_safe(Pxbuf *pxbuf, unsigned int row)
{
        return &pxbuf->buf[(pxbuf->heightpx - row) * pxbuf->bytewidth];
}

static unsigned char *
pxbuf_colptr_safe(Pxbuf *pxbuf, unsigned int row, unsigned int col)
{
        unsigned int idx = (pxbuf->heightpx - row) * pxbuf->bytewidth
                           + pxbuf->depth * col;
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
        int i, max = 0;
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
 *
 * Note:
 * If the filter results in out-of-bounds RGB values, then the row will
 * be normalized ONLY FOR THE COLOR THAT WAS OUT OF BOUNDS.
 */
void
pxbuf_filter_row(Pxbuf *pxbuf, int row,
                 const unsigned int *filter, int filterlen)
{
        unsigned char *rowptr;
        unsigned int *tbuf, *tcolor;
        int off;

        if (row >= pxbuf->heightpx)
                return;

        tbuf = malloc((pxbuf->widthpx + filterlen) * sizeof(*tbuf));
        if (!tbuf)
                goto e_tbuf;

        tcolor = malloc(pxbuf->widthpx * sizeof(*tcolor));
        if (!tcolor)
                goto e_tcolor;

        rowptr = pxbuf_rowptr_safe(pxbuf, row);
        for (off = 0; off < pxbuf->depth; off++) {
                int i;
                for (i = 0; i < pxbuf->widthpx; i++)
                        tcolor[i] = rowptr[i * pxbuf->depth + off];

                convolve(tbuf, tcolor, filter, pxbuf->widthpx, filterlen);
                normalize(tbuf, pxbuf->widthpx + filterlen, true);
                /* XXX REVISIT: Do I want to adjust for phase lag? */
                for (i = 0; i < pxbuf->widthpx; i++)
                        rowptr[i * pxbuf->depth + off] = tbuf[i];
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
 * pxbuf_filter_row - Filter a row of a bitmap
 * @pxbuf: Bitmap
 * @row: Row to filter
 * @filter: Array of filter
 * @filterlen: Length of filter
 *
 * FIXME:
 * If the filter results in out-of-bounds RGB values, then the
 * column will be normalized ONLY FOR THE COLOR THAT WAS OUT OF
 * BOUNDS. All three arrays should be normalized instead!
 */
void
pxbuf_filter_column(Pxbuf *pxbuf, int col,
                    const unsigned int *filter, int filterlen)
{
        unsigned int *tbuf, *tcolor;
        unsigned int off;
        unsigned char *start;

        if (col >= pxbuf->widthpx)
                return;

        tbuf = malloc((pxbuf->heightpx + filterlen) * sizeof(*tbuf));
        if (!tbuf)
                goto e_tbuf;

        tcolor = malloc(pxbuf->heightpx * sizeof(*tcolor));
        if (!tcolor)
                goto e_tcolor;

        start = &pxbuf->buf[pxbuf->depth * col];
        for (off = 0; off < pxbuf->depth; off++, start++) {
                int row;
                unsigned char *p;

                p = start;
                for (row = 0; row < pxbuf->heightpx; row++) {
                        tcolor[row] = *p;
                        p += pxbuf->bytewidth;
                }

                convolve(tbuf, tcolor, filter, pxbuf->heightpx, filterlen);
                normalize(tbuf, pxbuf->heightpx + filterlen, true);

                p = start;
                for (row = 0; row < pxbuf->heightpx; row++) {
                        *p = tbuf[row];
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
        ret->bps = 24;
        ret->depth = ret->bps / 8;
        ret->bytewidth = ret->widthpx * ret->depth;
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
