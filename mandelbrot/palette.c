#include "mandelbrot_common.h"
#include "fractal_common.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

enum {
        NCOLOR = 128,
};

static unsigned int palette[NCOLOR];
#define NO_COLOR ((unsigned int)~0ul)
static unsigned int inside_color = NO_COLOR;

static unsigned int
interp_helper(unsigned int color1, unsigned int color2, mfloat_t frac)
{
        return color1 + (int)(frac * ((mfloat_t)color2 - (mfloat_t)color1) + 0.5);
}

static void
channelize(unsigned int color, unsigned int *r, unsigned int *g, unsigned int *b)
{
        *r = (color >> 16) & 0xffu;
        *g = (color >> 8) & 0xffu;
        *b = color & 0xffu;
}

static unsigned int
linear_interp(unsigned int color1, unsigned int color2, mfloat_t frac)
{
        unsigned int r1, g1, b1;
        unsigned int r2, g2, b2;
        channelize(color1, &r1, &g1, &b1);
        channelize(color2, &r2, &g2, &b2);
        r1 = interp_helper(r1, r2, frac);
        g1 = interp_helper(g1, g2, frac);
        b1 = interp_helper(b1, b2, frac);
        assert(r1 < 256);
        assert(g1 < 256);
        assert(b1 < 256);
        return TO_RGB(r1, g1, b1);
}

enum { FILT_SIZE = 20, };
static void
convolve_helper(unsigned int *filt, unsigned int *buf)
{
        unsigned int tbuf[NCOLOR + FILT_SIZE];
        convolve(tbuf, buf, filt, NCOLOR, FILT_SIZE);
        memcpy(buf, tbuf, NCOLOR * sizeof(*buf));
}

static void
normalize(unsigned int *buf)
{
        int i;
        unsigned int max = 0, min = 1000000;
        for (i = 0; i < NCOLOR; i++) {
                if (buf[i] > max)
                        max = buf[i];
                if (buf[i] < min)
                        min = buf[i];
        }
        if (max == 0)
                return;

        for (i = 0; i < NCOLOR; i++) {
                unsigned int v = (buf[i] - min) * 256 / (max - min);
                if (v > 256) {
                        printf("@%d %d * 256 / (%d - %d) - %d = %d\n",
                               i, buf[i], max, min, min, v);
                }
                /* In case we "touch the boundary" of 256 */
                if (v >= 255)
                        v = 255;
                buf[i] = v;
        }
}

/* "transitionate" because I don't have a thesaurus handy */
static void
initialize_palette(void)
{
        int i;
        static unsigned int filt[FILT_SIZE];
        static unsigned int red[NCOLOR];
        static unsigned int green[NCOLOR];
        static unsigned int blue[NCOLOR];

        switch (gbl.palette) {
        default:
        case 1:
                inside_color = COLOR_BLACK;
                for (i = 0; i < FILT_SIZE; i+= 2)
                        filt[i] = 1;
                for (i = 1; i < FILT_SIZE; i+= 2)
                        filt[i] = 2;
                for (i = 1; i < 30; i++)
                        blue[i] = red[i] = green[i] = 12;
                for (i = 30; i < 40; i++) {
                        blue[i] = 255;
                        red[i] = 130;
                }
                for (i = 40; i < 50; i++) {
                        blue[i] = 255;
                        red[i] = 55;
                }
                for (i = 50; i < 65; i++)
                        green[i] = red[i] = 255;
                for (i = 65; i < 80; i++)
                        red[i] = 255;
                for (i = 80; i < 110; i++)
                        green[i] = 255;
                for (i = 110; i < NCOLOR; i++)
                        red[i] = green[i] = blue[i] = 255;
                convolve_helper(filt, red);
                convolve_helper(filt, blue);
                convolve_helper(filt, green);
                normalize(red);
                normalize(blue);
                normalize(green);
                for (i = 0; i < NCOLOR; i++)
                        palette[i] = TO_RGB(red[i], green[i], blue[i]);

                break;
        case 2:
                inside_color = COLOR_WHITE;
                for (i = 0; i < NCOLOR; i++) {
                        /* slightly yellow */
                        unsigned int rg = i * 256 / NCOLOR;
                        unsigned int b = rg * 204 / 256;
                        rg = (int)(sqrt((mfloat_t)rg/256.0) * 256.0);
                        if (rg > 255)
                                rg = 255;
                        b = b * b / 256;
                        palette[i] = TO_RGB(rg, rg, b);
                }
                break;
        case 3:
                inside_color = COLOR_BLACK;
                for (i = 0; i < NCOLOR; i++) {
                        static const double PHY_SCALAR = 6.283185307 / (double)NCOLOR;
                        double phi = (double)i * PHY_SCALAR;
                        red[i] = (int)(abs(sin(phi)) * 255.0);
                        blue[i] = (int)(abs(cos(phi)) * 0.8 * 256.0);
                        green[i] = i < NCOLOR/2
                                   ? (i * 256 / NCOLOR)
                                   : (i - NCOLOR) * 256 / NCOLOR;
                        palette[i] = TO_RGB(red[i], green[i], blue[i]);
                }
                break;
        case 4:
                inside_color = COLOR_WHITE;
                for (i = 0; i < NCOLOR; i++) {
                        double phi = (double)i * 6.283185307 / (double)NCOLOR;
                        red[i] = (int)(sin(phi) * 255.0) + 128;
                        green[i] = (int)(cos(phi) * 0.8 * 256.0) + 128;
                        blue[i] = (int)(cos(phi + 2) * 0.25 * 256.0) + 32;
                        if ((int)red[i] < 0)
                                red[i] = 0;
                        else if (red[i] > 255)
                                red[i] = 255;
                        if ((int)green[i] < 0)
                                green[i] = 0;
                        else if ((int)green[i] > 255)
                                green[i] = 255;
                        if ((int)blue[i] < 0)
                                blue[i] = 0;
                        else if (blue[i] > 255)
                                blue[i] = 255;
                        palette[i] = TO_RGB(red[i], green[i], blue[i]);
                }
                break;
        }
}

/*
 * Return black-and-white gradient.
 * Works best when bailout radius and number of iterations are high.
 */
static unsigned int
distance_to_color(mfloat_t dist, mfloat_t max)
{
        unsigned int magn;

        if (dist <= 0.0L)
                return COLOR_BLACK;

        /* TODO: Make the root be a command-line option */
        magn = (unsigned int)(255.0 * pow(dist / max, 0.25));
        if (magn > 255)
                magn = 255;

        return TO_RGB(magn, magn, magn);
}

/*
 * Return color of palette[count modulo NCOLOR].
 * Works best when number of iterations is at least NCOLOR.
 */
static unsigned int
iteration_to_color(mfloat_t iter_count)
{
        int i;
        unsigned int v1, v2;
        long double dummy = 0;

        if (inside_color == NO_COLOR)
                initialize_palette();

        if (iter_count <= 0.0L || (int)iter_count >= gbl.n_iteration)
                return inside_color;

        /* Linear interpolation of palette[esc_count % NCOLOR] */
        i = (int)iter_count % NCOLOR;
        v1 = palette[i];
        v2 = palette[i == NCOLOR - 1 ? 0 : i + 1];
        return linear_interp(v1, v2, modfl(iter_count, &dummy));
}

unsigned int
get_color(mfloat_t esc_val, mfloat_t min, mfloat_t max)
{
        if (gbl.distance_est) {
                return distance_to_color(esc_val, max);
        } else {
                return iteration_to_color(esc_val);
        }
}

/* XXX REVISIT: Hierarchically asymmetrical to get_color() */
void
print_palette(void)
{
        int row, col;
        if (inside_color == NO_COLOR)
                initialize_palette();
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        double idx = (double)col * (double)NCOLOR / (double)gbl.width;
                        unsigned int i, v1, v2, color;
                        i = (unsigned int)idx;
                        assert(i < NCOLOR);
                        v1 = palette[i];
                        v2 = palette[i == NCOLOR-1 ? 0 : i + 1];
                        color = linear_interp(v1, v2, modf(idx, &idx));
                        pxbuf_fill_pixel(gbl.pxbuf, row, col, color);
                }
        }
}

