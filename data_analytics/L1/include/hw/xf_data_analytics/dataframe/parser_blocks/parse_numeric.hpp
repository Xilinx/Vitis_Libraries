/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _DB_JSON_PARSE_NUMERICV2_HPP_
#define _DB_JSON_PARSE_NUMERICV2_HPP_
#include "hls_stream.h"
#include "ap_int.h"
#include <limits>

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

static void parseNumeric(hls::stream<ap_uint<10> >& i_strm, hls::stream<ap_uint<64> >& o_numeric_strm) {
    // init
    ap_uint<55> significand = 0;

    bool is_neg = false;

    bool is_fract = false;
    bool reach_max = false;

    ap_int<8> exp_frac = 0;
    ap_uint<8> in;
    bool e = false;
    bool nb = false;
    bool vld = true;
    const ap_uint<55> MAX_VLD_VALUE = 0xCCCCCCCCCCCCC; //(2^55-1)/10
    ap_uint<64> out = 0;
    ap_uint<10> in10;
loop_parseNumeric:
    while (!e) {
#pragma HLS pipeline II = 2
        if (!vld && nb) {
            ap_int<56> significand2;
            if (is_neg)
                significand2 = -significand;
            else
                significand2 = significand;
            out(55, 0) = significand2(55, 0);
            out(63, 56) = exp_frac(7, 0);
            // std::cout << "significand=" << significand << ", exp_frac=" << exp_frac << std::endl;
            o_numeric_strm.write(out);
            // reset
            significand = 0;
            exp_frac = 0;
            is_neg = false;
            is_fract = false;
            reach_max = false;
        }
        nb = i_strm.read_nb(in10);
        if (nb) {
            in = in10(7, 0);
            vld = in10[8];
            e = in10[9];
            // std::cout << "e=" << e << ", vld=" << vld << ", in=" << in << std::endl;
            if (vld) {
                if (in == '-')
                    is_neg = true;
                else {
                    if (in == '.') {
                        is_fract = true;
                    } else {
                        //} else if (in >= '0' && in <= '9') {
                        // if (!reach_max && (significand > MAX_VLD_VALUE || (significand == MAX_VLD_VALUE && in >
                        // '5'))) {
                        //    reach_max = true;
                        //    if (in >= '5') significand++;
                        if (!reach_max && (significand >= MAX_VLD_VALUE)) {
                            reach_max = true;
                        } else if (!reach_max) {
                            //#pragma HLS bind_op variable = significand op = mul impl = dsp latency = 1
                            significand = significand * (ap_uint<4>)10 + (in - '0');
                        }
                        if (reach_max && !is_fract)
                            exp_frac += 1;
                        else if (!reach_max && is_fract)
                            exp_frac -= 1;
                    }
                }
            }
        }
    }
};

} // namespace internal
} // dataframe
} // namespace data_analytics
} // namespace xf

#endif
