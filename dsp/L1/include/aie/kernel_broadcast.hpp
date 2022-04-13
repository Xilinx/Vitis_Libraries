/*
 * Copyright 2022 Xilinx, Inc.
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
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
INLINE_DECL void windowBroadcast(input_window<TT_DATA>* inWindow, output_stream_cacc48* outCascade) {
    using buff_type = typename T_buff_256b<int32>::v_type;
    buff_type buff256;
    buff_type* __restrict inWindowPtr = (buff_type*)inWindow->head;
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
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
INLINE_DECL void windowBroadcast(input_stream_cacc48* inCascade,
                                 output_stream_cacc48* outCascade,
                                 input_window<TT_DATA>* outWindow) {
    using buff_type = typename T_buff_256b<int32>::v_type;
    buff_type buff256;
    buff_type* __restrict outWindowPtr = (buff_type*)outWindow->head;
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
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
INLINE_DECL void windowBroadcast(input_stream_cacc48* inCascade, input_window<TT_DATA>* outWindow) {
    using buff_type = typename T_buff_256b<int32>::v_type;
    buff_type buff256;
    buff_type* __restrict outWindowPtr = (buff_type*)outWindow->head;
    // const int samplesPer256Buff = sizeof(buff_type)/sizeof(TT_DATA);
    const int samplesPer256Buff = 256 / 8 / sizeof(TT_DATA);
    static_assert(TP_INPUT_WINDOW_VSIZE % samplesPer256Buff == 0, "Error: Window size must be a multiple of 256-bits");

    for (int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += samplesPer256Buff) {
        // copy 256-bit vector at a time
        buff256 = get_scd_v8int32();
        *outWindowPtr++ = buff256;
    }
}

template <typename TT_DATA, unsigned int TP_DUAL_IP = 0, unsigned int TP_API = 0>
INLINE_DECL void windowAcquire(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface) {
    // Do nothing.
}

template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void windowAcquire(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface) {
    // Reset async input window pointer
    inInterface.inWindow->ptr = inInterface.inWindow->head;
}

template <typename TT_DATA, bool TP_CASC_IN, unsigned int TP_DUAL_IP, unsigned int TP_API>
INLINE_DECL void windowReset(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    // Reset async window pointers when windows are used.
    // Pointer reset is needed to revert the window after the previous iteration.
    if
        constexpr(TP_API == 0 && TP_CASC_IN == CASC_IN_TRUE) { inInterface.inWindow->ptr = inInterface.inWindow->head; }
}

// Window Lock Release. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void windowRelease(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface) {
    // Do nothing.
    // When Cascade is not present, Window is a sync connection
    // No need for internal syncronization mechanisms.
}

// Window Lock Release. Overloaded with IO interface.
template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void windowRelease(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface) {
    // acquire a lock on async broadcast window
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API = 0,
          unsigned int TP_DUAL_IP = 0,
          kernelPositionState kernelPosEnum = only_kernel>
INLINE_DECL void windowBroadcast(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                 T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    // Do nothing.
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API = 0,
          unsigned int TP_DUAL_IP = 0,
          kernelPositionState kernelPosEnum = first_kernel_in_chain>
INLINE_DECL void windowBroadcast(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                 T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    static_assert((kernelPosEnum == first_kernel_in_chain || kernelPosEnum == only_kernel),
                  "We only expect to get here when we're the only kernel or the first in a cascade chain.");

    // Last kernel with casc_out_true should not broadcast window data - checking if we're the first kernel only is not
    // suffcient.
    if
        constexpr(TP_API == 0) {
            if
                constexpr(kernelPosEnum == first_kernel_in_chain) {
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inWindow, outInterface.outCascade);
                }
            else if
                constexpr(kernelPosEnum == only_kernel) {
                    // If we're the only kernel (ie, no cascade chain), don't do any broadcast.
                }
            else {
                // Should have been caught by static assert
            }
        }
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API = 0,
          unsigned int TP_DUAL_IP = 0,
          kernelPositionState kernelPosEnum = middle_kernel_in_chain>
INLINE_DECL void windowBroadcast(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                 T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface) {
    if
        constexpr(TP_API == 0) {
            if
                constexpr(kernelPosEnum == middle_kernel_in_chain) {
                    // Normal middle kernel processing - take data from the cascde, put it in our window and also
                    // broadcast it out to the output cascade
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inCascade, outInterface.outCascade,
                                                                    inInterface.inWindow);
                }
            else if
                constexpr(kernelPosEnum == first_kernel_in_chain) {
                    // Call the overload that takes data from the window and broadcasts it on the cascade only.
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inWindow, outInterface.outCascade);
                }
            else if
                constexpr(kernelPosEnum == last_kernel_in_chain) {
                    // Call the overload that takes data from the cascade and doesn't also broadcast it on the cascade.
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inCascade, inInterface.inWindow);
                }
            else if
                constexpr(kernelPosEnum == only_kernel) {
                    // don't do any window broadcast - we're not in a cascade chain context.
                }
            else {
                // error
            }
        }
}

// Wrapper. Overloaded with IO interface.
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API = 0,
          unsigned int TP_DUAL_IP = 0,
          kernelPositionState kernelPosEnum = last_kernel_in_chain>
INLINE_DECL void windowBroadcast(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                 T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    if
        constexpr(TP_API == 0) {
            static_assert((kernelPosEnum == only_kernel || kernelPosEnum == last_kernel_in_chain),
                          "Middle kernel or first kernel in chain is not expected to have CASC_OUT_FALSE");

            if
                constexpr(kernelPosEnum == last_kernel_in_chain) {
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inCascade, inInterface.inWindow);
                }
            else if
                constexpr(kernelPosEnum == only_kernel) {
                    // no cascade chain - don't do any window broadcast
                }
            else {
                // error should have been caught by static assert.
            }
        }
}
}
}
}
} // namespaces
#endif // _DSPLIB_KERNEL_BROADCAST_HPP_
