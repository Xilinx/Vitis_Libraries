/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
INLINE_DECL void windowBroadcast(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<16> >* inWindow,
                                 output_cascade_cacc* outCascade) {
    const int samplesPerBuffWrite = MCD_SIZE / 8 / sizeof(TT_DATA);
    const int _32bsamplesInUnitWrite = 256 / 8 / sizeof(int32);
    const int samplesInUnitWrite = 256 / 8 / sizeof(TT_DATA);
    const int writesPerLoop = MCD_SIZE / 8 / sizeof(int32);
    const int loopCount = TP_INPUT_WINDOW_VSIZE / samplesPerBuffWrite;
    using buff_type = typename ::aie::vector<int32, writesPerLoop>;
    buff_type writeBuff;
    buff_type* buff_ptr;
    auto inItr = ::aie::begin_random_circular(*(inWindow));
    static_assert(TP_INPUT_WINDOW_VSIZE % samplesInUnitWrite == 0, "Error: Window size must be a multiple of 256-bits");

    for (int i = 0; i < loopCount; i++) {
        buff_ptr = (buff_type*)&*inItr;
        writeBuff = *buff_ptr;
        inItr += samplesPerBuffWrite;
        put_mcd(writeBuff);
    }
    if
        constexpr(TP_INPUT_WINDOW_VSIZE > loopCount * samplesPerBuffWrite) // for AIE2 cases when window size is not a
                                                                           // multiple of the basic unit of cascade
                                                                           // read/write sizes (512 bits)
        {
            using unit_buff_type = typename ::aie::vector<int32, _32bsamplesInUnitWrite>;
            unit_buff_type unitWrBuff;
            unit_buff_type* unit_buff_ptr;
            unit_buff_ptr = (unit_buff_type*)&*inItr;
            writeBuff.insert(0, *unit_buff_ptr);
            inItr += _32bsamplesInUnitWrite;
            put_mcd(writeBuff);
        }
}

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
INLINE_DECL void windowBroadcast(input_async_buffer<TT_DATA>* inWindow, output_cascade_cacc* outCascade) {
    const int _32bsamplesInUnitWrite = 256 / 8 / sizeof(int32);
    const int samplesInUnitWrite = 256 / 8 / sizeof(TT_DATA);
    const int writesPerLoop = MCD_SIZE / 8 / sizeof(int32);
    const int samplesPerBuffWrite = MCD_SIZE / 8 / sizeof(TT_DATA);
    const int loopCount = TP_INPUT_WINDOW_VSIZE / samplesPerBuffWrite;
    using buff_type = typename ::aie::vector<int32, writesPerLoop>;
    buff_type writeBuff;
    buff_type* inItr = (buff_type*)inWindow->data();

    static_assert(TP_INPUT_WINDOW_VSIZE % samplesInUnitWrite == 0, "Error: Window size must be a multiple of 256-bits");
    for (int i = 0; i < loopCount; i++) {
        // copy 256/512-bit vector at a time
        writeBuff = *inItr++;
        put_mcd(writeBuff);
    }
    if
        constexpr(TP_INPUT_WINDOW_VSIZE > loopCount * samplesPerBuffWrite) // for AIE2 cases when window size is not a
                                                                           // multiple of the basic unit of cascade
                                                                           // read/write sizes (512 bits)
        {
            using unit_buff_type = typename ::aie::vector<int32, _32bsamplesInUnitWrite>;
            unit_buff_type unitWrBuff;
            unit_buff_type* unit_buff_ptr;
            unit_buff_ptr = (unit_buff_type*)&*inItr;
            writeBuff.insert(0, *unit_buff_ptr);
            inItr += _32bsamplesInUnitWrite;
            put_mcd(writeBuff);
        }
}

// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
INLINE_DECL void windowBroadcast(input_cascade_cacc* inCascade,
                                 output_cascade_cacc* outCascade,
                                 input_async_buffer<TT_DATA>* outWindow) {
    const int samplesPerLoop = MCD_SIZE / 8 / sizeof(int32);
    const int samplesPerBuffWrite = MCD_SIZE / 8 / sizeof(TT_DATA);
    const int _32bsamplesInUnitWrite = 256 / 8 / sizeof(int32);
    const int samplesInUnitWrite = 256 / 8 / sizeof(TT_DATA);
    const int loopCount = TP_INPUT_WINDOW_VSIZE / samplesPerBuffWrite;
    using buff_type = typename ::aie::vector<int32, samplesPerLoop>;
    buff_type rwBuff;
    buff_type* outItr = (buff_type*)outWindow->data();
    static_assert(TP_INPUT_WINDOW_VSIZE % samplesInUnitWrite == 0, "Error: Window size must be a multiple of 256-bits");

    for (int i = 0; i < loopCount; i++) {
// copy 256-bit vector at a time
#if SCD_SIZE == 256
        rwBuff = get_scd_v8int32(); // int32
#else
        rwBuff = get_scd_v16int32();
#endif // SCD_SIZE
        put_mcd(rwBuff);
        *outItr++ = rwBuff;
    }
    if
        constexpr(TP_INPUT_WINDOW_VSIZE > loopCount * samplesPerBuffWrite) // for AIE2 cases when window size is not a
                                                                           // multiple of the basic unit of cascade
                                                                           // read/write sizes (512 bits)
        {
#if SCD_SIZE == 512
            rwBuff = get_scd_v16int32();
#endif
            put_mcd(rwBuff);
            using unit_buff_type = typename ::aie::vector<int32, _32bsamplesInUnitWrite>;
            unit_buff_type* unit_buff_ptr;
            unit_buff_ptr = (unit_buff_type*)&*outItr;
            *unit_buff_ptr++ = rwBuff.template extract<_32bsamplesInUnitWrite>(0);
        }
}

// To optimize performance, 256-bit vectors are copied, so storage element must be padded to 256-bits.
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
INLINE_DECL void windowBroadcast(input_cascade_cacc* inCascade, input_async_buffer<TT_DATA>* outWindow) {
    const int _32bsamplesInUnitRead = 256 / 8 / sizeof(int32);
    const int samplesInUnitRead = 256 / 8 / sizeof(TT_DATA);
    const int samplesPerBuffRead = MCD_SIZE / 8 / sizeof(TT_DATA);
    const int _32bSamplesPerBuffRead = MCD_SIZE / 8 / sizeof(int32);
    const int loopCount = TP_INPUT_WINDOW_VSIZE / samplesPerBuffRead;
    using buff_type = typename ::aie::vector<int32, _32bSamplesPerBuffRead>;
    buff_type readBuff;
    buff_type* outItr = (buff_type*)outWindow->data();
    static_assert(TP_INPUT_WINDOW_VSIZE % samplesInUnitRead == 0, "Error: Window size must be a multiple of 256-bits");

    for (int i = 0; i < loopCount; i++) {
// copy 256-bit vector at a time
#if MCD_SIZE == 256
        readBuff = get_scd_v8int32();
#else
        readBuff = get_scd_v16int32();
#endif
        *outItr++ = readBuff;
    }
    if
        constexpr(TP_INPUT_WINDOW_VSIZE > loopCount * samplesPerBuffRead) // for AIE2 cases when window size is not a
                                                                          // multiple of the basic unit of cascade
                                                                          // read/write sizes (512 bits)
        {
#if SCD_SIZE == 512
            readBuff = get_scd_v16int32();
#endif
            using unit_buff_type = typename ::aie::vector<int32, _32bsamplesInUnitRead>;
            unit_buff_type subBuff;
            unit_buff_type* unit_buff_ptr;
            unit_buff_ptr = (unit_buff_type*)&*outItr;
            subBuff = readBuff.template extract<_32bsamplesInUnitRead>(0);
            *unit_buff_ptr = subBuff;
        }
}

template <typename TT_DATA, unsigned int TP_DUAL_IP = 0, unsigned int TP_API = 0>
INLINE_DECL void windowAcquire(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface) {
    // Do nothing.
}

template <typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void windowAcquire(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface) {
    // Reset async input window pointer
    inInterface.inWindowLin->ptr = inInterface.inWindowLin->head;
}

template <typename TT_DATA, bool TP_CASC_IN, unsigned int TP_DUAL_IP, unsigned int TP_API>
INLINE_DECL void windowReset(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    // Reset async window pointers when windows are used.
    // Pointer reset is needed to revert the window after the previous iteration.
    if
        constexpr(TP_API == 0 && TP_CASC_IN == CASC_IN_TRUE) {
            //        inInterface.inWindow->ptr = inInterface.inWindow->head; //redundant for IO buffer, since iterator
            //        initializes to start each iteration.
        }
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
                    // if (inInterface.inWindow == NULL) {
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inWindowCirc, outInterface.outCascade);
                    //} else {
                    //  windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inWindow, outInterface.outCascade);
                    //}
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
                                                                    inInterface.inWindowLin);
                }
            else if
                constexpr(kernelPosEnum == first_kernel_in_chain) {
                    // Call the overload that takes data from the window and broadcasts it on the cascade only.
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inWindowCirc, outInterface.outCascade);
                }
            else if
                constexpr(kernelPosEnum == last_kernel_in_chain) {
                    // Call the overload that takes data from the cascade and doesn't also broadcast it on the cascade.
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inCascade, inInterface.inWindowLin);
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
                    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE>(inInterface.inCascade, inInterface.inWindowLin);
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
