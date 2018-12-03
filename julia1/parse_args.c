#include "julia1_common.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

static void
bad_arg(const char *type, const char *optarg)
{
        fprintf(stderr, "Bad %s option: `%s'\n", type, optarg);
        exit(EXIT_FAILURE);
}

const char *
parse_args(int argc, char **argv)
{
        char *endptr;
        int opt;
        const char *outfile = "julia1.bmp";

        while ((opt = getopt(argc, argv, "Db:d:z:x:y:w:h:n:R:I:p:o:")) != -1) {
                switch (opt) {
                case 'D':
                        gbl.distance_est = true;
                        break;
                case 'b':
                        gbl.bailout = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-b", optarg);
                        gbl.bailoutsq = gbl.bailout * gbl.bailout;
                        break;
                case 'd':
                        gbl.dither = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-d", optarg);
                        break;
                case 'z':
                        gbl.zoom_pct = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-z", optarg);
                        break;
                case 'x':
                        gbl.zoom_xoffs = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-x", optarg);
                        break;
                case 'y':
                        gbl.zoom_yoffs = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-y", optarg);
                        break;
                case 'w':
                        gbl.width = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-w", optarg);
                        break;
                case 'h':
                        gbl.height = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-h", optarg);
                        break;
                case 'n':
                        gbl.n_iteration = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-n", optarg);
                        break;
                case 'o':
                        outfile = optarg;
                        break;
                case 'p':
                        gbl.pallette = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-p", optarg);
                        break;
                case 'R': /* Real part of c */
                        gbl.cx = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-R", optarg);
                        break;
                case 'I': /* Imaginary part of c */
                        gbl.cy = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-I", optarg);
                        break;
                default:
                        fprintf(stderr, "Unknown arg");
                        exit(EXIT_FAILURE);
                }
        }
        return outfile;
}


