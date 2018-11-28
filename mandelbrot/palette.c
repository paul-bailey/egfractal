#include "mandelbrot_common.h"
#include "fractal_common.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

enum {
        NCOLOR = 128,
};

static const unsigned int palette_buffers[][NCOLOR] = { {
                0x000000, 0x000000, 0x010101, 0x010101, 0x020202, 0x020202, 0x030303, 0x040404,
                0x040404, 0x050505, 0x060606, 0x060606, 0x070707, 0x070707, 0x080808, 0x080808,
                0x090909, 0x0a0a0a, 0x0a0a0a, 0x0b0b0b, 0x0c0c0c, 0x0c0c0c, 0x0c0c0c, 0x0c0c0c,
                0x0c0c0c, 0x0c0c0c, 0x0c0c0c, 0x0c0c0c, 0x0c0c0c, 0x0c0c0c, 0x0f0b14, 0x170a24,
                0x1b0a2c, 0x23093c, 0x270944, 0x2f0855, 0x33085d, 0x3b076d, 0x3f0675, 0x470686,
                0x48058e, 0x4b049e, 0x4d04a6, 0x4f03b6, 0x5103be, 0x5402cf, 0x5502d7, 0x5801e7,
                0x5900ef, 0x5c00ff, 0x6108f7, 0x6919e6, 0x6d22dd, 0x7533cc, 0x7a3bc4, 0x824cb3,
                0x8655aa, 0x8f6699, 0x936e91, 0x9b8080, 0xa28877, 0xaf9966, 0xb6a25d, 0xc3b34c,
                0xcabb44, 0xd7c433, 0xdebb2a, 0xebc419, 0xf2bb11, 0xffc400, 0xffb300, 0xffaa00,
                0xff9900, 0xff9100, 0xff8000, 0xff7700, 0xff6600, 0xff5d00, 0xff4c00, 0xff4400,
                0xf73b00, 0xe64400, 0xdd3b00, 0xcc4400, 0xc43b00, 0xb34c00, 0xaa5500, 0x996600,
                0x916e00, 0x808000, 0x778800, 0x669900, 0x5da200, 0x4cb300, 0x44bb00, 0x33cc00,
                0x2ad500, 0x19e600, 0x11ee00, 0x00ff00, 0x00ff00, 0x00ff00, 0x00ff00, 0x00ff00,
                0x00ff00, 0x00ff00, 0x00ff00, 0x00ff00, 0x00ff00, 0x00ff00, 0x08ff08, 0x19ff19,
                0x22ff22, 0x33ff33, 0x3bff3b, 0x4cff4c, 0x55ff55, 0x66ff66, 0x6eff6e, 0x80ff80,
                0x88ff88, 0x99ff99, 0xa2ffa2, 0xb3ffb3, 0xbbffbb, 0xccffcc, 0xd5ffd5, 0xe6ffe6
        }, {
                0x000000, 0x161600, 0x202000, 0x272700, 0x2d2d00, 0x323200, 0x373700, 0x3b3b00,
                0x404000, 0x434300, 0x474700, 0x4b4b01, 0x4e4e01, 0x515101, 0x545401, 0x575702,
                0x5a5a02, 0x5d5d02, 0x606003, 0x626203, 0x656503, 0x676704, 0x6a6a04, 0x6c6c05,
                0x6e6e05, 0x717105, 0x737306, 0x757507, 0x777707, 0x797908, 0x7b7b08, 0x7d7d09,
                0x80800a, 0x81810a, 0x83830b, 0x85850b, 0x87870c, 0x89890d, 0x8b8b0e, 0x8d8d0f,
                0x8f8f0f, 0x909010, 0x929211, 0x949412, 0x969613, 0x979713, 0x999914, 0x9b9b15,
                0x9c9c16, 0x9e9e17, 0xa0a018, 0xa1a119, 0xa3a31a, 0xa4a41b, 0xa6a61c, 0xa7a71d,
                0xa9a91e, 0xaaaa1f, 0xacac21, 0xadad22, 0xafaf23, 0xb0b024, 0xb2b225, 0xb3b327,
                0xb5b528, 0xb6b629, 0xb7b72b, 0xb9b92b, 0xbaba2d, 0xbbbb2e, 0xbdbd30, 0xbebe31,
                0xc0c032, 0xc1c134, 0xc2c235, 0xc3c337, 0xc5c539, 0xc6c63a, 0xc7c73c, 0xc9c93d,
                0xcaca3f, 0xcbcb41, 0xcccc42, 0xcece44, 0xcfcf45, 0xd0d047, 0xd1d149, 0xd3d34a,
                0xd4d44c, 0xd5d54d, 0xd6d64f, 0xd7d752, 0xd9d953, 0xdada55, 0xdbdb56, 0xdcdc59,
                0xdddd5b, 0xdede5c, 0xe0e05f, 0xe1e160, 0xe2e262, 0xe3e364, 0xe4e466, 0xe5e569,
                0xe6e66a, 0xe7e76c, 0xe8e86e, 0xeaea70, 0xebeb73, 0xecec74, 0xeded77, 0xeeee79,
                0xefef7b, 0xf0f07e, 0xf1f17f, 0xf2f282, 0xf3f384, 0xf4f487, 0xf5f58a, 0xf6f68b,
                0xf7f78e, 0xf8f890, 0xf9f993, 0xfafa96, 0xfbfb97, 0xfcfc9a, 0xfdfd9c, 0xfefe9f
        }, {
                0x0000cc, 0x000200, 0x000400, 0x000600, 0x000800, 0x000a00, 0x000c00, 0x000e00,
                0x001000, 0x001200, 0x001400, 0x001600, 0x001800, 0x001a00, 0x001c00, 0x001e00,
                0x002000, 0x002200, 0x002400, 0x002600, 0x002800, 0x002a00, 0x002c00, 0x002e00,
                0x003000, 0x003200, 0x003400, 0x003600, 0x003800, 0x003a00, 0x003c00, 0x003e00,
                0xff4000, 0x004200, 0x004400, 0x004600, 0x004800, 0x004a00, 0x004c00, 0x004e00,
                0x005000, 0x005200, 0x005400, 0x005600, 0x005800, 0x005a00, 0x005c00, 0x005e00,
                0x006000, 0x006200, 0x006400, 0x006600, 0x006800, 0x006a00, 0x006c00, 0x006e00,
                0x007000, 0x007200, 0x007400, 0x007600, 0x007800, 0x007a00, 0x007c00, 0x007e00,
                0xff80cc, 0xff8200, 0xff8400, 0xff8600, 0xff8800, 0xff8a00, 0xff8c00, 0xff8e00,
                0xff9000, 0xff9200, 0xff9400, 0xff9600, 0xff9800, 0xff9a00, 0xff9c00, 0xff9e00,
                0xffa000, 0xffa200, 0xffa400, 0xffa600, 0xffa800, 0xffaa00, 0xffac00, 0xffae00,
                0xffb000, 0xffb200, 0xffb400, 0xffb600, 0xffb800, 0xffba00, 0xffbc00, 0xffbe00,
                0xffc000, 0xffc200, 0xffc400, 0xffc600, 0xffc800, 0xffca00, 0xffcc00, 0xffce00,
                0xffd000, 0xffd200, 0xffd400, 0xffd600, 0xffd800, 0xffda00, 0xffdc00, 0xffde00,
                0xffe000, 0xffe200, 0xffe400, 0xffe600, 0xffe800, 0xffea00, 0xffec00, 0xffee00,
                0xfff000, 0xfff200, 0xfff400, 0xfff600, 0xfff800, 0xfffa00, 0xfffc00, 0xfffe00
        }, {
                0x80ff06, 0x8cff03, 0x98ff00, 0xa5ff00, 0xb1ff00, 0xbdff00, 0xcaff00, 0xd5ff00,
                0xe1ff00, 0xedff00, 0xf8ff00, 0xffff00, 0xffff00, 0xffff00, 0xffff00, 0xffff00,
                0xffff00, 0xffff00, 0xffff00, 0xfff900, 0xfff100, 0xffe900, 0xffe000, 0xffd700,
                0xffce00, 0xffc400, 0xffbb00, 0xffb100, 0xffa700, 0xff9e00, 0xff9400, 0xff8a00,
                0xff8000, 0xff7600, 0xff6c00, 0xff6200, 0xff5900, 0xff4f00, 0xff4500, 0xff3c00,
                0xff3200, 0xff2900, 0xff2000, 0xff1700, 0xff0f00, 0xff0702, 0xff0004, 0xff0007,
                0xff000a, 0xff000d, 0xff0010, 0xff0013, 0xff0016, 0xff0019, 0xf8001d, 0xed0020,
                0xe10022, 0xd50025, 0xca0028, 0xbd002b, 0xb1002e, 0xa50031, 0x980034, 0x8c0037,
                0x80003a, 0x74003d, 0x680040, 0x5b0042, 0x4f0045, 0x430047, 0x36004a, 0x2b004c,
                0x1f004e, 0x130050, 0x080052, 0x000054, 0x000056, 0x000058, 0x000059, 0x00005a,
                0x00005b, 0x00005d, 0x00005d, 0x00075e, 0x000f5f, 0x00175f, 0x00205f, 0x00295f,
                0x00325f, 0x003c5f, 0x00455f, 0x004f5e, 0x00595e, 0x00625d, 0x006c5c, 0x00765b,
                0x00805a, 0x008a58, 0x009457, 0x009e55, 0x00a753, 0x00b151, 0x00bb4f, 0x00c44d,
                0x00ce4b, 0x00d749, 0x00e046, 0x00e944, 0x00f141, 0x00f93e, 0x00ff3c, 0x00ff39,
                0x00ff36, 0x00ff33, 0x00ff30, 0x00ff2d, 0x00ff2a, 0x00ff27, 0x08ff23, 0x13ff20,
                0x1fff1e, 0x2bff1b, 0x36ff18, 0x43ff15, 0x4fff12, 0x5bff0f, 0x68ff0c, 0x74ff09
        }, {
                0x80e614, 0x86e614, 0x8ce514, 0x92e514, 0x98e414, 0x9fe314, 0xa5e114, 0xabe014,
                0xb0de14, 0xb6dc14, 0xbcda14, 0xc1d714, 0xc7d514, 0xccd214, 0xd1cf14, 0xd5cb14,
                0xdac814, 0xdec414, 0xe2c014, 0xe6bc14, 0xeab814, 0xedb414, 0xf0b014, 0xf3ab14,
                0xf6a714, 0xf8a214, 0xfa9d14, 0xfc9814, 0xfd9314, 0xfe8f14, 0xff8a14, 0xff8514,
                0xff8014, 0xff7b14, 0xff7614, 0xfe7114, 0xfd6d14, 0xfc6814, 0xfa6314, 0xf85e14,
                0xf65914, 0xf35514, 0xf05014, 0xed4c14, 0xea4814, 0xe64414, 0xe24014, 0xde3c14,
                0xda3814, 0xd53514, 0xd13114, 0xcc2e14, 0xc72b14, 0xc12914, 0xbc2614, 0xb62414,
                0xb02214, 0xab2014, 0xa51f14, 0x9f1d14, 0x981c14, 0x921b14, 0x8c1b14, 0x861a14,
                0x801a14, 0x7a1a14, 0x741b14, 0x6e1b14, 0x681c14, 0x611d14, 0x5b1f14, 0x552014,
                0x502214, 0x4a2414, 0x442614, 0x3f2914, 0x392b14, 0x342e14, 0x2f3114, 0x2b3514,
                0x263814, 0x223c14, 0x1e4014, 0x1a4414, 0x164814, 0x134c14, 0x105014, 0x0d5514,
                0x0a5914, 0x085e14, 0x066314, 0x046814, 0x036d14, 0x027114, 0x017614, 0x017b14,
                0x008014, 0x018514, 0x018a14, 0x028f14, 0x039314, 0x049814, 0x069d14, 0x08a214,
                0x0aa714, 0x0dab14, 0x10b014, 0x13b414, 0x16b814, 0x1abc14, 0x1ec014, 0x22c414,
                0x26c814, 0x2bcb14, 0x2fcf14, 0x34d214, 0x39d514, 0x3fd714, 0x44da14, 0x4adc14,
                0x50de14, 0x55e014, 0x5be114, 0x61e314, 0x68e414, 0x6ee514, 0x74e514, 0x7ae614
        }
};

static const unsigned int INSIDE_COLORS[] = {
        COLOR_BLACK,
        COLOR_WHITE,
        COLOR_BLACK,
        COLOR_BLACK,
        COLOR_WHITE
};

static const unsigned int *palette;
#define NO_COLOR ((unsigned int)~0ul)
static unsigned int inside_color = NO_COLOR;

static unsigned int
interp_helper(unsigned int color1, unsigned int color2, mfloat_t frac)
{
        return color1 + (int)(frac * ((mfloat_t)color2 - (mfloat_t)color1) + 0.5);
}

static void
channelize(unsigned int color, unsigned int *r, unsigned int *g, unsigned int *b)
{
        *r = (color >> 16) & 0xffu;
        *g = (color >> 8) & 0xffu;
        *b = color & 0xffu;
}

static unsigned int
linear_interp(unsigned int color1, unsigned int color2, mfloat_t frac)
{
        unsigned int r1, g1, b1;
        unsigned int r2, g2, b2;
        channelize(color1, &r1, &g1, &b1);
        channelize(color2, &r2, &g2, &b2);
        r1 = interp_helper(r1, r2, frac);
        g1 = interp_helper(g1, g2, frac);
        b1 = interp_helper(b1, b2, frac);
        assert(r1 < 256);
        assert(g1 < 256);
        assert(b1 < 256);
        return TO_RGB(r1, g1, b1);
}

#ifndef ARRAYLEN
# define ARRAYLEN(a_)   (sizeof(a_) / sizeof((a_)[0]))
#endif

/* "transitionate" because I don't have a thesaurus handy */
static void
initialize_palette(void)
{
        assert(ARRAYLEN(palette_buffers) == ARRAYLEN(INSIDE_COLORS));

        /* It's dumb, but we index from 1 on the command line. */
        int p = gbl.palette - 1;

        /* Use a default if we got the wrong argument. */
        if (p >= ARRAYLEN(palette_buffers)) {
                fprintf(stderr, "Invalid palette number. Defaulting to 1.\n");
                p = 0;
        }

        inside_color = INSIDE_COLORS[p];
        palette = palette_buffers[p];
}

/*
 * Return black-and-white gradient.
 * Works best when bailout radius and number of iterations are high.
 */
static unsigned int
distance_to_color(mfloat_t dist, mfloat_t max)
{
        unsigned int magn;

        if (dist <= 0.0L)
                return COLOR_BLACK;

        /* TODO: Make the root be a command-line option */
        magn = (unsigned int)(255.0 * pow(dist / max, gbl.distance_root));
        if (magn > 255)
                magn = 255;

        return TO_RGB(magn, magn, magn);
}

/*
 * Return color of palette[count modulo NCOLOR].
 * Works best when number of iterations is at least NCOLOR.
 */
static unsigned int
iteration_to_color(mfloat_t iter_count)
{
        int i;
        unsigned int v1, v2;
        long double dummy = 0;

        if (inside_color == NO_COLOR)
                initialize_palette();

        if (iter_count <= 0.0L || (int)iter_count >= gbl.n_iteration)
                return inside_color;

        /* Linear interpolation of palette[esc_count % NCOLOR] */
        i = (int)iter_count % NCOLOR;
        v1 = palette[i];
        v2 = palette[i == NCOLOR - 1 ? 0 : i + 1];
        return linear_interp(v1, v2, modfl(iter_count, &dummy));
}

unsigned int
get_color(mfloat_t esc_val, mfloat_t min, mfloat_t max)
{
        if (gbl.distance_est) {
                return distance_to_color(esc_val, max);
        } else {
                return iteration_to_color(esc_val);
        }
}

/* XXX REVISIT: Hierarchically asymmetrical to get_color() */
void
print_palette_to_bmp(Pxbuf *pxbuf)
{
        int row, col;
        if (inside_color == NO_COLOR)
                initialize_palette();
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        double idx = (double)col * (double)NCOLOR / (double)gbl.width;
                        unsigned int i, v1, v2, color;
                        i = (unsigned int)idx;
                        assert(i < NCOLOR);
                        v1 = palette[i];
                        v2 = palette[i == NCOLOR-1 ? 0 : i + 1];
                        color = linear_interp(v1, v2, modf(idx, &idx));
                        pxbuf_fill_pixel(pxbuf, row, col, color);
                }
        }
}


