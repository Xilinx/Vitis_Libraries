#ifndef _XF_AUTOGAIN_HPP_
#define _XF_AUTOGAIN_HPP_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "hls_math.h"
#include "hls_stream.h"
#include <iostream>
#include <assert.h>

namespace xf {
namespace cv {

template <int SRC_T, int NPC = 1, int HISTSIZE>
void autogain(unsigned int histogram[XF_CHANNELS(SRC_T, NPC)][HISTSIZE], uint16_t gain[XF_CHANNELS(SRC_T, NPC)]) {
#pragma HLS ARRAY_PARTITION variable = histogram complete dim = 1
#pragma HLS ARRAY_PARTITION variable = gain complete dim = 1

    const char CLUSTER = 32;
    const unsigned short GAIN_ARRAY_SIZE = HISTSIZE / CLUSTER;
    uint16_t gain_array[GAIN_ARRAY_SIZE] = {0};

    for (unsigned short i = 0; i < GAIN_ARRAY_SIZE; i++) {
#pragma HLS UNROLL
        gain_array[i] = (1 << XF_DTPIXELDEPTH(SRC_T, NPC)) - (i * CLUSTER);
    }

    unsigned int hist_cluster_of_32bins[XF_CHANNELS(SRC_T, NPC)][GAIN_ARRAY_SIZE] = {0};
    ap_uint<13> hist_index = 0;
    unsigned int max_gain[XF_CHANNELS(SRC_T, NPC)] = {0};

#pragma HLS ARRAY_PARTITION variable = max_gain complete dim = 1
#pragma HLS ARRAY_PARTITION variable = hist_cluster_of_32bins complete dim = 1

RowLoop:
    for (unsigned short i = 0; i < GAIN_ARRAY_SIZE; i++) {
#pragma HLS LOOP_TRIPCOUNT min = GAIN_ARRAY_SIZE max = GAIN_ARRAY_SIZE
    ColLoop:
        for (char j = 0; j < CLUSTER; j++) {
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS pipeline II = 1

            for (char ch = 0; ch < XF_CHANNELS(SRC_T, NPC); ch++) {
#pragma HLS UNROLL

                hist_cluster_of_32bins[ch][i] += histogram[ch][hist_index];
                if (max_gain[ch] < hist_cluster_of_32bins[ch][i]) {
                    max_gain[ch] = hist_cluster_of_32bins[ch][i];
                    gain[ch] = gain_array[i];
                }
            }
            hist_index++;
        }
    }
}

} // namespace cv
} // namespace xf

#endif