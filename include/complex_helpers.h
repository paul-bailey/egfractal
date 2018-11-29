/*
 * complex_helpers.h - frequently used complex-number helpers.
 *
 * I'm using this because for some reason, glibc's <complex.h> functions
 * and even language operations are bog slow, at least on my computer.
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
#ifndef COMPLEX_HELPERS_H
#define COMPLEX_HELPERS_H

#include <math.h>

/* TODO: Some CLEAN way of making this a program-by-program option */
typedef long double mfloat_t;

typedef struct complex_t {
        mfloat_t re;
        mfloat_t im;
} complex_t;

/* Multiply two complex numbers with each other. */
static inline complex_t complex_mul(complex_t a, complex_t b)
{
        complex_t ret;
        ret.re = a.re * b.re - a.im * b.im;
        ret.im = a.im * b.re + a.re * b.im;
        return ret;
}

/*
 * modulus(v)^2, since it's faster to square both sides of an equation
 * than to take the square root of one side.
 */
static inline mfloat_t complex_modulus2(complex_t v)
        { return v.re * v.re + v.im * v.im; }

/* Return the modulus of complex number v */
static inline mfloat_t complex_modulus(complex_t v)
        { return sqrt(complex_modulus2(v)); }

/* Multiply a complex number by itself */
static inline complex_t complex_sq(complex_t v)
{
        /* XXX: Faster to have just a mfloat_t tmp var & return v? */
        complex_t ret;
        ret.re = v.re * v.re - v.im * v.im;
        ret.im = 2.0L * v.im * v.re;
        return ret;
}

/* Add two complex numbers to each other */
static inline complex_t complex_add(complex_t a, complex_t b)
{
        a.re += b.re;
        a.im += b.im;
        return a;
}

/* Add a real number to a complex number */
static inline complex_t complex_addr(complex_t c, long double re)
{
        c.re += re;
        return c;
}

/* Multiply a complex number to a real number. */
static inline complex_t complex_mulr(complex_t c, long double re)
{
        c.re *= re;
        c.im *= re;
        return c;
}

/* Return an integer power of a complex number. */
static inline complex_t complex_pow(complex_t c, int pow)
{
        complex_t ret = c;
        while (pow-- > 1)
                ret = complex_mul(ret, c);
        return ret;
}

#endif /* COMPLEX_HELPERS_H */
