#ifndef FRACTALMAKE_H

#include "fractal_common.h"

struct gbl_t {
        int height;
        int width;
        int depth;
        int dither; /* true or false */
        int pallette;
        float zoom_xoffs;
        float zoom_yoffs;
        float zoom_pct;
        unsigned long bufsize;
        unsigned long n_iteration;
};

struct rgb_t {
	unsigned int red;
	unsigned int green;
	unsigned int blue;
};

enum { N_PALLETTE = 4 };

/* main.c */
extern struct gbl_t gbl;

/* mandelbrot.c */
extern void mandelbrot24(unsigned char *buf, int n_iteration);

/* pallette.c */
extern void pallette_1_rgb(struct rgb_t *rgb, double i, int n);
extern void pallette_1_init(void);
extern void pallette_2_rgb(struct rgb_t *rgb, double i, int n);
#define pallette_2_init pallette_1_init
extern void pallette_3_init(void);
#define pallette_3_rgb pallette_2_rgb

#endif /* FRACTALMAKE_H */

