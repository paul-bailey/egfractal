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

#include "config.h"
#include "fractal_common.h"
#include "pxbuf.h"

#ifndef EGFRACTAL_MULTITHREADED
# define EGFRACTAL_MULTITHREADED 0
#endif

#if 0
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
#endif
#define TO_RGB(r_, g_, b_)  (((r_) << 16) | ((g_) << 8) | (b_))

/* main.c */
extern struct gbl_t {
        unsigned long n_iteration;
        unsigned int dither;
        unsigned int height;
        unsigned int width;
        unsigned int palette;
        unsigned int nthread;
        mfloat_t zoom_pct;
        mfloat_t zoom_xoffs;
        mfloat_t zoom_yoffs;
        mfloat_t bailout;
        mfloat_t bailoutsqu;
        mfloat_t distance_root;
        mfloat_t log_d;
        mfloat_t equalize_root;
        mfloat_t rmout_scale;
        unsigned int min_iteration;
        bool fit;
        bool distance_est;
        bool verbose;
        bool negate;
        bool color_distance;
        bool have_equalize;
        bool rmout;
        bool linked;
        complex_t (*formula)(complex_t, complex_t);
        complex_t (*dformula)(complex_t, complex_t);
} gbl;

#define OLD_XY_TO_COMPLEX 1
struct thread_info_t {
        mfloat_t min;
        mfloat_t max;
        mfloat_t *buf;
        mfloat_t bailoutsqu;
        mfloat_t log_d;
        bool distance_est;
        bool dither;
        int skip;
        int rowstart;
        int rowend;
        int colstart;
        int colend;
        int bufsize;
#if OLD_XY_TO_COMPLEX
        int height;
        int width;
        mfloat_t zoom_pct;
        mfloat_t zoom_yoffs;
        mfloat_t zoom_xoffs;
#endif
        complex_t (*formula)(complex_t, complex_t);
        complex_t (*dformula)(complex_t, complex_t);
        long n_iteration;
        /* Early calculations to reduce math in iterator */
        mfloat_t w4; /* global width / 4.0 */
        mfloat_t h4; /* global height / 4.0 */
        mfloat_t zx; /* 2*(zoom_pct)-zoom_xoffs */
        mfloat_t zy; /* 2*(zoom_pct)-zoom_yoffs */
};

/* palette.c */
extern void get_color(mfloat_t idx, mfloat_t min,
                        mfloat_t max, struct pixel_t *px);
extern void print_palette_to_bmp(Pxbuf *pxbuf);

/* parse_args.c */
struct optflags_t {
        bool print_palette;
        const char *outfile;
};
extern void parse_args(int argc, char **argv, struct optflags_t *optflags);

/* mbrot_thread.c */
extern void *mbrot_thread(void *arg);

#endif /* MANDELBROT_COMMON_H */

