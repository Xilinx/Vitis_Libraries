#ifndef _XF_GRAPH_TWOHOP_KERNEL_HPP_
#define _XF_GRAPH_TWOHOP_KERNEL_HPP_

#include "xf_graph_L2.hpp"
#include <ap_int.h>

extern "C" void twoHop_kernel(unsigned numPairs,
                              ap_uint<64>* pair,

                              unsigned* offsetOneHop,
                              unsigned* indexOneHop,
                              unsigned* offsetTwoHop,
                              unsigned* indexTwoHop,

                              unsigned* cnt_res);

#endif
