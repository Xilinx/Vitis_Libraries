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

#ifndef CLARKE_DIRECT_H
#define CLARKE_DIRECT_H

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdint.h>

/// \brief A testcase for the use of the Clarke_Direct_2p_T_AP
/// \brief Note if the width of input/output > 16bit, then better refine the hard code width in this case
/// \brief set the system to 2 phase system (Ia+Ib+Ic=0), not define will set 3 phase system
#define TWOPHASESYSTEM

/// \brief Maximum positive value for saturated arithmetic.
const int max_limit = 32767;

/// \brief Minimum negative value for saturated arithmetic.
const int min_limit = -32767;

/// \brief The number \f$\frac{1}{\sqrt{3}}\f$ in the Q16.16 format.
const int sqrt3a = 37837; // 0x000093CD
const int sqrt3a_shif = 16;

/// \brief data width
const int in_width = 16;
const int out_width = 16;
/// \brief data width for middle data
const int mid_width = 32;

/**
 * @brief Clark transform as AXI4-Stream IP core
 *
 * It calculates the values \f$I_\alpha\f$ and \f$I_\beta\f$ in the ouput AXI4-Stream \p m_axis
 * by using the following equations:
 * \f{equation}{
 *  I_\alpha = I_a,
 * \f}
 * \f{equation}{
 *  I_\beta = \frac{I_a + 2I_b}{\sqrt{3}},
 * \f}
 * where \f$I_a\f$ and \f$I_b\f$ are from the input AXI4-Stream \p s_axis.
 *
 * @param s_axis Input AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..15: First phase current \f$I_a\f$, from the ADC.
 *   <li> Bits 16..31: Second phase current \f$I_b\f$, from the ADC.
 *   <li> Bits 32..47: Speed, in RPM, just passed through.
 *   <li> Bits 48..63: Angle, in encoder steps, just passed through.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * @param m_axis Output AXI4-Stream with the following layout:
 * <ul>
 *   <li> Bits 0..15: \f$I_\alpha\f$
 *   <li> Bits 16..31: \f$I_\beta\f$
 *   <li> Bits 32..47: when open TWOPHASESYSTEM, Speed, in RPM; else close TWOPHASESYSTEM, \f$I_\homop\f$.
 *   <li> Bits 48..63: Angle, in encoder steps.
 * </ul>
 * All values are 16-bit signed integers.
 *
 * \return void - functions implementing an IP core do not return a value.
 */
void Clarke_Direct_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis);

#endif // CLARKE_DIRECT_H
