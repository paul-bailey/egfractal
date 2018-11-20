#ifndef FRACTAL_COMMON_H

#include <stdio.h>

/* bmp_print.c */
extern void bmp_print(FILE *fp, const unsigned char *array,
                      int width, int height, int depth);

/* convolve.c */
extern void convolve(unsigned int *dest, const unsigned int *f,
                     const unsigned int *g, size_t fsize, size_t gsize);


#endif /* FRACTAL_COMMON_H */

