/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <adf.h>
#include "aie_kernel.h"
#define N 1

using namespace adf;

class simpleGraph : public graph {
   private:
    kernel adder[N];

   public:
    port<input> in_real;
    port<input> in_imag;
    port<output> out_real;
    port<output> out_imag;

    simpleGraph() {
        for (int i = 0; i < N; i++) {
            adder[i] = kernel::create(aie_adder);

            if (i == 0) {
                connect<stream>(in_real, adder[i].in[0]);
                connect<stream>(in_imag, adder[i].in[1]);
            } else {
                connect<stream>(adder[i - 1].out[0], adder[i].in[0]);
                connect<stream>(adder[i - 1].out[1], adder[i].in[1]);
            }

            if (i == N - 1) {
                connect<stream>(adder[i].out[0], out_real);
                connect<stream>(adder[i].out[1], out_imag);
            }

            source(adder[i]) = "aie_adder.cc";

            runtime<ratio>(adder[i]) = 1.0;

            stack_size(adder[i]) = 13000;
        }
    };
};

#endif /**********__GRAPH_H__**********/
