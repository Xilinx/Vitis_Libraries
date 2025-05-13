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
#ifndef _DSPLIB_MERGE_SORT_HPP_
#define _DSPLIB_MERGE_SORT_HPP_

#include <adf.h>
#include "merge_sort_traits.hpp"
#include "device_defs.h"
using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {
/*
* @brief Acts as a wrapper and the entry point from the graph.
*/

// kStreamApi
// kWindowApi
// kStreamCascApi
// kCascApi

// kWindow - kStream
// kWindow - kCasc
// kStreamCasc - kStream
// kStreamCasc - kCasc
// kStreamStream - kStream

template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
class kernelClass {
   private:
    static constexpr int kSamplesInVec = 128 / 8 / sizeof(TT_DATA);
    static constexpr int kSamplesInWindowVec = 256 / 8 / sizeof(TT_DATA);
    static constexpr int kVecInFrame = TP_DIM / kSamplesInVec;
    static constexpr int kVecInWindow = (TP_DIM / 2) / kSamplesInWindowVec;

    // get min or max values of datatypes.
    static constexpr TT_DATA maxValueOfType = std::numeric_limits<TT_DATA>::max();
    // min() for float returns ~0. Min value we need is -max()
    static constexpr TT_DATA minValueOfType =
        (isFloat<TT_DATA>()) ? -maxValueOfType : std::numeric_limits<TT_DATA>::min();
    static constexpr TT_DATA sortLimit = (TP_ASCENDING) ? maxValueOfType : minValueOfType;

    void mergeSortSelectArch(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);
    void mergeSortStreamIn(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);
    void mergeSortBufferIn(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);

   public:
    // Constructor
    kernelClass() {}
    void mergeSortKernel(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);
};
// dual stream in, single stream out
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
class merge_sort : public kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_DIM, TP_ASCENDING> {
   public:
    // Constructor
    merge_sort() {}
    static void registerKernelClass() { REGISTER_FUNCTION(merge_sort::bitonic_merge_main); }
    void bitonic_merge_main(input_stream<TT_DATA>* __restrict inStream0,
                            input_stream<TT_DATA>* __restrict inStream1,
                            output_stream<TT_DATA>* __restrict outStream);
};
// stream casc in, stream out
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
class merge_sort<TT_DATA, kStreamCascAPI, kStreamAPI, TP_DIM, TP_ASCENDING>
    : public kernelClass<TT_DATA, kStreamCascAPI, kStreamAPI, TP_DIM, TP_ASCENDING> {
   public:
    // Constructor
    merge_sort() {}

    static void registerKernelClass() { REGISTER_FUNCTION(merge_sort::bitonic_merge_main); }
    void bitonic_merge_main(input_stream<TT_DATA>* __restrict inStream0,
                            input_cascade<TT_DATA>* __restrict inCascade,
                            output_stream<TT_DATA>* __restrict outStream);
};
// stream casc in, casc out
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
class merge_sort<TT_DATA, kStreamCascAPI, kCascAPI, TP_DIM, TP_ASCENDING>
    : public kernelClass<TT_DATA, kStreamCascAPI, kCascAPI, TP_DIM, TP_ASCENDING> {
   public:
    // Constructor
    merge_sort() {}

    static void registerKernelClass() { REGISTER_FUNCTION(merge_sort::bitonic_merge_main); }
    void bitonic_merge_main(input_stream<TT_DATA>* __restrict inStream0,
                            input_cascade<TT_DATA>* __restrict inCascade,
                            output_cascade<TT_DATA>* __restrict outCascade);
};
// buffers in, stream out
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
class merge_sort<TT_DATA, kWindowAPI, kStreamAPI, TP_DIM, TP_ASCENDING>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamAPI, TP_DIM, TP_ASCENDING> {
   public:
    // Constructor
    merge_sort() {}

    static void registerKernelClass() { REGISTER_FUNCTION(merge_sort::bitonic_merge_main); }

    void bitonic_merge_main(input_buffer<TT_DATA>& __restrict inWindow0,
                            input_buffer<TT_DATA>& __restrict inWindow1,
                            output_stream<TT_DATA>* __restrict outStream);
};
// buffers in, casc out
template <typename TT_DATA,
          //   unsigned int TP_IN_API,
          //   unsigned int TP_OUT_API,
          unsigned int TP_DIM,
          unsigned int TP_ASCENDING>
class merge_sort<TT_DATA, kWindowAPI, kCascAPI, TP_DIM, TP_ASCENDING>
    : public kernelClass<TT_DATA, kWindowAPI, kCascAPI, TP_DIM, TP_ASCENDING> {
   public:
    // Constructor
    merge_sort() {}

    static void registerKernelClass() { REGISTER_FUNCTION(merge_sort::bitonic_merge_main); }

    void bitonic_merge_main(input_buffer<TT_DATA>& __restrict inWindow0,
                            input_buffer<TT_DATA>& __restrict inWindow1,
                            output_cascade<TT_DATA>* __restrict outCascade);
};
}
}
}
}

#endif
