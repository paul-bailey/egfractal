#include "fractal_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <complex.h>

enum {
	NCOLOR = 128,
	TRACK_PROGRESS = 1,
};

static struct gbl_t {
	Pxbuf *pxbuf;
	unsigned long n_iteration;
	unsigned int dither;
	unsigned int height;
	unsigned int width;
	unsigned int pallette;
	long double zoom_pct;
	long double zoom_xoffs;
	long double zoom_yoffs;
	long double bailout;
	long double bailoutsqu;
	unsigned int min_iteration;
	bool distance_est;
} gbl = {
	.pxbuf = NULL,
	.n_iteration = 1000,
	.dither = 0,
	.height = 600,
	.width = 600,
	.pallette = 2,
	.zoom_pct = 1.0,
	.zoom_xoffs = 0.0,
	.zoom_yoffs = 0.0,
	.bailout = 2.0,
	.bailoutsqu = 4.0,
	.min_iteration = 0,
	.distance_est = false,
};

/* Initialized to log2l(2.0L) */
static long double log_2;
static const long double INSIDE = -1.0L;

/* scale pixels to points of mandelbrot set and handle zoom. */
static void
x0y0(long double *x0, long double *y0, int row, int col)
{
        long double x, y, w, h;

        /*
         * Swapping col with row here because this
         * is a**-backwards, too.
         */
        x = (long double)col;
        y = (long double)row;
        w = (long double)gbl.width;
        h = (long double)gbl.height;

        x = 4.0 * x / w - 2.0;
        y = 4.0 * y / h - 2.0;

        x *= gbl.zoom_pct;
        y *= gbl.zoom_pct;

        x -= gbl.zoom_xoffs;
        y -= gbl.zoom_yoffs;

        *x0 = x;
        *y0 = y;
}

/* **********************************************************************
 *  			Pallette section
 ***********************************************************************/

static unsigned int pallette[NCOLOR];
#define NO_COLOR ((unsigned int)~0ul)
static unsigned int inside_color = NO_COLOR;

static unsigned int
interp_helper(unsigned int color1, unsigned int color2, long double frac)
{
	return color1 + (int)(frac * ((long double)color2 - (long double)color1) + 0.5);
}

static void
channelize(unsigned int color, unsigned int *r, unsigned int *g, unsigned int *b)
{
	*r = (color >> 16) & 0xffu;
	*g = (color >> 8) & 0xffu;
	*b = color & 0xffu;
}

static unsigned int
linear_interp(unsigned int color1, unsigned int color2, long double frac)
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
transitionate_pallette(unsigned int a[NCOLOR])
{
	/* So we don't have jarring transitions */
	int i, j;
	for (i = 0, j = NCOLOR-1; i < NCOLOR/2; i++, j--) {
		unsigned int tmp = (a[j] + a[i]) / 2;
		a[i] = a[j] = tmp;
	}
	/* Do this once more */
	normalize(a);
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
		if (false && gbl.pallette == 3) {
			transitionate_pallette(red);
			transitionate_pallette(green);
			transitionate_pallette(blue);
		} 
		for (i = 0; i < NCOLOR; i++)
			pallette[i] = TO_RGB(red[i], green[i], blue[i]);

		break;
	case 2:
		inside_color = COLOR_WHITE;
		for (i = 0; i < NCOLOR; i++) {
			/* slightly yellow */
			unsigned int rg = i * 256 / NCOLOR;
			unsigned int b = rg * 204 / 256;
                        rg = (int)(sqrt((long double)rg/256.0) * 256.0);
                        if (rg > 255)
                                rg = 255;
                        b = b * b / 256;
			pallette[i] = TO_RGB(rg, rg, b);
		}
		break;
	case 3:
		inside_color = COLOR_BLACK;
		for (i = 0; i < NCOLOR; i++) {
			static const long double PHY_SCALAR = 6.283185307 / (double)NCOLOR; 
			double phi = (double)i * PHY_SCALAR;
			red[i] = (int)(abs(sin(phi)) * 255.0);
			blue[i] = (int)(abs(cos(phi)) * 0.8 * 256.0);
			green[i] = i < NCOLOR/2 
				  ? (i * 256 / NCOLOR) 
				  : (i - NCOLOR) * 256 / NCOLOR;
			pallette[i] = TO_RGB(red[i], green[i], blue[i]);
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
			pallette[i] = TO_RGB(red[i], green[i], blue[i]);
		}
		break;
	}
}

static unsigned int
get_color(long double idx, long double min, long double max)
{
	int i;
	unsigned int v1, v2;

	if (inside_color == NO_COLOR) {
		/* Need to initialize pallette */
		initialize_pallette();
		if (gbl.distance_est)
			inside_color = COLOR_BLACK;
	}
	
	if (idx <= 0.0L)
		return inside_color;

	if (gbl.distance_est) {
		/* Black and white */
		unsigned int magn = (unsigned int)(255.0 * pow(idx / max, 0.25));
		if (magn > 255)
			magn = 255;
		return TO_RGB(magn, magn, magn);
	}

	if ((int)idx >= gbl.n_iteration)
		return inside_color;

	/* Linear interpolation of pallette[idx % NCOLOR] */
	i = (int)idx % NCOLOR;
	v1 = pallette[i];
	v2 = pallette[i == NCOLOR - 1 ? 0 : i + 1];
	return linear_interp(v1, v2, modfl(idx, &idx));
}


/* **********************************************************************
 * 			Error helpers
 ***********************************************************************/

static void
usage(void)
{
	fprintf(stderr, "Bad arg\n");
	exit(EXIT_FAILURE);
}

static void
oom(void)
{
	fprintf(stderr, "OOM!\n");
	exit(EXIT_FAILURE);
}

/* **********************************************************************
 * 			The algorithm
 ***********************************************************************/

static long double
modulusqu(long double complex c)
{
	return creal(c) * creal(c) + cimag(c) * cimag(c);
}

static long double
modulus(long double complex c)
{
	return sqrt(modulusqu(c));
}

static long double
iterate_normal(long double x0, long double y0)
{
	long double ret;
	unsigned long n = gbl.n_iteration;
	unsigned long i;
	long double x = 0.0L, y = 0.0L;
	for (i = 0; i < n; i++) {
		long double xnew = x * x - y * y + x0;
		long double ynew = 2.0L * x * y + y0;
		if (xnew == x && ynew == y)
			return INSIDE;
		if ((xnew * xnew + ynew * ynew) > gbl.bailoutsqu) 
			break;
		x = xnew;
		y = ynew;
	}

	if (i == n)
		return INSIDE;

	ret = (long double)i;
	if (i < n && gbl.dither > 0) {
		if (!!(gbl.dither & 01)) {
			/* Smooth with distance estimate */
			long double log_zn = logl(x * x + y * y) / 2.0L;
			long double nu = logl(log_zn / log_2) / log_2;
			if (!isfinite(log_zn) || !isfinite(nu)) {
				fprintf(stderr, 
					"Cannot use distance estimation. "
					"Try a larger bailout radius.\n");
			} else {
				ret += 1.0L - nu;
			}
		}

		if (!!(gbl.dither & 02)) {
			/* dither */
			int v = rand();
			/* Don't guess if the compiler uses ASR instead of LSR */
		       	if (v < 0)
				v = -1 * (v & 0xff);
			else
				v &= 0xff;
			long double diff = (long double)v / 128.0L;
			ret += diff;
		}	

		if (ret < 0.0L)
			ret = 0.0L;
		else if (ret > (long double)n)
			ret = (long double)n;
	}
	return ret;
}

typedef struct complex_t {
	long double re;
	long double im;
} complex_t;

static complex_t
complex_mul(complex_t a, complex_t b)
{
	complex_t ret;
	ret.re = a.re * b.re - a.im * b.im;
	ret.im = a.im * b.re + a.re * b.im;
	return ret;
}

static long double
complex_modsqu(complex_t v)
{
	return v.re * v.re + v.im * v.im;
}

static long double
complex_mod(complex_t v)
{
	return sqrt(complex_modsqu(v));
}

static complex_t
complex_sq(complex_t v)
{
	complex_t ret;
	ret.re = v.re * v.re - v.im * v.im;
	ret.im = 2.0L * v.im * v.re;
	return ret;
}

static complex_t
complex_add(complex_t a, complex_t b)
{
	complex_t ret;
	ret.re = a.re + b.re;
	ret.im = a.im + b.im;
	return ret;
}

static long double
iterate_distance(long double x0, long double y0)
{
	/* Accept defeat until I get this method working. */
	if (false)
		return iterate_normal(x0, y0);

	unsigned long n = gbl.n_iteration;
	unsigned long i;
	if (false) {
		complex_t c = { .re = x0, .im = y0 };
		complex_t z = { .re = 0.0L, .im = 0.0L };
		complex_t dz = { .re = 0.0L, .im = 0.0L };
		long double m2;

		for (i = 0; i < n; i++) {
			dz = complex_mul(z, dz);
			dz.re *= 2.0L;
			dz.re += 1.0L;
			dz.im *= 2.0L;
			z = complex_add(complex_sq(z), c);
			if ((m2 = complex_modsqu(z)) > gbl.bailoutsqu)
				break;
		}
		return sqrtl(m2 / complex_modsqu(dz)) * 0.5L * logl(m2);
	} else {
		complex_t c = { .re = x0, .im = y0 };
		complex_t z = { .re = 0.0L, .im = 0.0L };
		complex_t dz = { .re = 1.0L, .im = 0.0L };
		for (i = 0; i < n; i++) {
			complex_t znew, dznew;
			znew = complex_add(complex_sq(z), c);
			dznew = complex_mul(z, dz);
			dznew.re = dznew.re * 2.0L + 1.0L;
			dznew.im = dznew.im * 2.0L;
			z.re = znew.re;
			z.im = znew.im;
			dz.re = dznew.re;
			dz.im = dznew.im;
			if (complex_modsqu(z) > gbl.bailoutsqu)
				break;
		}

		/* Return distance normalized to the colorspace */
		return (complex_mod(z) * logl(complex_mod(z)) / complex_mod(dz)) * 255 / 4.0;
	}
}

static long double
mandelbrot_px(int row, int col)
{
	/* XXX: Quite an arbitrary choice */
	enum { THRESHOLD = 10 };
	long double x0, y0;

	x0y0(&x0, &y0, row, col);
#warning "put pack"
	if (true && gbl.n_iteration > THRESHOLD) {
		/* 
		 * Faster to do this and throw away pixels that are in
		 * the main cardioid and circle.
		 *
		 * XXX: Is the 0.25 meant to be sqrt(bailout)? I forget.
		 */
		long double xp = x0 - 0.25L;
		long double ysq = y0 * y0;
		long double q = xp * xp + ysq;
		if ((q * (q + xp)) < (0.25L * ysq))
			return INSIDE;
		xp = x0 + 1.0L;
		if ((xp * xp + ysq) < (0.25L * ysq))
			return INSIDE;
	}

	if (gbl.distance_est)
		return iterate_distance(x0, y0);
	else
		return iterate_normal(x0, y0);
}

static void
mandelbrot(void)
{
	/* TODO: Determine here if out zoomed image touches the
	 * cardioid or largest circle.  That way we won't have the
	 * additional check in the iterative algorithm.
	 */
	int row, col; 
	long double *ptbuf, *tbuf, min, max;

	tbuf = malloc(gbl.width * gbl.height * sizeof(*tbuf));
	if (!tbuf)
		oom();

	if (TRACK_PROGRESS) {
		printf("Row %9d col %9d", 0, 0);
		fflush(stdout);
	}
	ptbuf = tbuf;
	min = 1.0e16;
	max = 0.0;
	for (row = 0; row < gbl.height; row++) {
		for (col = 0; col < gbl.width; col++) {
			long double v;
			if (TRACK_PROGRESS) {
				printf("\e[23D%9d col %9d", row, col);
				fflush(stdout);
			}
			v = mandelbrot_px(row, col);
			if (min > v)
				min = v;
			if (max < v)
				max = v;
			*ptbuf++ = v;
		}

	}
	putchar('\n');
	printf("min: %Lg max: %Lg\n", min, max);
	ptbuf = tbuf;
	for (row = 0; row < gbl.height; row++) {
		for (col = 0; col < gbl.width; col++) {
			unsigned int color = get_color(*ptbuf++, min, max);
			pxbuf_fill_pixel(gbl.pxbuf, row, col, color);
		}
	}
	free(tbuf);
}

int
main(int argc, char **argv)
{
	const char *outfile = "mandelbrot.bmp";
	int opt;
	char *endptr;
	FILE *fp;

	/* need to set these "consts" first */
	log_2 = logl(2.0L);

	while ((opt = getopt(argc, argv, "p:d:Db:n:h:w:y:x:z:o:")) != -1) {
		switch (opt) {
		case 'p':
			gbl.pallette = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
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
		case 'n':
			gbl.n_iteration = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'b':
			gbl.bailout = strtold(optarg, &endptr);
			if (endptr == optarg)
				usage();
			gbl.bailoutsqu = gbl.bailout * gbl.bailout;
			break;
		case 'd':
			gbl.dither = strtoul(optarg, &endptr, 0);
			if (endptr == optarg)
				usage();
			break;
		case 'D':
			gbl.distance_est = true;
			break;
		default:
			usage();
		}
	}
	gbl.pxbuf = pxbuf_create(gbl.width, gbl.height, COLOR_WHITE);
	if (!gbl.pxbuf)
		oom();

	mandelbrot();

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

