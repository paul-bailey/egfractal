#include "julia1_common.h"
#include <assert.h>
#include <string.h>

enum {
        NCOLOR = 128,
};

static unsigned int pallette[NCOLOR];
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
        unsigned int max = 0;
        mfloat_t scalar;
        for (i = 0; i < NCOLOR; i++)
                if (buf[i] > max)
                        max = buf[i];
        if (max == 0)
                return;

        scalar = 256.0 / (mfloat_t)max;
        for (i = 0; i < NCOLOR; i++) {
                buf[i] = (unsigned)((mfloat_t)buf[i] * scalar + 0.5);
                if (buf[i] >= 255)
                        buf[i] = 255;
        }
}

static void
initialize_pallette(void)
{
        int i;
        static unsigned int filt[FILT_SIZE];
        static unsigned int red[NCOLOR];
        static unsigned int green[NCOLOR];
        static unsigned int blue[NCOLOR];

        switch (gbl.pallette) {
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
                        pallette[i] = TO_RGB(red[i], green[i], blue[i]);

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
                        pallette[i] = TO_RGB(rg, rg, b);
                }
                break;
        }
}

unsigned int
get_color(mfloat_t idx)
{
        int i;
        unsigned int v1, v2;
        long double dummy = 0.0L;

        if (inside_color == NO_COLOR) {
                /* Need to initialize pallette */
                initialize_pallette();
        }
        if (idx < 0.0L || (unsigned long)idx >= gbl.n_iteration)
                return inside_color;

        /* Linear interpolation of pallette[idx % NCOLOR] */
        i = (int)idx % NCOLOR;
        v1 = pallette[i];
        v2 = pallette[i == NCOLOR - 1 ? 0 : i + 1];
        return linear_interp(v1, v2, modfl(idx, &dummy));
}


