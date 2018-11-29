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
        unsigned long points;
        bool singlechan;
        bool do_hist;
        bool verbose;
        bool use_line_y;
        bool use_line_x;
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
save_to_hist(unsigned long *buf, complex_t c)
{
        static const mfloat_t THIRD = 1.0 / 3.0;

        unsigned int col = (int)((mfloat_t)gbl.width * ((c.re + 2.0) * THIRD) + 0.5);
        unsigned int row = (int)((mfloat_t)gbl.height * ((c.im + 1.5) * THIRD) + 0.5);
        if (col < gbl.width && row < gbl.height)
                buf[row * gbl.width + col]++;
}

static void
iterate_r(complex_t c, unsigned long *buf, int n, bool isdivergent)
{
        int i;
        int min = gbl.min;
        complex_t z = { .re = 0.0L, .im = 0.0L };
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

static void
iterate(complex_t c, unsigned long *buf, int n)
{
        if (inside_cardioid_or_bulb(c))
                return;
        iterate_r(c, buf, n, false);
}

static void
normalize(unsigned long *buf, unsigned int npx, unsigned long new_max)
{
        int i;
        unsigned int max = 0L;
        for (i = 0; i < npx; i++) {
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
                        printf("\e[3D%3d", npct);
                        fflush(stdout);
                }
                for (chan = 0; chan < nchan; chan++)
                        iterate(c, chanbuf[chan], n[chan]);
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
        if (gbl.do_hist)
                pxbuf_eq(pxbuf, gbl.eq_exp, true);
        free(buffer);
}

static const char *
parse_args(int argc, char **argv)
{
        static const struct option long_options[] = {
                { "xline",          required_argument, NULL, 1 },
                { "yline",          required_argument, NULL, 2 },
                { "equalize",       optional_argument, NULL, 3 },
                { "histogram",      optional_argument, NULL, 4 },
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
                int this_option_optind = optind ? optind : 1;
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
