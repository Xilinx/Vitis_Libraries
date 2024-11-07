/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_CONV_CORR_HPP_
#define _DSPLIB_CONV_CORR_HPP_

/*
CONV_CORR
This file exists to capture the definition of the CONV_CORR kernel class.
The class definition holds defensive checks on parameter range and other legality.
The constructor definition is held in this class because this class must be accessible to graph
level aie compilation.
The main runtime function is captured elsewhere as it contains aie intrinsics which are not
included in aie graph level compilation.
*/

/* Coding conventions
TT_      template type suffix
TP_      template parameter suffix
*/

/* Design Notes
*/

#include <adf.h>
#include <vector>
#include <array>
#include "device_defs.h"
#include "fir_utils.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "conv_corr_traits.hpp"

using namespace adf;

#ifndef _DSPLIB_CONV_CORR_HPP_DEBUG_
//#define _DSPLIB_CONV_CORR_HPP_DEBUG_
#endif //_DSPLIB_CONV_CORR_HPP_DEBUG_
#define V8SIZEOFACC 8
namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {
//-----------------------------------------------------------------------------------------------------
// Single kernel specialization for io bufer

template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION,
          bool TP_CASC_IN = CASC_IN_FALSE,
          bool TP_CASC_OUT = CASC_OUT_FALSE>
class conv_corr {
   private:
    // constants derived from configuration parameters
    // number of multiplications per lane for main intrinsic
    static constexpr unsigned int m_kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>();

    // number of lanes that intrinsic would operate
    static constexpr unsigned int m_kLanes = getNumofLanes<TT_DATA_F, TT_DATA_G>();

    // number of points that intrinsic would operate
    static constexpr unsigned int m_kPoints = getNumofPoints<TT_DATA_F, TT_DATA_G>();

    // Loop count that aie would operate
    static constexpr unsigned int m_kPaddedLenData =
        getPaddedLength<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

    // length of F vector that aie would operate
    static constexpr unsigned int m_kLoopCount = getLoopCount<TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

    // load max possible elements each time based on sample size from memory that aie would operate
    static constexpr unsigned int m_kVecLoadF = (maxBitsLoadOnAie() / (aie_CC_Fsample_Size<TT_DATA_F>()));

    // load max possible elements each time based on sample size from memory that aie would operate
    static constexpr unsigned int m_kVecLoadG = (maxBitsLoadOnAie() / (aie_CC_Gsample_Size<TT_DATA_G>()));

   public:
    // Constructor of con_corr class
    conv_corr() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr::conv_corrMain); }
    // Conv_Corr
    void conv_corrMain(input_buffer<TT_DATA_F>& __restrict inWindowF,
                       input_buffer<TT_DATA_G>& __restrict inWindowG,
                       output_buffer<TT_DATA_OUT>& __restrict outWindow);
};

#if (__HAS_ACCUM_PERMUTES__ == 1)
// Conv-Corr Product class - stream specialization
// Partially specialized classes for cascaded interface

// First Kernel of the cascade
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
class conv_corr<TT_DATA_F,
                TT_DATA_G,
                TT_DATA_OUT,
                TP_FUNCT_TYPE,
                TP_COMPUTE_MODE_IS_VALID_MODE(),
                TP_F_LEN,
                TP_G_LEN,
                TP_SHIFT,
                TP_API_IS_ONE(),
                TP_RND,
                TP_SAT,
                TP_NUM_FRAMES,
                TP_CASC_LEN,
                TP_PHASES,
                TP_KERNEL_POSITION,
                TP_PH_POSITION,
                CASC_IN_FALSE,
                CASC_OUT_TRUE> {
   private:
    // number of muls that intrinsic would operate
    static constexpr unsigned int m_kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>();
    static constexpr unsigned int m_kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of lanes
    static constexpr unsigned int m_kPoints = (m_kMuls / m_kLanes);                   // Num of Points
    static constexpr unsigned int m_kCores = (TP_CASC_LEN * TP_PHASES);
    static constexpr unsigned int m_kMaxMuls = (m_kLanes * m_kPoints * m_kCores);
    static constexpr unsigned int m_kMacsPerCore = (CEIL(TP_G_LEN, m_kMaxMuls)) / m_kMaxMuls;
    static constexpr unsigned int m_kDataBuffLen =
        ((m_kMacsPerCore * m_kPoints) < dataBuffLenFactor())
            ? minDataBuffLen()
            : ((m_kMacsPerCore * m_kPoints) << mulFactor2()); // Len of Data Buffer

    // Note, both these variables are non-static, so all kernels can update this variable
    alignas(__ALIGN_BYTE_SIZE__) int delayBuff[(m_kDataBuffLen * sizeof(TT_DATA_F)) / sizeof(int)] = {0};
    alignas(__ALIGN_BYTE_SIZE__) int delayAcc[(V8SIZEOFACC * sizeof(cacc48)) / sizeof(int)] = {0};
    int doInit = 1;

   public:
    // Constructor of con_corr class
    conv_corr() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr::conv_corrMain); }
    // Conv_Corr
    void conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                       input_stream<TT_DATA_F>* __restrict instream2F,
                       input_buffer<TT_DATA_G>& __restrict inWindowG,
                       output_cascade<cacc48>* __restrict outcascade);
};

// Conv-Corr Product class - stream specialization
// Intermediate Kernels of the cascade
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
class conv_corr<TT_DATA_F,
                TT_DATA_G,
                TT_DATA_OUT,
                TP_FUNCT_TYPE,
                TP_COMPUTE_MODE_IS_VALID_MODE(),
                TP_F_LEN,
                TP_G_LEN,
                TP_SHIFT,
                TP_API_IS_ONE(),
                TP_RND,
                TP_SAT,
                TP_NUM_FRAMES,
                TP_CASC_LEN,
                TP_PHASES,
                TP_KERNEL_POSITION,
                TP_PH_POSITION,
                CASC_IN_TRUE,
                CASC_OUT_TRUE> {
   private:
    // number of muls that intrinsic would operate
    static constexpr unsigned int m_kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>();
    static constexpr unsigned int m_kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of Lanes
    static constexpr unsigned int m_kPoints = m_kMuls / m_kLanes;                     // Num of Points
    static constexpr unsigned int m_kCores = TP_CASC_LEN * TP_PHASES;
    static constexpr unsigned int m_kMaxMuls = m_kLanes * m_kPoints * m_kCores;
    static constexpr unsigned int m_kMacsPerCore = (CEIL(TP_G_LEN, m_kMaxMuls)) / m_kMaxMuls;
    static constexpr unsigned int m_kDataBuffLen =
        ((m_kMacsPerCore * m_kPoints) < dataBuffLenFactor())
            ? minDataBuffLen()
            : ((m_kMacsPerCore * m_kPoints) << mulFactor2()); // Len of Data Buffer

    // Note, both these variables are non-static, so all kernels can update this variable
    alignas(__ALIGN_BYTE_SIZE__) int delayBuff[(m_kDataBuffLen * sizeof(TT_DATA_F)) / sizeof(int)] = {0};
    alignas(__ALIGN_BYTE_SIZE__) int delayAcc[(V8SIZEOFACC * sizeof(cacc48)) / sizeof(int)] = {0};
    int doInit = 1;

   public:
    // Constructor of con_corr class
    conv_corr() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr::conv_corrMain); }
    // Conv_Corr
    void conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                       input_stream<TT_DATA_F>* __restrict instream2F,
                       input_cascade<cacc48>* __restrict incascade,
                       output_cascade<cacc48>* __restrict outcascade);
};

// Last Kernel of the cascade
// Partially specialized classes for cascaded interface
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
class conv_corr<TT_DATA_F,
                TT_DATA_G,
                TT_DATA_OUT,
                TP_FUNCT_TYPE,
                TP_COMPUTE_MODE_IS_VALID_MODE(),
                TP_F_LEN,
                TP_G_LEN,
                TP_SHIFT,
                TP_API_IS_ONE(),
                TP_RND,
                TP_SAT,
                TP_NUM_FRAMES,
                TP_CASC_LEN,
                TP_PHASES,
                TP_KERNEL_POSITION,
                TP_PH_POSITION,
                CASC_IN_TRUE,
                CASC_OUT_FALSE> {
   private:
    // number of muls that intrinsic would operate
    static constexpr unsigned int m_kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>();
    static constexpr unsigned int m_kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of Lanes
    static constexpr unsigned int m_kPoints = (m_kMuls / m_kLanes);                   // Num of Points

    static constexpr unsigned int m_kCores = TP_CASC_LEN * TP_PHASES;
    static constexpr unsigned int m_kMaxMuls = m_kLanes * m_kPoints * m_kCores;
    static constexpr unsigned int m_kMacsPerCore = (CEIL(TP_G_LEN, m_kMaxMuls)) / m_kMaxMuls;
    static constexpr unsigned int m_kDataBuffLen =
        ((m_kMacsPerCore * m_kPoints) < dataBuffLenFactor())
            ? minDataBuffLen()
            : ((m_kMacsPerCore * m_kPoints) << mulFactor2()); // Len of Data Buffer

    // Note, both these variables are non-static, so all kernels can update this variable
    alignas(__ALIGN_BYTE_SIZE__) int delayBuff[(m_kDataBuffLen * sizeof(TT_DATA_F)) / sizeof(int)] = {0};
    alignas(__ALIGN_BYTE_SIZE__) int delayAcc[(V8SIZEOFACC * sizeof(cacc48)) / sizeof(int)] = {0};
    int doInit = 1;

   public:
    // Constructor of con_corr class
    conv_corr() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr::conv_corrMain); }
    // Conv_Corr
    void conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                       input_stream<TT_DATA_F>* __restrict instream2F,
                       input_cascade<cacc48>* __restrict incascade,
                       output_stream<TT_DATA_OUT>* __restrict outstream);
};

// Single Kernel per phase
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
class conv_corr<TT_DATA_F,
                TT_DATA_G,
                TT_DATA_OUT,
                TP_FUNCT_TYPE,
                TP_COMPUTE_MODE_IS_VALID_MODE(),
                TP_F_LEN,
                TP_G_LEN,
                TP_SHIFT,
                TP_API_IS_ONE(),
                TP_RND,
                TP_SAT,
                TP_NUM_FRAMES,
                TP_CASC_LEN,
                TP_PHASES,
                TP_KERNEL_POSITION,
                TP_PH_POSITION,
                CASC_IN_FALSE,
                CASC_OUT_FALSE> {
   private:
    // number of Lanes that intrinsic would operate
    static constexpr unsigned int m_kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>();
    static constexpr unsigned int m_kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of Lanes
    static constexpr unsigned int m_kPoints = m_kMuls / m_kLanes;                     // Num of Points

    static constexpr unsigned int m_kCores = TP_CASC_LEN * TP_PHASES;
    static constexpr unsigned int m_kMaxMuls = m_kLanes * m_kPoints * m_kCores;
    static constexpr unsigned int m_kMacsPerCore = (CEIL(TP_G_LEN, m_kMaxMuls)) / m_kMaxMuls;
    static constexpr unsigned int m_kDataBuffLen =
        ((m_kMacsPerCore * m_kPoints) < dataBuffLenFactor())
            ? minDataBuffLen()
            : ((m_kMacsPerCore * m_kPoints) << mulFactor2()); // Len of Data Buffer

    // Note, both these variables are non-static, so all kernels can update this variable
    alignas(__ALIGN_BYTE_SIZE__) int delayBuff[(m_kDataBuffLen * sizeof(TT_DATA_F)) / sizeof(int)] = {0};
    alignas(__ALIGN_BYTE_SIZE__) int delayAcc[(V8SIZEOFACC * sizeof(cacc48)) / sizeof(int)] = {0};
    int doInit = 1;

   public:
    // Constructor of con_corr class
    conv_corr() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr::conv_corrMain); }
    // Conv_Corr
    void conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                       input_stream<TT_DATA_F>* __restrict instream2F,
                       input_buffer<TT_DATA_G>& __restrict inWindowG,
                       output_stream<TT_DATA_OUT>* __restrict outstream);
};

#endif

} // namespace conv_corr
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_CONV_CORR_HPP_
