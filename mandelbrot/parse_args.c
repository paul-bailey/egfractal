#include "mandelbrot_common.h"
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

void
parse_args(int argc, char **argv, struct optflags_t *optflags)
{
        static const struct option long_options[] = {
                { "print-palette",  no_argument,       NULL, 0 },
                { "distance-root",  required_argument, NULL, 1 },
                { "verbose",        no_argument,       NULL, 'v' },
                { "bailout",        required_argument, NULL, 'b' },
                { "smooth-option",  required_argument, NULL, 'd' },
                { "x-offs",         required_argument, NULL, 'x' },
                { "y-offs",         required_argument, NULL, 'y' },
                { "zoom-pct",       required_argument, NULL, 'z' },
                { "help",           no_argument,       NULL, '?' },
                { NULL,             0,                 NULL, 0 },
        };
        static const char *optstr = "Db:d:h:n:o:p:vw:x:y:z:?";

        for (;;) {
                char *endptr;
                int option_index = 0;
                int opt = getopt_long(argc, argv, optstr, long_options, &option_index);
                if (opt == -1)
                        break;

                switch (opt) {
                case 0:
                        optflags->print_palette = true;
                        break;
                case 1:
                    {
                        int v = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("--distance-root", optarg);
                        gbl.distance_root = 1.0L / (mfloat_t)v;
                        break;
                    }
                case 'D':
                        gbl.distance_est = true;
                        break;
                case 'b':
                        gbl.bailout = strtold(optarg, &endptr);
                        if (endptr == optarg || !isfinite(gbl.bailout))
                                bad_arg("-b (bailout radius)", optarg);
                        gbl.bailoutsqu = gbl.bailout * gbl.bailout;
                        break;
                case 'd':
                        gbl.dither = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-d (smoothing method)", optarg);
                        break;
                case 'h':
                        gbl.height = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-h (pixel-height", optarg);
                        break;
                case 'n':
                        gbl.n_iteration = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-n (no. iterations)", optarg);
                        break;
                case 'o':
                        optflags->outfile = optarg;
                        break;
                case 'p':
                        gbl.palette = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-p (palette number)", optarg);
                        break;
                case 'v':
                        gbl.verbose = true;
                        break;
                case 'w':
                        gbl.width = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("-w (pixel-width)", optarg);
                        break;
                case 'x':
                        gbl.zoom_xoffs = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-x --x-offs", optarg);
                        break;
                case 'y':
                        gbl.zoom_yoffs = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-y --y-offs", optarg);
                        break;
                case 'z':
                        gbl.zoom_pct = strtold(optarg, &endptr);
                        if (endptr == optarg)
                                bad_arg("-z --zoom-pct", optarg);
                        break;
                case '?':
                default:
                        fprintf(stderr, "Unknown option -%c\n", opt);
                        exit(EXIT_FAILURE);
                }
        }
}


