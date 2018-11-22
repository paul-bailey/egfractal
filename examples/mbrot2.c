#include "fractal_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

static struct gbl_t {
        Pxbuf *pxbuf;
        int n_red;
	int n_blue;
	int n_green;
        int height;
        int width;
        double zoom_pct;
	double zoom_pct_inv;
        double zoom_xoffs;
        double zoom_yoffs;
        unsigned long points;
} gbl = {
        .pxbuf = NULL,
        .n_red = 5000,
	.n_blue = 500,
	.n_green = 50,
        .height = 600,
        .width = 600,
        .zoom_pct = 1.0,
        .zoom_xoffs = 0.0,
        .zoom_yoffs = 0.0,
        .points = 500000,
};

static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(1);
}

static void
graph_to_pixel(double x, double y, unsigned int *px, unsigned int *py)
{
	static const double third = 1.0 / 3.0;
	x = (double)gbl.width * ((x + 2.0) * third * gbl.zoom_pct_inv + gbl.zoom_xoffs) + 0.5;
	y = (double)gbl.height * ((y + 1.5) * third * gbl.zoom_pct_inv + gbl.zoom_yoffs) + 0.5;
	*px = (int)x;
	*py = (int)y;
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
			unsigned int drawnx, drawny;
			graph_to_pixel(xnew, ynew, &drawnx, &drawny);
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
        int n[3] = { gbl.n_red, gbl.n_green, gbl.n_blue };
        int i, row, col, npct;
	unsigned long onepct = gbl.points / 100;
	unsigned long pctcount;
        unsigned long *buffer;
        unsigned long *chanbuf[3];

	/* Initialize this */
	gbl.zoom_pct_inv = 1.0 / gbl.zoom_pct;

        buffer = malloc(gbl.width * gbl.height * sizeof(*buffer) * 3);
        if (!buffer)
                oom();
        memset(buffer, 0, gbl.width * gbl.height * sizeof(*buffer) * 3);
        chanbuf[0] = buffer;
        chanbuf[1] = &buffer[gbl.width * gbl.height];
        chanbuf[2] = &buffer[gbl.width * gbl.height * 2];

	pctcount = npct = 0;
	printf("Progress     ");
        for (i = 0; i < gbl.points; i++) {
                double x = (drand48() * gbl.zoom_pct + gbl.zoom_xoffs) * 3.0 - 2.0;
                double y = (drand48() * gbl.zoom_pct + gbl.zoom_yoffs) * 3.0 - 1.5; 
                int chan;
		if (pctcount++ == onepct) {
			npct++;
			pctcount = 0;
			printf("\e[3D%3d", npct);
			fflush(stdout);
		}
                for (chan = 0; chan < 3; chan++)
                        iterate(x, y, chanbuf[chan], n[chan]);
        }
	putchar('\n');

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

static void
usage(void)
{
	fprintf(stderr, "Bad arg\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	FILE *fp;
	char *endptr;
	int opt;
	while ((opt = getopt(argc, argv, "z:x:y:w:h:r:g:b:p:")) != -1) {
		switch (opt) {
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
		case 'r':
			gbl.n_red = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'g':
			gbl.n_blue = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'b':
			gbl.n_green = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'p':
			gbl.points = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		}
	}
        fp = fopen("mbrot2.bmp", "wb");
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
