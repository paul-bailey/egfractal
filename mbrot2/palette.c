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
#include "fractal_common.h"
#include "pxbuf.h"
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
        }, {
                0x0000ff, 0x0606ff, 0x0c0cff, 0x1212ff, 0x1818ff, 0x1e1eff, 0x2424ff, 0x2a2aff,
                0x3030ff, 0x3636ff, 0x3c3cff, 0x4242ff, 0x4848ff, 0x4e4eff, 0x5555ff, 0x5b5bff,
                0x6161ff, 0x6767ff, 0x6d6dff, 0x7373ff, 0x7979ff, 0x7f7fff, 0x8585ff, 0x8b8bff,
                0x9191ff, 0x9797ff, 0x9d9dff, 0xa3a3ff, 0xaaaaff, 0xb0b0ff, 0xb6b6ff, 0xbcbcff,
                0xc2c2ff, 0xc8c8ff, 0xceceff, 0xd4d4ff, 0xdadaff, 0xe0e0ff, 0xe6e6ff, 0xececff,
                0xf2f2ff, 0xf8f8ff, 0xffffff, 0xfff8f8, 0xfff2f2, 0xffecec, 0xffe6e6, 0xffe0e0,
                0xffdada, 0xffd4d4, 0xffcece, 0xffc8c8, 0xffc2c2, 0xffbcbc, 0xffb6b6, 0xffb0b0,
                0xffaaaa, 0xffa3a3, 0xff9d9d, 0xff9797, 0xff9191, 0xff8b8b, 0xff8585, 0xff7f7f,
                0xff7979, 0xff7373, 0xff6d6d, 0xff6767, 0xff6161, 0xff5b5b, 0xff5555, 0xff4e4e,
                0xff4848, 0xff4242, 0xff3c3c, 0xff3636, 0xff3030, 0xff2a2a, 0xff2424, 0xff1e1e,
                0xff1818, 0xff1212, 0xff0c0c, 0xff0606, 0xff0000, 0xff0000, 0xf90005, 0xf3000b,
                0xed0011, 0xe70017, 0xe1001d, 0xdb0023, 0xd50029, 0xcf002f, 0xc90035, 0xc3003b,
                0xbd0041, 0xb70047, 0xb1004d, 0xab0053, 0xa60058, 0xa0005e, 0x9a0064, 0x94006a,
                0x8e0070, 0x880076, 0x82007c, 0x7c0082, 0x760088, 0x70008e, 0x6a0094, 0x64009a,
                0x5e00a0, 0x5800a6, 0x5300ab, 0x4d00b1, 0x4700b7, 0x4100bd, 0x3b00c3, 0x3500c9,
                0x2f00cf, 0x2900d5, 0x2300db, 0x1d00e1, 0x1700e7, 0x1100ed, 0x0b00f3, 0x0500f9
        }, {
                0xff0000, 0xfb0300, 0xf70700, 0xf30b00, 0xef0f00, 0xeb1300, 0xe71700, 0xe31b00,
                0xdf1f00, 0xdb2300, 0xd72700, 0xd32b00, 0xcf2f00, 0xcb3300, 0xc73700, 0xc33b00,
                0xbf3f00, 0xbb4300, 0xb74700, 0xb34b00, 0xaf4f00, 0xab5300, 0xa75700, 0xa35b00,
                0x9f5f00, 0x9b6300, 0x976700, 0x936b00, 0x8f6f00, 0x8b7300, 0x877700, 0x837b00,
                0x7f7f00, 0x7b8300, 0x778700, 0x738b00, 0x6f8f00, 0x6b9300, 0x679700, 0x639b00,
                0x5f9f00, 0x5ba300, 0x57a700, 0x53ab00, 0x4faf00, 0x4bb300, 0x47b700, 0x43bb00,
                0x3fbf00, 0x3bc300, 0x37c700, 0x33cb00, 0x2fcf00, 0x2bd300, 0x27d700, 0x23db00,
                0x1fdf00, 0x1be300, 0x17e700, 0x13eb00, 0x0fef00, 0x0bf300, 0x07f700, 0x03fb00,
                0x00ff00, 0x03fb00, 0x07f700, 0x0bf300, 0x0fef00, 0x13eb00, 0x17e700, 0x1be300,
                0x1fdf00, 0x23db00, 0x27d700, 0x2bd300, 0x2fcf00, 0x33cb00, 0x37c700, 0x3bc300,
                0x3fbf00, 0x43bb00, 0x47b700, 0x4bb300, 0x4faf00, 0x53ab00, 0x57a700, 0x5ba300,
                0x5f9f00, 0x639b00, 0x679700, 0x6b9300, 0x6f8f00, 0x738b00, 0x778700, 0x7b8300,
                0x7f7f00, 0x837b00, 0x877700, 0x8b7300, 0x8f6f00, 0x936b00, 0x976700, 0x9b6300,
                0x9f5f00, 0xa35b00, 0xa75700, 0xab5300, 0xaf4f00, 0xb34b00, 0xb74700, 0xbb4300,
                0xbf3f00, 0xc33b00, 0xc73700, 0xcb3300, 0xcf2f00, 0xd32b00, 0xd72700, 0xdb2300,
                0xdf1f00, 0xe31b00, 0xe71700, 0xeb1300, 0xef0f00, 0xf30b00, 0xf70700, 0xfb0300
        }, {
                0xffff80, 0xe0fc70, 0xd2f869, 0xc8f464, 0xc0f060, 0xb9ed5c, 0xb2ea59, 0xace656,
                0xa6e353, 0xa1e050, 0x9cdd4e, 0x98da4c, 0x93d749, 0x8fd447, 0x8bd245, 0x87cf43,
                0x84cd42, 0x80ca40, 0x7dc83e, 0x79c53c, 0x76c33b, 0x73c139, 0x70bf38, 0x6dbd36,
                0x6abb35, 0x68b934, 0x65b732, 0x62b531, 0x60b330, 0x5db22e, 0x5bb02d, 0x58ae2c,
                0x56ad2b, 0x54ab2a, 0x52aa29, 0x50a928, 0x4ea727, 0x4ba625, 0x49a524, 0x48a424,
                0x46a223, 0x44a122, 0x42a021, 0x409f20, 0x3e9e1f, 0x3d9e1e, 0x3b9d1d, 0x399c1c,
                0x389b1c, 0x369a1b, 0x359a1a, 0x339919, 0x329919, 0x309818, 0x2f9817, 0x2d9716,
                0x2c9716, 0x2a9715, 0x299614, 0x289614, 0x279613, 0x259612, 0x249612, 0x239511,
                0x229511, 0x219510, 0x209610, 0x1e960f, 0x1d960e, 0x1c960e, 0x1b960d, 0x1a970d,
                0x19970c, 0x18970c, 0x17980b, 0x16980b, 0x16990b, 0x15990a, 0x149a0a, 0x139a09,
                0x129b09, 0x119c08, 0x119d08, 0x109e08, 0x0f9e07, 0x0e9f07, 0x0ea007, 0x0da106,
                0x0ca206, 0x0ca406, 0x0ba505, 0x0aa605, 0x0aa705, 0x09a904, 0x09aa04, 0x08ab04,
                0x08ad04, 0x07ae03, 0x07b003, 0x06b203, 0x06b303, 0x05b502, 0x05b702, 0x04b902,
                0x04bb02, 0x04bd02, 0x03bf01, 0x03c101, 0x03c301, 0x02c501, 0x02c801, 0x02ca01,
                0x02cd01, 0x01cf00, 0x01d200, 0x01d400, 0x01d700, 0x00da00, 0x00dd00, 0x00e000,
                0x00e300, 0x00e600, 0x00ea00, 0x00ed00, 0x00f000, 0x00f400, 0x00f800, 0x00fc00
        }
};

#define COLOR_BLACK { .x = { 0., 0., 0. } }
#define COLOR_WHITE { .x = { 255., 255., 255. } }
static const struct pixel_t INSIDE_COLORS[] = {
        COLOR_BLACK,
        COLOR_WHITE,
        COLOR_BLACK,
        COLOR_BLACK,
        COLOR_WHITE,
        COLOR_BLACK,
        COLOR_BLACK,
        COLOR_BLACK,
};

static const unsigned int *palette;
static struct pixel_t inside_color__;
static struct pixel_t *inside_color = NULL;

static float crop_255f(float v)
        { return v < 0.0 ? 0.0 : (v > 255.0 ? 255.0 : v); }

static float
interp_helper(float color1, float color2, mfloat_t frac)
{
        return crop_255f(color1 + frac * (color2 - color1));
}

static void
channelize(unsigned int color, struct pixel_t *px)
{
        px->x[0] = (float)(color & 0xffu);
        px->x[1] = (float)((color >> 8) & 0xffu);
        px->x[2] = (float)((color >> 16) & 0xffu);
}

static void
linear_interp(unsigned int color1, unsigned int color2,
                mfloat_t frac, struct pixel_t *px)
{
        struct pixel_t px1, px2;
        channelize(color1, &px1);
        channelize(color2, &px2);
        assert(frac >= 0.0L && frac < 1.0L);
        px->x[0] = interp_helper(px1.x[0], px2.x[0], frac);
        px->x[1] = interp_helper(px1.x[1], px2.x[1], frac);
        px->x[2] = interp_helper(px1.x[2], px2.x[2], frac);
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

        inside_color = &inside_color__;
        memcpy(inside_color, &INSIDE_COLORS[p], sizeof(*inside_color));
        palette = palette_buffers[p];
}

static double
scaled_distance(double d, double max, double min)
{
        return pow((d-min) / (max-min), gbl.distance_root);
}

/*
 * Return black-and-white gradient.
 * Works best when bailout radius and number of iterations are high.
 */
static void
distance_to_color_bw(mfloat_t dist, mfloat_t min,
                        mfloat_t max, struct pixel_t *px)
{
        unsigned int magn;

        if (inside_color == NULL)
                initialize_palette();

        if (dist <= 0.0L) {
#warning "Pick one"
                if (0)
                        memcpy(px, inside_color, sizeof(*px));
                else /* Return black */
                        memset(px, 0, sizeof(*px));
                return;
        }

        assert(dist <= max && dist >= min && max >= min);
        magn = (256.0 * scaled_distance(dist, max, min));
        px->x[0] = magn;
        px->x[1] = magn;
        px->x[2] = magn;
}

/* XXX: D.R.Y. violations with iteration_to_color() */
static void
distance_to_color_palette(mfloat_t dist, mfloat_t min,
                        mfloat_t max, struct pixel_t *px)
{
        mfloat_t d;
        unsigned int i, v1, v2;
        long double dummy = 0;

        if (inside_color == NULL)
                initialize_palette();

        if (dist < 0.0L) {
                memcpy(px, inside_color, sizeof(*px));
                return;
        }

        assert(dist <= max);
        assert(max > min);
        assert(dist >= min);

        d = scaled_distance(dist, max, min) * (mfloat_t)NCOLOR;
        assert(isfinite(d));
        assert(d >= 0.0L);
        i = (int)d;
        if (i >= NCOLOR)
                i = NCOLOR-1;
        v1 = palette[i];
        v2 = palette[i == NCOLOR - 1 ? 0 : i + 1];
        linear_interp(v1, v2, modfl(d, &dummy), px);

}

static void
distance_to_color_spread(mfloat_t dist, mfloat_t min,
                        mfloat_t max, struct pixel_t *px)
{
        double d;
        if (inside_color == NULL) {
                gbl.palette = 3;
                initialize_palette();
        }

        if (dist < 0.0) {
                memcpy(px, inside_color, sizeof(*px));
                return;
        }

        d = scaled_distance(dist, max, min);

        /*
         * "1.0 -..." to make it brighter the nearer it reaches
         * the set and darker the further away it gets.
         *
         * XXX: Faster if we save the inverse of gbl.xxxspread
         * at argparse time.
         */
        px->x[0] = d > gbl.bluespread ? 0.0 : 1.0 - d / gbl.bluespread;
        px->x[1] = d > gbl.greenspread ? 0.0 : 1.0 - d / gbl.greenspread;
        px->x[2] = d > gbl.redspread ? 0.0 : 1.0 - d / gbl.redspread;
}

static void
distance_to_color(mfloat_t dist, mfloat_t min,
                        mfloat_t max, struct pixel_t *px)
{
        if (gbl.color_distance) {
                if (gbl.color_spread)
                        distance_to_color_spread(dist, min, max, px);
                else
                        distance_to_color_palette(dist, min, max, px);
        } else {
                distance_to_color_bw(dist, min, max, px);
        }
}

/*
 * Return color of palette[count modulo NCOLOR].
 * Works best when number of iterations is at least NCOLOR.
 */
static void
iteration_to_color(mfloat_t iter_count, struct pixel_t *px)
{
        int i;
        unsigned int v1, v2;
        long double dummy = 0;

        if (inside_color == NULL)
                initialize_palette();

        if (iter_count <= 0.0L || (int)iter_count >= gbl.n_iteration) {
                memcpy(px, inside_color, sizeof(*px));
                return;
        }

        /* Linear interpolation of palette[esc_count % NCOLOR] */
        i = (int)iter_count % NCOLOR;
        v1 = palette[i];
        v2 = palette[i == NCOLOR - 1 ? 0 : i + 1];
        linear_interp(v1, v2, modfl(iter_count, &dummy), px);
}

void
get_color(mfloat_t esc_val, mfloat_t min, mfloat_t max, struct pixel_t *px)
{
        if (gbl.distance_est) {
                return distance_to_color(esc_val, min, max, px);
        } else {
                return iteration_to_color(esc_val, px);
        }
}

/* XXX REVISIT: Hierarchically asymmetrical to get_color() */
void
print_palette_to_bmp(Pxbuf *pxbuf)
{
        int row, col;
        double winv = (double)NCOLOR / (double)gbl.width;
        if (inside_color == NULL)
                initialize_palette();
        for (row = 0; row < gbl.height; row++) {
                for (col = 0; col < gbl.width; col++) {
                        double idx = (double)col * winv;
                        unsigned int i, v1, v2;
                        struct pixel_t px;
                        i = (unsigned int)idx;
                        assert(i < NCOLOR);
                        v1 = palette[i];
                        v2 = palette[i == NCOLOR-1 ? 0 : i + 1];
                        linear_interp(v1, v2, modf(idx, &idx), &px);
                        pxbuf_set_pixel(pxbuf, &px, row, col);
                }
        }
}

