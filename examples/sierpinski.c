/*
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
#include "pxbuf.h"
#include <stdlib.h>
#include <stdio.h>

enum {
	/* Should not get bigger than this because of stack */
	N = 13
};

static int width = 8192;
static int height = 8192;
static Pxbuf *pxbuf = NULL;

static void
draw_square(int rowstart, int colstart, int size, struct pixel_t *px)
{
	int col, row;
        int colend = colstart + size;
        int rowend = rowstart + size;
	for (row = rowstart; row < rowend; row++) {
		for (col = colstart; col < colend; col++) {
			pxbuf_set_pixel(pxbuf, px, row, col);
		}
	}
}

static void
oom(void)
{
	fprintf(stderr, "OOM!\n");
	exit(EXIT_FAILURE);
}

static void
koch_square(int depth, int rowstart, int colstart, int size)
{
        struct pixel_t px;
	if (depth == N || size == 0)
		return;
        px.x[0] = px.x[1] = px.x[2] = (float)depth;

	size /= 2;
	draw_square(rowstart, colstart + size / 2, size, &px);
	draw_square(rowstart + size, colstart, size, &px);
	draw_square(rowstart + size, colstart + size, size, &px);
	koch_square(depth + 1, rowstart, colstart + size / 2, size);
	koch_square(depth + 1, rowstart + size, colstart, size);
	koch_square(depth + 1, rowstart + size, colstart + size, size);
}

int main(void)
{
	FILE *fp;
	pxbuf = pxbuf_create(width, height);
	if (!pxbuf)
		oom();
	koch_square(0, 0, 0, width);
	fp = fopen("koch.bmp", "wb");
	if (!fp) {
		fprintf(stderr, "Cannot open output file\n");
		return 1;
	}
	pxbuf_print_to_bmp(pxbuf, fp, PXBUF_NORM_SCALE);
	fclose(fp);
	pxbuf_destroy(pxbuf);
	return 0;
}
