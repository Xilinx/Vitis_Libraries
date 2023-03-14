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

#ifndef PARK_INVERSE_H
#define PARK_INVERSE_H

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdint.h>

const int in_width = 16; // Bit width of the input data. ex. short or ap_int<16> is fit for Q16.16
const int mid_width =
    32; // Bit width  of the data in the middle of calculation. ex. int_32t or ap_int<32> is fit for Q16.16
const int out_width = 16;     // Bit width  of the output data. ex. short or ap_int<16> is fit for Q16.16
const int max_limit = 32767;  // Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
const int min_limit = -32767; // Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16 data width for
                              // middle data in the calculation
const int sincos_width = 16;  // Bit width of Trigonometric function table. ex. ap_int<16> is fit for Q16.16
const int sincos_scale =
    sincos_width - 1; // Number significant bits of Trigonometric function table. ex. SINCOS_SCALE is 16 for Q16.16

/**
 * \brief Inverse Park's transform as an AXI4-Stream IP core
 *
 * It calculates the values \f$V_\alpha\f$ and \f$V_\beta\f$ in the output AXI4-Stream \p m_axis
 * by using the following equations:
 * \f{equation}{
 *  V_\alpha = V_d \cos \theta - V_q \sin \theta,
 * \f}
 * \f{equation}{
 *  V_\beta = V_d \sin \theta + V_q \cos \theta,
 * \f}
 * where \f$V_d\f$, \f$V_q\f$ and \f$\theta\f$ are from the input AXI4-Stream \p s_axis
 *
 * \param s_axis Input AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..15: \f$V_d\f$.
 *   <li> Bits 16..31: \f$V_q\f$.
 *   <li> Bits 32..47: Angle \f$\theta\f$, in encoder steps.
 *   <li> Bits 48..63: Not used.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * \param m_axis Output AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..15: \f$V_\alpha\f$
 *   <li> Bits 16..31: \f$V_\beta\f$
 *   <li> Bits 32..47: Angle \f$\theta\f$, in encoder steps.
 *   <li> Bits 48..63: 0.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * \return void - functions implementing an IP core do not return a value.
 */
void Park_Inverse_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis);

#endif // PARK_INVERSE_H
