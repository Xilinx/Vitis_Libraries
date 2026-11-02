/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef DATAMOVER_GET_COUNTER_HPP
#define DATAMOVER_GET_COUNTER_HPP

#include "ap_int.h"
#include "hls_stream.h"

namespace xf {
namespace data_mover {

/**
 * Write Count To DDR
 * This function is to store counter from stream to ddr.

 * @param cnt_str streams port
 * @param counter the output including start counter, end counter and total data number transfered in byte for each
 process of loading_ddr_to_stream or storing_stream_to_ddr
 * @param num_ch number of cnt_str
 */
void
#ifndef __SYNTHESIS__
    __attribute__((weak))
#endif
    write_count(hls::stream<uint64_t>* cnt_str, uint64_t* cnt_data, uint64_t* counter, int num_ch) {

    for (int i = 0; i < num_ch; i++) {
        counter[i] = cnt_str[i].read();
        counter[i + num_ch] = cnt_str[i].read();
        counter[i + 2 * num_ch] = cnt_data[i];
    }
}

/**
 * Get start_count and end_count from the stream which stores the start and end position.
 *
 * @param vld_str stream which stores the start and end position.
 * @param cnt_str stream which records the start counter and end counter.
 */
void
#ifndef __SYNTHESIS__
    __attribute__((weak))
#endif
    get_count(hls::stream<ap_uint<1> >& vld_str, hls::stream<uint64_t>& cnt_str) {
    uint64_t cnt = 0;
    ap_uint<1> vld = 0;
    ap_uint<2> status = 0; // status=0b00: init; status=0b01: start; status=0b10: end; status=0b11: exit;

    while (status < 3) {
#pragma HLS PIPELINE II = 1
        cnt++;
        if (status == 0) {
            if (!vld_str.empty()) {
                vld_str.read_nb(vld);
                status = 0b01;
                cnt_str.write_nb(cnt);
            }
        } else if (status == 1) {
            if (!vld_str.empty()) {
                vld_str.read_nb(vld);
                status = 0b10;
                cnt_str.write_nb(cnt);
            }
        } else if (status == 2) {
            status = 0b11;
        }
    }
}

} /* data_mover */
} /* xf */
#endif
