#pragma once

#ifndef _DSPLIB_KERNEL_BROADCAST_HPP_
#define _DSPLIB_KERNEL_BROADCAST_HPP_

// This file holds sets of templated types and overloaded (or template specialized) functions
// for use by multiple kernels.
// Functions in this file as a rule use intrinsics from a single set. For instance, a set
// may contain a MAC with pre-add which uses a single 1024 bit buffer for both forward
// and reverse data. In cases where a library element has to use an intrinsic which differs
// by more than the types used for some combinations of library element parameter types
// then the set of templatized functions will be particular to that library element and should
// therefore be in <library_element>_utils.hpp

#include <stdio.h>
#include <adf.h>
#include "fir_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {

// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, bool TP_CASC_IN>
inline void windowBroadcast(input_window<TT_DATA>* inWindow, output_stream_cacc48* outCascade) {
    using buff_type = typename T_buff_256b<int32>::v_type;
    buff_type buff256;
    buff_type* restrict inWindowPtr = (buff_type*)inWindow->head;
    // const int samplesPer256Buff = sizeof(buff_type)/sizeof(TT_DATA);
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_DATA);
    static_assert(TP_INPUT_WINDOW_VSIZE % samplesPer256Buff == 0, "Error: Window size must be a multiple of 256-bits");

    for (int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += samplesPer256Buff) {
        // copy 256-bit vector at a time
        buff256 = *inWindowPtr++;
        put_mcd(buff256);
    }
}

// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, bool TP_CASC_IN>
inline void windowBroadcast(input_stream_cacc48* inCascade,
                            output_stream_cacc48* outCascade,
                            input_window<TT_DATA>* outWindow) {
    using buff_type = typename T_buff_256b<int32>::v_type;
    buff_type buff256;
    // buff_type* restrict inWindowPtr   = (buff_type*) inWindow->head;
    buff_type* restrict outWindowPtr = (buff_type*)outWindow->head;
    // const int samplesPer256Buff = sizeof(buff_type)/sizeof(TT_DATA);
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_DATA);
    static_assert(TP_INPUT_WINDOW_VSIZE % samplesPer256Buff == 0, "Error: Window size must be a multiple of 256-bits");

    for (int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += samplesPer256Buff) {
        // copy 256-bit vector at a time
        buff256 = get_scd_v8int32();
        put_mcd(buff256);
        *outWindowPtr++ = buff256;
    }
}

// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, bool TP_CASC_IN>
inline void windowBroadcast(input_stream_cacc48* inCascade, input_window<TT_DATA>* outWindow) {
    using buff_type = typename T_buff_256b<int32>::v_type;
    buff_type buff256;
    buff_type* restrict outWindowPtr = (buff_type*)outWindow->head;
    // const int samplesPer256Buff = sizeof(buff_type)/sizeof(TT_DATA);
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_DATA);
    static_assert(TP_INPUT_WINDOW_VSIZE % samplesPer256Buff == 0, "Error: Window size must be a multiple of 256-bits");

    for (int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += samplesPer256Buff) {
        // copy 256-bit vector at a time
        buff256 = get_scd_v8int32();
        *outWindowPtr++ = buff256;
    }
}

// Window Lock Acquire. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
inline void windowAcquire(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface) {
    // Do nothing.
    // When Cascade is not present, Window is a sync connection
    // No need for internal syncronization mechanisms.
}
// Window Lock Acquire. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
inline void windowAcquire(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface) {
    // acquire a lock on async input window
    // window_acquire(inInterface.inWindow);
    inInterface.inWindow->ptr = inInterface.inWindow->head;
}

// Window Lock Release. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
inline void windowRelease(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface) {
    // Do nothing.
    // When Cascade is not present, Window is a sync connection
    // No need for internal syncronization mechanisms.
}

// Window Lock Release. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
inline void windowRelease(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface) {
    // acquire a lock on async broadcast window
    // window_release(inInterface.inWindow);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_DUAL_IP = 0>
inline void windowBroadcast(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                            T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    // Do nothing.
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_DUAL_IP = 0>
inline void windowBroadcast(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                            T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE, CASC_IN_FALSE>(inInterface.inWindow, outInterface.outCascade);
    // chess_memory_fence();
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_DUAL_IP = 0>
inline void windowBroadcast(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                            T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE, CASC_IN_TRUE>(inInterface.inCascade, outInterface.outCascade,
                                                                  inInterface.inWindow);
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_DUAL_IP = 0>
inline void windowBroadcast(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                            T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE, CASC_IN_FALSE>(inInterface.inCascade, inInterface.inWindow);
}
}
}
}
} // namespaces
#endif // _DSPLIB_KERNEL_BROADCAST_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
