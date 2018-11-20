#include "fractal_common.h"
#include <stdio.h>
#include <stdlib.h>

enum {
        DEPTH = 24,
        WIDTH = 3000,
        HEIGHT = 3000,
        NPIX = WIDTH * HEIGHT,
        NBYTES = NPIX * DEPTH / 8,
};

static unsigned char buf[NBYTES];

int
main(void)
{
        int i;
        FILE *fp = fopen("out.bmp", "wb");
        if (!fp) {
                fprintf(stderr, "Cannot open output file\n");
                return 1;
        }
        fseek(fp, 0, SEEK_SET);
        for (i = 0; i < NBYTES; i++) {
                int v = rand();
                buf[i] = v & 0xff;
        }
        bmp_print(fp, buf, WIDTH, HEIGHT, DEPTH);
        fclose(fp);
        return 0;
}
