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
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>

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
        complex_t (*formula)(complex_t, complex_t);
};

struct thread_info_t {
        int width;
        int height;
        int nchan;
        int min;
        unsigned long points;
        int n[3];
        unsigned long *_chanbuf_base;
        unsigned long *chanbuf[3];
        unsigned short seeds[6];
        complex_t (*formula)(complex_t, complex_t);
        mfloat_t wthird;
        mfloat_t hthird;
        mfloat_t bailsqu;
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

static inline void __attribute__((always_inline))
save_to_hist(struct thread_info_t *ti, int chan, complex_t c)
{
        unsigned int col = (int)(ti->wthird * (c.re + 2.0) + 0.5);
        unsigned int row = (int)(ti->hthird * (c.im + 1.5) + 0.5);
        if (col < ti->width && row < ti->height)
                ti->chanbuf[chan][row * ti->width + col]++;
}

/* Return true if inside cardioid or main bulb */
static bool
inside_cardioid_or_bulb(complex_t c)
{
        mfloat_t xp = c.re - 0.25L;
        mfloat_t ysq = c.im * c.im;
        mfloat_t q = xp * xp + ysq;
        if ((q * (q + xp)) < (0.25L * ysq))
                return true;
        xp = c.re + 1.0;
        if ((xp * xp + ysq) < (0.25L * ysq))
                return true;
        return false;
}

/*
 * This matters most when running histogram equalization
 * FIXME: This is only useful for *local* pixel regions.
 */
static void
shave_outliers(unsigned long *buf, unsigned int npx, double rmout_scale)
{
        double mean;
        double sumsq;
        double stddev;
        unsigned long sum, stdmin, stdmax, max;
        int i;
        for (max = 0, sum = 0, i = 0; i < npx; i++) {
                sum += buf[i];
                if (max < buf[i])
                        max = buf[i];
        }
        mean = (double)sum / (double)npx;
        for (sumsq = 0.0, i = 0; i < npx; i++) {
                double diff = (double)buf[i] - mean;
                sumsq += diff * diff;
        }
        /* "n" instead of "n-1", because we have the whole population */
        stddev = sqrt(sumsq / (double)npx);

        /* define "outlier as 3 time the standard deviation */
        stdmin = (unsigned long)(mean - rmout_scale * stddev);
        stdmax = (unsigned long)(mean + rmout_scale * stddev);

        /*
         * We might not be following normal distribution,
         * so make sure we don't go below zero.
         */
        if ((long)stdmin < 0) {
                /*
                 * XXX REVISIT: Should stdmax be brought in
                 * by the same amount?
                 */
                stdmin = 0;
        }
        if (stdmax > max)
                stdmax = max;

        assert(stdmax >= stdmin);

        for (i = 0; i < npx; i++) {
                if (buf[i] < stdmin) {
                        buf[i] = stdmin;
                } else if (buf[i] > stdmax) {
                        buf[i] = stdmax;
                }
        }
}

static void
normalize(struct params_t *params, unsigned long *buf,
          unsigned int npx, unsigned long new_max)
{
        int i;
        unsigned int max;

        if (params->rmout)
                shave_outliers(buf, npx, params->rmout_scale);
        for (max = 0L, i = 0; i < npx; i++) {
                if (max < buf[i])
                        max = buf[i];
        }
        if (max == 0)
                return;
        for (i = 0; i < npx; i++) {
                /* XXX arbitrary division */
                buf[i] = buf[i] * (new_max+1) / max;
                if (buf[i] > new_max)
                        buf[i] = new_max;
        }
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

/*
 * This function runs twice - First just to see if it diverges
 * and again to save the points of the path from @c if it does
 * in fact diverge.
 *
 * Repeating this long iterative process twice sounds like it takes
 * a long time, and it does.  One alternative is to save everything
 * into the histogram @buf and save all the same points in a second
 * buffer, so that we can undo our modification of @buf should the
 * path not diverge.  However, this has experimentally proven to
 * take much longer.  It turns out that the process of saving data
 * into @hist (which includes the slightly mathy save_to_hist() above)
 * is time-consuming, and it's just faster if we don't do that unless
 * we already know the path diverges.
 *
 * TODO: Another option is to have a number of threads that run on
 * different CPUs, each with its own copy of @buf, running this algo
 * on its own subset of the plane (ie. its own series of @c arguments),
 * and when all the threads are complete, sum all the threads' @buf's
 * together.  It may not be useful on a quad-core Hewlett Crappard,
 * but a more powerful computer with lots of cores and lots of RAM
 * would benefit greatly from that.
 */
static void
iterate_r(complex_t c, unsigned int chan,
                struct thread_info_t *ti, bool isdivergent)
{
        int i;
        complex_t z = { .re = 0.0L, .im = 0.0L };
        if (ti->formula) {
                for (i = 0; i < ti->n[chan]; i++) {
                        complex_t ztmp = ti->formula(z, c);
                        if (isdivergent && i > ti->min)
                                save_to_hist(ti, chan, ztmp);

                        /* Check both bailout and periodicity */
                        if (ztmp.re == z.re && ztmp.im == z.im)
                                return;
                        if (complex_modulus2(ztmp) >= ti->bailsqu) {
                                if (!isdivergent)
                                        iterate_r(c, chan, ti, true);
                                return;
                        }

                        z = ztmp;
                }
        } else {
                for (i = 0; i < ti->n[chan]; i++) {
                        /* next z = z^2 + c */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        if (isdivergent && i > ti->min)
                                save_to_hist(ti, chan, ztmp);

                        /* Check both bailout and periodicity */
                        if (complex_modulus2(ztmp) >= ti->bailsqu
                            || (ztmp.re == z.re && ztmp.im == z.im)) {
                                if (!isdivergent)
                                        iterate_r(c, chan, ti, true);
                                return;
                        }

                        z = ztmp;
                }
        }
}

/* NORM3 converts result of rand48_ll to some point in [0:3) */
#define NORM3  (3.0 / (double)MASK48)
#define MASK48 (((uint64_t)1 << 48) - 1)

/*
 * My own inline erand48().
 *
 * Besides removing the overhead of a function call for every point,
 * I also reduce a mfloat_t multiplication, because I can directly
 * "normalize" to [0:3) instead of multiplying erand48()'s already-
 * normalized-to-[0:1) result.
 *
 * XXX REVISIT: Multiplying a 48-bit value with a 32-bit value will
 * cause overflow in a 64-bit word.
 * Will the result of an overflowing multiplication
 * always be modulo the correct answer, on any architecture?
 * If yes, then I don't care.
 * If no, then I need to handle this with 80-bit storage.
 */
static inline __attribute__((always_inline)) uint64_t
rand48_il(uint64_t old)
{
        /* Formula given by man (3) erand48 */
        return (old * 0x5DEECE66Dul + 0xB) & MASK48;
}

static void *
bbrot_thread(void *arg)
{
        uint64_t s48_x, s48_y;
        struct thread_info_t *ti = (struct thread_info_t *)arg;
        unsigned long i;

        s48_x = (uint64_t)ti->seeds[0] << 32
                | (uint64_t)ti->seeds[1] << 16 | (uint64_t)ti->seeds[2];
        s48_y = (uint64_t)ti->seeds[3] << 32
                | (uint64_t)ti->seeds[4] << 16 | (uint64_t)ti->seeds[5];

        for (i = 0; i < ti->points; i++) {
                complex_t c;
                int chan;

                s48_x = rand48_il(s48_x);
                s48_y = rand48_il(s48_y);
                c.re = (double)s48_x * NORM3 - 2.0;
                c.im = (double)s48_y * NORM3 - 1.5;
                if (!ti->formula && inside_cardioid_or_bulb(c))
                        continue;
                for (chan = 0; chan < ti->nchan; chan++) {
                        iterate_r(c, chan, ti, false);
                }
        }
        return NULL;
}

static void
bbrot2_get_data(struct params_t *params, unsigned long *sumbuf,
                int nchan, int npx)
{
        struct thread_info_t *ti;
        pthread_t *id;
        pthread_attr_t attr;
        int nthread = params->nthread;
        size_t bufsize = sizeof(unsigned long) * npx * nchan;
        int i, res;

        id = malloc(sizeof(*id) * nthread);
        ti = malloc(sizeof(*ti) * nthread);
        if (!ti || !id)
                oom();

        res = pthread_attr_init(&attr);
        if (res != 0) {
                perror("pthread_attr_init error");
                exit(EXIT_FAILURE);
        }
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
                /*
                 * This initializes to different
                 * values for each set of seeds.
                 */
                initialize_seeds(ti[i].seeds);

                res = pthread_create(&id[i], &attr,
                                &bbrot_thread, &ti[i]);
                if (res != 0) {
                        perror("pthread_create error");
                        exit(EXIT_FAILURE);
                }
        }

        res = pthread_attr_destroy(&attr);
        if (res != 0) {
                perror("pthread_attr_destroy error");
                exit(EXIT_FAILURE);
        }
        for (i = 0; i < nthread; i++) {
                void *s;
                int res = pthread_join(id[i], &s);
                if (res != 0 || s != NULL) {
                        fprintf(stderr, "pthread_join[%d] failed (%s)\n",
                                i, strerror(errno));
                        fprintf(stderr, "Continuing anyway\n");
                }
        }

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
        free(id);
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
                normalize(params, &sumbuf[npx * i], npx, 255);

        for (i = 0; i < nchan; i++)
                chanbuf[i] = &sumbuf[npx * i];

        /* Fill each pixel in pixelbuf */
        i = 0;
        for (row = 0; row < params->height; row++) {
                for (col = 0; col < params->width; col++) {
                        unsigned int r, g, b;
                        r = chanbuf[0][i];
                        if (nchan > 1) {
                                g = chanbuf[1][i];
                                b = chanbuf[2][i];
                        } else {
                                g = b = r;
                        }
                        i++;

                        /*
                         * Some minor peaking (==256) could occur,
                         * but there should be no overshoot
                         */
                        assert(r <= 256 && g <= 256 && b <= 256);
                        if (r > 255)
                                r = 255;
                        if (g > 255)
                                g = 255;
                        if (b > 255)
                                b = 255;
                        pxbuf_fill_pixel(pxbuf, row, col, TO_RGB(r, g, b));
                }

        }

        /*
         * XXX REVISIT: equalization looks terribly grainy if done
         * after `normalizing' the array to [0:255].  Find a better
         * equalization routine while the array is still [0:#hits].
         */
        if (params->do_hist)
                pxbuf_eq(pxbuf, params->eq_exp, true);
        if (params->negate)
                pxbuf_negate(pxbuf);

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
        params->nthread    = 4;
        params->bailsqu    = 4.0;
        params->bailout    = 2.0;
        params->points     = 500000;
        params->singlechan = false;
        params->do_hist    = false;
        params->verbose    = false;
        params->use_line_y = false;
        params->use_line_x = false;
        params->negate     = false;
        params->formula    = NULL;
        params->rmout_scale = 3.0;
        params->rmout      = false;
        params->eq_exp     = 5.0;

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

int
main(int argc, char **argv)
{
        struct params_t params;
        FILE *fp;
        const char *outfile = parse_args(argc, argv, &params);
        Pxbuf *pxbuf = pxbuf_create(params.width, params.height, COLOR_WHITE);
        if (!pxbuf)
                oom();

        bbrot2(pxbuf, &params);

        fp = fopen(outfile, "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }
        pxbuf_rotate(pxbuf);
        pxbuf_print(pxbuf, fp);
        fclose(fp);

        pxbuf_free(pxbuf);
        return 0;
}
