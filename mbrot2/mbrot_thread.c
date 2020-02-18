#include "mandelbrot_common.h"
#include <stdlib.h>

static const mfloat_t INSIDE = -1.0L;

static mfloat_t
iterate_normal(complex_t c, struct thread_info_t *ti)
{
        mfloat_t ret;
        unsigned long n = ti->n_iteration;
        unsigned long i;
        complex_t z = { .re = 0.0L, .im = 0.0L };

        /*
         * This is an ugly D.R.Y. violation,
         * but it keeps our normal algorithm
         * much faster.
         */
        if (ti->formula) {
                for (i = 0; i < n; i++) {
                        complex_t ztmp = ti->formula(z, c);
                        /* Too precise for our data types. Assume inside. */
                        if (ztmp.re == z.re && ztmp.im == z.im)
                                return INSIDE;

                        if (!complex_isfinite(ztmp)
                            || complex_modulus2(ztmp) > ti->bailoutsqu) {
                                break;
                        }

                        z = ztmp;
                }
        } else {
                for (i = 0; i < n; i++) {
                        /* new z = z^2 + c */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        /* Too precise for our data types. Assume inside. */
                        if (ztmp.re == z.re && ztmp.im == z.im)
                                return INSIDE;
                        if (complex_modulus2(ztmp) > ti->bailoutsqu)
                                break;
                        z = ztmp;
                }
        }
        if (i == n)
                return INSIDE;

        /* i < n from here */
        ret = (mfloat_t)i;
        if (ti->dither > 0) {
                if (!!(ti->dither & 01)) {
                        /* Smooth with distance estimate */
                        /*
                         * FIXME: This math is no longer accurate
                         * if we're not using z^2+c formula.
                         */
                        mfloat_t log_zn = logl(complex_modulus2(z)) / 2.0L;
                        mfloat_t nu = logl(log_zn / ti->log_d) / ti->log_d;
                        if (isfinite(log_zn) && isfinite(nu))
                                ret += 1.0L - nu;
                        /* if not finite, can't smooth with distance est. */
                }

                if (!!(ti->dither & 02)) {
                        /* dither */
                        int v = rand();
                        /*
                         * Don't guess if the compiler uses
                         * ASR instead of LSR
                         */
                        if (v < 0)
                                v = (~0ul << 8) | (v & 0xff);
                        else
                                v &= 0xff;
                        mfloat_t diff = (mfloat_t)v / 128.0L;
                        ret += diff;
                }

                if (ret < 0.0L)
                        ret = 0.0L;
                else if (ret > (mfloat_t)n)
                        ret = (mfloat_t)n;
        }
        return ret;
}

static mfloat_t
iterate_distance(complex_t c, struct thread_info_t *ti)
{
        unsigned long n = ti->n_iteration;
        unsigned long i;
        complex_t z = { .re = 0.0L, .im = 0.0L };
        complex_t dz = { .re = 1.0L, .im = 0.0L };
        mfloat_t zmod;
        if (ti->formula) {
                for (i = 0; i < n; i++) {
                        /* use different formula than our usual */
                        complex_t ztmp = ti->formula(z, c);
                        if (!complex_isfinite(ztmp))
                                break;

                        /* "dz = f'(z)*dz + 1.0" */
                        dz = complex_mul(dz, ti->dformula(z, c));
                        dz = complex_addr(dz, 1.0L);
                        z = ztmp;
                        if (complex_modulus2(z) > ti->bailoutsqu)
                                break;
                }
        } else {
                /*
                 * Standard Mandelbrot.
                 * Theoretically it's faster to just inline it here
                 * rather than use ti->formula as above.
                 */
                for (i = 0; i < n; i++) {
                        /* "z = z^2 + c" and "dz = 2.0 * z * dz + 1.0" */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        dz = complex_mul(z, dz);
                        dz = complex_mulr(dz, 2.0L);
                        dz = complex_addr(dz, 1.0L);
                        z = ztmp;
                        if (complex_modulus2(z) > ti->bailoutsqu)
                                break;
                }
        }
        zmod = complex_modulus(z);
        return zmod * logl(zmod) / complex_modulus(dz);
}

#if OLD_XY_TO_COMPLEX
static inline __attribute__((always_inline)) complex_t
xy_to_complex(int row, int col, struct thread_info_t *ti)
{
        complex_t c;

        c.re = 4.0L * (mfloat_t)col / (mfloat_t)ti->width  - 2.0L;
        c.im = 4.0L * (mfloat_t)row / (mfloat_t)ti->height - 2.0L;

        c.re = c.re * ti->zoom_pct - ti->zoom_xoffs;
        c.im = c.im * ti->zoom_pct - ti->zoom_yoffs;
        return c;
}
#else
/* scale pixels to points of mandelbrot set and handle zoom. */
static inline __attribute__((always_inline)) complex_t
xy_to_complex(int row, int col, struct thread_info_t *ti)
{
        complex_t c;
        /*
         * c.re = 4.0 * col / width - 2.0;
         * c.im = 4.0 * row / height - 2.0;
         *
         * Since "4.0", "2.0", width, and height are known
         * before the start of all these algorithms, @ti is
         * filled in with shortcuts for simplified math,
         * which is why the code below looks nothing like
         * the formula above.
         */
        c.re = (mfloat_t)col * ti->w4 - ti->zx;
        c.im = (mfloat_t)row * ti->h4 - ti->zy;
        return c;
}
#endif

static mfloat_t
mandelbrot_px(int row, int col, struct thread_info_t *ti)
{
        /* XXX: Quite an arbitrary choice */
        enum { THRESHOLD = 10 };
        mfloat_t ret;

        complex_t c = xy_to_complex(row, col, ti);
        if (!ti->formula && ti->n_iteration > THRESHOLD) {
                /*
                 * We know the formula for the main cardioid and bulb,
                 * and we know every point inside will converge.  So we
                 * can check that first before diving into the long
                 * iterative process.
                 */
                mfloat_t xp = c.re - 0.25L;
                mfloat_t ysq = c.im * c.im;
                mfloat_t q = xp * xp + ysq;
                if ((q * (q + xp)) < (0.25L * ysq))
                        return INSIDE;
                xp = c.re + 1.0L;
                if ((xp * xp + ysq) < (0.25L * ysq))
                        return INSIDE;
        }

        if (ti->distance_est)
                ret = iterate_distance(c, ti);
        else
                ret = iterate_normal(c, ti);
        if (!isfinite(ret))
                ret = INSIDE;
        return ret;
}

void *
mbrot_thread(void *arg)
{
        struct thread_info_t *ti = (struct thread_info_t *)arg;
        int row, col;

        mfloat_t *pbuf = ti->buf;
        for (row = ti->rowstart; row < ti->rowend; row += ti->skip) {
                for (col = ti->colstart; col < ti->colend; col++) {
                        mfloat_t v;
                        v = mandelbrot_px(row, col, ti);
                        if (v >= 0.0L && ti->min > v)
                                ti->min = v;
                        if (ti->max < v)
                                ti->max = v;
                        *pbuf++ = v;
                }
        }
        return NULL;
}
