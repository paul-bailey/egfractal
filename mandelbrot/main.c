#include "mandelbrot.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

struct gbl_t gbl = {
        .height = 300,
        .width = 300,
        .depth = 3,
        .zoom_xoffs = 0.0,
        .zoom_yoffs = 0.0,
        .zoom_pct = 1.0,
        .bufsize = 0,
        .n_iteration = 1000,
        /* TODO: Make this a command-line option */
        .dither = 0,
        .pallette = 1,
};

static void
check_args(void)
{
        /* Only use 24-bit bmp for now. */
        if (gbl.depth == 3)
                gbl.depth = 24;
        if (gbl.depth != 24) {
                fprintf(stderr, "Invalid depth\n");
                exit(EXIT_FAILURE);
        }
}

static void
parse_args(int argc, char **argv)
{
        int opt;
        char *endptr;
        int have_width = 0;
        int have_height = 0;
        int have_ctr = 0;

        while ((opt = getopt(argc, argv, "z:cdw:h:x:y:n:p:")) != -1) {
                switch (opt) {
                case 'z':
                        gbl.zoom_pct = strtod(optarg, &endptr);
                        if (endptr == optarg || errno) {
                                fprintf(stderr, "Badd zoom value\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'c':
                        have_ctr = 1;
                        break;
                case 'd':
                        gbl.dither = 1;
                        break;
                case 'w':
                        gbl.width = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg || errno) {
                                fprintf(stderr, "Bad width\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'h':
                        gbl.height = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg || errno) {
                                fprintf(stderr, "Bad height\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'x':
                        gbl.zoom_xoffs = strtod(optarg, &endptr);
                        if (endptr == optarg || errno) {
                                fprintf(stderr, "Bad X offset\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'y':
                        gbl.zoom_yoffs = strtod(optarg, &endptr);
                        if (endptr == optarg || errno) {
                                fprintf(stderr, "Bad Y offset\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'p':
                        gbl.pallette = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg || errno || gbl.pallette >= N_PALLETTE) {
                                fprintf(stderr, "Badd pallette\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'n':
                        gbl.n_iteration = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg || errno) {
                                fprintf(stderr, "Bad no. iterations\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                default:
                        fprintf(stderr, "Bad arg\n");
                        break;
                }
        }

        /* One of these without the other implies square */
        if (have_width && !have_height)
                gbl.height = gbl.width;
        else if (!have_width && have_height)
                gbl.width = gbl.height;

        if (have_ctr)
                gbl.zoom_yoffs = 0.5 - gbl.zoom_pct / 2.0;

        check_args();
}

int
main(int argc, char **argv)
{
        FILE *fp;
        unsigned char *buf;

        parse_args(argc, argv);

        gbl.bufsize = gbl.height * gbl.width * gbl.depth / 8;
        buf = malloc(gbl.bufsize);
        if (!buf) {
                fprintf(stderr, "Can't alloc!?!?\n");
                return 1;
        }

        mandelbrot24(buf, gbl.n_iteration);
        fp = fopen("out.bmp", "wb");
        if (!fp) {
                fprintf(stderr, "Can't open out.bmp\n");
                return 1;
        }
        bmp_print(fp, buf, gbl.width, gbl.height, gbl.depth);
        fclose(fp);
        free(buf);
        return 0;
}



