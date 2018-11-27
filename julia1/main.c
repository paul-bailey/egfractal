#include "julia1_common.h"
#include "fractal_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <string.h>

static const mfloat_t pi = 3.14159265359;

struct gbl_t gbl = {
        .pxbuf = NULL,
        .n_iteration = 1000,
        .dither = false,
        .height = 600,
        .width = 600,
        .pallette = 2,
        .zoom_pct = 1.0,
        .zoom_xoffs = 0.0,
        .zoom_yoffs = 0.0,
        .cx = -0.70176,
        .cy = -0.3842,
};

/* Error helpers */
static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(EXIT_FAILURE);
}

static void
usage(void)
{
        fprintf(stderr, "Bad arg\n");
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
julia_px(int row, int col)
{
        mfloat_t ret;
        unsigned long i;
        complex_t z;
        complex_t c = { .re = gbl.cx, .im = gbl.cy };

        z = xy_to_complex(row, col);
        for (i = 0; i < gbl.n_iteration; i++) {
                if (complex_modulus2(z) >= 4.0)
                        break;
                /* "z = z^2 + c */
                z = complex_add(complex_sq(z), c);
        }

        if (i == gbl.n_iteration)
                return INSIDE;

        /* TODO: Dither here */
        ret = (mfloat_t)i;
        if (i < gbl.n_iteration) {
                if (gbl.dither) {
                        /* Smooth by dithering */
                        int v = rand();
                        if (v < 0)
                                v = -1 * (v & 0xff);
                        else
                                v &= 0xff;
                        mfloat_t diff = (mfloat_t)v / 128.0;
                        ret += diff;
                }

                /* TODO: Smooth by distance */
                if (gbl.dither) {
                        mfloat_t log_zn = log10(complex_modulus2(z)) / 2.0;
                        mfloat_t log_2 = log10(2.0);
                        mfloat_t nu = log10(log_zn / log_2) / log_2;
                        ret += 1.0 - nu;
                }

                if (ret >= (mfloat_t)gbl.n_iteration)
                        ret = (mfloat_t)(gbl.n_iteration - 1);
                else if (ret < 0.0)
                        ret = 0.0;
        }
        return ret;
}

static void
julia(void)
{
        int row, col;
        unsigned long total;
        unsigned long *histogram;
        mfloat_t *ptbuf, *tbuf;
        histogram = malloc(gbl.n_iteration * sizeof(*histogram));
        if (!histogram)
                oom();
        tbuf = malloc(gbl.width * gbl.height * sizeof(*tbuf));
        if (!tbuf)
                oom();
        memset(histogram, 0, gbl.n_iteration * sizeof(*histogram));
        total = 0;

        ptbuf = tbuf;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        mfloat_t i = julia_px(row, col);
                        histogram[(int)i]++;
                        total += (int)i;
                        *ptbuf++ = i;
                }
        }
        ptbuf = tbuf;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        unsigned int color;
                        mfloat_t i = *ptbuf++;
                        i += ((mfloat_t)histogram[(int)i] / (mfloat_t)total + 0.5);
                        color = get_color(i);
                        pxbuf_fill_pixel(gbl.pxbuf, row, col, color);
                }
        }
}

int
main(int argc, char **argv)
{
        FILE *fp;
        char *endptr;
        int opt;
        const char *outfile = "julia1.bmp";
        while ((opt = getopt(argc, argv, "dz:x:y:w:h:n:R:I:p:o:")) != -1) {
                switch (opt) {
                case 'd':
                        gbl.dither = true;
                        break;
                case 'z':
                        gbl.zoom_pct = strtod(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'x':
                        gbl.zoom_xoffs = strtod(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'y':
                        gbl.zoom_yoffs = strtod(optarg, &endptr);
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
                case 'p':
                        gbl.pallette = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                usage();
                        break;
                case 'R': /* Real part of c */
                        gbl.cx = strtod(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        if (endptr[0] == 'p' && endptr[1] == 'i')
                                gbl.cx *= pi;
                        break;
                case 'I': /* Imaginary part of c */
                        gbl.cy = strtod(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        if (endptr[0] == 'p' && endptr[1] == 'i')
                                gbl.cy *= pi;
                        break;
                default:
                        usage();
                }
        }
        gbl.pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_BLACK);
        if (!gbl.pxbuf) {
                fprintf(stderr, "OOM!\n");
                return 1;
        }

        fp = fopen(outfile, "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }
        julia();
        pxbuf_print(gbl.pxbuf, fp);
        fclose(fp);
        pxbuf_free(gbl.pxbuf);
        return 0;
}
