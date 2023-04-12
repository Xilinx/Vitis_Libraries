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

#include "clarke_direct.h"
#include "clarke_2p.hpp"
#include "clarke_3p.hpp"

using namespace hls;

typedef short T_Iabc;

//--------------------------------------------------------------------------
// Clarke Direct 2 phase
// Ia + Ib + Ic = 0
// Ialpha = Ia
// Ibeta = (Ia + 2Ib)/sqrt(3)
// Where Ia+Ib+Ic = 0
//--------------------------------------------------------------------------
void Clarke_Direct_2p_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis) {
#pragma HLS INLINE
    int64_t in_data, res;
    int16_t Ia, Ib, Theta, RPM;
    int32_t Ialpha, Ibeta, Ibd;

    // Decode Input stream
    in_data = s_axis.read();                   // Read one value from AXI4-Stream
    Ia = int16_t(in_data & 0xFFFF);            // Extract Ia - bits[15..0] from input stream
    Ib = int16_t((in_data >> 16) & 0xFFFF);    // Extract Ib - bits[32..16] from input stream
    RPM = int16_t((in_data >> 32) & 0xFFFF);   // Extract RPM - bits[47..32] from input stream
    Theta = int16_t((in_data >> 48) & 0xFFFF); // Extract Angle - bits[63..48] from input stream

    // Process data
    ap_int<out_width> ia_out;
    ap_int<out_width> ib_out;
    ap_int<in_width> ia_in = Ia;
    ap_int<in_width> ib_in = Ib;
    Clarke_Direct_2p_T_AP<in_width, mid_width, out_width, max_limit, min_limit, sqrt3a, sqrt3a_shif>(ia_out, ib_out,
                                                                                                     ia_in, ib_in);
    Ialpha = ia_out;
    Ibeta = ib_out;

    // Write output stream
    res = (((int64_t)Theta << 48) & 0xFFFF000000000000) | // Put Angle bits[63:48]
          (((int64_t)RPM << 32) & 0x0000FFFF00000000) |   // Put RPM bits[47:32]
          (((int64_t)Ibeta << 16) & 0x00000000FFFF0000) | // Put Ibeta bits[31:16]
          ((int64_t)Ialpha & 0x000000000000FFFF);         // Put Ialpha bits[15:0]
    m_axis.write(res);                                    // Write result to the output stream
};

//--------------------------------------------------------------------------
// Clarke Direct 3 phase
// Ialpha = 2/3*Ia- 1/3*(Ib -Ic)
// Ibeta = 2*(Ib-Ic)/sqrt(3)
// Ihomop = 2/3*(Ia+Ib+Ic)
// Where Ia+Ib+Ic != 0 ?
// iα and iβ components in an orthogonal reference frame and
// io the homopolar component of the system
//--------------------------------------------------------------------------
#if 0


void Clarke_Direct_3p_axi(hls::stream<int64_t> &s_axis, hls::stream<int64_t> &m_axis){
#pragma HLS INLINE
	int64_t in_data, res;
	int16_t Ia, Ib, Theta, Ic;
	int32_t Ialpha, Ibeta, Ibd, Ihomop;

	// Decode Input stream
	in_data = s_axis.read();					// Read one value from AXI4-Stream
	Ia = int16_t(in_data & 0xFFFF);				// Extract Ia - bits[15..0] from input stream
	Ib = int16_t((in_data >> 16) & 0xFFFF);		// Extract Ib - bits[32..16] from input stream
	Ic = int16_t((in_data >> 32) & 0xFFFF);		// Extract Ic - bits[47..32] from input stream
	Theta = int16_t((in_data >> 48) & 0xFFFF);	// Extract Angle - bits[63..48] from input stream

	// Process data
    T_Iabc ia_out;  
    T_Iabc ib_out;
	T_Iabc ic_out;
    T_Iabc ia_in = Ia;
    T_Iabc ib_in = Ib;
	T_Iabc ic_in = Ic;
    Clarke_Direct_3p_core( &ia_out, &ib_out, &ic_out, ia_in, ib_in, ic_in);
    Ialpha = ia_out;
    Ibeta = ib_out;
	Ihomop = ic_out;

	// Write output stream
	res = 	(((int64_t)Theta << 48)	& 0xFFFF000000000000) | // Put Angle bits[63:48]
			(((int64_t)Ihomop << 32)& 0x0000FFFF00000000) | // Put RPM bits[47:32]
			(((int64_t)Ibeta << 16) & 0x00000000FFFF0000) | // Put Ibeta bits[31:16]
			( (int64_t)Ialpha		& 0x000000000000FFFF);	// Put Ialpha bits[15:0]
	m_axis.write(res);								// Write result to the output stream
};
#endif

void Clarke_Direct_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis

#ifdef TWOPHASESYSTEM
    Clarke_Direct_2p_axi(s_axis, m_axis);
#else
    Clarke_Direct_3p_axi(s_axis, m_axis);
#endif
}
