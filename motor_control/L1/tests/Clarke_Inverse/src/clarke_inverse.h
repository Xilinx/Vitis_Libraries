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
#ifndef CLARKE_INVERSE_H
#define CLARKE_INVERSE_H

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdint.h>

/// \brief Maximum positive value for saturated arithmetic.
const int max_limit = 32767;

/// \brief Minimum negative value for saturated arithmetic.
const int min_limit = -32767;

/// \brief The number \f$\frac{1}{\sqrt{3}}\f$ in the Q16.16 format.
const int sqrt3c = 56756; // 0x0000DDB4
const int sqrt3c_shif = 15;

/// \brief data width
const int in_width = 16;
const int out_width = 16;
/// \brief data width for middle data
const int mid_width = 32;

/**
 * \brief Inverse Clarke transform as AXI4-Stream IP core.
 *
 * It calculates the values \f$V_a\f$, \f$V_b\f$ and \f$V_c\f$ in the ouput AXI4-Stream \p m_axis
 * by using the following equations:
 * \f{equation}{
 *  V_a = V_\alpha,
 * \f}
 * \f{equation}{
 *  V_b = \frac{ -V_\alpha + \sqrt{3}V_\beta}{2},
 * \f}
 * \f{equation}{
 *  V_c = \frac{ -V_\alpha - \sqrt{3}V_\beta}{2}.
 * \f}
 * where \f$V_\alpha\f$ and \f$V_\beta\f$ are from the input AXI4-Stream \p s_axis.
 *
 * \param s_axis Input AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..16: \f$V_\alpha\f$
 *   <li> Bits 17..31: \f$V_\beta\f$
 *   <li> Bits 32..47: Angle, in encoder steps.
 *   <li> Bits 48..63: Unused.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * \param m_axis Output AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..15: \f$V_a\f$.
 *   <li> Bits 16..31: \f$V_b\f$.
 *   <li> Bits 32..47: \f$V_c\f$.
 *   <li> Bits 48..63: Angle, in encoder steps.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * \return void - functions implementing an IP core do not return a value.
 */
void Clarke_Inverse_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis);

#endif // CLARKE_INVERSE_H
