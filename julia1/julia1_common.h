#ifndef JULIA1_COMMON_H
#define JULIA1_COMMON_H

#include "fractal_common.h"

/* main.c */
extern struct gbl_t {
        Pxbuf *pxbuf;
        unsigned long n_iteration;
        bool dither;
        int height;
        int width;
        int pallette;
        double zoom_pct;
        double zoom_xoffs;
        double zoom_yoffs;
        double cx;
        double cy;
} gbl;

/* palette.c */
extern unsigned int get_color(double idx);

#endif /* JULIA1_COMMON_H */

