/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file XModified.hpp
 * @brief lepton XModified function API.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef __cplusplus
#error " XModified.hpp hls::stream<> interface, and thus requires C++"
#endif

#ifndef _XMODIFIED_HPP_
#define _XMODIFIED_HPP_
//#include "model.hh"
#include "XAcc_common.hpp"
#include "XAcc_model.hpp"
//#include "bool_encoder.hh"
//#include "boolwriter.hh"

enum hls_Index : uint8_t {
#ifdef OPTIMIZED_7x7
    AC_7x7_INDEX = 0,
    AC_7x7_END = 49,
    DC_INDEX = 49,
    ROW_X_INDEX = 50,
    ROW_X_END = 57,
    ROW_Y_INDEX = 57,
    ROW_Y_END = 64
#else
    // AC_7x7_INDEX = 9,
    // AC_7x7_END = 63,
    DC_INDEX = 0,
// ROW_X_INDEX = 1,
// ROW_X_END = 7,
// ROW_Y_INDEX = 57,
// ROW_Y_END = 64
#endif
};

struct COEF8 {
    int16_t data[8];
};

struct struct_edge_data {
    uint16_t v[8];
    uint16_t h[8];
};
struct struct_ctx_edge {
    uint16_t here_v[8];
    uint16_t here_h[8];
    uint16_t left_v[8];
    uint16_t above_h[8];
};
extern uint8_t Shift_table[256][2];

class stt_dis {
#define STTDIS (8)
   public:
    hls_Branch* history[STTDIS];
    int cnt_dis[STTDIS];
    unsigned long total;
    int pos;
    stt_dis() {
        total = 0;
        pos = 0;
        for (int i = 0; i < STTDIS; i++) {
            history[i] = 0;
            cnt_dis[i] = 0;
        }
    }
    void print_dis() {
        for (int i = 1; i < STTDIS; i++) {
            printf("dis_%d = %d Percentage:%f\% \n", i, cnt_dis[i], (float)cnt_dis[i] / (float)total * 100.0);
        }
    }
    int get_dis(hls_Branch* pb) {
        int ret = 0;
        history[pos++] = pb;
        for (int i = 1; i < STTDIS; i++) {
            int ph = pos - i - 1;
            if (ph < 0) ph += STTDIS;
            total++;
            if (pos == STTDIS) pos = 0;
            if (i >= total) return 0;
            if (history[ph] == pb) {
                cnt_dis[ret + 1]++;
                return ret + 1;
            }
            ret++;
        }
        return 0;
    }
};

class hls_AlignedBlock {
   public:
    int16_t coef[64];
    enum Index : uint8_t {
        AC_7x7_INDEX = 0,
        AC_7x7_END = 49,
        DC_INDEX = 49,
        ROW_X_INDEX = 50,
        ROW_X_END = 57,
        ROW_Y_INDEX = 57,
        ROW_Y_END = 64
    };

   public:
    hls_AlignedBlock() {}
    int16_t* raw_data() { return coef; }
    const int16_t* raw_data() const { return coef; }
    uint8_t recalculate_coded_length() const {
        uint8_t num_nonzeros_7x7 = 0;
        /* how many tokens are we going to encode? */
        for (uint8_t index = 0; index < 64; index++) {
            uint8_t xy = hls_jpeg_zigzag_to_raster[index];
            uint8_t x = xy & 7;
            uint8_t y = xy >> 3;
            if (coef[hls_raster_to_aligned[xy]]) {
                // coded_length_ = index + 1;
                if (x > 0 && y > 0) {
                    ++num_nonzeros_7x7;
                }
            }
        }
        return num_nonzeros_7x7;
    }

    int16_t& dc() { return coef[DC_INDEX]; }
    int16_t dc() const { return coef[DC_INDEX]; }

    int16_t& mutable_coefficients_raster(uint8_t index) { return coef[hls_raster_to_aligned[index]]; }
    int16_t coefficients_raster(uint8_t index) const { return coef[hls_raster_to_aligned[index]]; }

    int16_t& mutable_coefficients_zigzag(uint8_t index) { return coef[hls_zigzag_to_aligned[index]]; }
    int16_t coefficients_zigzag(uint8_t index) const { return coef[hls_zigzag_to_aligned[index]]; }
};

namespace xf {
namespace codec {
namespace details {

int hls_color_index(int c);

uint8_t hls_get_num_nonzeros_context(
    bool all_present, bool above_present, bool left_present, uint8_t num_nonzeros_above, uint8_t num_nonzeros_left);

uint16_t hls_compute_aavrg(bool all_present,
                           bool left_present,
                           bool above_present,
                           uint16_t abs_coef_left,
                           uint16_t abs_coef_above,     //[64],
                           uint16_t abs_coef_above_left //[64]

                           );

uint16_t abs16(int16_t din);
/*int32_t hls_compute_lak(int COLOR,
                        unsigned int band,
                        bool all_present,
                        bool left_present,
                        bool above_present,
                        int16_t coef_here[64],
                        int16_t coef_left[64],
                        int16_t coef_above[64]);*/

// void vpx_enc(
//	uint16_t block_width,
//
//    hls::stream<bool>&    strm_bit,
//    hls::stream<uint8_t>& strm_prob,
//    hls::stream<bool>&    strm_e,
//	hls::stream<uint8_t>& strm_tab_dbg,
//
//    vpx_writer& boolwriter
//
//);
} // namespace details
} // namespace codec
} // namespace xf

#endif
