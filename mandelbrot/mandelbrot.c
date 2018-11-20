#include "mandelbrot.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* scale pixels to points of mandelbrot set and handle zoom. */
static void
x0y0(double *x0, double *y0, int row, int col)
{
        double x, y, w, h;

        /*
         * Swapping col with row here because this
         * is a**-backwards, too.
         */
        x = (double)col;
        y = (double)row;
        w = (double)gbl.width;
        h = (double)gbl.height;

        x *= gbl.zoom_pct;
        y *= gbl.zoom_pct;

        x = 3.5 * x / w - 2.5;
        y = 2.5 * y / h - 1.25;

        x += gbl.zoom_xoffs;
        y += gbl.zoom_yoffs;

        *x0 = x;
        *y0 = y;
}

/* Returns position from 0 to n, normalized to [0, 1.0) */
static double
mandelbrot_helper(int row, int col, int n)
{
        int i;
        double ret;
        double x0, y0;
        double x = 0.0;
        double y = 0.0;

        x0y0(&x0, &y0, row, col);

        for (i = 0; i < n; i++) {
                if ((x * x + y * y) >= (double)(1u << 16))
                        break;
                double tmp = x * x - y * y + x0;
                y = 2 * x * y + y0;
                x = tmp;
        }

        /* Used to smooth out color gradients */
        ret = (double)i;
        if (i < n) {
                /*
                 * XXX: It seems both of these algorithms are necessary.
                 * But that doesn't really make sense...
                 */
                if (gbl.dither) {
                        /* dither */
                        int v = rand();
                        if (v < 0)
                                v = -1 * (v & 0xff);
                        else
                                v &= 0xff;
                        double diff = (double)v / 128.0;
                        ret += diff;
                }

                if (gbl.dither) {
                        /* Smooth by distance estimate */
                        double dummy = 0;
                        double log_zn = log2(x * x + y * y) / 2.0;
                        double log_2 = log2(2.0);
                        double nu = log2(log_zn / log_2) / log_2;
                        nu = modf(nu, &dummy);
                        (void)dummy;
                        ret += 1.0 - nu;
                }

                if (ret >= (double)n)
                        ret = (double)(n - 1);
        }

        return ret;
}

void
mandelbrot24(unsigned char *buf, int n_iteration)
{
        int row;
        int height = gbl.height;
        int width = gbl.width;
        unsigned long total;
        unsigned char *pbuf;
        double *tbuf, *ptbuf;
        unsigned long *histogram;
        void (*pallette_init)(void);
        void (*pallette_rgb)(struct rgb_t *, double, int);

        switch (gbl.pallette) {
        case 1:
                pallette_init = pallette_1_init;
                pallette_rgb  = pallette_1_rgb;
                break;
        case 2:
                pallette_init = pallette_2_init;
                pallette_rgb  = pallette_2_rgb;
                break;
        case 3:
                pallette_init = pallette_3_init;
                pallette_rgb  = pallette_3_rgb;
                break;
        default:
                assert(0);
                exit(EXIT_FAILURE);
                break;
        }

	pallette_init();

        histogram = malloc(n_iteration * sizeof(*histogram));
        if (!histogram) {
                fprintf(stderr, "OOM!\n");
                exit(EXIT_FAILURE);
        }
        tbuf = malloc(width * height * sizeof(*tbuf));
        if (!tbuf) {
                fprintf(stderr, "OOM!\n");
                exit(EXIT_FAILURE);
        }
        memset(histogram, 0, n_iteration * sizeof(*histogram));

        ptbuf = tbuf;
        total = 0;
        for (row = 0; row < height; row++) {
                int col;
                for (col = 0; col < width; col++) {
                        int i = mandelbrot_helper(row, col, n_iteration);
                        histogram[i]++;
                        total += i;
                        *ptbuf++ = (double)i;
                }
        }

        /* Bitmap files are a**-backwards */
        pbuf = buf + gbl.bufsize - 1;
        ptbuf = tbuf;
        for (row = 0; row < height; row++) {
                int col;
                for (col = 0; col < width; col++) {
                        double i = *ptbuf++;
			struct rgb_t rgb;
                        i += ((double)histogram[(int)i] / (double)total + 0.5);
			pallette_rgb(&rgb, i, n_iteration);
			*pbuf-- = rgb.red;
			*pbuf-- = rgb.green;
			*pbuf-- = rgb.blue;
                }
        }
        free(histogram);
        free(tbuf);
}


