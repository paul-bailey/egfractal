#ifndef MANDELBROT_COMMON_H
#define MANDELBROT_COMMON_H

#include "fractal_common.h"

typedef long double mfloat_t;

/* main.c */
extern struct gbl_t {
        unsigned long n_iteration;
        unsigned int dither;
        unsigned int height;
        unsigned int width;
        unsigned int palette;
        mfloat_t zoom_pct;
        mfloat_t zoom_xoffs;
        mfloat_t zoom_yoffs;
        mfloat_t bailout;
        mfloat_t bailoutsqu;
        mfloat_t distance_root;
        unsigned int min_iteration;
        bool distance_est;
        bool verbose;
} gbl;

/* palette.c */
extern unsigned int get_color(mfloat_t idx, mfloat_t min, mfloat_t max);
extern void print_palette_to_bmp(Pxbuf *pxbuf);

/* parse_args.c */
struct optflags_t {
        bool print_palette;
        const char *outfile;
};
extern void parse_args(int argc, char **argv, struct optflags_t *optflags);

#endif /* MANDELBROT_COMMON_H */

