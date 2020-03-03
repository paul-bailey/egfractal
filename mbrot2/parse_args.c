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

/* Don't require _GNU_SOURCE */
static char *
local_strchrnul(const char *haystack, int needle)
{
        while (*haystack && *haystack != needle)
                ++haystack;
        return (char *)haystack;
}

static int
parse_norm_helper(const char *s, char *endptr)
{
        static const struct norm_lut_t {
                const char *name;
                enum pxbuf_norm_t method;
        } NORM_LUT[] = {
                { "rmout",      PXBUF_NORM_CROP, },
                { "eq",         PXBUF_NORM_EQ, },
                { "scale",      PXBUF_NORM_SCALE },
                { "clip",       PXBUF_NORM_CLIP },
                { "fit",        PXBUF_NORM_FIT },
                { NULL,         -1 },
        };
        const struct norm_lut_t *t;
        for (t = NORM_LUT; t->name != NULL; t++) {
                char *ep2;

                ep2 = (char *)local_strchrnul(s, '=');
                if (ep2 > endptr)
                        ep2 = endptr;
                if (!strncmp(t->name, s, ep2 - s)) {
                        int idx = gbl.nnorm++;
                        gbl.norm_method[idx] = t->method;
                        switch (*ep2) {
                        case '=':
                                s = ep2 + 1;
                                gbl.norm_scale[idx] = strtod(s, &ep2);
                                if (ep2 == s)
                                        bad_arg("-N or --norm", s);
                                break;
                        case ',':
                        case '\0':
                                gbl.norm_scale[idx] =
                                        t->method == PXBUF_NORM_CROP
                                        ? 3.0 : 1.0;
                                break;
                        default:
                                bad_arg("-N or --norm", s);
                        }
                        /* We found it, so move on */
                        break;
                }
        }
        return t->name ? 0 : -1;
}

static int
parse_norm(const char *arg)
{
        const char *s = arg;
        do {
                char *endptr;

                if (gbl.nnorm >= MAX_NORM_METHODS) {
                        fprintf(stderr, "Sorry, too many --norm options");
                        exit(EXIT_FAILURE);
                }

                endptr = local_strchrnul(s, ',');
                if (parse_norm_helper(s, endptr) < 0)
                        return -1;

                s = endptr;
                if (*s)
                        ++s;
        } while (*s != '\0');
        return 0;
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
                { "negate",         no_argument,       NULL, 2 },
		{ "formula",        required_argument, NULL, 3 },
                { "color-distance", no_argument,       NULL, 4 },
                { "nthread",        required_argument, NULL, 7 },
                { "spread",         optional_argument, NULL, 8 },
                { "distance",       optional_argument, NULL, 'D' },
                { "norm",           required_argument, NULL, 'N' },
                { "bailout",        required_argument, NULL, 'b' },
                { "smooth-option",  required_argument, NULL, 'd' },
                { "linked",         no_argument,       NULL, 'l' },
                { "verbose",        no_argument,       NULL, 'v' },
                { "x-offs",         required_argument, NULL, 'x' },
                { "y-offs",         required_argument, NULL, 'y' },
                { "zoom-pct",       required_argument, NULL, 'z' },
                { "help",           no_argument,       NULL, '?' },
                { NULL,             0,                 NULL, 0 },
        };
        static const char *optstr = "DN:b:d:h:ln:o:p:vw:x:y:z:?";

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
                case 'N':
                        if (parse_norm(optarg) < 0)
                                bad_arg("-N or --norm", optarg);
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

        if (gbl.nnorm == 0) {
                /* At least use the default normalization method */
                gbl.nnorm = 1;
                gbl.norm_method[0] = PXBUF_NORM_SCALE;
                gbl.norm_scale[0] = 1.0;
        }

        if (!EGFRACTAL_MULTITHREADED)
                gbl.nthread = 1;
}


