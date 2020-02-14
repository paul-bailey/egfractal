/*
 * bbrot1 - option set 1 for Buddhabrot
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
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <getopt.h>

static struct gbl_t {
        int n_red;
        int n_green;
        int n_blue;
        int height;
        int width;
        int min;
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
} gbl = {
        .n_red      = 5000,
        .n_green    = 500,
        .n_blue     = 50,
        .height     = 600,
        .width      = 600,
        .min        = 3,
        .bailsqu    = 4.0,
        .bailout    = 2.0,
        .points     = 500000,
        .singlechan = false,
        .do_hist    = false,
        .verbose    = false,
        .use_line_y = false,
        .use_line_x = false,
        .negate     = false,
        .formula    = NULL,
        .rmout_scale = 3.0,
        .rmout      = false,
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

/* These are set before all the iterations */
static mfloat_t wthird, hthird;

static inline void __attribute__((always_inline))
save_to_hist(unsigned long *buf, complex_t c)
{
        /*
         * REVISIT: Somehow leaving the "+0.5" in
         * increases the speed of this operation.
         * That's great, but why?
         */
        unsigned int col = (int)(wthird * (c.re + 2.0) + 0.5);
        unsigned int row = (int)(hthird * (c.im + 1.5) + 0.5);
        if (col < gbl.width && row < gbl.height)
                buf[row * gbl.width + col]++;
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
iterate_r(complex_t c, unsigned long *buf, int n, bool isdivergent)
{
        int i;
        int min = gbl.min;
        complex_t z = { .re = 0.0L, .im = 0.0L };
        /*
         * Separating this before the for loop, in case it helps
         * with any inline-optimizations.
         */
        if (gbl.formula) {
                for (i = 0; i < n; i++) {
                        complex_t ztmp = gbl.formula(z, c);
                        if (isdivergent && i > min)
                                save_to_hist(buf, ztmp);

                        /* Check both bailout and periodicity */
                        if (ztmp.re == z.re && ztmp.im == z.im)
                                return;
                        if (complex_modulus2(ztmp) >= gbl.bailsqu) {
                                if (!isdivergent)
                                        iterate_r(c, buf, n, true);
                                return;
                        }

                        z = ztmp;
                }
        } else {
                for (i = 0; i < n; i++) {
                        /* next z = z^2 + c */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        if (isdivergent && i > min)
                                save_to_hist(buf, ztmp);

                        /* Check both bailout and periodicity */
                        if (complex_modulus2(ztmp) >= gbl.bailsqu
                            || (ztmp.re == z.re && ztmp.im == z.im)) {
                                if (!isdivergent)
                                        iterate_r(c, buf, n, true);
                                return;
                        }

                        z = ztmp;
                }
        }
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
shave_outliers(unsigned long *buf, unsigned int npx)
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
        stdmin = (unsigned long)(mean - gbl.rmout_scale * stddev);
        stdmax = (unsigned long)(mean + gbl.rmout_scale * stddev);

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

        assert(stdmax > stdmin);

        for (i = 0; i < npx; i++) {
                if (buf[i] < stdmin) {
                        buf[i] = stdmin;
                } else if (buf[i] > stdmax) {
                        buf[i] = stdmax;
                }
        }
}

static void
normalize(unsigned long *buf, unsigned int npx, unsigned long new_max)
{
        int i;
        unsigned int max;

        if (gbl.rmout)
                shave_outliers(buf, npx);
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
        time_t t = time(NULL);
        seeds[0] = seeds[3] = t & 0xffffu;
        seeds[1] = seeds[4] = (t >> 16) & 0xffffu;
        /*
         * These need to be different to converge x from y,
         * or else x == y always, and we are only ever sampling
         * the diagonal line.  This makes for a trippy picture,
         * but not the one we're looking for.
         */
        seeds[2] = 5;
        seeds[5] = 6;
}

static void
bbrot1(Pxbuf *pxbuf)
{
        unsigned short seeds[6];
        int n[3] = { gbl.n_red, gbl.n_green, gbl.n_blue };
        int row, col, npct, nchan;
        unsigned long i; /* must be wide as gbl.points */
        unsigned long onepct = gbl.points / 100;
        unsigned long pctcount;
        unsigned long *buffer;
        unsigned long *chanbuf[3];
        /* Since I use this all over */
        unsigned int npx =  gbl.width * gbl.height;

        /* just because we know it now and it will never change */
        wthird = gbl.width / 3.0;
        hthird = gbl.height / 3.0;

        initialize_seeds(seeds);

        nchan = gbl.singlechan ? 1 : 3;

        buffer = malloc(npx * sizeof(*buffer) * nchan);
        if (!buffer)
                oom();
        memset(buffer, 0, npx * sizeof(*buffer) * nchan);
        for (i = 0; i < nchan; i++)
                chanbuf[i] = &buffer[npx * i];

        pctcount = npct = 0;
        if (gbl.verbose)
                printf("Progress     ");
        for (i = 0; i < gbl.points; i++) {
                complex_t c;
                int chan;

                if (gbl.use_line_x)
                        c.re = gbl.line_x;
                else
                        c.re = erand48(&seeds[0]) * 3.0 - 2.0;
                if (gbl.use_line_y)
                        c.im = gbl.line_y;
                else
                        c.im = erand48(&seeds[3]) * 3.0 - 1.5;
                if (gbl.verbose && (pctcount++ == onepct)) {
                        npct++;
                        pctcount = 0;
                        /*
                         * Year 2018.  If your terminal doesn't support
                         * ANSI escape sequences then you probably also
                         * haven't upgraded to a horseless carriage.
                         */
                        printf("\e[3D%3d", npct);
                        fflush(stdout);
                }
                if (!gbl.formula && inside_cardioid_or_bulb(c))
                        continue;
                for (chan = 0; chan < nchan; chan++)
                        iterate_r(c, chanbuf[chan], n[chan], false);
        }
        if (gbl.verbose)
                putchar('\n');

        for (i = 0; i < nchan; i++)
                normalize(chanbuf[i], npx, 255);

        i = 0;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
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
        if (gbl.do_hist)
                pxbuf_eq(pxbuf, gbl.eq_exp, true);
        if (gbl.negate)
                pxbuf_negate(pxbuf);
        free(buffer);
}

static const char *
parse_args(int argc, char **argv)
{
        static const struct option long_options[] = {
                { "xline",          required_argument, NULL, 1 },
                { "yline",          required_argument, NULL, 2 },
                { "equalize",       optional_argument, NULL, 3 },
                { "histogram",      optional_argument, NULL, 3 },
                { "negate",         no_argument,       NULL, 4 },
                { "formula",        required_argument, NULL, 5 },
                { "rmout",          optional_argument, NULL, 6 },
                { "verbose",        no_argument,       NULL, 'v' },
                { "bailout",        required_argument, NULL, 'b' },
                { "help",           no_argument,       NULL, '?' },
                { NULL,             0,                 NULL, 0 },
        };
        static const char *optstr = "B:b:g:h:m:o:p:r:svw:";
        const char *outfile = "buddhabrot1.bmp";

        for (;;) {
                char *endptr;
                int option_index = 0;
                int opt = getopt_long(argc, argv, optstr, long_options, &option_index);
                if (opt == -1)
                        break;

                switch (opt) {
                case 1:
                        gbl.line_x = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("--xline", optarg);
                        gbl.use_line_x = true;
                        break;
                case 2:
                        gbl.line_y = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("--yline", optarg);
                        gbl.use_line_y = true;
                        break;
                case 3:
                        gbl.do_hist = true;
                        /* FIXME: optarg is NULL whether there's an arg or not. */
                        if (optarg != NULL) {
                                gbl.eq_exp = strtold(optarg, &endptr);
                                if (endptr == optarg)
                                        bad_arg("--equalize,--histogram", optarg);
                        } else {
                                gbl.eq_exp = 5.0;
                        }
                        break;
                case 4:
                        gbl.negate = true;
                        break;
                case 5:
                    {
                        const struct formula_t *f;
                        f = parse_formula(optarg);
                        if (f == NULL)
                                bad_arg("--formula", optarg);
                        gbl.formula = f->fn;
                        /* buddhabrot doesn't need derivative or order. */
                        break;
                    }
                case 6:
                        gbl.rmout = true;
                        if (optarg != NULL) {
                                gbl.rmout_scale = strtod(optarg, &endptr);
                                if (endptr == optarg)
                                        bad_arg("--rmout", optarg);
                        }
                        break;
                case 'B':
                        gbl.bailout = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-B --bailout", optarg);
                        gbl.bailsqu = gbl.bailout * gbl.bailout;
                        break;
                case 'b':
                        gbl.n_blue = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-b (blue-channel iterations)", optarg);
                        break;
                case 'g':
                        gbl.n_green = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-g (green-channel iterations", optarg);
                        break;
                case 'h':
                        gbl.height = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-h (pixel height)", optarg);
                        break;
                case 'm':
                        gbl.min = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-m (minimum iterations)", optarg);
                        break;
                case 'o':
                        outfile = optarg;
                        break;
                case 'p':
                        gbl.points = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-p (no. of points)", optarg);
                        break;
                case 'r':
                        gbl.n_red = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-r (red-channel iterations)", optarg);
                        break;
                case 's':
                        gbl.singlechan = true;
                        break;
                case 'v':
                        gbl.verbose = true;
                        break;
                case 'w':
                        gbl.width = strtoul(optarg, &endptr, 0);
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
        if (gbl.min >= gbl.n_red) {
                fprintf(stderr, "min too high!\n");
                exit(EXIT_FAILURE);
        }

        if (!gbl.singlechan && (gbl.min >= gbl.n_green || gbl.min >= gbl.n_blue)) {
                fprintf(stderr, "min too high!\n");
                exit(EXIT_FAILURE);
        }

        return outfile;
}

int
main(int argc, char **argv)
{
        FILE *fp;
        const char *outfile = parse_args(argc, argv);
        Pxbuf *pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_WHITE);
        if (!pxbuf)
                oom();

        bbrot1(pxbuf);

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
