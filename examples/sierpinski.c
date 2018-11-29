#include "fractal_common.h"
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
draw_square(int rowstart, int colstart, int size, int color)
{
	int col, row, colend = colstart + size, rowend = rowstart + size;
	for (row = rowstart; row < rowend; row++) {
		for (col = colstart; col < colend; col++) {
			pxbuf_fill_pixel(pxbuf, row, col, TO_RGB(color, color, color));
		}
	}
}

static inline int
tocolor(int depth)
{
	return (depth * 256) / N;
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
	if (depth == N || size == 0) 
		return;

	size /= 2;
	draw_square(rowstart, colstart + size / 2, size, tocolor(depth));
	draw_square(rowstart + size, colstart, size, tocolor(depth));
	draw_square(rowstart + size, colstart + size, size, tocolor(depth));
	koch_square(depth + 1, rowstart, colstart + size / 2, size);
	koch_square(depth + 1, rowstart + size, colstart, size);
	koch_square(depth + 1, rowstart + size, colstart + size, size);
}

int main(void)
{
	FILE *fp;
	pxbuf = pxbuf_create(width, height, COLOR_BLACK);
	if (!pxbuf)
		oom();
	koch_square(0, 0, 0, width);
	fp = fopen("koch.bmp", "wb");
	if (!fp) {
		fprintf(stderr, "Cannot open output file\n");
		return 1;
	}
	pxbuf_print(pxbuf, fp);
	fclose(fp);
	pxbuf_free(pxbuf);
	return 0;
}
