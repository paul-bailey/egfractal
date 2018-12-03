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
#include "fractal_common.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* TODO: Remove this if I don't get rata, ratb formulae working */

static int pow_exp = 1;
static complex_t
pow_fml(complex_t z, complex_t c)
{
        return complex_add(c, complex_pow(z, pow_exp));
}

static complex_t
pow_dfml(complex_t z, complex_t c)
{
        if (pow_exp == 0) {
                complex_t ret = { 0.0, 0 };
                return ret;
        }
        return complex_mulr(complex_pow(z, pow_exp - 1), pow_exp);
}

static complex_t
sine_fml(complex_t z, complex_t c)
{
        return complex_add(c, complex_sin(z));
}

static complex_t
cosine_fml(complex_t z, complex_t c)
{
        return complex_add(c, complex_cos(z));
}

static complex_t
cosine_dfml(complex_t z, complex_t c)
{
        /* d/dx cos(x) = -sin(x) */
        return complex_neg(complex_sin(z));
}

static complex_t
sine_dfml(complex_t z, complex_t c)
{
        /* d/dx sin(x) = cos(x) */
        return complex_cos(z);
}

static complex_t
burnship_fml(complex_t z, complex_t c)
{
        z.re = fabsl(z.re);
        z.im = fabsl(z.im);

        return complex_add(complex_sq(z), c);
}

/*
 * formulas like z = (|re| + i|im|)^2 are not differential everywhere,
 * so just use d(z^2)/dz like with regular Mandelbrot and hope for the
 * best.
 */
static complex_t
burnship_dfml(complex_t z, complex_t c)
{
        return complex_mulr(z, 2.0);
}

struct lutbl_t {
        const char *name;
        struct formula_t formula;
};

static struct lutbl_t lut[] = {
        {
                .name = "sin",
                .formula = { .fn = sine_fml, .dfn = sine_dfml }
        }, {
                .name = "cos",
                .formula = { .fn = cosine_fml, .dfn = cosine_dfml }
        }, {
                .name = "burnship",
                .formula = { .fn = burnship_fml, .dfn = burnship_dfml }
        }, {
                NULL, { NULL, NULL, }
        },
};

static struct formula_t pow_formula = {
        .fn = pow_fml,
        .dfn = pow_dfml,
};

const struct formula_t *
parse_formula(const char *name)
{
        struct lutbl_t *t;
        if (!strncmp(name, "pow", 3)) {
                char *endptr;
                double exp;
                name += 3;
                exp = strtod(name, &endptr);
                if (endptr == name)
                        return NULL;
                pow_exp = exp;
                pow_formula.log_d = logl((long double)exp);
                return &pow_formula;

        }

        for (t = lut; t->name != NULL; t++) {
                if (!strcmp(t->name, name)) {
                        /* TODO: Actual order */
                        t->formula.log_d = logl(2.0L);
                        return &t->formula;
                }
        }

        return NULL;
}

