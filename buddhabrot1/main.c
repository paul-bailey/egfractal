/* bbrot1 - option set 1 for Buddhabrot */
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
	int n_green;
	int n_blue;
        int height;
        int width;
	int min;
        mfloat_t zoom_pct;
	mfloat_t zoom_pct_inv;
        mfloat_t zoom_xoffs;
        mfloat_t zoom_yoffs;
        mfloat_t bailout;
        mfloat_t bailsqu;
        unsigned long points;
	bool singlechan;
        bool do_hist;
} gbl = {
        .pxbuf      = NULL,
        .n_red      = 5000,
	.n_green    = 500,
	.n_blue     = 50,
        .height     = 600,
        .width      = 600,
	.min        = 3,
        .zoom_pct   = 1.0,
        .zoom_xoffs = 0.0,
        .zoom_yoffs = 0.0,
        .bailsqu    = 4.0,
        .bailout    = 2.0,
        .points     = 500000,
	.singlechan = false,
        .do_hist    = false,
};

/* Error helpers */
static void
oom(void)
{
        fprintf(stderr, "OOM!\n");
        exit(1);
}

static void
usage(void)
{
	fprintf(stderr, "Bad arg\n");
	exit(EXIT_FAILURE);
}

static void
graph_to_pixel(complex_t c, unsigned int *px, unsigned int *py)
{
	static const mfloat_t THIRD = 1.0 / 3.0;
        /*
         * TODO: Factor in zoom.  We can't zoom for calculations,
         * but we can for final result.  This would require 7 gazillion
         * samples and would take forever, but we wouldn't be allocating
         * such a large buffer while we're doing it.
         */
	*px = (int)((mfloat_t)gbl.width * ((c.re + 2.0) * THIRD) + 0.5);
	*py = (int)((mfloat_t)gbl.height * ((c.im + 1.5) * THIRD) + 0.5);
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
                if (isdivergent && i > min) {
			unsigned int drawnx, drawny;
			graph_to_pixel(ztmp, &drawnx, &drawny);
                        if (drawnx < gbl.width && drawny < gbl.height) {
                                buf[drawny * gbl.height + drawnx]++;
                        }
                }

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
hist_eq(unsigned long *chanbuf, unsigned int npx)
{
        enum { HIST_SIZE = 256, };
        static unsigned long long histogram[HIST_SIZE];
        static unsigned long long cdf[HIST_SIZE];
        static unsigned long cdfmax, cdfrange;
        unsigned int i;

        memset(histogram, 0, sizeof(histogram));
        normalize(chanbuf, npx, HIST_SIZE-1);
        for (i = 0; i < npx; i++) {
                histogram[chanbuf[i]]++;
        }


        cdfmax = 0;
        for (i = 0; i < HIST_SIZE; i++) {
                cdfmax += histogram[i];
                cdf[i] = cdfmax;
        }

        /*
         * Slumpify, because a true equalization is too intense for the
         * image that we expect.  Here we use the cuve of x^5 on [0, 1).
         *
         * XXX REVISIT: At this point, better to just use Photoshop
         */
        for (i = 0; i < HIST_SIZE; i++) {
                unsigned long long v = cdf[i];
                v = (int)((double)cdfmax * pow((double)v / (double)cdfmax, 5));
                cdf[i] = v;
        }

        /*
         * We just know there's going to be a lot of solid black pixels,
         * so assume our minimum is zero (not even cdf[0]).
         */
        cdfrange = cdfmax;
        for (i = 0; i < npx; i++) {
                chanbuf[i] = (cdf[chanbuf[i]] - cdf[0]) * HIST_SIZE / cdfrange;
        }
}

static void
bbrot1(void)
{
        unsigned short seeds[6] = {
                /* TODO: See some gnarly stuff when these are identical! */
                0xe66d, 0xdeec, 6,
                0xe66d, 0xdeec, 5,
        };
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
                complex_t c;
                int chan;

                c.re = erand48(&seeds[0]) * 3.0 - 2.0;
                c.im = erand48(&seeds[3]) * 3.0 - 1.5;
		if (pctcount++ == onepct) {
			npct++;
			pctcount = 0;
			printf("\e[3D%3d", npct);
			fflush(stdout);
		}
        	for (chan = 0; chan < nchan; chan++)
			iterate(c, chanbuf[chan], n[chan]);
        }
	putchar('\n');

	for (i = 0; i < nchan; i++) {
                if (gbl.do_hist)
                        hist_eq(chanbuf[i], npx);
		normalize(chanbuf[i], npx, 255);
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

int
main(int argc, char **argv)
{
	FILE *fp;
	char *endptr;
	int opt;
        char *outfile = "buddhabrot1.bmp";
	while ((opt = getopt(argc, argv, "HB:o:z:x:y:w:h:r:g:b:p:sm:")) != -1) {
		switch (opt) {
                case 'B':
                        gbl.bailout = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                usage();
                        gbl.bailsqu = gbl.bailout * gbl.bailout;
                        break;
                case 'H':
                        gbl.do_hist = true;
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
                case 'o':
                        outfile = optarg;
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
        if (gbl.min >= gbl.n_red) {
		fprintf(stderr, "min too high!\n");
		return 1;
        }

	if (!gbl.singlechan && (gbl.min >= gbl.n_green || gbl.min >= gbl.n_blue)) {
		fprintf(stderr, "min too high!\n");
		return 1;
	}

        gbl.pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_WHITE);
        if (!gbl.pxbuf)
                oom();

        bbrot1();

        fp = fopen(outfile, "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }
        pxbuf_print(gbl.pxbuf, fp);
        fclose(fp);

        pxbuf_free(gbl.pxbuf);
        return 0;
}
