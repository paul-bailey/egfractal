#include "fractal_common.h"
#include <stdlib.h>

/**
 * convolve - Discrete-time-convolve @f with @g
 * @dest: Integer array to store result.  It must have an array length of
 *        at least @fsize plus @gsize.
 * @f: Integer array to convolve with @g
 * @g: Integer array to convolve with @f
 * @fsize: Array length (NOT bit length!) of @f
 * @gsize: Array lenght (NOT bit length!) of @g
 *
 * Return an array of length @fsize plus @gsize that is the convolution
 * of @f and @g, not normalized.  This should be freed with free() when
 * you are finished with it.
 */
void
convolve(unsigned int *dest, const unsigned int *f,
         const unsigned int *g, size_t fsize, size_t gsize)
{
        int n, m;

        for (n = 0; n < fsize + gsize; n++) {
                unsigned int sum = 0;
                for (m = 0; m < gsize; m++) {
                        if (n - m > fsize || m > n)
                                continue;
                        sum += f[n - m] * g[m];
                }
                dest[n] = sum;
        }
}


