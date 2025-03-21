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

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <adf.h>

#include "config.h"
#include "kernels.h"

extern uint8_t u_buf[4800];
extern uint8_t v_buf[4800];
extern uint8_t outu_buf[192];
extern uint8_t outv_buf[192];

using namespace adf;

class myGraph : public adf::graph {
   public:
    kernel k1;
    input_plio yinptr;
    output_plio youtptr;

    myGraph() {
        k1 = kernel::create(nv12resize);

        yinptr = input_plio::create("DataIn1", adf::plio_128_bits, "data/in_y.txt");
        youtptr = output_plio::create("DataOut1", adf::plio_128_bits, "data/out_y.txt");

        adf::connect<>(yinptr.out[0], k1.in[0]);
        adf::connect<>(k1.out[0], youtptr.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_OUT_WITH_METADATA};

        source(k1) = "xf_nv12resize.cc";

        // Initial mapping
        runtime<ratio>(k1) = 0.5;
    };
};
class myGraph1 : public adf::graph {
   public:
    kernel k1;

    input_plio uvinptr;
    output_plio uvoutptr;

    myGraph1() {
        k1 = kernel::create(nv12resize_uv);

        uvinptr = input_plio::create("DataIn2", adf::plio_128_bits, "data/in_uv.txt");
        uvoutptr = output_plio::create("DataOut2", adf::plio_128_bits, "data/out_uv.txt");

        adf::connect<>(uvinptr.out[0], k1.in[0]);
        adf::connect<>(k1.out[0], uvoutptr.in[0]);

        adf::dimensions(k1.in[0]) = {UV_ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {UV_ELEM_OUT_WITH_METADATA};

        source(k1) = "xf_nv12resize.cc";

        auto u_buff1 = parameter::array(u_buf);
        auto v_buff1 = parameter::array(v_buf);
        auto outu_buff1 = parameter::array(outu_buf);
        auto outv_buff1 = parameter::array(outv_buf);

        connect<>(u_buff1, k1);
        connect<>(v_buff1, k1);
        connect<>(outu_buff1, k1);
        connect<>(outv_buff1, k1);

        runtime<ratio>(k1) = 0.5;
    };
};

/* class myGraph : public adf::graph {
   public:
    kernel k1;
    input_plio yinptr;
    input_plio uvinptr;
    output_plio youtptr;
    output_plio uvoutptr;

    myGraph() {
        k1 = kernel::create(nv12resize);

        yinptr = input_plio::create("DataIn1", 256, 1000);
        youtptr = output_plio::create("DataOut1", 256, 1000);
        uvinptr = input_plio::create("gmioIn2", 256, 1000);
        uvoutptr = output_plio::create("gmioOut2", 256, 1000);

        adf::connect<>(yinptr.out[0], k1.in[0]);
        adf::connect<>(uvinptr.out[0], k1.in[1]);
        adf::connect<>(k1.out[0], youtptr.in[0]);
        adf::connect<>(k1.out[1], uvoutptr.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_OUT_WITH_METADATA};

        adf::dimensions(k1.in[1]) = {UV_ELEM_WITH_METADATA};
        adf::dimensions(k1.out[1]) = {UV_ELEM_OUT_WITH_METADATA};

        source(k1) = "xf_nv12resize.cc";

        auto u_buff1 = parameter::array(u_buf);
        auto v_buff1 = parameter::array(v_buf);
        auto outu_buff1 = parameter::array(outu_buf);
        auto outv_buff1 = parameter::array(outv_buf);

        connect<>(u_buff1, k1);
        connect<>(v_buff1, k1);
        connect<>(outu_buff1, k1);
        connect<>(outv_buff1, k1);

        //  	location<kernel>(k1) = tile(15, 0);

    //     location<parameter>(u_buff1) = {address(15, 0, (2*16384))};
    //     location<parameter>(v_buff1) = {address(15, 0, (3*16384))};
    //     location<parameter>(outu_buff1) = {address(15, 0, (2*16384)+4800)};
    //     location<parameter>(outv_buff1) = {address(15, 0, (3*16384)+4800)};
    //
            // Initial mapping
        runtime<ratio>(k1) = 0.5;
    };
};
 */
#endif
