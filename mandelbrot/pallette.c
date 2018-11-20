/* pallette.c */
#include "mandelbrot.h"
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

enum {
        RGB_SIZE = 100,
        FILT_SIZE = 20,
};

static unsigned int red[RGB_SIZE];
static unsigned int green[RGB_SIZE];
static unsigned int blue[RGB_SIZE];
static unsigned int filt[FILT_SIZE];

static unsigned int
linear_interp(unsigned int *buf, double i, int n, int isrolly)
{
        /* Modulo-repeat */
        unsigned int ret;
        double frac, diff;
        int idx = (int)i;

        /* Use modulo and wrap around */
        assert(idx >= 0);
	if (idx == 1)
		return 255;
        if (idx == 0)
                return 0;
	idx %= RGB_SIZE;
        if (isrolly && idx < RGB_SIZE/2)
		idx = RGB_SIZE - idx;

        /* Linear interpolation */
        frac = modf(i, &i);
        diff = (double)buf[idx + 1] - (double)buf[idx];
        ret = buf[idx] + (unsigned int)(frac * diff + 0.5);

        /* dither */
        if (0)
                ret += rand() & 0x7;

        if (ret > 255)
                ret = 255;

        return ret;
}

static void
convolve_helper(unsigned int *buf)
{
        unsigned int tbuf[RGB_SIZE + FILT_SIZE];
        convolve(tbuf, buf, filt, RGB_SIZE, FILT_SIZE);
        memcpy(buf, tbuf, RGB_SIZE * sizeof(*buf));
}

static void
normalize(unsigned int *buf)
{
        int i;
        unsigned int max = 0;
        double scalar;
        for (i = 0; i < RGB_SIZE; i++) {
                if (buf[i] > max)
                        max = buf[i];
        }
        if (max == 0)
                return;

        if (1 && max < 255)
                fprintf(stderr, "Blowing up, may lose precision\n");

        scalar = 256.0 / (double)max;
        for (i = 0; i < RGB_SIZE; i++) {
                buf[i] = (unsigned)((double)buf[i] * scalar + 0.5);
                if (buf[i] >= 255)
                        buf[i] = 255;
        }
}

static void
splash_rgb(const char *color, const unsigned int *buf)
{
        int i;

        printf("%s:\n", color);
        for (i = 0; i < RGB_SIZE; i++) {
                if (i > 0)
                        putchar(!!(i & 7) ? ' ' : '\n');
                printf("%3d", buf[i]);
        }
        putchar('\n');
}

void
pallette_3_init(void)
{
	int i = 0;
	do {
		int j;
		for (j = 0; j < RGB_SIZE/4; j++) {
			if (i >= RGB_SIZE)
				break;
			green[i] = 0;
			blue[i] = j;
			red[i] = RGB_SIZE/4 - j;
			green[i] = i * i;
			i++;
		}
		for (j = 0; j < RGB_SIZE/4; j++) {
			if (i >= RGB_SIZE)
				break;
			green[i] = 0;
			blue[i] = RGB_SIZE/4 - j;
			red[i] = j;
			green[i] = i * i;
			i++;
		}
	} while (i < RGB_SIZE);
	for (i = 0; i < FILT_SIZE; i+=2)
		filt[i] = 1;
	convolve_helper(red);
	convolve_helper(blue);
	convolve_helper(green);
	normalize(red);
	normalize(blue);
	normalize(green);
	red[0] = blue[0] = green[0] = 0;
}

void
pallette_1_init(void)
{
        int i;

        /* TODO: May have more sophisticated filter */
        for (i = 0; i < FILT_SIZE; i+= 2)
                filt[i] = 1;
        for (i = 1; i < FILT_SIZE; i+=2)
                filt[i] = 2;

        /* Something to start with... */
        for (i = 1; i < 30; i++) {
                blue[i] = 12;
                red[i] = 12;
                green[i] = 12;
        }

        /* Blue tap down low */
        for (i = 30; i < 37; i++) {
                blue[i] = 255;
                green[i] = 30;
        }

        /* Still mostly blue, but add others to keep "dithered" */
        for (i = 37; i < 45; i++) {
                blue[i] = 255;
                red[i] = 25;
        }

        /* Yellow tap next */
        for (i = 45; i < 60; i++) {
                green[i] = 255;
                red[i] = 255;
        }

        /* Red tap next */
        for (i = 60; i < 75; i++)
                red[i] = 255;

        /* Green tap next */
        for (i = 75; i < 90; i++)
                green[i] = 255;

        /* White tap on top */
        for (i = 90; i < RGB_SIZE; i++) {
                red[i] = 255;
                blue[i] = 255;
                green[i] = 255;
        }

        convolve_helper(red);
        convolve_helper(blue);
        convolve_helper(green);

        normalize(red);
        normalize(green);
        normalize(blue);

        /* for now, for debugging... */
        if (1) {
                splash_rgb("red", red);
                splash_rgb("green", green);
                splash_rgb("blue", blue);
        }
}

void
pallette_1_rgb(struct rgb_t *rgb, double i, int n)
{
	rgb->red = linear_interp(red, i, n, 0);
	rgb->green = linear_interp(green, i, n, 0);
	rgb->blue = linear_interp(blue, i, n, 0);
}

void
pallette_2_rgb(struct rgb_t *rgb, double i, int n)
{
	rgb->red = linear_interp(red, i, n, 0);
	rgb->green = linear_interp(green, i, n, 0);
	rgb->blue = linear_interp(blue, i, n, 0);
}

