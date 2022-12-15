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

// accelerable?

// void outer(input_window<float>* in1, input_stream<float>* in2, output_window<float>* out){
//
//	aie::vector<float, SPACE_DIMENSION> op1 = aie::zeros<float, SPACE_DIMENSION>();
//	aie::vector<float, SPACE_DIMENSION> op2 = aie::zeros<float, SPACE_DIMENSION>();
//	aie::vector<float, SPACE_DIMENSION> op3 = aie::zeros<float, SPACE_DIMENSION>();
//	aie::vector<float, SPACE_DIMENSION> res = aie::zeros<float, SPACE_DIMENSION>();
//
//	op2 = readincr_v<SPACE_DIMENSION>(in2);
//
//	for(unsigned i = 0; i < LENGTH; i+=SPACE_DIMENSION){
//
//		window_readincr_v(in1, op1);
//
//		for(unsigned j = 0; j < SPACE_DIMENSION; j++){
//			op3 = aie::broadcast<float, SPACE_DIMENSION>(op1[j]);
//			res = aie::mul(op3, op2);
//			window_writeincr(out, res);
//		}
//	}
//
//};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void outer(input_window<T>* in1, input_stream<T>* in2, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = readincr_v<SPACE_DIMENSION>(in2);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        window_readincr_v(in1, op1);

        for (unsigned j = 0; j < VECDIM; j++) {
            op3 = aie::broadcast<T, VECDIM>(op1[j]);
            res = aie::mul(op3, op2);
            window_writeincr(out, res);
        }
    }
};

// void outerStream(input_stream<float>* in1, input_stream<float>* in2, output_window<float>* out){
//
//	aie::vector<float, SPACE_DIMENSION> op1 = aie::zeros<float, SPACE_DIMENSION>();
//	aie::vector<float, SPACE_DIMENSION> op2 = aie::zeros<float, SPACE_DIMENSION>();
//	aie::vector<float, SPACE_DIMENSION> op3 = aie::zeros<float, SPACE_DIMENSION>();
//	aie::vector<float, SPACE_DIMENSION> res = aie::zeros<float, SPACE_DIMENSION>();
//
//	op2 = readincr_v<SPACE_DIMENSION>(in2);
//
//	for(unsigned i = 0; i < LENGTH; i+=SPACE_DIMENSION){
//
//		op1 = readincr_v<SPACE_DIMENSION>(in1);
//
//		for(unsigned j = 0; j < SPACE_DIMENSION; j++){
//			op3 = aie::broadcast<float, SPACE_DIMENSION>(op1[j]);
//			res = aie::mul(op3, op2);
//			window_writeincr(out, res);
//		}
//	}
//
//};

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void outerStream(input_stream<T>* in1, input_stream<T>* in2, output_window<T>* out) {
    aie::vector<T, VECDIM> op1 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op2 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> op3 = aie::zeros<T, VECDIM>();
    aie::vector<T, VECDIM> res = aie::zeros<T, VECDIM>();

    op2 = readincr_v<SPACE_DIMENSION>(in2);

    for (unsigned i = 0; i < LEN; i += INCREMENT) {
        op1 = readincr_v<SPACE_DIMENSION>(in1);

        for (unsigned j = 0; j < VECDIM; j++) {
            op3 = aie::broadcast<T, VECDIM>(op1[j]);
            res = aie::mul(op3, op2);
            window_writeincr(out, res);
        }
    }
};
} // namespace L1
} // namespace us
