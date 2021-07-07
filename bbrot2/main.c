/*
 * bbrot2 - option set 2 for Buddhabrot
 *
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
 *
 * The Buddhabrot algorithm:
 * =========================
 *
 * For each random point: If it's in the Mandelbrot set (determined to
 * be points that do not escape before the max number of iterations),
 * then do nothing.  If it's outside the Mandelbrot set, then trace its
 * path from f(z=0) to f(z=bailout).  (I use the word ``trace" loosely,
 * since the path is not necessarily continuous.)
 *
 * The `trace' is done using a simple histogram: an array of counters for
 * each pixel.  For every z[i] before z == bailout, increment the counter
 * corresponding to z[i].  (In an analog world, the brightness of any
 * area of the picture represents the density of the `traces" through
 * that area, but in the digital world we can be simpler and say each
 * pixel's brightness represents the number of times a `trace' hit that
 * pixel, even though those hits are in fact at different points *within*
 * the pixel, assuming a truly exclusive random-number generator.)
 *
 * There are three such histograms, one for each RGB channel.
 * Their counts will be different from each other for the following
 * reasons:
 *
 * 1. Slightly different paths will be traced each time due to
 *    there being different random-generator seeds for each channel.
 * 2. The selection of which paths to include may be different because
 *    of command-line options that allow a user to select different
 *    bailout radii and number of iterations for each channel.
 * 3. An additional per-channel command-line option allows user
 *    to throw out a different number of first paths for each channel.
 *
 * Because of this, you can make Buddhabrot pictures that are not
 * only black and white.
 *
 * The histogram looks like a one-dimensional array in the code, but
 * that's just because the dimensions are command-line options.  In
 * purpose it's really a two-dimensional array like the z plane and the
 * bitmap.
 *
 * Optimizations:
 * --------------
 *
 * These optimizations were taken straight out of the Wikipedia pages
 * for Buddhabrot and Mandelbrot set.
 *
 * For points that exist inside the Mandelbrot set, this algorithm can
 * take extra long, since it means maxing out iterations.  This code uses
 * two optimizations for that:
 *
 * 1. Since we know the formula for the main cardioid and largest bulb,
 *    we can more quickly check for that first and bail if it's inside.
 *    This covers the vast majority of samples inside the Mandelbrot set.
 * 2. If z=z^2+c is unchanging, assume it's inside the Mandelbrot set.
 *    (XXX: This might be useful for zoomed-in Mandelbrot-set algorithms,
 *    but I'm not sure it helps much with Buddhabrot.)
 *
 * Thread optimizations:
 * ---------------------
 *
 * This differs from buddhabrot1 (sorry for the inconsistent naming
 * convention) only in that we use POSIX threads here, so that if the
 * operating system permits, we can run the program on multiple cores.
 * On a 6-core Ubuntu machine this has been shown to be dramatically
 * faster.  But if it's on a single-core system, or one that doesn't
 * handle POSIX threads at the kernel level (TODO: Is this true about
 * Cygwin? macOS?), it's theoretically slower due to the overhead of
 * additional context switching.
 *
 * Wanted Optimizations:
 * ---------------------
 *
 * 1. Multi-threading: see comments to iterate_r() in the code below
 *
 * 2. If there's a fast way to tell if a sample is just next to, but
 *    outside, the Mandelbrot set, then you wouldn't have the problem of
 *    having to run close-to-max iterations twice.  But I don't know any
 *    such optimization.
 *
 * 3. I also don't know any cheats like the cardioid check for formulas
 *    other than z^2+c.  This is unfortunate, since you can get some really
 *    gnarly images from other such formulas, like the burning ship algo.
 */
#include "config.h"
#include "pxbuf.h"
#include "bbrot2.h"
#include "fractal_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <sys/mman.h>
#ifndef EGFRACTAL_MULTITHREADED
# define EGFRACTAL_MULTITHREADED 0
#else
# include <pthread.h>
#endif

struct params_t {
        int n_red;
        int n_green;
        int n_blue;
        int height;
        int width;
        int min;
        int nthread;
        mfloat_t bailout;
        mfloat_t bailsqu;
        mfloat_t line_y;
        mfloat_t line_x;
        double eq_exp;
        double rmout_scale;
        unsigned long points;
        bool singlechan;
        bool do_hist;
        bool verbose;
        bool use_line_y;
        bool use_line_x;
        bool negate;
        bool rmout;
        bool linked;
        complex_t (*formula)(complex_t, complex_t);
        const char *overlay;
};

/* Error helpers */
static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(1);
}

static void
bad_arg(const char *type, const char *optarg)
{
        fprintf(stderr, "Bad %s option: `%s'\n", type, optarg);
        exit(EXIT_FAILURE);
}

static void
initialize_seeds(unsigned short seeds[6])
{
        clock_t c, tmp;
        time_t t = time(NULL);
        /*
         * At the cost of likely a microsecond,
         * ensure the ticks have changed, so that
         * seeds are different for every call to
         * this function.
         */
        c = tmp = clock();
        while (c == tmp)
                c = clock();
        seeds[0] = seeds[3] = t & 0xffffu;
        seeds[1] = seeds[4] = (t >> 16) & 0xffffu;
        /*
         * These need to be different to converge x from y,
         * or else x == y always, and we are only ever sampling
         * the diagonal line.  This makes for a trippy picture,
         * but not the one we're looking for.
         */
        seeds[2] = (unsigned short)c;
        seeds[5] = (unsigned short)c + 1;
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
                        &helper->attr, &bbrot_thread, &ti[threadno]);
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
#warning Remove
printf("Not multithreaded\n");
        return;
}

static void
join_threads(struct thread_helper_t *helper,
                struct thread_info_t *ti, int nthread)
{
        /* Only one thread, so call it directly */
        bbrot_thread(&ti[0]);
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

static void
bbrot2_get_data(struct params_t *params, unsigned long *sumbuf,
                int nchan, int npx)
{
        struct thread_info_t *ti;
        struct thread_helper_t helper;
        int nthread = params->nthread;
        size_t bufsize = sizeof(unsigned long) * npx * nchan;
        int i;

        ti = malloc(sizeof(*ti) * nthread);
        if (!ti)
                oom();

        init_thread_helper(&helper, nthread);

        for (i = 0; i < nthread; i++) {
                unsigned long *chanbase = malloc(bufsize);
                if (!chanbase)
                        oom();
                memset(chanbase, 0, bufsize);

                /* XXX This assumes points is a multiple of nthread */
                ti[i].points            = params->points / nthread;
                ti[i].width             = params->width;
                ti[i].height            = params->height;
                ti[i].nchan             = nchan;
                ti[i].min               = params->min;
                ti[i].n[0]              = params->n_red;
                ti[i].n[1]              = params->n_green;
                ti[i].n[2]              = params->n_blue;
                ti[i]._chanbuf_base     = chanbase;
                ti[i].chanbuf[0]        = &chanbase[0];
                ti[i].chanbuf[1]        = &chanbase[npx];
                ti[i].chanbuf[2]        = &chanbase[npx * 2];
                ti[i].wthird            = params->width / 3.0;
                ti[i].hthird            = params->height / 3.0;
                ti[i].bailsqu           = params->bailsqu;
                ti[i].formula           = params->formula;
                ti[i].line_x            = params->line_x;
                ti[i].line_y            = params->line_y;
                ti[i].use_line_x        = params->use_line_x;
                ti[i].use_line_y        = params->use_line_y;
                /*
                 * This initializes to different
                 * values for each set of seeds.
                 */
                initialize_seeds(ti[i].seeds);

                create_thread(&helper, ti, i);
        }

        join_threads(&helper, ti, nthread);

        /*
         * Sum the threads' results together.
         * They SHOULD have received different
         * rand() seeds, so the buffers SHOULD
         * all be different.
         */
        memset(sumbuf, 0, bufsize);
        for (i = 0; i < nthread; i++) {
                int j;
                unsigned long *chanbase = ti[i]._chanbuf_base;
                for (j = 0; j < npx * nchan; j++)
                        sumbuf[j] += chanbase[j];
                free(ti[i]._chanbuf_base);
        }
        free_thread_helper(&helper);
        free(ti);
}

static void
bbrot2(Pxbuf *pxbuf, struct params_t *params)
{
        int npx, nchan, i, row, col;
        unsigned long *sumbuf;
        /* Pointers into sumbuf, for convenience */
        unsigned long *chanbuf[3];

        nchan = params->singlechan ? 1 : 3;
        npx = params->width * params->height;
        sumbuf = malloc(sizeof(*sumbuf) * npx * nchan);
        if (!sumbuf)
                oom();

        bbrot2_get_data(params, sumbuf, nchan, npx);

        for (i = 0; i < nchan; i++)
                chanbuf[i] = &sumbuf[npx * i];

        /* Fill each pixel in pixelbuf */
        i = 0;
        for (row = 0; row < params->height; row++) {
                for (col = 0; col < params->width; col++) {
                        unsigned int r, g, b;
                        struct pixel_t px;
                        r = chanbuf[0][i];
                        if (nchan > 1) {
                                g = chanbuf[1][i];
                                b = chanbuf[2][i];
                        } else {
                                g = b = r;
                        }
                        i++;

                        px.x[PXBUF_RED]        = (float)r;
                        px.x[PXBUF_GREEN]      = (float)g;
                        px.x[PXBUF_BLUE]       = (float)b;

                        pxbuf_set_pixel(pxbuf, &px, row, col);
                }

        }

        free(sumbuf);
}

static const char *
parse_args(int argc, char **argv, struct params_t *params)
{
        static const struct option long_options[] = {
                { "xline",          required_argument, NULL, 1 },
                { "yline",          required_argument, NULL, 2 },
                { "equalize",       optional_argument, NULL, 3 },
                { "histogram",      optional_argument, NULL, 3 },
                { "negate",         no_argument,       NULL, 4 },
                { "formula",        required_argument, NULL, 5 },
                { "rmout",          optional_argument, NULL, 6 },
                { "nthread",        required_argument, NULL, 7 },
                { "overlay",        required_argument, NULL, 8 },
                { "linked",         no_argument,       NULL, 'l' },
                { "verbose",        no_argument,       NULL, 'v' },
                { "bailout",        required_argument, NULL, 'b' },
                { "help",           no_argument,       NULL, '?' },
                { NULL,             0,                 NULL, 0 },
        };
        static const char *optstr = "B:b:g:h:m:o:p:r:svw:";
        const char *outfile = "bbrot2.bmp";

        /* Set to initial values */
        params->n_red      = 5000;
        params->n_green    = 500;
        params->n_blue     = 50;
        params->height     = 600;
        params->width      = 600;
        params->min        = 3;
        params->linked     = false;
        params->nthread    = 4;
        params->bailsqu    = 4.0;
        params->bailout    = 2.0;
        params->points     = 500000;
        params->singlechan = false;
        params->do_hist    = false;
        params->verbose    = false;
        params->use_line_y = false;
        params->use_line_x = false;
        params->line_x     = 0.0;
        params->line_y     = 0.0;
        params->negate     = false;
        params->formula    = NULL;
        params->rmout_scale = 3.0;
        params->rmout      = false;
        params->eq_exp     = 5.0;
        params->overlay    = NULL;

        for (;;) {
                char *endptr;
                int option_index = 0;
                int opt = getopt_long(argc, argv, optstr,
                                        long_options, &option_index);
                if (opt == -1)
                        break;

                switch (opt) {
                case 1:
                        params->line_x = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("--xline", optarg);
                        params->use_line_x = true;
                        break;
                case 2:
                        params->line_y = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("--yline", optarg);
                        params->use_line_y = true;
                        break;
                case 3:
                        params->do_hist = true;
                        if (optarg != NULL) {
                                params->eq_exp = strtold(optarg, &endptr);
                                if (endptr == optarg) {
                                        bad_arg("--equalize,--histogram",
                                                optarg);
                                }
                        }
                        break;
                case 4:
                        params->negate = true;
                        break;
                case 5:
                    {
                        const struct formula_t *f;
                        f = parse_formula(optarg);
                        if (f == NULL)
                                bad_arg("--formula", optarg);
                        params->formula = f->fn;
                        break;
                    }
                case 6:
                        params->rmout = true;
                        if (optarg != NULL) {
                                params->rmout_scale = strtod(optarg, &endptr);
                                if (endptr == optarg)
                                        bad_arg("--rmout", optarg);
                        }
                        break;
                case 7:
                        params->nthread = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("--nthread", optarg);
                        /* warn user they're being stupid */
                        if (params->nthread > 20) {
                                fprintf(stderr, "%d threads! You cray!\n",
                                        params->nthread);
                        }
                        break;
                case 8:
                        params->overlay = optarg;
                        break;
                case 'B':
                        params->bailout = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-B --bailout", optarg);
                        params->bailsqu = params->bailout * params->bailout;
                        break;
                case 'b':
                        params->n_blue = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-b (blue-channel iterations)", optarg);
                        break;
                case 'g':
                        params->n_green = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-g (green-channel iterations", optarg);
                        break;
                case 'h':
                        params->height = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-h (pixel height)", optarg);
                        break;
                case 'l':
                        params->linked = true;
                        break;
                case 'm':
                        params->min = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-m (minimum iterations)", optarg);
                        break;
                case 'o':
                        outfile = optarg;
                        break;
                case 'p':
                        params->points = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-p (no. of points)", optarg);
                        break;
                case 'r':
                        params->n_red = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-r (red-channel iterations)", optarg);
                        break;
                case 's':
                        params->singlechan = true;
                        break;
                case 'v':
                        params->verbose = true;
                        break;
                case 'w':
                        params->width = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-w (pixel width)", optarg);
                        break;
                case '?':
                default:
                        fprintf(stderr, "Unknown option -%c\n", opt);
                        exit(EXIT_FAILURE);
                }
        }

        if (optind < argc) {
                fprintf(stderr, "Excess arguments beginning with `%s'\n",
                        argv[optind]);
                exit(EXIT_FAILURE);
        }

        if (!EGFRACTAL_MULTITHREADED)
                params->nthread = 1;

        /* One quick sanity check */
        if (params->min >= params->n_red) {
                fprintf(stderr, "min too high!\n");
                exit(EXIT_FAILURE);
        }

        if (!params->singlechan
            && (params->min >= params->n_green
                || params->min >= params->n_blue)) {
                fprintf(stderr, "min too high!\n");
                exit(EXIT_FAILURE);
        }

        return outfile;
}

static void
normalize(Pxbuf *pxbuf, struct params_t *params)
{
        enum pxbuf_norm_t method;
        if (params->rmout) {
                method = PXBUF_NORM_CROP;
        } else if (params->do_hist) {
                method = PXBUF_NORM_EQ;
        } else {
                method = PXBUF_NORM_SCALE;
        }

        pxbuf_normalize(pxbuf, method,
                        params->rmout_scale, params->linked);
}

int
main(int argc, char **argv)
{
        struct params_t params;
        FILE *fp;
        const char *outfile = parse_args(argc, argv, &params);
        Pxbuf *pxbuf, *p2 = NULL;
        double overlay_ratio = 1.0;

        if (params.overlay != NULL) {
                char *endptr;
                char *s = strchr(params.overlay, ',');
                if (s != NULL) {
                        *s++ = '\0';
                        overlay_ratio = strtod(s, &endptr);
                        if (errno || endptr == s) {
                                perror("Invalid ratio");
                                return 1;
                        }
                }
                fp = fopen(params.overlay, "rb");
                if (!fp) {
                        perror("Cannot open input file");
                        return 1;
                }
                p2 = pxbuf_read_from_bmp(fp);
                fclose(fp);
                if (!p2) {
                        perror("Cannot read input bitmap");
                        return 1;
                }

                normalize(p2, &params);
                if (params.negate)
                        pxbuf_negate(p2);

                /*
                 * Overwrite params with overlay's dimensions.
                 * If a user's trying to be fancy by overlaying
                 * a dissimilar image, they can use Photoshop.
                 */
                pxbuf_get_dimensions(p2, &params.width, &params.height);
        }

        pxbuf = pxbuf_create(params.width, params.height);
        if (!pxbuf)
                oom();

        bbrot2(pxbuf, &params);

        fp = fopen(outfile, "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }

        normalize(pxbuf, &params);
        if (params.negate)
                pxbuf_negate(pxbuf);

        pxbuf_rotate(pxbuf, false);
        if (p2)
                pxbuf_overlay(pxbuf, p2, overlay_ratio);
        pxbuf_print_to_bmp(pxbuf, fp, PXBUF_NORM_CLIP);
        fclose(fp);

        pxbuf_destroy(pxbuf);
        return 0;
}
