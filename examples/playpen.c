#include "fractal_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <string.h>

static const double pi = 3.14159265359;

enum {
	NCOLOR = 128,
};

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
        double cx;
        double cy;
} gbl = {
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

        x = 4.0 * x / w - 2.0;
        y = 4.0 * y / h - 2.0;

        x *= gbl.zoom_pct;
        y *= gbl.zoom_pct;

        x -= gbl.zoom_xoffs;
        y -= gbl.zoom_yoffs;

        *x0 = x;
        *y0 = y;
}

static unsigned int pallette[NCOLOR];
#define NO_COLOR ((unsigned int)~0ul)
static unsigned int inside_color = NO_COLOR;

static unsigned int
interp_helper(unsigned int color1, unsigned int color2, double frac)
{
	return color1 + (int)(frac * ((double)color2 - (double)color1) + 0.5);
}

static void
channelize(unsigned int color, unsigned int *r, unsigned int *g, unsigned int *b)
{
	*r = (color >> 16) & 0xffu;
	*g = (color >> 8) & 0xffu;
	*b = color & 0xffu;
}

static unsigned int
linear_interp(unsigned int color1, unsigned int color2, double frac)
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
	double scalar;
	for (i = 0; i < NCOLOR; i++)
		if (buf[i] > max)
			max = buf[i];
	if (max == 0)
		return;

	scalar = 256.0 / (double)max;
	for (i = 0; i < NCOLOR; i++) {
		buf[i] = (unsigned)((double)buf[i] * scalar + 0.5);
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
                        rg = (int)(sqrt((double)rg/256.0) * 256.0);
                        if (rg > 255)
                                rg = 255;
                        b = b * b / 256;
			pallette[i] = TO_RGB(rg, rg, b);
		}
		break;
	}
}
static unsigned int
get_color(double idx)
{
	int i;
	unsigned int v1, v2;

	if (inside_color == NO_COLOR) {
		/* Need to initialize pallette */
		initialize_pallette();
	}
	if ((int)idx >= gbl.n_iteration)
		return inside_color;

	/* Linear interpolation of pallette[idx % NCOLOR] */
	i = (int)idx % NCOLOR;
	v1 = pallette[i];
	v2 = pallette[i == NCOLOR - 1 ? 0 : i + 1];
	return linear_interp(v1, v2, modf(idx, &idx));
}


static double
julia_px(int row, int col)
{
	double x, y, zx, zy, ret;
	int i;

	x0y0(&x, &y, row, col);
	zx = x;
	zy = y;
	for (i = 0; i < gbl.n_iteration; i++) {
		double xtmp;
		if ((zx * zx + zy * zy) >= 4.0)
			break;
		xtmp = zx * zx - zy * zy;
                zy = 2.0 * zx * zy + gbl.cy;
                zx = xtmp + gbl.cx;
	}

	/* TODO: Dither here */
	ret = (double)i;
	if (i < gbl.n_iteration) {
		if (gbl.dither) {
			/* Smooth by dithering */
			int v = rand();
			if (v < 0)
				v = -1 * (v & 0xff);
			else
				v &= 0xff;
			double diff = (double)v / 128.0;
			ret += diff;
		}
		/* TODO: Smooth by distance */
		if (gbl.dither) {
			double log_zn = log10(zx * zx + zy * zy) / 2.0;
			double log_2 = log10(2.0);
			double nu = log10(log_zn / log_2) / log_2;
			ret += 1.0 - nu;
		}
		if (ret >= (double)gbl.n_iteration)
			ret = (double)(gbl.n_iteration - 1);
		else if (ret < 0.0)
			ret = 0.0;
	}
	return ret;
}

static void
oom(void)
{
	fprintf(stderr, "OOM!\n");
	exit(EXIT_FAILURE);
}

static void
julia(void)
{
	int row, col;
	unsigned long total;
	unsigned long *histogram;
	double *ptbuf, *tbuf;
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
			double i = julia_px(row, col);
			histogram[(int)i]++;
			total += (int)i;
			*ptbuf++ = i;
		}
	}
	ptbuf = tbuf;
	for (row = 0; row < gbl.height; row++) {
		for (col = 0; col < gbl.width; col++) {
			unsigned int color;
			double i = *ptbuf++;
			i += ((double)histogram[(int)i] / (double)total + 0.5);
			color = get_color(i);
			pxbuf_fill_pixel(gbl.pxbuf, row, col, color);
		}
	}
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
	while ((opt = getopt(argc, argv, "dz:x:y:w:h:n:R:I:")) != -1) {
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
	fp = fopen("julia1.bmp", "wb");
	if (!fp) {
		fprintf(stderr, "Cannot open output file\n");
		return 1;
	}

	gbl.pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_BLACK);
	if (!gbl.pxbuf) {
		fprintf(stderr, "OOM!\n");
		return 1;
	}

	julia();
	pxbuf_print(gbl.pxbuf, fp);
	fclose(fp);
	pxbuf_free(gbl.pxbuf);
	return 0;
}
