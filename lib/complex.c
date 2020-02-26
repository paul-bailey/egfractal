/*
 * complex.c - Functions too big to inline in complex_helpers.h
 *
 * Copyright (c) 2018, Paul Bailey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "complex_helpers.h"

/*
 * TODO: Add some strange functions that were discovered while
 * debugging some of these.
 *
 * 1.  A happy accident when incorrectly writing sinhcosh():
 *
 *      static const double EINV = 0.3678794412;
 *      ex = exp(-x);
 *      *s = (1.0 - ex * EINV) / (2.0 * ex);
 *      *c = (1.0 + ex * EINV) / (2.0 * ex);
 *
 * 2.  Another incorrect sinhcosh(), where I got rh and lh mixed up:
 *
 *      mfloat_t ex = exp(-x);
 *      mfloat_t rh = 1.0 / (2.0 * ex);
 *      mfloat_t lh = ex / 2.0;
 *      *s = lh - rh;
 *      *c = lh + rh;
 *
 */

/*
 * This has been tested to agree with glibc's sinh() and cosh()
 * functions to within a billionth of a percent for x in range
 * of [-22:22).  Any further algebraic reduction has been tested
 * on my PC to not make any difference in speed.
 */
static inline __attribute__((always_inline)) void
sinhcosh(mfloat_t x, mfloat_t *s, mfloat_t *c)
{
        mfloat_t ex = exp(-x);
        *s = (1.0 - ex * ex) / (2.0 * ex);
        *c = (1.0 + ex * ex) / (2.0 * ex);
}

/* return sin(c) */
complex_t
complex_sin(complex_t c)
{
        complex_t ret;
        mfloat_t imcosh, imsinh;

        /*
         * ret.re = sin(c.re) * cosh(c.im);
         * ret.im = cos(c.re) * sinh(c.im);
         */
        sinhcosh(c.im, &imsinh, &imcosh);

        ret.re = sin(c.re) * imcosh;
        ret.im = cos(c.re) * imsinh;
        return ret;
}

/* return cos(c) */
complex_t
complex_cos(complex_t c)
{
        complex_t ret;
        mfloat_t imcosh, imsinh;

        /*
         * ret.re = cos(c.re) * cosh(c.im);
         * ret.im = sin(c.re) * sinh(c.im);
         */
        sinhcosh(c.im, &imsinh, &imcosh);

        /*
         * XXX REVISIT: You'd think sincos() is faster,
         * but it's a pain to set up config.h for this,
         * and I've tested it to actually make no difference in
         * execution time anyway.
         *
         * It doesn't show up clearly in the disassembly,
         * but perhaps gcc sees that the args to sin() and cos()
         * are identical and calls sincos() anyway.
         */
        ret.re = cos(c.re) * imcosh;
        ret.im = sin(c.re) * imsinh;
        return ret;
}


/* return 1.0 / c */
complex_t
complex_inverse(complex_t c)
{
        /*
         * Avoid overhead and additional arithmetic of
         * complex_div(tmp={1,0}, c) by calculating it inline here.
         */
        complex_t ret;
        mfloat_t m = complex_modulus2(c);
        if (m == 0.0) {
                ret.re = ret.im = INFINITY;
        } else {
                ret.re = c.re / m;
                ret.im = -c.im / m;
        }
        return ret;
}

/* return num / den */
complex_t
complex_div(complex_t num, complex_t den)
{
        complex_t ret;
        mfloat_t m = complex_modulus2(den);
        if (m == 0.0) {
                ret.re = ret.im = INFINITY;
        } else {
                ret.re = num.re * den.re / m;
                ret.im = (num.im * den.re - num.re * den.im) / m;
        }
        return ret;
}

/**
 * complex_poly - Return polynomial function of @c
 * @c: complex number
 * @coef: Array of length @order plus one containing the coefficients.
 *        coef[0] is the 0th order.
 * @order: Highest order of equation.
 *
 * Return coef[0] * c^0 + ... + coef[order] * c^order
 */
complex_t
complex_poly(complex_t c, const mfloat_t *coef, int order)
{
        int i;
        complex_t ret = { .re = coef[0], 0.0 };
        for (i = 1; i < order; i++) {
                /* Don't do a bunch of math that amounts to nothing */
                if (coef[i] == 0.0)
                        continue;
                ret = complex_add(ret,
                        complex_mulr(complex_pow(ret, i), coef[i]));
        }
        return ret;
}

/* complex_cpoly - like complex_poly, except with complex coeficients */
complex_t
complex_cpoly(complex_t c, const complex_t *coef, int order)
{
        int i;
        complex_t ret = coef[0];
        for (i = 1; i < order; i++) {
                if (coef[i].im == 0.0 && coef[i].re == 0.0)
                        continue;
                ret = complex_add(ret,
                        complex_mul(coef[i], complex_pow(ret, i)));
        }
        return ret;
}

/* Return an integer power of a complex number. */
complex_t
complex_pow(complex_t c, int pow)
{
        int sign = 1;
        complex_t ret = c;

        if (pow == 0) {
                ret.re = 1.0;
                ret.im = 0.0;
                return ret;
        }

        if (pow < 0) {
                sign = -1;
                pow = -pow;
        }

        while (pow-- > 1)
                ret = complex_mul(ret, c);

        if (sign < 0)
                ret = complex_inverse(ret);
        return ret;
}


