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

static int
parse_spread(const char *arg)
{
        char *endptr;
        double r, g, b;

        r = strtod(arg, &endptr);
        if (endptr == arg || !isfinite(r) || r < 0.0 || *endptr != ':')
                return -1;

        ++endptr;
        g = strtod(endptr, &endptr);
        if (endptr == arg || !isfinite(g) || g < 0.0 || *endptr != ':')
                return -1;

        ++endptr;
        b = strtod(endptr, &endptr);
        if (endptr == arg || !isfinite(b) || b < 0.0 || *endptr != '\0')
                return -1;

        gbl.bluespread  = b;
        gbl.redspread   = r;
        gbl.greenspread = g;
        return 0;
}

void
parse_args(int argc, char **argv, struct optflags_t *optflags)
{
        static const struct option long_options[] = {
                { "print-palette",  no_argument,       NULL, 0 },
                { "distance",       optional_argument, NULL, 'D' },
                { "negate",         no_argument,       NULL, 2 },
		{ "formula",        required_argument, NULL, 3 },
                { "color-distance", no_argument,       NULL, 4 },
                { "equalize",       optional_argument, NULL, 5 },
                { "rmout",          optional_argument, NULL, 6 },
                { "nthread",        required_argument, NULL, 7 },
                { "spread",         optional_argument, NULL, 8 },
                { "fit",            no_argument,       NULL, 'f' },
                { "linked",         no_argument,       NULL, 'l' },
                { "verbose",        no_argument,       NULL, 'v' },
                { "bailout",        required_argument, NULL, 'b' },
                { "smooth-option",  required_argument, NULL, 'd' },
                { "x-offs",         required_argument, NULL, 'x' },
                { "y-offs",         required_argument, NULL, 'y' },
                { "zoom-pct",       required_argument, NULL, 'z' },
                { "help",           no_argument,       NULL, '?' },
                { NULL,             0,                 NULL, 0 },
        };
        static const char *optstr = "Db:d:h:lfn:o:p:vw:x:y:z:?";

        for (;;) {
                char *endptr;
                int option_index = 0;
                int opt = getopt_long(argc, argv, optstr,
                                      long_options, &option_index);
                if (opt == -1)
                        break;

                switch (opt) {
                case 0:
                        optflags->print_palette = true;
                        break;
                case 2:
                        gbl.negate = true;
                        break;
		case 3:
                    {
                        const struct formula_t *f;
			if ((f = parse_formula(optarg)) == NULL)
				bad_arg("--formula", optarg);
                        gbl.formula  = f->fn;
                        gbl.dformula = f->dfn;
                        gbl.log_d    = f->log_d;
			break;
                    }
                case 4:
                        gbl.color_distance = true;
                        break;
                case 5:
                        gbl.have_equalize = true;
                        if (optarg) {
                                gbl.equalize_root = strtold(optarg, &endptr);
                                if (endptr == optarg)
                                        bad_arg("--equalize", optarg);
                        } else {
                                gbl.equalize_root = 5.0;
                        }
                        break;
                case 6:
                        gbl.rmout = true;
                        if (optarg != NULL) {
                                gbl.rmout_scale = strtod(optarg, &endptr);
                                if (endptr == optarg)
                                        bad_arg("--rmout", optarg);
                        }
                        break;
                case 7:
                        gbl.nthread = strtoul(optarg, &endptr, 0);
                        if (endptr == optarg)
                                bad_arg("--nthread", optarg);
                        /* warn user they're being stupid */
                        if (gbl.nthread > 20) {
                                fprintf(stderr, "%d threads! You cray!\n",
                                        gbl.nthread);
                        }
                        break;
                case 8:
                        gbl.color_spread = true;
                        /* Above implies these two things */
                        gbl.color_distance = true;
                        gbl.distance_est = true;
                        if (optarg && parse_spread(optarg) < 0) {
                                bad_arg("--spread", optarg);
                        }
                        break;
                case 'D':
                        gbl.distance_est = true;
                        if (optarg) {
                                int v = strtoul(optarg, &endptr, 0);
                                if (endptr == optarg)
                                        bad_arg("--distance", optarg);
                                gbl.distance_root = 1.0L / (mfloat_t)v;
                        }
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
                case 'f':
                        gbl.fit = true;
                        break;
                case 'l':
                        gbl.linked = true;
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
                case 1:
                default:
                        fprintf(stderr, "Unknown option -%c\n", opt);
                        exit(EXIT_FAILURE);
                }
        }

        if (!EGFRACTAL_MULTITHREADED)
                gbl.nthread = 1;
}


