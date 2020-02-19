#include "bbrot2.h"
#include "fractal_common.h"
#include <stdint.h>

static inline void __attribute__((always_inline))
save_to_hist(struct thread_info_t *ti, int chan, complex_t c)
{
        unsigned int col = (int)(ti->wthird * (c.re + 2.0) + 0.5);
        unsigned int row = (int)(ti->hthird * (c.im + 1.5) + 0.5);
        if (col < ti->width && row < ti->height)
                ti->chanbuf[chan][row * ti->width + col]++;
}

/* Return true if inside cardioid or main bulb */
static bool
inside_cardioid_or_bulb(complex_t c)
{
        mfloat_t xp = c.re - 0.25L;
        mfloat_t ysq = c.im * c.im;
        mfloat_t q = xp * xp + ysq;
        if ((q * (q + xp)) < (0.25L * ysq))
                return true;
        xp = c.re + 1.0;
        if ((xp * xp + ysq) < (0.25L * ysq))
                return true;
        return false;
}

/*
 * This function runs twice - First just to see if it diverges
 * and again to save the points of the path from @c if it does
 * in fact diverge.
 *
 * Repeating this long iterative process twice sounds like it takes
 * a long time, and it does.  One alternative is to save everything
 * into the histogram @buf and save all the same points in a second
 * buffer, so that we can undo our modification of @buf should the
 * path not diverge.  However, this has experimentally proven to
 * take much longer.  It turns out that the process of saving data
 * into @hist (which includes the slightly mathy save_to_hist() above)
 * is time-consuming, and it's just faster if we don't do that unless
 * we already know the path diverges.
 */
static void
iterate_r(complex_t c, unsigned int chan,
                struct thread_info_t *ti, bool isdivergent)
{
        int i;
        complex_t z = { .re = 0.0L, .im = 0.0L };
        if (ti->formula) {
                for (i = 0; i < ti->n[chan]; i++) {
                        complex_t ztmp = ti->formula(z, c);
                        if (isdivergent && i > ti->min)
                                save_to_hist(ti, chan, ztmp);

                        /* Check both bailout and periodicity */
                        if (ztmp.re == z.re && ztmp.im == z.im)
                                return;
                        if (complex_modulus2(ztmp) >= ti->bailsqu) {
                                if (!isdivergent)
                                        iterate_r(c, chan, ti, true);
                                return;
                        }

                        z = ztmp;
                }
        } else {
                for (i = 0; i < ti->n[chan]; i++) {
                        /* next z = z^2 + c */
                        complex_t ztmp = complex_add(complex_sq(z), c);
                        if (isdivergent && i > ti->min)
                                save_to_hist(ti, chan, ztmp);

                        /* Check both bailout and periodicity */
                        if (complex_modulus2(ztmp) >= ti->bailsqu
                            || (ztmp.re == z.re && ztmp.im == z.im)) {
                                if (!isdivergent)
                                        iterate_r(c, chan, ti, true);
                                return;
                        }

                        z = ztmp;
                }
        }
}

/* NORM3 converts result of rand48_ll to some point in [0:3) */
#define NORM3  (3.0 / (double)MASK48)
#define MASK48 (((uint64_t)1 << 48) - 1)

/*
 * My own inline erand48().
 *
 * Besides removing the overhead of a function call for every point,
 * I also reduce a mfloat_t multiplication, because I can directly
 * "normalize" to [0:3) instead of multiplying erand48()'s already-
 * normalized-to-[0:1) result.
 *
 * XXX REVISIT: Multiplying a 48-bit value with a 32-bit value will
 * cause overflow in a 64-bit word.
 * Will the result of an overflowing multiplication
 * always be modulo the correct answer, on any architecture?
 * If yes, then I don't care.
 * If no, then I need to handle this with 80-bit storage.
 */
static inline __attribute__((always_inline)) uint64_t
rand48_il(uint64_t old)
{
        /* Formula given by man (3) erand48 */
        return (old * 0x5DEECE66Dul + 0xB) & MASK48;
}

/**
 * bbrot_thread - POSIX thread routine for bbrot2,
 *              or just the main routine if no pthreads
 */
void *
bbrot_thread(void *arg)
{
        uint64_t s48_x, s48_y;
        struct thread_info_t *ti = (struct thread_info_t *)arg;
        unsigned long i;

        s48_x = (uint64_t)ti->seeds[0] << 32
                | (uint64_t)ti->seeds[1] << 16 | (uint64_t)ti->seeds[2];
        s48_y = (uint64_t)ti->seeds[3] << 32
                | (uint64_t)ti->seeds[4] << 16 | (uint64_t)ti->seeds[5];

        for (i = 0; i < ti->points; i++) {
                complex_t c;
                int chan;

                if (ti->use_line_x) {
                        c.re = ti->line_x;
                } else {
                        s48_x = rand48_il(s48_x);
                        c.re = (double)s48_x * NORM3 - 2.0;
                }
                if (ti->use_line_y) {
                        c.im = ti->line_y;
                } else {
                        s48_y = rand48_il(s48_y);
                        c.im = (double)s48_y * NORM3 - 1.5;
                }
                if (!ti->formula && inside_cardioid_or_bulb(c))
                        continue;
                for (chan = 0; chan < ti->nchan; chan++) {
                        iterate_r(c, chan, ti, false);
                }
        }
        return NULL;
}


