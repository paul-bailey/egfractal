#include "fractal_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

static struct gbl_t {
        Pxbuf *pxbuf;
        int n_iteration;
        bool dither;
        int height;
        int width;
        int pallette;
        double zoom_pct;
        double zoom_xoffs;
        double zoom_yoffs;
        unsigned long points;
} gbl = {
        .pxbuf = NULL,
        .n_iteration = 1000,
        .dither = false,
        .height = 3000,
        .width = 3000,
        .pallette = 1,
        .zoom_pct = 1.0,
        .zoom_xoffs = 0.0,
        .zoom_yoffs = 0.0,
        .points = 50000000,
};

static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(1);
}

static void
iterate_r(double x0, double y0, unsigned long *buf, int n, bool isdivergent)
{
        int i;
        double x = 0.0;
        double y = 0.0;
        for (i = 0; i < n; i++) {
                double xnew = x * x - y * y + x0;
                double ynew = 2 * x * y + y0;
                if (isdivergent && i > 3) {
                        /*
                         * XXX REVISIT: It should be fairly easy to apply zoom
                         * here.
                         */
                        unsigned int drawnx = (int)((double)gbl.width * (xnew + 2.0) / 3.0 + 0.5);
                        unsigned int drawny = (int)((double)gbl.height * (ynew + 1.5) / 3.0 + 0.5);
                        if (drawnx < gbl.width && drawny < gbl.height) {
                                buf[drawny * gbl.height + drawnx]++;
                        }
                }

                if ((xnew * xnew + ynew * ynew) > 4.0) {
                        if (!isdivergent)
                                iterate_r(x0, y0, buf, n, true);
                        return;
                }

                x = xnew;
                y = ynew;
        }
}

static void
iterate(double x0, double y0, unsigned long *buf, int n)
{
        iterate_r(x0, y0, buf, n, false);
}

static void
normalize(unsigned long *buf)
{
        int i;
        unsigned int max = 0L;
        for (i = 0; i < gbl.width * gbl.height; i++) {
                if (max < buf[i])
                        max = buf[i];
        }
        if (max == 0)
                return;
        for (i = 0; i < gbl.width * gbl.height; i++) {
                /* XXX arbitrary division */
                buf[i] = buf[i] * 256 / max;
        }
}

static void
mbrot2(void)
{
        static const int n[3] = { 5000, 500, 50 };
        int i, row, col;
        unsigned long *buffer;
        unsigned long *chanbuf[3];
        buffer = malloc(gbl.width * gbl.height * sizeof(*buffer) * 3);
        if (!buffer)
                oom();
        memset(buffer, 0, gbl.width * gbl.height * sizeof(*buffer) * 3);
        chanbuf[0] = buffer;
        chanbuf[1] = &buffer[gbl.width * gbl.height];
        chanbuf[2] = &buffer[gbl.width * gbl.height * 2];

        for (i = 0; i < gbl.points; i++) {
                double x = (double)drand48() * 3.0 - 2.0;
                double y = (double)drand48() * 3.0 - 1.5;
                int chan;
                for (chan = 0; chan < 3; chan++)
                        iterate(x, y, chanbuf[chan], n[chan]);
        }

        /* TODO: Need to blur rasters, perhaps by convolving with 1,1,1... */
        normalize(chanbuf[0]);
        normalize(chanbuf[1]);
        normalize(chanbuf[2]);

        i = 0;
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        unsigned int r, g, b;
                        r = chanbuf[0][i];
                        g = chanbuf[1][i];
                        b = chanbuf[2][i];
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
                        pxbuf_fill_pixel(gbl.pxbuf, row, col, TO_RGB(r, g, b));
                }

        }
        free(buffer);
}

int
main(int argc, char **argv)
{
        FILE *fp = fopen("mbrot2.bmp", "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }
        gbl.pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_WHITE);
        if (!gbl.pxbuf)
                oom();

        mbrot2();
        pxbuf_print(gbl.pxbuf, fp);
        fclose(fp);
        pxbuf_free(gbl.pxbuf);
        return 0;
}
