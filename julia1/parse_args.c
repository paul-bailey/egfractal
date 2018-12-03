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
        static const char *optstr = "Db:d:z:x:y:w:h:n:R:I:p:o:";
        static const struct option long_options[] = {
                { "distance-root",  required_argument, NULL, 1 },
                { "negate",         no_argument,       NULL, 2 },
                { "equalize",       optional_argument, NULL, 3 },
                { "color-distance", no_argument,       NULL, 4 },
                { NULL,             0,                 NULL, 0 },
        };
        char *endptr;
        int opt;
        const char *outfile = "julia1.bmp";
        int option_index = 0;

        while ((opt = getopt_long(argc, argv, optstr, long_options, &option_index)) != -1) {
                switch (opt) {
                case 1:
                    {
                        int v = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("--distance-root", optarg);
                        gbl.distance_root = 1.0L / (mfloat_t)v;
                        break;
                    }
                case 2:
                        gbl.negate = true;
                        break;
                case 3:
                        gbl.equalize = true;
                        if (optarg) {
                                gbl.eq_option = strtold(optarg, &endptr);
                                if (endptr == optarg)
                                        bad_arg("--equalize", optarg);
                        } else {
                                gbl.eq_option = 1.0L;
                        }
                        break;
                case 4:
                        gbl.color_distance = true;
                        break;
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
                        fprintf(stderr, "Unknown arg\n");
                        exit(EXIT_FAILURE);
                }
        }
        return outfile;
}


