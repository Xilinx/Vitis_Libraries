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

#ifndef PARK_DIRECT_H
#define PARK_DIRECT_H

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdint.h>

const int scale_park = 15;

/// \brief Maximum positive value for saturated arithmetic.
const int max_limit = 32767;

/// \brief Minimum negative value for saturated arithmetic.
const int min_limit = -32767;

/// \brief data buffer width, used as fixed size to buffer for data width lower than in_width
const int in_width = 16;
const int out_width = 16;
/// \brief data width for middle data
const int mid_width = 32;
/// for angle
const int sincos_width = 16;

/**
 * \brief Park's transform as AXI4-Stream IP core
 *
 * It calculates the values \f$I_d\f$ and \f$I_s\f$ in the ouput AXI4-Stream \p m_axis
 * by using the following equations:
 * \f{equation}{
 *  I_d = I_\alpha \cos \theta + I_\beta \sin \theta,
 * \f}
 * \f{equation}{
 *  I_q = - I_\alpha \sin \theta + I_\beta \cos \theta,
 * \f}
 * where \f$I_\alpha\f$, \f$I_\beta\f$ and \f$\theta\f$ are from the input AXI4-Stream \p s_axis.
 *
 * \param s_axis Input AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..15: \f$I_a\f$, from the ADC.
 *   <li> Bits 16..31: \f$I_b\f$, from the ADC.
 *   <li> Bits 32..47: Speed, in RPM, just passed through.
 *   <li> Bits 48..63: Angle, in encoder steps.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * \param m_axis Output AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..16: \f$I_d\f$.
 *   <li> Bits 17..31: \f$I_q\f$.
 *   <li> Bits 32..47: Speed, in RPM.
 *   <li> Bits 48..63: Angle, in encoder steps.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * \return void - functions implementing an IP core do not return a value.
 */
void Park_Direct_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis);

const int QmW_park = 5;
const int QnW_park = 9;

void Park_Direct_axi_ap_fixed(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis);
// void Park_Direct_axi_Qmn(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis);

#endif // PARK_DIRECT_H
