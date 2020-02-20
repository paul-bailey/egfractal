#ifndef BBROT2_H
#define BBROT2_H

#include "complex_helpers.h"

struct thread_info_t {
        int width;
        int height;
        int nchan;
        int min;
        unsigned long points;
        int n[3];
        unsigned long *_chanbuf_base;
        unsigned long *chanbuf[3];
        unsigned short seeds[6];
        complex_t (*formula)(complex_t, complex_t);
        mfloat_t wthird;
        mfloat_t hthird;
        mfloat_t bailsqu;
        double line_x, line_y;
        bool use_line_x, use_line_y;
};

/* bbrot_thread.c */
extern void *bbrot_thread(void *arg);

#endif /* BBROT2_H */

