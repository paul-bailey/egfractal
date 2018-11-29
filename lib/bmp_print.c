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
#include "fractal_common.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

/**
 * bmp_print - Make a header for @array and print it to @fp as a bmp file.
 */
void
bmp_print(FILE *fp, const unsigned char *array,
          int width, int height, int depth)
{
        enum {
                T_SIZE = 14, /* file header */
                DIB_SIZE = 40, /* BMP info header */
                HDR_SIZE = DIB_SIZE + T_SIZE,
        };
        int padding, row;
        unsigned long arr_size;
        unsigned char buffer[HDR_SIZE];
        unsigned char *p = buffer;
        unsigned char *end;

        if (depth >= 8)
                depth /= 8;
        else
                depth = 1;

        padding = (width * depth) % 4;
        arr_size = (width * depth + padding) * height;

        /* Pack buffer */
        *p++ = 'B';
        *p++ = 'M';
        p = pack32(p, HDR_SIZE + arr_size);
        p = pack32(p, 0);
        p = pack32(p, HDR_SIZE);
        /* Pack dib */
        p = pack32(p, DIB_SIZE);
        p = pack32(p, width);
        p = pack32(p, height);
        p = pack16(p, 1);
        p = pack16(p, depth * 8);
        p = pack32(p, BI_RGB);
        p = pack32(p, 0);
        p = pack32(p, 11811); /* Defaulting to 300 dpi */
        p = pack32(p, 11811);
        p = pack32(p, 0);
        p = pack32(p, 0);

        fwrite(buffer, sizeof(buffer), 1, fp);
        /* Now use buffer as our zero-padding write buffer */
        memset(buffer, 0, sizeof(buffer));
        assert(padding <= sizeof(buffer));

        p = (unsigned char *)array;
        end = (unsigned char *)&array[width * depth * height];
        for (row = 0; row < height; row++) {
                fwrite(p, width, depth, fp);
                if (padding)
                        fwrite(buffer, padding, 1, fp);
                p += width * depth;
                assert(p < end || row == height - 1);
        }
}


