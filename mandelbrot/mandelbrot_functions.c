/*
 * mandelbrot_functions.c - alternative algorithms for a Mandelbrot set.
 *
 * Add the return values of these functions to c inside the
 * Mandelbrot iteration.  The more common "z * z" algorithm
 * is inlined in main.c rather put than here, to reduce
 * repetitive functional overhead in the iterative algorithm.
 * If you're using one of these then you should expect it to
 * be slow, because you're asking for something special.
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

/* TODO: These should go in library */
static complex_t
complex_sin(complex_t z)
{
	complex_t ret;
	ret.re = sinl(z.re) * coshl(z.im);
	ret.im = cosl(z.re) * sinhl(z.im);
	return ret;
}

static complex_t
complex_cos(complex_t z)
{
	complex_t ret;
	ret.re = cosl(z.re) * coshl(z.im);
	ret.im = sinl(z.re) * sinhl(z.im);
	return ret;
}

static complex_t
complex_inverse(complex_t z)
{
	complex_t ret;
	mfloat_t m = complex_modulus2(z);
	ret.re = z.re / m;
	ret.im = -z.im / m;
	return ret;
}

static complex_t
complex_div(complex_t num, complex_t den)
{
	complex_t ret;
	mfloat_t m = complex_modulus2(den);
	ret.re = num.re * den.re / m;
	ret.im = (num.im * den.re - num.re * den.im) / m;
	return ret;
}

static complex_t
sine_formula(complex_t z)
{
	return complex_sin(z);
}

static complex_t
cosine_formula(complex_t z)
{
	return complex_cos(z);
}

/* returns "(1 - z * z) / (z - z * z cos(z))" */
/* 
 * XXX: Too specific.  
 * Allow an arg like "rat=1,2.4,7.6:0,0,12,2"
 */
static complex_t
rat2_formula(complex_t z)
{
	complex_t num, den;
	/* num = "1 - z^2" */
	num = complex_addr(complex_mulr(complex_sq(z), -1), 1);

	/* "z^2 cos(z)" */
	den = complex_mul(complex_cos(z), complex_sq(z));
	/* "z - z^2 cos(z)" */
	den = complex_add(z, complex_mulr(den, -1));

	if (1)
		return complex_div(num, den);
	else
		return complex_mul(num, complex_inverse(den));
}

static int pow_exp = 1;
static complex_t
pow_formula(complex_t z)
{
	return complex_pow(z, pow_exp);
}

static const struct lut_t {
	const char *name;
	formula_t fn;
} lut[] = {
	{ "sin", sine_formula },
	{ "cos", cosine_formula },
	{ "rat2", rat2_formula },
	{ NULL, NULL },
};

formula_t
parse_formula(const char *name)
{
	const struct lut_t *t;
	if (!strncmp(name, "pow", 3)) {
		char *endptr;
		double exp;
		name += 3;
		exp = strtod(name, &endptr);
		if (endptr == name)
			return NULL;
		pow_exp = exp;
		return pow_formula;

	}
	for (t = lut; t->name != NULL; t++) {
		if (!strcmp(t->name, name))
			return t->fn;
	}
	return NULL;
}

