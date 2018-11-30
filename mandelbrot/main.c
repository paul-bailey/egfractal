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
#include "mandelbrot_common.h"
#include "fractal_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

struct gbl_t gbl = {
        .n_iteration    = 1000,
        .dither         = 0,
        .height         = 600,
        .width          = 600,
        .palette        = 2,
        .zoom_pct       = 1.0,
        .zoom_xoffs     = 0.0,
        .zoom_yoffs     = 0.0,
        .bailout        = 2.0,
        .bailoutsqu     = 4.0,
        .min_iteration  = 0,
        .distance_est   = false,
        .verbose        = false,
        .distance_root  = 0.25,
        .negate         = false,
        .formula        = NULL,
};

/* Initialized to log2l(2.0L) */
static mfloat_t log_2;
static const mfloat_t INSIDE = -1.0L;

static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(EXIT_FAILURE);
}

/* scale pixels to points of mandelbrot set and handle zoom. */
static complex_t
xy_to_complex(int row, int col)
{
        complex_t c;

        c.re = 4.0L * (mfloat_t)col / (mfloat_t)gbl.width  - 2.0L;
        c.im = 4.0L * (mfloat_t)row / (mfloat_t)gbl.height - 2.0L;

        c.re = c.re * gbl.zoom_pct - gbl.zoom_xoffs;
        c.im = c.im * gbl.zoom_pct - gbl.zoom_yoffs;
        return c;
}

static mfloat_t
iterate_normal(complex_t c)
{
        mfloat_t ret;
        unsigned long n = gbl.n_iteration;
        unsigned long i;
        complex_t z = { .re = 0.0L, .im = 0.0L };

        /*
         * This is an ugly D.R.Y. violation,
         * but it keeps our normal algorithm
         * much faster.
         */
        if (gbl.formula) {
                for (i = 0; i < n; i++) {
                        /* new z = z^2 + c */
                        complex_t ztmp = complex_add(gbl.formula(z), c);
                        /* Too precise for our data types. Assume inside. */
                        if (ztmp.re == z.re && ztmp.im == z.im)
                                return INSIDE;
                        if (complex_modulus2(ztmp) > gbl.bailoutsqu)
                                break;
                        z = ztmp;
                }
        } else {
                for (i = 0; i < n; i++) {
                        /* new z = z^2 + c */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        /* Too precise for our data types. Assume inside. */
                        if (ztmp.re == z.re && ztmp.im == z.im)
                                return INSIDE;
                        if (complex_modulus2(ztmp) > gbl.bailoutsqu)
                                break;
                        z = ztmp;
                }
        }

        if (i == n)
                return INSIDE;

        /* i < n from here */
        ret = (mfloat_t)i;
        if (gbl.dither > 0) {
                if (!!(gbl.dither & 01)) {
                        /* Smooth with distance estimate */
                        /* FIXME: No longer true if gbl.formula != NULL */
                        mfloat_t log_zn = logl(complex_modulus2(z)) / 2.0L;
                        mfloat_t nu = logl(log_zn / log_2) / log_2;
                        if (isfinite(log_zn) && isfinite(nu))
                                ret += 1.0L - nu;
                        /* if not finite, can't smooth with distance est. */
                }

                if (!!(gbl.dither & 02)) {
                        /* dither */
                        int v = rand();
                        /* Don't guess if the compiler uses ASR instead of LSR */
                        if (v < 0)
                                v = (~0ul << 8) | (v & 0xff);
                        else
                                v &= 0xff;
                        mfloat_t diff = (mfloat_t)v / 128.0L;
                        ret += diff;
                }

                if (ret < 0.0L)
                        ret = 0.0L;
                else if (ret > (mfloat_t)n)
                        ret = (mfloat_t)n;
        }
        return ret;
}

static mfloat_t
iterate_distance(complex_t c)
{
        unsigned long n = gbl.n_iteration;
        unsigned long i;
        complex_t z = { .re = 0.0L, .im = 0.0L };
        complex_t dz = { .re = 1.0L, .im = 0.0L };
        if (gbl.formula) {
                for (i = 0; i < n; i++) {
                        /* use different formula than our usual */
                        complex_t ztmp = complex_add(gbl.formula(z), c);
                        dz = complex_mul(z, dz);
                        dz = complex_mulr(dz, 2.0L);
                        dz = complex_addr(dz, 1.0L);
                        z.re = ztmp.re;
                        z.im = ztmp.im;
                        if (complex_modulus2(z) > gbl.bailoutsqu)
                                break;
                }
        } else {
                for (i = 0; i < n; i++) {
                        /* "z = z^2 + c" and "dz = 2.0 * z * dz + 1.0" */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        dz = complex_mul(z, dz);
                        dz = complex_mulr(dz, 2.0L);
                        dz = complex_addr(dz, 1.0L);
                        z.re = ztmp.re;
                        z.im = ztmp.im;
                        if (complex_modulus2(z) > gbl.bailoutsqu)
                                break;
                }
        }

        /* Return distance normalized to the colorspace */
        /* XXX: This is no longer true if gbl.formula != NULL */
        return complex_modulus(z) * logl(complex_modulus(z)) / complex_modulus(dz);
}

static mfloat_t
mandelbrot_px(int row, int col)
{
        /* XXX: Quite an arbitrary choice */
        enum { THRESHOLD = 10 };

        complex_t c = xy_to_complex(row, col);
        if (!gbl.formula && gbl.n_iteration > THRESHOLD) {
                /*
                 * We know the formula for the main cardioid and bulb,
                 * and we know every point inside will converge.  So we
                 * can check that first before diving into the long
                 * iterative process.
                 */
                mfloat_t xp = c.re - 0.25L;
                mfloat_t ysq = c.im * c.im;
                mfloat_t q = xp * xp + ysq;
                if ((q * (q + xp)) < (0.25L * ysq))
                        return INSIDE;
                xp = c.re + 1.0L;
                if ((xp * xp + ysq) < (0.25L * ysq))
                        return INSIDE;
        }

        if (gbl.distance_est)
                return iterate_distance(c);
        else
                return iterate_normal(c);
}

static void
mandelbrot(Pxbuf *pxbuf)
{
        int row, col;
        mfloat_t *ptbuf, *tbuf, min, max;

        tbuf = malloc(gbl.width * gbl.height * sizeof(*tbuf));
        if (!tbuf)
                oom();

        if (gbl.verbose) {
                printf("Row %9d col %9d", 0, 0);
                fflush(stdout);
        }
        ptbuf = tbuf;
        min = 1.0e16;
        max = 0.0;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        mfloat_t v;
                        if (gbl.verbose) {
                                printf("\e[23D%9d col %9d", row, col);
                                fflush(stdout);
                        }
                        v = mandelbrot_px(row, col);
                        if (min > v)
                                min = v;
                        if (max < v)
                                max = v;
                        *ptbuf++ = v;
                }
        }
        if (gbl.verbose)
                putchar('\n');
        printf("min: %Lg max: %Lg\n", (long double)min, (long double)max);
        ptbuf = tbuf;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        unsigned int color = get_color(*ptbuf++, min, max);
                        pxbuf_fill_pixel(pxbuf, row, col, color);
                }
        }
        free(tbuf);
}

int
main(int argc, char **argv)
{
        struct optflags_t optflags = {
                .outfile = "mandelbrot.bmp",
                .print_palette = false,
        };
        Pxbuf *pxbuf;
        FILE *fp;

        /* need to set these "consts" first */
        log_2 = logl(2.0L);

        parse_args(argc, argv, &optflags);

        pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_WHITE);
        if (!pxbuf)
                oom();

        /*
         * Do this before fopen(), because we could be here for a very
         * time, and it's impolite to have a file open for that long.
         */
        if (!optflags.print_palette)
                mandelbrot(pxbuf);

        fp = fopen(optflags.outfile, "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file `%s'\n",
                        optflags.outfile);
                return 1;
        }

        if (optflags.print_palette)
                print_palette_to_bmp(pxbuf);
        if (gbl.negate)
                pxbuf_negate(pxbuf);
        pxbuf_print(pxbuf, fp);
        fclose(fp);
        pxbuf_free(pxbuf);
        return 0;
}

