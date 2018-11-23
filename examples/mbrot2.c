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
	int min;
        double zoom_pct;
	double zoom_pct_inv;
        double zoom_xoffs;
        double zoom_yoffs;
        unsigned long points;
	bool singlechan;
} gbl = {
        .pxbuf = NULL,
        .n_red = 5000,
	.n_blue = 500,
	.n_green = 50,
        .height = 600,
        .width = 600,
	.min = 3,
        .zoom_pct = 1.0,
        .zoom_xoffs = 0.0,
        .zoom_yoffs = 0.0,
        .points = 500000,
	.singlechan = false,
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

#if 0
/* XXX: This is unfortunately slower than just doing it myself. */
#include <complex.h>
static void
iterate_r(double complex z0, unsigned long *buf, int n, bool isdivergent)
{
        int i;
	double complex z = 0 + I * 0;
        for (i = 0; i < n; i++) {
		z = z * z + z0;
                if (isdivergent && i > 3) {
			unsigned int drawnx, drawny;
			graph_to_pixel(creal(z), cimag(z), &drawnx, &drawny);
                        if (drawnx < gbl.width && drawny < gbl.height) {
                                buf[drawny * gbl.height + drawnx]++;
                        }
                }

		if (cabs(z) > 4.0) {
                        if (!isdivergent)
                                iterate_r(z0, buf, n, true);
                        return;
                }
        }
}

static void
iterate(double x0, double y0, unsigned long *buf, int n)
{
	double complex z0 = x0 + I * y0;
        iterate_r(z0, buf, n, false);
}
#else
static void
iterate_r(double x0, double y0, unsigned long *buf, int n, bool isdivergent)
{
        int i;
	int min = gbl.min;
        double x = 0.0;
        double y = 0.0;
        for (i = 0; i < n; i++) {
                double xnew = x * x - y * y + x0;
                double ynew = 2 * x * y + y0;
                if (isdivergent && i > min) {
			unsigned int drawnx, drawny;
			graph_to_pixel(xnew, ynew, &drawnx, &drawny);
                        if (drawnx < gbl.width && drawny < gbl.height) {
                                buf[drawny * gbl.height + drawnx]++;
                        }
                }

		/* Check both bailout and periodicity */
                if ((xnew * xnew + ynew * ynew) > 4.0 
		    || (xnew == x && ynew == y)) {
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
	/* XXX: Quite an arbitrary choice */
	enum { THRESHOLD = 10 };
	if (n > THRESHOLD) {
		/* 
		 * Faster to do this and throw away pixels that are in
		 * the main cardioid and circle.
		 */
		double xp = x0 - 0.25;
		double ysq = y0 * y0;
		double q = xp * xp + ysq;
		if ((q * (q + xp)) < (0.25 * ysq))
			return;
		xp = x0 + 1.0;
		if ((xp * xp + ysq) < (0.25 * ysq))
			return;
	}
        iterate_r(x0, y0, buf, n, false);
}
#endif

static void
normalize(unsigned long *buf, unsigned int npx)
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
                buf[i] = buf[i] * 256 / max;
		if (buf[i] > 255)
			buf[i] = 255;
        }
}

static void
mbrot2(void)
{
        int n[3] = { gbl.n_red, gbl.n_green, gbl.n_blue };
        int row, col, npct, nchan;
	unsigned long i; /* must be wide as gbl.points */
	unsigned long onepct = gbl.points / 100;
	unsigned long pctcount;
        unsigned long *buffer;
        unsigned long *chanbuf[3];
	/* Since I use this all over */
	unsigned int npx =  gbl.width * gbl.height;

	nchan = gbl.singlechan ? 1 : 3;

	/* Initialize this */
	gbl.zoom_pct_inv = 1.0 / gbl.zoom_pct;

        buffer = malloc(npx * sizeof(*buffer) * nchan);
        if (!buffer)
                oom();
        memset(buffer, 0, npx * sizeof(*buffer) * nchan);
	for (i = 0; i < nchan; i++) 
        	chanbuf[i] = &buffer[npx * i];

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
        	for (chan = 0; chan < nchan; chan++) 
			iterate(x, y, chanbuf[chan], n[chan]);
        }
	putchar('\n');

        /* TODO: Need to blur rasters, perhaps by convolving with 1,1,1... */
	for (i = 0; i < nchan; i++) {
		int j;
		unsigned long long histogram[256];
		/* This could be less than gbl.points, so we need to find it. */
		unsigned long long total = 0;
		/* 
		 * Set every value in chanbuf to 0...255 
		 * TODO: This may introduce artifacts.
		 * Instead normalize it twice; first to a much bigger value
		 * with a much bigger histogram, and second to 0...255.
		 */
		normalize(chanbuf[i], npx);
		for (j = 0; j < npx; j++) {
			unsigned long v = chanbuf[i][j];
			histogram[v]++;
			total += chanbuf[i][j];
		}
		unsigned long hmax = 0;
		unsigned long hmin = gbl.points;
		for (j = 0; j < 256; j++) {
			if (hmax < histogram[j])
				hmax = histogram[j];
			if (hmin > histogram[j])
				hmin = histogram[j];
		}

		for (j = 0; j < npx; j++) {
			unsigned long v = chanbuf[i][j];
			v = v * (hmax - hmin) + hmin;
		}
		normalize(chanbuf[i], npx);
	}

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
	while ((opt = getopt(argc, argv, "z:x:y:w:h:r:g:b:p:sm:")) != -1) {
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
			gbl.n_green = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'b':
			gbl.n_blue = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'm':
			gbl.min = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'p':
			gbl.points = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 's':
			gbl.singlechan = true;
			break;
		default:
			usage();
		}
	}
	/* One quick sanity check */
	if (gbl.min >= gbl.n_red || gbl.min >= gbl.n_green || gbl.min >= gbl.n_blue) {
		fprintf(stderr, "min too high!\n");
		return 1;
	}
        gbl.pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_WHITE);
        if (!gbl.pxbuf)
                oom();

        mbrot2();

        fp = fopen("mbrot2.bmp", "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }
        pxbuf_print(gbl.pxbuf, fp);
        fclose(fp);

        pxbuf_free(gbl.pxbuf);
        return 0;
}
