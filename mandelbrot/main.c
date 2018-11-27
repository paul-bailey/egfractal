#include "mandelbrot_common.h"
#include "fractal_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <string.h>

enum {
        TRACK_PROGRESS = 0,
};

struct gbl_t gbl = {
        .pxbuf          = NULL,
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
};

/* Initialized to log2l(2.0L) */
static mfloat_t log_2;
static const mfloat_t INSIDE = -1.0L;


/* **********************************************************************
 *                           Error helpers
 ***********************************************************************/

static void
usage(void)
{
        fprintf(stderr, "Bad arg\n");
        exit(EXIT_FAILURE);
}

static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(EXIT_FAILURE);
}


/* **********************************************************************
 *                      The algorithm
 ***********************************************************************/

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

        if (i == n)
                return INSIDE;

        /* i < n from here */
        ret = (mfloat_t)i;
        if (gbl.dither > 0) {
                if (!!(gbl.dither & 01)) {
                        /* Smooth with distance estimate */
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

        /* Return distance normalized to the colorspace */
        return complex_modulus(z) * logl(complex_modulus(z)) / complex_modulus(dz);
}

static mfloat_t
mandelbrot_px(int row, int col)
{
        /* XXX: Quite an arbitrary choice */
        enum { THRESHOLD = 10 };

        complex_t c = xy_to_complex(row, col);
        if (gbl.n_iteration > THRESHOLD) {
                /*
                 * Faster to do this and throw away pixels that are in
                 * the main cardioid and circle.
                 *
                 * XXX: Is the 0.25 meant to be sqrt(bailout)? I forget.
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
mandelbrot(void)
{
        /* TODO: Determine here if out zoomed image touches the
         * cardioid or largest circle.  That way we won't have the
         * additional check in the iterative algorithm.
         */
        int row, col;
        mfloat_t *ptbuf, *tbuf, min, max;

        tbuf = malloc(gbl.width * gbl.height * sizeof(*tbuf));
        if (!tbuf)
                oom();

        if (TRACK_PROGRESS) {
                printf("Row %9d col %9d", 0, 0);
                fflush(stdout);
        }
        ptbuf = tbuf;
        min = 1.0e16;
        max = 0.0;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        mfloat_t v;
                        if (TRACK_PROGRESS) {
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
        if (TRACK_PROGRESS)
                putchar('\n');
        printf("min: %Lg max: %Lg\n", (long double)min, (long double)max);
        ptbuf = tbuf;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        unsigned int color = get_color(*ptbuf++, min, max);
                        pxbuf_fill_pixel(gbl.pxbuf, row, col, color);
                }
        }
        free(tbuf);
}

int
main(int argc, char **argv)
{
        const char *outfile = "mandelbrot.bmp";
        int opt;
        char *endptr;
        FILE *fp;
        bool print_palette_only = false;

        /* need to set these "consts" first */
        log_2 = logl(2.0L);

        while ((opt = getopt(argc, argv, "Pp:d:Db:n:h:w:y:x:z:o:")) != -1) {
                switch (opt) {
                case 'P':
                        print_palette_only = true;
                        break;
                case 'p':
                        gbl.palette = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'z':
                        gbl.zoom_pct = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'x':
                        gbl.zoom_xoffs = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'y':
                        gbl.zoom_yoffs = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'w':
                        gbl.width = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'h':
                        gbl.height = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'n':
                        gbl.n_iteration = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'o':
                        outfile = optarg;
                        break;
                case 'b':
                        gbl.bailout = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        gbl.bailoutsqu = gbl.bailout * gbl.bailout;
                        break;
                case 'd':
                        gbl.dither = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'D':
                        gbl.distance_est = true;
                        break;
                default:
                        usage();
                }
        }
        gbl.pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_WHITE);
        if (!gbl.pxbuf)
                oom();

        /*
         * Do this before fopen(), because we could be here for a very
         * time, and it's impolite to have a file open for that long.
         */
        if (!print_palette_only)
                mandelbrot();

        fp = fopen(outfile, "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }

        if (print_palette_only)
                print_palette();
        pxbuf_print(gbl.pxbuf, fp);
        fclose(fp);
        pxbuf_free(gbl.pxbuf);
        return 0;
}

