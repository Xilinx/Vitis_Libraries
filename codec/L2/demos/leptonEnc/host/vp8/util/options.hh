#ifndef _OPTIONS_HH_
#define _OPTIONS_HH_

enum {
    VECTORIZE = 0,
    MICROVECTORIZE = 0,
    MAX_NUM_THREADS = 1, // 8,
    SIMD_WIDTH = 1
};
extern unsigned int NUM_THREADS;
#endif
