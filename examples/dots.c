/* Print an RGB file with some smoothed out dots. */
#include "fractal_common.h"
#include <math.h>

enum {
        SIZE = 300,
        FILTERLEN = 10,
        SQUARE_SIZE = 5,
};

static unsigned int filter[FILTERLEN];

static void
fill_square(Pxbuf *pxbuf, int row, int col, unsigned int color)
{
        int i, j;
        for (i = 0; i < SQUARE_SIZE; i++) {
                for (j = 0; j < SQUARE_SIZE; j++)
                        pxbuf_fill_pixel(pxbuf, row + i, col + j, color);
        }
}

int
main(void)
{
        FILE *fp;
        int row, col, i;
        Pxbuf *pxbuf = pxbuf_create(SIZE, SIZE, COLOR_YELLOW);
        if (!pxbuf) {
                fprintf(stderr, "OOM!\n");
                return 1;
        }

        fp = fopen("out.bmp", "wb");
        if (!fp) {
                pxbuf_free(pxbuf);
                fprintf(stderr, "Cannot open outfile\n");
                return 1;
        }

        for (i = 0; i < FILTERLEN; i++)
                filter[i] = FILTERLEN - i;

        for (row = 0; row < SIZE-SQUARE_SIZE; row += 20) {
                for (col = 0; col < SIZE-SQUARE_SIZE; col += 20)
                        fill_square(pxbuf, row, col, COLOR_BLUE);
        }

        for (row = 0; row < SIZE; row++)
                pxbuf_filter_row(pxbuf, row, filter, FILTERLEN);
        for (col = 0; col < SIZE; col++)
                pxbuf_filter_column(pxbuf, col, filter, FILTERLEN);

        pxbuf_print(pxbuf, fp);
        pxbuf_free(pxbuf);
        fclose(fp);
        return 0;
}

