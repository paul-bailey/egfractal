#ifndef FRACTAL_COMMON_H
#define FRACTAL_COMMON_H

#include <stdio.h>
#include <stdbool.h>

#include "complex_helpers.h"

/* bmp_print.c */
extern void bmp_print(FILE *fp, const unsigned char *array,
                      int width, int height, int depth);

/* convolve.c */
extern void convolve(unsigned int *dest, const unsigned int *f,
                     const unsigned int *g, size_t fsize, size_t gsize);

/* pxbuf.c */
struct pxbuf_t;
typedef struct pxbuf_t Pxbuf;
enum {
        /* common color enumerations */
        COLOR_RED       = 0xff0000u,
        COLOR_GREEN     = 0x00ff00u,
        COLOR_BLUE      = 0x0000ffu,
        COLOR_CYAN      = 0x00ffffu,
        COLOR_MAGENTA   = 0xff00ffu,
        COLOR_YELLOW    = 0xffff00u,
        COLOR_WHITE     = 0xffffffu,
        COLOR_BLACK     = 0,
};
#define TO_RGB(r_, g_, b_)  (((r_) << 16) | ((g_) << 8) | (b_))
extern unsigned char *pxbuf_rowptr(Pxbuf *pxbuf, unsigned int row);
extern unsigned char *pxbuf_colptr(Pxbuf *pxbuf, unsigned int row,
                                   unsigned int col);
extern void pxbuf_fill_pixel(Pxbuf *pxbuf, unsigned int row,
                             unsigned int col, unsigned int color);
extern void pxbuf_filter_row(Pxbuf *pxbuf, int row,
                             const unsigned int *filter, int filterlen);
extern void pxbuf_filter_column(Pxbuf *pxbuf, int col,
                             const unsigned int *filter, int filterlen);
extern void pxbuf_print(Pxbuf *pxbuf, FILE *fp);
extern Pxbuf *pxbuf_create(int width, int height, unsigned int color);
extern void pxbuf_free(Pxbuf *pxbuf);

#endif /* FRACTAL_COMMON_H */

