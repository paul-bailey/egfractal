#include "pxbuf.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct pxbuf_t {
        unsigned int height;
        unsigned int width;
        struct pixel_t *buf;
};

enum {
        BI_RGB = 0,
        BI_RLE8,
        BI_RLE4,
        BI_BITFIELDS,
        BI_JPEG,
        BI_PNG,
        BI_ALPHABITFIELDS,
        BI_CMYK = 11,
        BI_CMYKRLE8,
        BI_CMYKRLE4,
};

static unsigned char *
pack32(unsigned char *p, unsigned long v)
{
        *p++ = v & 0xff;
        v >>= 8;
        *p++ = v & 0xff;
        v >>= 8;
        *p++ = v & 0xff;
        v >>= 8;
        *p++ = v & 0xff;
        return p;
}

static unsigned char *
pack16(unsigned char *p, unsigned long v)
{
        *p++ = v & 0xff;
        v >>= 8;
        *p++ = v & 0xff;
        return p;
}

static unsigned long
unpack16(unsigned char *p)
{
        unsigned long ret;
        ret = p[1];
        ret <<= 8;
        return ret + p[0];
}

static unsigned long
unpack32(unsigned char *p)
{
        unsigned long ret;
        ret = p[3];
        ret <<= 8;
        ret += p[2];
        ret <<= 8;
        ret += p[1];
        ret <<= 8;
        return ret + p[0];
}


static const float CLIP_MAX = 255.0 / 256.0;

static inline unsigned char crop_255(unsigned int v)
        { return v >= 255 ? 255 : v; }

static inline float crop_255f(float v)
        { return (v > CLIP_MAX) ? CLIP_MAX : (v < 0.0 ? 0.0 : v); }


static inline struct pixel_t *
pxptr(Pxbuf *pxbuf, unsigned int row, unsigned col)
{
        return &pxbuf->buf[row * pxbuf->width + col];
}

struct pixel_t *
pxbuf_get_pixel(Pxbuf *pxbuf, unsigned int row, unsigned int col)
{
        if (row >= pxbuf->height || col >= pxbuf->width)
                return NULL;
        return pxptr(pxbuf, row, col);
}

int
pxbuf_set_pixel(Pxbuf *pxbuf, struct pixel_t *pixel,
                unsigned int row, unsigned int col)
{
        assert(row < pxbuf->height);
        assert(col < pxbuf->width);
        if (row >= pxbuf->height || col >= pxbuf->width)
                return -1;
        memcpy(pxptr(pxbuf, row, col), pixel, sizeof(*pixel));
        return 0;
}

#define for_each_pixel(iter, pxbuf) \
        for (px = (pxbuf)->buf; \
             px < &(pxbuf)->buf[pxbuf->height * pxbuf->width]; \
             px++)

#if DBG_PXBUF
bool
pxbuf_check_finite(Pxbuf *pxbuf)
{
        struct pixel_t *px;
        for_each_pixel(px, pxbuf) {
                if (!isfinite(px->x[0]))
                        return false;
                if(!isfinite(px->x[1]))
                        return false;
                if(!isfinite(px->x[2]))
                        return false;
        }
        return true;
}
#endif

/* helper to hist_eq */
static float
cdf_scale(float f, unsigned long *cdf, float maxl, float range)
{
        int v = crop_255((int)(f * 256.0 + 0.5));
        v = (cdf[v] - cdf[0]) * maxl / range;
        return crop_255f((double)v / 256.0);
}

/* Helper to hist_eq */
static void
save_to_hist(float f, unsigned long *hist)
{
        int v = crop_255((int)(f * 256.0 + 0.5));
        hist[v]++;
}

static void
hist_eq(Pxbuf *pxbuf, float max, enum pxbuf_chan_t chan)
{
        /* TODO: Implement this */
        enum { HIST_SIZE = 256 };
        unsigned long histogram[256];
        unsigned long cdf[256];
        unsigned long cdfmax, cdfrange;
        struct pixel_t *px;
        int i;
        int maxl = crop_255((int)max * 256.0 + 0.5);

        memset(histogram, 9, sizeof(histogram));
        memset(cdf, 0, sizeof(cdf));

        for_each_pixel(px, pxbuf) {
                if (chan < 0) {
                        for (i = 0; i < 3; i++)
                                save_to_hist(px->x[i], histogram);
                } else {
                        save_to_hist(px->x[chan], histogram);
                }
        }
        cdfmax = 0;
        for (i = 0; i < 256; i++) {
                cdfmax += histogram[i];
                cdf[i] = cdfmax;
        }

        /* TODO: Maybe slumpify */
        cdfrange = cdfmax - cdf[0];
        for_each_pixel(px, pxbuf) {
                if (chan < 0) {
                        for (i = 0; i < 3; i++) {
                                px->x[i] = cdf_scale(px->x[i],
                                                cdf, maxl, cdfrange);
                        }
                } else {
                        px->x[chan] = cdf_scale(px->x[chan],
                                                cdf, maxl, cdfrange);
                }
        }
}

/*
 * Shift offset of @pxbuf, but only if its lowest
 * value is less than zero.  We'd rather not shift things
 * if everything is positive, becase after all it just
 * might be a bright image with no pure black in it.
 *
 * Return the maximum value found.
 */
static float
maybe_offset_correct(Pxbuf *pxbuf, bool force, enum pxbuf_chan_t chan)
{
        float max = -INFINITY, min = INFINITY;
        struct pixel_t *px;
        int i;

        PXBUF_SANITY(pxbuf);
        for_each_pixel(px, pxbuf) {
                if (chan < 0) {
                        for (i = 0; i < 3; i++) {
                                if (max < px->x[i])
                                        max = px->x[i];
                                if (min > px->x[i])
                                        min = px->x[i];
                        }
                } else {
                        if (max < px->x[chan])
                                max = px->x[chan];
                        if (min > px->x[chan])
                                min = px->x[chan];
                }
        }

        PXBUF_SANITY(pxbuf);
        if (force || min < 0.0) {
                max -= min;
                for_each_pixel(px, pxbuf) {
                        if (chan < 0) {
                                for (i = 0; i < 3; i++)
                                        px->x[i] -= min;
                        } else {
                                px->x[chan] -= min;
                        }
                }
        }
        PXBUF_SANITY(pxbuf);
        return max;
}

static float
shave_outliers(Pxbuf *pxbuf, float max,
                float deviation, enum pxbuf_chan_t chan)
{
        /* "n" instead of "n-1" because we have the whole population */
        double divn = 1.0 / ((double)(pxbuf->height * pxbuf->width));
        double mean, sumsq, stddev, sum, stdmin, stdmax;
        struct pixel_t *px;

        if (chan >= 0)
                divn /= 3.0;

        sum = 0.0;
        for_each_pixel(px, pxbuf) {
                if (chan < 0) {
                        int i;
                        for (i = 0; i < 3; i++) {
                                sum += px->x[i];
                        }
                } else {
                        sum += px->x[chan];
                }
        }
        mean = sum * divn;
        /* offset correction should have occured before calling us */
        assert(mean >= 0.0);

        sumsq = 0.0;
        for_each_pixel(px, pxbuf) {
                double diff;
                if (chan < 0) {
                        int i;
                        for (i = 0; i < 3; i++) {
                                diff = px->x[i] - mean;
                                sumsq += diff * diff;
                        }
                } else {
                        diff = px->x[chan] - mean;
                        sumsq += diff * diff;
                }
        }
        stddev = sqrt(sumsq * divn);

        /* define "outlier" as @deviation times the standard deviation */
        stdmin = mean - deviation * stddev;
        stdmax = mean + deviation * stddev;
        for_each_pixel(px, pxbuf) {
                if (chan < 0) {
                        int i;
                        for (i = 0; i < 3; i++) {
                                if (px->x[i] < stdmin)
                                        px->x[i] = stdmin;
                                else if (px->x[i] > stdmax)
                                        px->x[i] = stdmax;
                        }
                } else {
                        if (px->x[chan] < stdmin)
                                px->x[chan] = stdmin;
                        else if (px->x[chan] > stdmax)
                                px->x[chan] = stdmax;
                }
        }
        return stdmax;
}

/* Make sure every channel of every pixel is in range [0:1) */
static void
normalize_helper(Pxbuf *pxbuf, float max, enum pxbuf_chan_t chan)
{
        struct pixel_t *px;
        float range_mult;

        if (max <= 0.0) {
                /* Spinal Tap album cover */
                range_mult = 0.0;
        } else {
                range_mult = 1.0 / max;
                /* Unlikely, but just in case */
                if (!isnormal(range_mult))
                        range_mult = 0.0;
        }

        PXBUF_SANITY(pxbuf);

        for_each_pixel(px, pxbuf) {
                if (chan < 0) {
                        px->x[0] = crop_255f(px->x[0] * range_mult);
                        px->x[1] = crop_255f(px->x[1] * range_mult);
                        px->x[2] = crop_255f(px->x[2] * range_mult);
                } else {
                        px->x[chan] = crop_255f(px->x[chan]
                                                * range_mult);
                }
        }

        PXBUF_SANITY(pxbuf);
}

static void
pxbuf_clip(Pxbuf *pxbuf, enum pxbuf_chan_t chan)
{
        struct pixel_t *px;
        for_each_pixel(px, pxbuf) {
                if (chan < 0) {
                        px->x[0] = crop_255f(px->x[0]);
                        px->x[1] = crop_255f(px->x[1]);
                        px->x[2] = crop_255f(px->x[2]);
                } else {
                        px->x[chan] = crop_255f(px->x[chan]);
                }
        }
}

void
pxbuf_negate(Pxbuf *pxbuf)
{
        struct pixel_t *px;
        float max;
        PXBUF_SANITY(pxbuf);
        max = maybe_offset_correct(pxbuf, false, -1);
        /* Set to true only while debugging */
        PXBUF_SANITY(pxbuf);
        for_each_pixel(px, pxbuf) {
                px->x[0] = max - px->x[0];
                px->x[1] = max - px->x[1];
                px->x[2] = max - px->x[2];
        }
        /* Set to true only while debugging */
        PXBUF_SANITY(pxbuf);
}

static int
pxbuf_normalize_helper(Pxbuf *pxbuf,
        enum pxbuf_norm_t method, float deviation,
        enum pxbuf_chan_t chan)
{
        if (chan >= 3)
                return -1;

        PXBUF_SANITY(pxbuf);
        if (method == PXBUF_NORM_CLIP) {
                pxbuf_clip(pxbuf, chan);
        } else {
                float max = maybe_offset_correct(pxbuf,
                                        method == PXBUF_NORM_FIT,
                                        chan);
                PXBUF_SANITY(pxbuf);
                switch (method) {
                case PXBUF_NORM_CROP:
                        max = shave_outliers(pxbuf, max, deviation, chan);
                        normalize_helper(pxbuf, max, chan);
                        break;
                case PXBUF_NORM_FIT:
                case PXBUF_NORM_SCALE:
                        normalize_helper(pxbuf, max, chan);
                        break;
                case PXBUF_NORM_EQ:
                        normalize_helper(pxbuf, max, chan);
                        hist_eq(pxbuf, max, chan);
                        break;
                default:
                        return -1;
                }
        }
        return 0;
}

int
pxbuf_normalize(Pxbuf *pxbuf, enum pxbuf_norm_t method,
                float deviation, bool linked)
{
        if (linked) {
                return pxbuf_normalize_helper(pxbuf, method,
                                                deviation, -1);
        } else {
                int i, res;
                for (i = 0; i < 3; i++) {
                        res = pxbuf_normalize_helper(
                                        pxbuf, method, deviation, i);
                        if (res != 0)
                                return res;
                }
        }
        return 0;
}

Pxbuf *
pxbuf_read_from_bmp(FILE *fp)
{
        struct stat st;
        unsigned char *buf, *p;
        size_t nread, offset;
        long width, height, row, col, padding;
        Pxbuf *ret;

        if (fstat(fileno(fp), &st) != 0)
                return NULL;
        /*
         * errno might have been set from rewind
         * if fp is stdin, so ignore it
         */
        errno = 0;
        buf = malloc(st.st_size);
        if (!buf)
                return NULL;
        nread = read(fileno(fp), buf, st.st_size);
        if (nread != st.st_size)
                goto err;

        /* Only support Windows-style BMP */
        if (buf[0] != 'B' || buf[1] != 'M') {
                fprintf(stderr, "%d%d != BM\n", buf[0], buf[1]);
                goto err;
        }

        /*
         * Sanity-check that the bmp offset doesn't
         * send us flying off the map.
         */
        offset = unpack32(&buf[10]);
        if (offset > st.st_size)
                goto err;

        /* Only accept Windows BITMAPINFOHEADER */
        if (unpack32(&buf[14]) != 40)
                goto err;

        width = unpack32(&buf[18]);
        height = unpack32(&buf[22]);
        if (unpack16(&buf[26]) != 1)
                goto err;
        /* Only accept 24-bit color depth */
        if (unpack16(&buf[28]) != 24)
                goto err;

        /* Only accept BI_RGB compression */
        if (unpack32(&buf[30]) != BI_RGB)
                goto err;

        /* Sanity-check the buffer size */
        if (((((3 * width) * 4) / 32) * height) > st.st_size)
                goto err;

        ret = pxbuf_create(width, height);
        if (!ret)
                goto err;

        padding = (width * 3) % 4;
        p = &buf[offset];
        for (row = 0; row < height; row++) {
                for (col = 0; col < width; col++) {
                        struct pixel_t *px;
                        px = pxbuf_get_pixel(ret, row, col);
                        /* TODO: is px BGR like BMP, or is it RGB? */
                        px->x[0] = *p++;
                        px->x[1] = *p++;
                        px->x[2] = *p++;
                }
                p += padding;
        }
        return ret;

err:
        free(buf);
        return NULL;
}

int
pxbuf_print_to_bmp(Pxbuf *pxbuf, FILE *fp, enum pxbuf_norm_t method)
{
        enum {
                T_SIZE = 14, /* file header */
                DIB_SIZE = 40, /* BMP info header */
                HDR_SIZE = DIB_SIZE + T_SIZE,
        };
        unsigned char buffer[HDR_SIZE];
        int row, col;
        int depth = 3; /* plain-vanilla 24-bit rgb */
        int padding = (pxbuf->width * depth) % 4;
        int arr_size = (pxbuf->width * depth + padding) * pxbuf->height;
        unsigned char *p;
        struct pixel_t *px;

        /* Pack header buffer */
        p = buffer;
        *p++ = 'B';
        *p++ = 'M';
        p = pack32(p, HDR_SIZE + arr_size);
        p = pack32(p, 0);
        p = pack32(p, HDR_SIZE);
        /* Pack dib */
        p = pack32(p, DIB_SIZE);
        p = pack32(p, pxbuf->width);
        p = pack32(p, pxbuf->height);
        p = pack16(p, 1);
        p = pack16(p, depth * 8);
        p = pack32(p, BI_RGB);
        p = pack32(p, 0);
        p = pack32(p, 11811); /* Defaulting to 300 dpi */
        p = pack32(p, 11811);
        p = pack32(p, 0);
        p = pack32(p, 0);

        fwrite(buffer, sizeof(buffer), 1, fp);
        /* repurpose buffer[] as our zero-padding write buffer */
        memset(buffer, 0, sizeof(buffer));
        assert(padding <= sizeof(buffer));

        pxbuf_normalize(pxbuf, method, 3.0, PXBUF_ALLCHAN);
        for (row = 0; row < pxbuf->height; row++) {
                for (col = 0; col < pxbuf->width; col++) {
                        unsigned char rgb[3];
                        px = pxptr(pxbuf, row, col);
                        /* Weird.  BMP orders it BGR instead of RGB */
                        rgb[0] = crop_255((px->x[PXBUF_BLUE] * 256.0 + 0.5));
                        rgb[1] = crop_255((px->x[PXBUF_GREEN] * 256.0 + 0.5));
                        rgb[2] = crop_255((px->x[PXBUF_RED] * 256.0 + 0.5));

                        fwrite(rgb, 3, 1, fp);
                }
                if (padding)
                        fwrite(buffer, 1, padding, fp);
        }
        return 0;
}

Pxbuf *
pxbuf_create(int width, int height)
{
        Pxbuf *ret = malloc(sizeof(*ret));
        if (!ret)
                return NULL;
        ret->buf = malloc(sizeof(*(ret->buf)) * width * height);
        if (!ret->buf) {
                free(ret);
                return NULL;
        }
        ret->width = width;
        ret->height = height;
        /* Initialize every pixel to 0.0 */
        memset(ret->buf, 0, sizeof(*ret->buf) * width * height);
        PXBUF_SANITY(ret);
        return ret;
}

void
pxbuf_destroy(Pxbuf *pxbuf)
{
        free(pxbuf->buf);
        free(pxbuf);
}

int
pxbuf_rotate(Pxbuf *pxbuf, bool cw)
{
        Pxbuf tmp;
        int row, col;

        tmp.height = pxbuf->width;
        tmp.width = pxbuf->height;
        tmp.buf = malloc(sizeof(*tmp.buf)
                          * pxbuf->width * pxbuf->height);
        if (!tmp.buf)
                return -1;
        PXBUF_SANITY(pxbuf);
        for (row = 0; row < pxbuf->height; row++) {
                for (col = 0; col < pxbuf->width; col++) {
                        struct pixel_t *px;
                        int new_row, new_col;
                        if (cw) {
                                new_row = col;
                                new_col = pxbuf->height - 1 - row;
                        } else {
                                new_row = pxbuf->width - 1 - col;
                                new_col = row;
                        }
                        px = pxbuf_get_pixel(pxbuf, row, col);
                        pxbuf_set_pixel(&tmp, px, new_row, new_col);
                }
        }

        PXBUF_SANITY(pxbuf);
        free(pxbuf->buf);
        memcpy(pxbuf, &tmp, sizeof(*pxbuf));
        return 0;
}

/* Assumes normalization occurs before and after, outside this function */
void
pxbuf_overlay(Pxbuf *dst, Pxbuf *src, double ratio)
{
        int ymax, xmax, row, col;
        struct pixel_t *pxsrc, *pxdst;

        /* TODO: Add baseline offsets as args */
        ymax = dst->height;
        if (ymax > src->height)
                ymax = src->height;
        xmax = dst->width;
        if (xmax > src->width)
                xmax = src->width;

        for (row = 0; row < ymax; row++) {
                for (col = 0; col < xmax; col++) {
                        int i;
                        pxsrc = pxbuf_get_pixel(src, row, col);
                        pxdst = pxbuf_get_pixel(dst, row, col);
                        for (i = 0; i < 3; i++)
                                pxdst->x[i] += ratio * pxsrc->x[i];
                }
        }
}

void
pxbuf_get_dimensions(Pxbuf *pxbuf, int *width, int *height)
{
        if (height)
                *height = pxbuf->height;
        if (width)
                *width = pxbuf->width;
}


