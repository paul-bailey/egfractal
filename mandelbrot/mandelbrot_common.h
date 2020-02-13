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
#ifndef MANDELBROT_COMMON_H
#define MANDELBROT_COMMON_H

#include "fractal_common.h"

/* main.c */
extern struct gbl_t {
        unsigned long n_iteration;
        unsigned int dither;
        unsigned int height;
        unsigned int width;
        unsigned int palette;
        mfloat_t zoom_pct;
        mfloat_t zoom_xoffs;
        mfloat_t zoom_yoffs;
        mfloat_t bailout;
        mfloat_t bailoutsqu;
        mfloat_t distance_root;
        mfloat_t log_d;
        mfloat_t equalize_root;
        unsigned int min_iteration;
        bool distance_est;
        bool verbose;
        bool negate;
        bool color_distance;
        bool have_equalize;
        complex_t (*formula)(complex_t, complex_t);
        complex_t (*dformula)(complex_t, complex_t);
} gbl;

/* palette.c */
extern unsigned int get_color(mfloat_t idx, mfloat_t min, mfloat_t max);
extern void print_palette_to_bmp(Pxbuf *pxbuf);

/* parse_args.c */
struct optflags_t {
        bool print_palette;
        const char *outfile;
};
extern void parse_args(int argc, char **argv, struct optflags_t *optflags);

#endif /* MANDELBROT_COMMON_H */

