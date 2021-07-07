#ifndef PXBUF_H
#define PXBUF_H

#include <stdio.h>
#include <stdbool.h>

#define DBG_PXBUF 0

#if DBG_PXBUF
/* Include <assert.h> in any C file that uses this */
# define PXBUF_SANITY(P_) assert(pxbuf_check_finite(P_))
#else
# define PXBUF_SANITY(P_) do { (void)0; } while (0)
#endif

/* XXX REVISIT: Is "float" precise enough? */
struct pixel_t {
        /* Indexed by enum pxbuf_chan_t */
        float x[3];
};

enum pxbuf_chan_t {
        PXBUF_ALLCHAN = -1,
        PXBUF_BLUE = 0,
        PXBUF_GREEN,
        PXBUF_RED,
};

/**
 * enum pxbuf_norm_t - Arg to pxbuf_print_to_bmp
 *                               and pxbuf_normalize
 * @PXBUF_NORM_CROP: Crop statistical outliers (defined as three times the
 *         standard deviation).  This blindly assumes that a pxbuf
 *         is normally distributed.
 * @PXBUF_NORM_CLIP: Clip anything out of the normal range without any
 *         additional processing.
 * @PXBUF_NORM_SCALE: Regular normalization.  If the lowest value is greater
 *         than zero, then it is scaled, but not stretched down to zero.
 *         If it is less than zero, then the image is offset-corrected
 *         before normalizing.
 * @PXBUF_NORM_FIT: Like @PXBUF_NORM_SCALE, except that the image is offset
 *         before normalizing so that the lowest value is zero.
 * @PXBUF_NORM_EQ: Perform histogram equalization.
 */
enum pxbuf_norm_t {
        PXBUF_NORM_CROP,
        PXBUF_NORM_CLIP,
        PXBUF_NORM_SCALE,
        PXBUF_NORM_FIT,
        PXBUF_NORM_EQ,
};

typedef struct pxbuf_t Pxbuf;

extern struct pixel_t *pxbuf_get_pixel(Pxbuf *pxbuf,
                        unsigned int row, unsigned int col);
extern int pxbuf_set_pixel(Pxbuf *pxbuf, struct pixel_t *pixel,
                        unsigned int row, unsigned int col);

int pxbuf_normalize(Pxbuf *pxbuf, enum pxbuf_norm_t method,
                        float deviation, bool linked);
extern int pxbuf_print_to_bmp(Pxbuf *pxbuf, FILE *fp,
                enum pxbuf_norm_t method);
extern Pxbuf *pxbuf_read_from_bmp(FILE *fp);

extern Pxbuf *pxbuf_create(int width, int height);
extern void pxbuf_destroy(Pxbuf *pxbuf);
extern int pxbuf_rotate(Pxbuf *pxbuf, bool cw);
extern void pxbuf_negate(Pxbuf *pxbuf);
extern void pxbuf_overlay(Pxbuf *dst, Pxbuf *src, double ratio);

extern void pxbuf_get_dimensions(Pxbuf *pxbuf, int *width, int *height);

#if DBG_PXBUF
extern bool pxbuf_check_finite(Pxbuf *pxbuf);
#endif

#endif /* PXBUF_H */

