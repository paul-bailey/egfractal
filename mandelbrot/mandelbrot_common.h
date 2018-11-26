#ifndef MANDELBROT_COMMON_H
#define MANDELBROT_COMMON_H

#include "fractal_common.h"

typedef long double mfloat_t;

extern struct gbl_t {
	Pxbuf *pxbuf;
	unsigned long n_iteration;
	unsigned int dither;
	unsigned int height;
	unsigned int width;
	unsigned int pallette;
	mfloat_t zoom_pct;
	mfloat_t zoom_xoffs;
	mfloat_t zoom_yoffs;
	mfloat_t bailout;
	mfloat_t bailoutsqu;
	unsigned int min_iteration;
	bool distance_est;
} gbl;

extern unsigned int get_color(mfloat_t idx, mfloat_t min, mfloat_t max);
extern void print_palette(void);

#endif /* MANDELBROT_COMMON_H */

