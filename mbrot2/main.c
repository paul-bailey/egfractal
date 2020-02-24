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
#include "pxbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#if EGFRACTAL_MULTITHREADED
#include <pthread.h>
#endif

struct gbl_t gbl = {
        .n_iteration    = 1000,
        .nthread        = 4,
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
        .fit            = false,
        .distance_root  = 0.25,
        .negate         = false,
        .linked         = false,
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

#if EGFRACTAL_MULTITHREADED

/* XXX Place up at top of file */
struct thread_helper_t {
        pthread_t *id;
        pthread_attr_t attr;
};

static void
init_thread_helper(struct thread_helper_t *helper, int nthread)
{
        helper->id = malloc(sizeof(*helper->id) * nthread);
        if (!helper->id)
                oom();
        if (pthread_attr_init(&helper->attr) != 0) {
                perror("pthread_attr_init error");
                exit(EXIT_FAILURE);
        }
}

static void
join_threads(struct thread_helper_t *helper,
                struct thread_info_t *ti, int nthread)
{
        int i;
        int res = pthread_attr_destroy(&helper->attr);
        if (res != 0) {
                perror("pthread_attr_destroy error");
                exit(EXIT_FAILURE);
        }
        for (i = 0; i < nthread; i++) {
                void *s;
                res = pthread_join(helper->id[i], &s);
                if (res != 0 || s != NULL) {
                        fprintf(stderr, "pthread_join[%d] failed (%s)\n",
                                i, strerror(errno));
                        fprintf(stderr, "Continuing anyway\n");
                }
        }

}

static void
create_thread(struct thread_helper_t *helper,
                struct thread_info_t *ti, int threadno)
{
        int res = pthread_create(&helper->id[threadno],
                        &helper->attr, &mbrot_thread, &ti[threadno]);
        if (res != 0) {
                perror("pthread_create error");
                exit(EXIT_FAILURE);
        }
}

static void
free_thread_helper(struct thread_helper_t *helper)
{
        free(helper->id);
}

#else /* !EGFRACTAL_MULTITHREADED */

struct thread_helper_t {
        int dummy;
};

static void
init_thread_helper(struct thread_helper_t *helper, int nthread)
{
        return;
}

static void
join_threads(struct thread_helper_t *helper,
                struct thread_info_t *ti, int nthread)
{
        /* Only one thread, so call it directly */
        mbrot_thread(&ti[0]);
}

static void
create_thread(struct thread_helper_t *helper,
                struct thread_info_t *ti, int threadno)
{
        return;
}

static void
free_thread_helper(struct thread_helper_t *helper)
{
        return;
}

#endif /* !EGFRACTAL_MULTITHREADED */

void
mbrot_get_data(mfloat_t *tbuf, mfloat_t *min, mfloat_t *max, int nthread)
{
        int nrows = gbl.height / nthread;
        int i;
        struct thread_info_t *ti;
        struct thread_helper_t helper;

        ti = malloc(sizeof(*ti) * nthread);
        if (!ti)
                oom();

        if (nrows <= 0) {
                fprintf(stderr,
                        "Error: height cannot be more than "
                        "the number of threads\n");
                exit(EXIT_FAILURE);
        }

        if (nrows * nthread != gbl.height) {
                fprintf(stderr,
                        "Warning: Cannot create image of height=%u;"
                        " instead cropping to height=%u\n",
                        gbl.height, nrows * nthread);
                /*
                 * No need to realloc tbuf, because for int,
                 * (a / b) * b <= a, always.
                 */
                gbl.height = nrows * nthread;
        }

        init_thread_helper(&helper, nthread);

        for (i = 0; i < nthread; i++) {
                ti[i].min          = 1.0e16;
                ti[i].max          = 0.0;
                ti[i].bailoutsqu   = gbl.bailoutsqu;
                ti[i].log_d        = gbl.log_d;
                ti[i].distance_est = gbl.distance_est;
                ti[i].dither       = gbl.dither;
                ti[i].skip         = nthread;
                ti[i].rowstart     = i;
                ti[i].rowend       = gbl.height;

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
                ti[i].bufsize = sizeof(mfloat_t) * gbl.width * nrows;
                ti[i].buf = malloc(ti[i].bufsize);
                if (!ti[i].buf)
                        oom();

                create_thread(&helper, ti, i);
        }
        join_threads(&helper, ti, nthread);

        /* Combine the threads' interleaved buffers */
        if (min)
                *min = INFINITY;
        if (max)
                *max = -INFINITY;
        for (i = 0; i < nthread; i++) {
                int row, col;
                mfloat_t *src = ti[i].buf;
                for (row = i; row < gbl.height; row += nthread) {
                        mfloat_t *dst = &tbuf[row * gbl.width];
                        for(col = 0; col < gbl.width; col++)
                                *dst++ = *src++;
                }
                if (min && *min > ti[i].min)
                        *min = ti[i].min;
                if (max && *max < ti[i].max)
                        *max = ti[i].max;

                free(ti[i].buf);
        }
        free(ti);
        free_thread_helper(&helper);
}

static void
mandelbrot(Pxbuf *pxbuf)
{
        int row, col;
        mfloat_t *ptbuf, *tbuf, min, max;

        tbuf = malloc(gbl.width * gbl.height * sizeof(*tbuf));
        if (!tbuf)
                oom();

        mbrot_get_data(tbuf, &min, &max, gbl.nthread);

        printf("min: %Lg max: %Lg\n", (long double)min, (long double)max);
        ptbuf = tbuf;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        /*
                         * FIXME: need to completely re-design
                         * palette.c to use arrays of struct pixel_t,
                         * for precision's sake.
                         */
                        mfloat_t v = *ptbuf++;
                        struct pixel_t px;
                        get_color(v, min, max, &px);
                        pxbuf_set_pixel(pxbuf, &px, row, col);
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
        int method;

        /* need to set these "consts" first */
        gbl.log_d = logl(2.0L);

        parse_args(argc, argv, &optflags);

        pxbuf = pxbuf_create(gbl.width, gbl.height);
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

        if (optflags.print_palette) {
                print_palette_to_bmp(pxbuf);
        } else {

                if (gbl.rmout) {
                        method = PXBUF_NORM_CROP;
                } else if (gbl.have_equalize) {
                        method = PXBUF_NORM_EQ;
                } else if (gbl.distance_est) {
                        method = PXBUF_NORM_SCALE;
                } else {
                        method = PXBUF_NORM_CLIP;
                }
                pxbuf_normalize(pxbuf, method,
                                gbl.rmout_scale, gbl.linked);

                if (gbl.fit)
                        pxbuf_normalize(pxbuf, PXBUF_NORM_FIT, 0., gbl.linked);
                if (gbl.negate)
                        pxbuf_negate(pxbuf);
        }

        pxbuf_print_to_bmp(pxbuf, fp, PXBUF_NORM_CLIP);
        fclose(fp);
        pxbuf_destroy(pxbuf);
        return 0;
}

