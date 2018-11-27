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
        mfloat_t zoom_pct;
        mfloat_t zoom_xoffs;
        mfloat_t zoom_yoffs;
        mfloat_t cx;
        mfloat_t cy;
} gbl;

/* palette.c */
extern unsigned int get_color(mfloat_t idx);

#endif /* JULIA1_COMMON_H */

