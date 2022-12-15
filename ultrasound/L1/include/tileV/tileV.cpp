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

#include "kernels.hpp"

namespace us {
namespace L1 {

// void tileV(input_stream<float>* in1, output_window<float>* out){
//
//	aie::vector<float, SPACE_DIMENSION> res = aie::vector<float, SPACE_DIMENSION>();
//
//	res = readincr_v<SPACE_DIMENSION>(in1);
//
//	for(unsigned i = 0; i < LENGTH; ++i){
//		window_writeincr(out, res);
//	}
//
// };

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void tileVApo(output_window<T>* out) {
    aie::vector<float, VECDIM> res = aie::vector<float, VECDIM>();

    res[0] = 0;
    res[1] = 0;
    res[2] = 1;
    res[3] = 0;

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_writeincr(out, res);
    }
};

} // namespace L1
} // namespace us
