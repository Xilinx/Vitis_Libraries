/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_QRD_HH_HPP_
#define _SOLVERLIB_QRD_HH_HPP_

/*
QRD Kernel.
This file exists to capture the definition of the QRD kernel class.
The class definition holds defensive checks on parameter range and other
legality.
The constructor definition is held in this class because this class must be
accessible to graph level aie compilation.
The main runtime function is captured elsewhere (cpp) as it contains aie
intrinsics (albeit aie api) which are not included in aie graph level
compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

// #include <vector>
#include <array>
#include <adf.h>
#include "qrd_hh_traits.hpp"

using namespace adf;

//#define _DSPLIB_QRD_HPP_DEBUG_

// #include "qrd_hh_traits.hpp" //for fnPointSizePwr

namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {

// QRD kernel R class -- default
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS,
          bool TP_ROUT_EN,
          bool TP_STREAM_EN>

class qrd_hh_kernel {
   private:
   public:
    static constexpr int vecSampleNum = kMaxReadInBytes / sizeof(TT_DATA);
    static constexpr int m_kRowChunkNum = TP_DIM_ROWS / vecSampleNum; // local row operations
    static constexpr int m_kColChunkNum = TP_DIM_COLS / vecSampleNum;
    static constexpr int m_kRcolNum =
        (TP_DIM_ROWS <= TP_DIM_COLS - (TP_KERNEL_POS * TP_DIM_ROWS))
            ? TP_DIM_ROWS
            : (TP_DIM_COLS - (TP_KERNEL_POS * TP_DIM_ROWS)); // Remaining col number for R update
    static constexpr int m_kRcolChunk = m_kRcolNum / vecSampleNum;

    static constexpr int m_kColChunkLeadingStart = m_kRowChunkNum * TP_KERNEL_POS; // Leading col chunk  start
    static constexpr int m_kColChunkLeadingEnd =
        m_kColChunkLeadingStart + m_kRowChunkNum - 1; // Leading col chunk to end

    static constexpr int m_kRowsChunksTotal = m_kRowChunkNum * TP_CASC_LEN;
    static constexpr int m_kColChunkFinalStart = m_kRowsChunksTotal - m_kRowChunkNum;

    static constexpr auto m_kKernelRolesR = det_kernel_roles_r<m_kColChunkNum,
                                                               m_kRowChunkNum,
                                                               m_kColChunkLeadingStart,
                                                               m_kColChunkLeadingEnd,
                                                               m_kColChunkFinalStart,
                                                               TP_KERNEL_POS,
                                                               TP_CASC_LEN>();

    static constexpr auto m_kKernelRolesQ = det_kernel_roles_q<m_kColChunkNum,
                                                               m_kRowChunkNum,
                                                               m_kColChunkLeadingStart,
                                                               m_kColChunkLeadingEnd,
                                                               m_kColChunkFinalStart,
                                                               TP_KERNEL_POS,
                                                               TP_CASC_LEN>();

    static constexpr int kSamplesRow_padded = CEIL(TP_DIM_ROWS, vecSampleNum) / vecSampleNum;
    static constexpr int kSamplesCol_padded = CEIL(TP_DIM_COLS, vecSampleNum) / vecSampleNum;

    static constexpr TT_DATA m_kZERO = constVals<TT_DATA>().c0;
    static constexpr TT_DATA m_kONE = constVals<TT_DATA>().c1;
    static constexpr TT_DATA m_kTWO = constVals<TT_DATA>().c2;

    alignas(__ALIGN_BYTE_SIZE__) TT_DATA vvectBuff[TP_DIM_ROWS] = {};
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA betavectBuff[TP_DIM_COLS] = {};

    // Constructor
    qrd_hh_kernel(){};

    // Householder function
    void qrd_hh_r_func(T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                       T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface);
    void qrd_hh_q_func(T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                       T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface,
                       int& frame_id);
    void qrd_hh_r_casc_func(T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                            T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface);
    void qrd_hh_q_casc_func(T_inputIF<TP_STREAM_EN, TT_DATA>& inInterface,
                            T_outputIF<TP_STREAM_EN, TP_ROUT_EN, TT_DATA>& outInterface,
                            int& frame_id);
};

// QRD kernel class -- default, single kernel, no streaming
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS,
          bool TP_ROUT_EN,
          bool TP_STREAM_EN>

class qrd_hh : qrd_hh_kernel<TT_DATA,
                             TP_DIM_ROWS,
                             TP_DIM_COLS,
                             TP_NUM_FRAMES,
                             TP_CASC_LEN,
                             TP_KERNEL_POS,
                             TP_ROUT_EN,
                             TP_STREAM_EN> {
   private:
   public:
    // Constructor
    qrd_hh()
        : qrd_hh_kernel<TT_DATA,
                        TP_DIM_ROWS,
                        TP_DIM_COLS,
                        TP_NUM_FRAMES,
                        TP_CASC_LEN,
                        TP_KERNEL_POS,
                        TP_ROUT_EN,
                        TP_STREAM_EN>(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(qrd_hh::qrd_hh_main); }

    // Main function
    void qrd_hh_main(input_buffer<TT_DATA>& __restrict inWindowA,
                     output_buffer<TT_DATA>& __restrict outWindowQ,
                     output_buffer<TT_DATA>& __restrict outWindowR);
};

// QRD kernel class -- multi kernel operating, streaming input and output of kernels, no R output due to Rs being
// ourputted from the upper kernels
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS>

class qrd_hh<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, false, true>
    : qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, false, true> {
   private:
   public:
    // Constructor
    qrd_hh()
        : qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, false, true>(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(qrd_hh::qrd_hh_main); }

    // Main function
    void qrd_hh_main(input_buffer<TT_DATA>& __restrict inWindowA,
                     input_stream<TT_DATA>* __restrict inStream,
                     output_buffer<TT_DATA>& __restrict outWindowQ,
                     output_stream<TT_DATA>* __restrict outStream);
};

// QRD kernel class -- multi kernel operating, streaming input and output of kernels, R output is enabled
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POS>

class qrd_hh<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, true, true>
    : qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, true, true> {
   private:
   public:
    // Constructor
    qrd_hh()
        : qrd_hh_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POS, true, true>(){};

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(qrd_hh::qrd_hh_main); }

    // Main function
    void qrd_hh_main(input_buffer<TT_DATA>& __restrict inWindowA,
                     input_stream<TT_DATA>* __restrict inStream,
                     output_buffer<TT_DATA>& __restrict outWindowQ,
                     output_buffer<TT_DATA>& __restrict outWindowR,
                     output_stream<TT_DATA>* __restrict outStream);
};

} // namespace qrd_hh
} //  namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_QRD_HPP_
