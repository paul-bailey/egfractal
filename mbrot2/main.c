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
#include <pthread.h>
#include <errno.h>

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
        .color_distance = false,
        .distance_root  = 0.25,
        .negate         = false,
        .formula        = NULL,
        .log_d          = 0.0,
        .rmout          = false,
        .rmout_scale    = 3.0,
};

static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(EXIT_FAILURE);
}

static mfloat_t
mbrot_eq(mfloat_t *buf, size_t size, mfloat_t *pmin, mfloat_t max)
{
        enum { HIST_SIZE = 64 * 1024 };
        static unsigned long histogram[HIST_SIZE];
        static unsigned long cdf[HIST_SIZE];
        unsigned long cdfmax, cdfrange;
        mfloat_t unit;
        mfloat_t min = *pmin;
        int i;

        unit = (max - min) / (mfloat_t)HIST_SIZE;

        for (i = 0; i < size; i++) {
                int hidx;
                if (buf[i] < 0)
                        continue;
                hidx = (int)((buf[i] - min) / unit);
                if (hidx < 0)
                        hidx = 0;
                else if (hidx >= HIST_SIZE)
                        hidx = HIST_SIZE;
                histogram[hidx]++;
        }

        cdfmax = 0;
        for (i = 0; i < HIST_SIZE; i++) {
                cdfmax += histogram[i];
                cdf[i] = cdfmax;
        }
        assert(cdfmax != cdf[0]);

        /* Slumpify option */
        for (i = 0; i < HIST_SIZE; i++) {
                unsigned long long v = cdf[i];
                v = (int)((double)cdfmax
                          * pow((double)v / (double)cdfmax,
                                gbl.equalize_root));
                cdf[i] = v;
        }

        cdfrange = cdfmax - cdf[0];
        for (i = 0; i < size; i++) {
                int hidx;
                if (buf[i] < 0)
                        continue;
                hidx = (int)((buf[i] - min) / unit);
                buf[i] = (mfloat_t)(cdf[hidx] - cdf[0]) * (max-min) / (mfloat_t)cdfrange;
        }

        /* Return new max/min */
        max = -1;
        min = 1e16;
        for (i = 0; i < size; i++) {
                if (min > buf[i])
                        min = buf[i];
                if (max < buf[i])
                        max = buf[i];
        }

        *pmin = min;
        return max;
}

static void
shave_outliers(mfloat_t *buf)
{
        mfloat_t mean, sumsq, stddev, sum, stdmin, stdmax, max;
        int i;
        int npx = gbl.height * gbl.width;

        for (max = 0.0, sum = 0.0, i = 0; i < npx; i++) {
                sum += buf[i];
                if (max < buf[i])
                        max = buf[i];
        }
        mean = sum / (mfloat_t)npx;
        for (sumsq = 0.0, i = 0; i < npx; i++) {
                mfloat_t diff = buf[i] - mean;
                sumsq += diff * diff;
        }
        /* "n" instead of "n-1", because we have the whole population */
        stddev = sqrtl(sumsq / (mfloat_t)npx);

        stdmin = mean - gbl.rmout_scale * stddev;
        stdmax = mean + gbl.rmout_scale * stddev;

        if (stdmin < 0.0)
                stdmin = 0.0;
        if (stdmax > max)
                stdmax = max;
        assert(stdmax > stdmin);

        for (i = 0; i < npx; i++) {
                if (buf[i] >= 0.0 && buf[i] < stdmin)
                        buf[i] = stdmin;
                else if (buf[i] > stdmax)
                        buf[i] = stdmax;
        }
}

void
mbrot_get_data(mfloat_t *tbuf, mfloat_t *min, mfloat_t *max)
{
        enum { NTHREAD = 4 };
        int nrows = gbl.height / NTHREAD;
        int rowstart = 0;
        int res;
        int i;
        struct thread_info_t *ti;
        pthread_t id[NTHREAD];
        pthread_attr_t attr;

        ti = malloc(sizeof(*ti) * NTHREAD);
        if (!ti)
                oom();

        res = pthread_attr_init(&attr);
        if (res != 0) {
                perror("pthread_attr_init error");
                exit(EXIT_FAILURE);
        }

        /*
         * TODO: Call mbrot_thread.
         * Split up by row, NOT colum!
         */
        for (i = 0; i < NTHREAD; i++) {
                ti[i].min          = 1.0e16;
                ti[i].max          = 0.0;
                ti[i].bailoutsqu   = gbl.bailoutsqu;
                ti[i].log_d        = gbl.log_d;
                ti[i].distance_est = gbl.distance_est;
                ti[i].dither       = gbl.dither;
                ti[i].rowstart     = rowstart;
                ti[i].rowend       = (i == NTHREAD-1)
                                    ? gbl.height : rowstart + nrows;

#if OLD_XY_TO_COMPLEX
                ti[i].height       = gbl.height;
                ti[i].width        = gbl.width;
                ti[i].zoom_pct     = gbl.zoom_pct;
                ti[i].zoom_yoffs   = gbl.zoom_yoffs;
                ti[i].zoom_xoffs   = gbl.zoom_xoffs;
#endif
                ti[i].colstart     = 0;
                ti[i].colend       = gbl.width;
                ti[i].formula      = gbl.formula;
                ti[i].dformula     = gbl.dformula;
                ti[i].n_iteration  = gbl.n_iteration;
                ti[i].w4 = 4.0L * gbl.zoom_pct / (mfloat_t)gbl.width;
                ti[i].h4 = 4.0L * gbl.zoom_pct / (mfloat_t)gbl.height;
                ti[i].zx = 2.0L * gbl.zoom_pct - gbl.zoom_xoffs;
                ti[i].zy = 2.0L * gbl.zoom_pct - gbl.zoom_yoffs;
                ti[i].bufsize = sizeof(mfloat_t) * gbl.width
                                * (ti[i].rowend - rowstart);
                ti[i].buf = malloc(ti[i].bufsize);
                if (!ti[i].buf)
                        oom();
                res = pthread_create(&id[i], &attr,
                                     &mbrot_thread, &ti[i]);
                if (res != 0) {
                        perror("pthread_create error");
                        exit(EXIT_FAILURE);
                }

                rowstart += nrows;
        }
        res = pthread_attr_destroy(&attr);
        if (res != 0) {
                perror("pthread_attr_destroy error");
                exit(EXIT_FAILURE);
        }
        for (i = 0; i < NTHREAD; i++) {
                void *s;
                int res = pthread_join(id[i], &s);
                if (res != 0 || s != NULL) {
                        fprintf(stderr, "pthread_join[%d] failed (%s)\n",
                                i, strerror(errno));
                        exit(EXIT_FAILURE);
                }
        }

        /*
         * Combine the threads' buffers.
         */
        if (min)
                *min = 1.0e16;
        if (max)
                *max = 0.0;
        rowstart = 0;
        for (i = 0; i < NTHREAD; i++) {
                mfloat_t *dst = &tbuf[rowstart * gbl.width];
                memcpy(dst, ti[i].buf, ti[i].bufsize);
                if (min && *min > ti[i].min)
                        *min = ti[i].min;
                if (max && *max < ti[i].max)
                        *max = ti[i].max;

                free(ti[i].buf);
                rowstart += nrows;
        }
        free(ti);
}

static void
mandelbrot(Pxbuf *pxbuf)
{
        int row, col;
        mfloat_t *ptbuf, *tbuf, min, max;

        tbuf = malloc(gbl.width * gbl.height * sizeof(*tbuf));
        if (!tbuf)
                oom();

        mbrot_get_data(tbuf, &min, &max);

        printf("min: %Lg max: %Lg\n", (long double)min, (long double)max);
        if (gbl.rmout)
                shave_outliers(tbuf);
        if (gbl.have_equalize) {
                max = mbrot_eq(tbuf, gbl.height * gbl.width, &min, max);
                max -= min;
                min = 0;
                printf("min: %Lg max: %Lg (after EQ)\n",
                       (long double)min, (long double)max);
        }
        ptbuf = tbuf;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        mfloat_t v = *ptbuf++;
                        unsigned int color = get_color(v, min, max);
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
        gbl.log_d = logl(2.0L);

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

