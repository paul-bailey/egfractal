/*
 * dragon.c - Very simple Heighway dragon "unfolding" algo.
 *
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int width = 3000;
static int height = 3000;
static Pxbuf *pxbuf = NULL;
enum {
	/*
         * WARNING!! Do not let this get too big.
         * The memory requirements grow exponentially.
         */
	N = 22,
        LINE_WIDTH = 2,
};


typedef struct {
        int x;
        int y;
} px_t;

struct line_t {
        px_t start;
        px_t stop;
        struct line_t *next;
        struct line_t *prev;
};

static void
oom(void)
{
	fprintf(stderr, "OOM!\n");
	exit(EXIT_FAILURE);
}

enum dir_t { CW, CCW };

static px_t
endpoint_dir(px_t prev_start, px_t prev_stop, enum dir_t turn)
{
        enum { HORIZ, VERT };

        px_t ret;
        int card = prev_stop.x == prev_start.x ? VERT : HORIZ;
        if (card == VERT)
                ret.y = prev_stop.y;
        else
                ret.x = prev_stop.x;

        if (turn == CW) {
                if (card == VERT) {
                        if (prev_stop.y > prev_start.y)
                                ret.x = prev_stop.x - 1;
                        else
                                ret.x = prev_stop.x + 1;
                } else {
                        if (prev_stop.x > prev_start.x)
                                ret.y = prev_stop.y + 1;
                        else
                                ret.y = prev_stop.y - 1;
                }
        } else {
                if (card == VERT) {
                        if (prev_stop.y > prev_start.y)
                                ret.x = prev_stop.x + 1;
                        else
                                ret.x = prev_stop.x - 1;
                } else {
                        if (prev_stop.x > prev_start.x)
                                ret.y = prev_stop.y - 1;
                        else
                                ret.y = prev_stop.y + 1;
                }
        }
        return ret;
}

static px_t
endpoint(px_t prev_start, px_t prev_stop, struct line_t *ref)
{
        enum dir_t turn;

        /* FIXME: Surely some boolean simplification can be done here. */
        if (ref->prev == NULL) {
                turn = CW;
        } else {
                /*
                 * Determine the direction of the 90-degree angle to make.
                 * It should be the opposite direction of the reference
                 * junction.
                 */
                px_t ref_start = ref->prev->start;
                px_t ref_junc = ref->prev->stop;

                assert(ref_junc.x == ref->start.x && ref_junc.y == ref->start.y);
                px_t ref_end = ref->stop;

                if (ref_junc.y == ref_start.y) {
                        /* Horizontal earlier reference, vert. later ref. */
                        if (ref_junc.x > ref_start.x)
                                turn = ref_end.y > ref_junc.y ? CCW : CW;
                        else
                                turn = ref_end.y > ref_junc.y ? CW: CCW;
                } else {
                        /* Vertical earlier reference, horiz. later ref. */
                        if (ref_junc.y > ref_start.y)
                                turn = ref_end.x > ref_junc.x ? CW : CCW;
                        else
                                turn = ref_end.x > ref_junc.x ? CCW : CW;
                }

        }
        return endpoint_dir(prev_start, prev_stop, turn);
}

static struct line_t *
append_tail(struct line_t *tail)
{
        struct line_t *ret = malloc(sizeof(*ret));
        if (!ret)
                oom();
        ret->prev = tail;
        ret->next = NULL;
        if (tail != NULL)
                tail->next = ret;
        return ret;
}

static struct line_t *
rotate_lines(struct line_t *tail)
{
        struct line_t *p;
        struct line_t *q = append_tail(tail);
        q->start = tail->stop;
        q->stop = endpoint_dir(tail->start, tail->stop, CW);
        for (p = tail; p != NULL; p = p->prev) {
                struct line_t *tmp = append_tail(q);
                tmp->start = q->stop;
                tmp->stop = endpoint(q->start, q->stop, p);
                q = tmp;
                if (0) {
                        printf("Appending tail from (%d, %d) to (%d, %d)\n",
                               q->start.x, q->start.y, q->stop.x, q->stop.y);
                }
        }
        return q;
}

static px_t
scale_helper(px_t px, px_t min, px_t max)
{
        px.x = ((px.x - min.x) * width) / (max.x - min.x);
        px.y = ((px.y - min.y) * height) / (max.y - min.y);
        if (px.x == width)
                px.x = width-1;
        if (px.y == height)
                px.y = height-1;
        assert(px.y >= 0 && px.x >= 0);
        assert(px.x >= 0 && px.x < width);
        assert(px.y >= 0 && px.y < height);
        return px;
}

static void
scale_lines(struct line_t *start)
{
        px_t max, min;
        struct line_t *tail;

        max.x = 0;
        max.y = 0;
        min.x = 1000000000;
        min.y = 1000000000;

        for (tail = start; tail != NULL; tail = tail->next) {
                if (min.x > tail->start.x)
                        min.x = tail->start.x;
                if (min.x > tail->stop.x)
                        min.x = tail->stop.x;
                if (max.x < tail->start.x)
                        max.x = tail->start.x;
                if (max.x < tail->stop.x)
                        max.x = tail->stop.x;
                if (min.y > tail->start.y)
                        min.y = tail->start.y;
                if (min.y > tail->stop.y)
                        min.y = tail->stop.y;
                if (max.y < tail->start.y)
                        max.y = tail->start.y;
                if (max.y < tail->stop.y)
                        max.y = tail->stop.y;
        }

        printf("Max range is (%d, %d) to (%d, %d)\n", min.x, min.y, max.x, max.y);
        for (tail = start; tail != NULL; tail = tail->next) {
                tail->start = scale_helper(tail->start, min, max);
                tail->stop = scale_helper(tail->stop, min, max);
        }
}

static void draw_one_line_r(struct line_t *line, int depth);

static void
draw_one_line_r(struct line_t *line, int depth)
{
        if (depth == 0)
                return;

        struct line_t t;
        memcpy(&t, line, sizeof(t));

        /*
         * We can make this easy, since we know these are
         * always either horizontal or vertical.
         */
        unsigned int color = COLOR_WHITE;
        if (line->stop.x == line->start.x) {
                int row;
                for (row = line->start.y; row < line->stop.y; row++) {
                        pxbuf_fill_pixel(pxbuf, row, line->start.x, color);
                }

                t.start.x = line->start.x-1;
                t.stop.x  = line->start.x-1;
                draw_one_line_r(&t, depth-1);

                t.start.x = line->start.x+1;
                t.stop.x  = line->start.x+1;
                draw_one_line_r(&t, depth-1);
        } else {
                int col;
                for (col = line->start.x; col < line->stop.x; col++) {
                        pxbuf_fill_pixel(pxbuf, line->start.y, col, color);
                }

                t.start.y = line->start.y-1;
                t.stop.y  = line->start.y-1;
                draw_one_line_r(&t, depth-1);

                t.start.y = line->start.y+1;
                t.stop.y  = line->start.y+1;
                draw_one_line_r(&t, depth-1);
        }
}

static void
draw_lines(struct line_t *start)
{
        struct line_t *tail;
        for (tail = start; tail != NULL; tail = tail->next) {
                if (0) {
                        printf("Drawing line from (%d, %d) to (%d, %d)\n",
                               tail->start.x, tail->start.y, tail->stop.x, tail->stop.y);
                }
                draw_one_line_r(tail, LINE_WIDTH / 2);
        }
}

static void
free_lines(struct line_t *start)
{
        while (start != NULL) {
                struct line_t *next = start->next;
                free(start);
                start = next;
        }
}

static int
sizeoflist(struct line_t *line)
{
        int count = 0;
        while (line != NULL) {
                ++count;
                line = line->next;
        }
        return count;
}

static void
dragon(void)
{
        int i;
        struct line_t *start, *tail;

        start = append_tail(NULL);
        start->start.x = 0;
        start->start.y = 0;
        start->stop.x = 1;
        start->stop.y = 0;

        tail = start;
        for (i = 0; i < N; i++)
                tail = rotate_lines(tail);

        if (0)
                printf("Size of list is %d\n", sizeoflist(start));
        scale_lines(start);
        draw_lines(start);
        free_lines(start);
}


int main(void)
{
	FILE *fp;
	pxbuf = pxbuf_create(width, height, COLOR_BLACK);
	if (!pxbuf)
		oom();
	dragon();
	fp = fopen("dragon.bmp", "wb");
	if (!fp) {
		fprintf(stderr, "Cannot open output file\n");
		return 1;
	}
	pxbuf_print(pxbuf, fp);
	fclose(fp);
	pxbuf_free(pxbuf);
	return 0;
}

