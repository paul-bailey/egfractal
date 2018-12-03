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
                        rg = (int)(sqrtl((mfloat_t)rg/256.0) * 256.0);
                        if (rg > 255)
                                rg = 255;
                        b = b * b / 256;
                        pallette[i] = TO_RGB(rg, rg, b);
                }
                break;
        }
}

static unsigned int
idx_to_color(mfloat_t idx)
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

static unsigned int
distance_to_color_palette(mfloat_t dist, mfloat_t max)
{
        mfloat_t d;
        unsigned int i, v1, v2;
        long double dummy = 0.0L;

        if (inside_color == NO_COLOR)
                initialize_pallette();

        if (dist < 0.0L)
                return inside_color;

        assert(dist <= max);

        /* Linear interpolation of pallette[idx % NCOLOR] */
        d = powl(dist / max, gbl.distance_root) * (mfloat_t)NCOLOR;
        i = (int)d;
        if (i >= NCOLOR)
                i = NCOLOR - 1;
        v1 = pallette[i];
        v2 = pallette[i == NCOLOR - 1 ? 0 : i + 1];
        return linear_interp(v1, v2, modfl(d, &dummy));
}

static unsigned int
distance_to_color_bw(mfloat_t dist, mfloat_t max)
{
        unsigned int magn;

        if (dist <= 0.0L)
                return COLOR_BLACK;

        /* TODO: Make the root be a command-line option */
        magn = (unsigned int)(255.0 * pow(dist / max, gbl.distance_root));
        if (magn > 255)
                magn = 255;

        return TO_RGB(magn, magn, magn);
}

static unsigned int
distance_to_color(mfloat_t dist, mfloat_t max)
{
        if (gbl.color_distance)
                return distance_to_color_palette(dist, max);
        else
                return distance_to_color_bw(dist, max);
}

unsigned int
get_color(mfloat_t idx, mfloat_t max)
{
        if (gbl.distance_est)
                return distance_to_color(idx, max);
        else
                return idx_to_color(idx);
}


