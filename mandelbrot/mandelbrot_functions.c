/*
 * mandelbrot_functions.c - alternative algorithms for a Mandelbrot set.
 *
 * The more common "z * z + c" algorithm is inlined in main.c rather put
 * here, to reduce repetitive functional overhead in the iterative
 * algorithm.  If you're using one of these then you should expect it to
 * be slow, because you're asking for something special.
 *
 * FIXME: Most of these are not formulas meant to be iterated from z=0,
 * because they return infinity upon the first test.
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
#include "mandelbrot_common.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* TODO: Remove this if I don't get rata, ratb formulae working */
static complex_t rat_r, rat_s;

static int pow_exp = 1;
static complex_t
pow_formula(complex_t z, complex_t c)
{
        return complex_add(c, complex_pow(z, pow_exp));
}

static complex_t
pow_dformula(complex_t z, complex_t c)
{
        if (pow_exp == 0) {
                complex_t ret = { 0.0, 0 };
                return ret;
        }
        return complex_mulr(complex_pow(z, pow_exp - 1), pow_exp);
}

static complex_t
sine_formula(complex_t z, complex_t c)
{
        return complex_add(c, complex_sin(z));
}

static complex_t
cosine_formula(complex_t z, complex_t c)
{
        return complex_add(c, complex_cos(z));
}

static complex_t
cosine_dformula(complex_t z, complex_t c)
{
        /* d/dx cos(x) = -sin(x) */
        return complex_neg(complex_sin(z));
}

static complex_t
sine_dformula(complex_t z, complex_t c)
{
        /* d/dx sin(x) = cos(x) */
        return complex_cos(z);
}

/* TODO: Make all this crap below work */
#if 0
/*
 * Return "(1 - z * z) / (z - z * z cos(z)) + c"
 *
 * XXX: Too specific.
 * Allow an arg like "rat=1,2.4,7.6:0,0,12,2"
 * Then build array and call complex_poly().
 */
static complex_t
rat2_formula(complex_t z, complex_t c)
{
        static const mfloat_t ncoef[] = { 1.0, 0.0, -1.0 };
        complex_t dcoef[] = { { 0.0, 0.0 }, { 1.0, 0.0 }, { 0.0, 0.0 } };
        complex_t num, den;

        dcoef[2] = complex_neg(complex_cos(z));
        num = complex_poly(z, ncoef, 2);
        den = complex_cpoly(z, dcoef, 2);
        return complex_add(c, complex_div(num, den));
}

static complex_t
rat_ab_helper(complex_t z, complex_t a, complex_t b)
{
        /*
         * FIXME: This was taken from something that surely is wrong.
         * When z starts at 0, the very first answer will have already
         * diverged to infinity.
         */
        complex_t ret = complex_add(complex_inverse(z), complex_add(z, b));
        return complex_div(ret, a);
}


/* c is on the A plane */
static complex_t
rata_formula(complex_t z, complex_t c)
{
        complex_t b = complex_add(complex_mul(rat_r, c), rat_s);
        return rat_ab_helper(z, c, b);
}

/* c is on the B plane */
static complex_t
ratb_formula(complex_t z, complex_t c)
{
        complex_t a = complex_add(complex_mul(rat_r, c), rat_s);
        return rat_ab_helper(z, a, c);
}
#endif

#undef CMPLX
#define CMPLX(re_, im_)  { .re = (re_), .im = (im_) }

#define RSREAL(r_, s_) .r = CMPLX(r_, 0.0), .s = CMPLX(s_, 0.0)
#define FML(name_)      .fn = name_##_formula, .dfn = name_##_dformula

static const struct lut_t {
        const char *name;
        complex_t (*fn)(complex_t, complex_t);
        complex_t (*dfn)(complex_t, complex_t);
        complex_t r;
        complex_t s;
} lut[] = {
        { "sin",      FML(sine),   RSREAL(0.0, 0.0) },
        { "cos",      FML(cosine), RSREAL(0.0, 0.0) },
#if 0
        { "rat2",     FML(rat2),   RSREAL(0.0, 0.0) },
        { "snowshoe", FML(rata),   RSREAL(-1.0, -2.0) },
        { "spyglass", FML(rata),   .r = CMPLX(-1.0, 0.4), .s = CMPLX(2.0,  0.0) },
        { "tie",      FML(ratb),   .r = CMPLX(-1.0, 0.0), .s = CMPLX(0.0,  200.0) },
        { "standard", FML(rata),   RSREAL(-1.0, 2.0) },
        { "clot",     FML(rata),   RSREAL(0.0, 0.0) },
#endif
        { NULL, NULL },
};
int
parse_formula(const char *name)
{
        const struct lut_t *t;
        if (!strncmp(name, "pow", 3)) {
                char *endptr;
                double exp;
                name += 3;
                exp = strtod(name, &endptr);
                if (endptr == name)
                        return -1;
                pow_exp = exp;
                gbl.log_d = logl((long double)exp);
                gbl.formula = pow_formula;
		gbl.dformula = pow_dformula;
		return 0;

        }

        for (t = lut; t->name != NULL; t++) {
                if (!strcmp(t->name, name)) {
                        rat_r = t->r;
                        rat_s = t->s;
                        gbl.formula = t->fn;
                        gbl.dformula = t->dfn;
                        return 0;
                }
        }

        return -1;
}

