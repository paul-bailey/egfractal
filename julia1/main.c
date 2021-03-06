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
#include "julia1_common.h"
#include "fractal_common.h"
#include "pxbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <string.h>

struct gbl_t gbl = {
        .n_iteration = 1000,
        .dither = 0,
        .height = 600,
        .width = 600,
        .pallette = 2,
        .zoom_pct = 1.0,
        .zoom_xoffs = 0.0,
        .zoom_yoffs = 0.0,
        .cx = -0.70176,
        .cy = -0.3842,
        .bailout = 2.0L,
        .bailoutsq = 4.0L,
        .distance_root = 0.25,
        .eq_option = 0.5L,
        .log_d = 0.,
        .formula = NULL,
        .dformula = NULL,
        .distance_est = false,
        .negate = false,
        .equalize = false,
        .color_distance = false,
        .verbose = false,
        .linked = false,
};

/* Error helpers */
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

#define INSIDE (-1.0L)

static mfloat_t
iterate_distance(complex_t z)
{
        unsigned long i, n = gbl.n_iteration;
        mfloat_t zmod;
        complex_t dz = { .re = 1.0L, .im = 0.0L };
        complex_t c = { .re = gbl.cx, .im = gbl.cy };
        if (gbl.formula) {
                for (i = 0; i < n; i++) {
                        complex_t ztmp = gbl.formula(z, c);
                        if (!complex_isfinite(ztmp))
                                break;
                        dz = complex_mul(gbl.dformula(z, c), dz);
                        z = ztmp;
                        if (complex_modulus2(z) >= gbl.bailoutsq)
                                break;
                }
        } else {
                for (i = 0; i < n; i++) {
                        /* "z = z^2 + c" and "dz = f'(c)*dz + 1.0" */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        dz = complex_mul(z, dz);
                        dz = complex_mulr(dz, 2.0L);
                        z = ztmp;
                        if (complex_modulus2(z) >= gbl.bailoutsq)
                                break;
                }
        }
        if (dz.re == 0.0 && dz.im == 0.0)
                return -1L;
        assert(z.re != 0.0 || z.im != 0.0);
        assert(dz.re != 0.0 || dz.im != 0.0);
        zmod = complex_modulus(z);
        return zmod * logl(zmod) / complex_modulus(dz);
}

static mfloat_t
iterate_normal(complex_t z)
{
        mfloat_t ret;
        unsigned long i, n = gbl.n_iteration;
        complex_t c = { .re = gbl.cx, .im = gbl.cy };

        if (gbl.formula) {
                for (i = 0; i < n; i++) {
                        if (!complex_isfinite(z))
                                break;
                        if (complex_modulus2(z) >= gbl.bailoutsq)
                                break;
                        z = gbl.formula(z, c);
                }
        } else {
                /* "z = z^2 + c */
                for (i = 0; i < gbl.n_iteration; i++) {
                        if (complex_modulus2(z) >= gbl.bailoutsq)
                                break;
                        z = complex_add(complex_sq(z), c);
                }
        }
        if (i == n)
                return INSIDE;

        /* TODO: Dither here */
        ret = (mfloat_t)i;
        if (gbl.dither > 0) {
                if (!!(gbl.dither & 01)) {
                        /*
                         * Smooth by distance
                         *
                         * XXX: This is the estimate for Mandelbrot set.
                         * Is this correct?
                         */
                        mfloat_t log_zn = logl(complex_modulus2(z)) / 2.0;
                        mfloat_t nu = logl(log_zn / gbl.log_d) / gbl.log_d;
                        if (isfinite(log_zn) && isfinite(nu))
                                ret += 1.0L - nu;
                }

                if (!!(gbl.dither & 02)) {
                        /* Smooth by dithering */
                        int v = rand();
                        if (v < 0)
                                v = (~0ul << 8) | (v & 0xff);
                        else
                                v &= 0xff;
                        mfloat_t diff = (mfloat_t)v / 128.0L;
                        ret += diff;
                }

                if (ret >= (mfloat_t)gbl.n_iteration)
                        ret = (mfloat_t)(gbl.n_iteration - 1);
                else if (ret < 0.0)
                        ret = 0.0;
        }
        return ret;
}

static mfloat_t
julia_px(int row, int col)
{
        complex_t z = xy_to_complex(row, col);
        if (gbl.distance_est)
                return iterate_distance(z);
        else
                return iterate_normal(z);
}

static void
julia(Pxbuf *pxbuf)
{
        int row, col;
        unsigned long total;
        mfloat_t *ptbuf, *tbuf, max;
        tbuf = malloc(gbl.width * gbl.height * sizeof(*tbuf));
        if (!tbuf)
                oom();
        total = 0;

        if (gbl.verbose) {
                printf("Row %9d col %9d", 0, 0);
                fflush(stdout);
        }
        ptbuf = tbuf;
        max = 0.0;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        mfloat_t i = julia_px(row, col);
                        if (gbl.verbose) {
                                printf("\e[23D%9d col %9d", row, col);
                                fflush(stdout);
                        }
                        if (i > max)
                                max = i;
                        total += (int)i;
                        *ptbuf++ = i;
                }
        }
        if (gbl.verbose)
                putchar('\n');
        ptbuf = tbuf;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        unsigned int color;
                        struct pixel_t px;
                        mfloat_t i = *ptbuf++;
                        color = get_color(i, max);
                        px.x[0] = (float)(color & 0xffu);
                        px.x[1] = (float)((color >> 8) & 0xffu);
                        px.x[2] = (float)((color >> 16) & 0xffu);
                        pxbuf_set_pixel(pxbuf, &px, row, col);
                }
        }
        free(tbuf);
}

int
main(int argc, char **argv)
{
        FILE *fp;
        const char *outfile;
        Pxbuf *pxbuf;

        /* Initialize this "constant" */
        gbl.log_d = logl(2.0L);

        outfile = parse_args(argc, argv);

        pxbuf = pxbuf_create(gbl.width, gbl.height);
        if (!pxbuf) {
                fprintf(stderr, "OOM!\n");
                return 1;
        }

        julia(pxbuf);

        fp = fopen(outfile, "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }
        /* TODO: Add --rmout option */
        if (gbl.equalize) {
                pxbuf_normalize(pxbuf, PXBUF_NORM_EQ,
                                gbl.eq_option, gbl.linked);
        } else {
                pxbuf_normalize(pxbuf, PXBUF_NORM_SCALE, 1.0, gbl.linked);
        }
        if (gbl.negate)
                pxbuf_negate(pxbuf);
        pxbuf_print_to_bmp(pxbuf, fp, PXBUF_NORM_CLIP);
        fclose(fp);
        pxbuf_destroy(pxbuf);
        return 0;
}
