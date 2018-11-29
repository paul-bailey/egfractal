/*
 * Copyright (c) 2018, Paul Bailey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
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

        /* Non-standard colors I rather like */
        COLOR_AMBER     = 0xe7b210u,
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
extern int pxbuf_rotate(Pxbuf *pxbuf);
extern void pxbuf_print(Pxbuf *pxbuf, FILE *fp);
extern Pxbuf *pxbuf_create(int width, int height, unsigned int color);
extern void pxbuf_free(Pxbuf *pxbuf);
extern void pxbuf_eq(Pxbuf *pxbuf, double exp, bool preserve_color);
extern void pxbuf_negate(Pxbuf *pxbuf);

#endif /* FRACTAL_COMMON_H */

