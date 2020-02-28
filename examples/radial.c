#include "pxbuf.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

enum {
        SIZE = 3000,
};

/* sqrt(2)/2 */
#define MAX_RADIUS  (0.7071067812)

static double bluespread = MAX_RADIUS;
static double redspread = 0.5;
static double greenspread = 0.25;

static double
to_xy(int rowcol)
{
        return (double)rowcol / (double)SIZE - 0.5;
}

static double
radius(int row, int col)
{
        double x = to_xy(col);
        double y = to_xy(row);
        return sqrt(x * x + y * y);
}

static void
rad_to_px(double radius, struct pixel_t *px)
{
        px->x[0] = radius > bluespread
                        ? 0.0 : 1.0 - radius / bluespread;
        px->x[1] = radius > greenspread
                        ? 0.0 : 1.0 - radius / greenspread;
        px->x[2] = radius > redspread
                        ? 0.0 : 1.0 - radius / redspread;
}

int
main(int argc, char **argv)
{
        struct pixel_t px;
        int row, col;
        FILE *fp;
        Pxbuf *pb;

        if (argc < 2) {
                fprintf(stderr, "Expected: FILENAME.bmp\n");
                return 1;
        }

        pb = pxbuf_create(SIZE, SIZE);
        if (!pb) {
                fprintf(stderr, "OOM!\n");
                return 1;
        }
        for (row = 0; row < SIZE; row++) {
                for (col = 0; col < SIZE; col++) {
                        rad_to_px(radius(row, col), &px);
                        pxbuf_set_pixel(pb, &px, row, col);
                }
        }
        pxbuf_normalize(pb, PXBUF_NORM_SCALE, 3.0, true);

        fp = fopen(argv[1], "wb");
        if (!fp) {
                perror("IO error");
                pxbuf_destroy(pb);
                return 1;
        }

        pxbuf_print_to_bmp(pb, fp, PXBUF_NORM_CLIP);
        fclose(fp);
        pxbuf_destroy(pb);
        return 0;
}

