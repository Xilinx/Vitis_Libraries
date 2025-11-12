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
#ifndef _SOLVERLIB_QRD_HPP_
#define _SOLVERLIB_QRD_HPP_

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
#include "qrd_traits.hpp"

using namespace adf;

//#define _DSPLIB_QRD_HPP_DEBUG_

// #include "qrd_traits.hpp" //for fnPointSizePwr

namespace xf {
namespace solver {
namespace aie {
namespace qrd {

// QRD kernel class -- default
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL,
          bool TP_CASC_IN,
          bool TP_CASC_OUT
          >
          
class qrd_kernel {

   private:

   public:

      static constexpr unsigned int vecSampleNum = kMaxReadInBytes / sizeof(TT_DATA); 
      static constexpr unsigned int m_kRowChunkNum = TP_DIM_ROWS/vecSampleNum;
      static constexpr unsigned int m_kColChunkNum = TP_DIM_COLS_TOTAL/vecSampleNum;

      static constexpr unsigned int kSamplesRow_padded = CEIL(TP_DIM_ROWS, vecSampleNum) / vecSampleNum;
      static constexpr unsigned int kSamplesCol_padded = CEIL(TP_DIM_COLS_TOTAL, vecSampleNum) / vecSampleNum;

      alignas(__ALIGN_BYTE_SIZE__) TT_DATA QrdCascData [TP_DIM_ROWS] = {};

      // Constructor
      qrd_kernel(){};

      // Modified Gram-Schmidt function
      void qrd_mgs(T_inputIF<TP_CASC_IN, TT_DATA>& inInterface,
                   T_outputIF<TP_CASC_OUT, TT_DATA>& outInterface,
                  int& frame_id);
      
      // Modified Gram-Schmidt function for the first kernel in the cascade
      void qrd_mgs_first_kernel(T_inputIF<TP_CASC_IN, TT_DATA>& inInterface,
                        T_outputIF<TP_CASC_OUT, TT_DATA>& outInterface);

      // Modified Gram-Schmidt function for cascaded kernels
      void qrd_mgs_casc(T_inputIF<TP_CASC_IN, TT_DATA>& inInterface,
                        T_outputIF<TP_CASC_OUT, TT_DATA>& outInterface);
};


// QRD kernel class -- default
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL,
          bool TP_CASC_IN,
          bool TP_CASC_OUT
          >
          
class qrd : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, TP_CASC_IN, TP_CASC_OUT> {


   private:

   public:

      // Constructor
      qrd() : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, TP_CASC_IN, TP_CASC_OUT>(){};


      // Register Kernel Class

      static void registerKernelClass() { REGISTER_FUNCTION(qrd::qrd_main); }

      // Main function
      void qrd_main(input_buffer<TT_DATA>& __restrict inWindowA,
                    output_buffer<TT_DATA>& __restrict outWindowQ,
                    output_buffer<TT_DATA>& __restrict outWindowR);
      
};


template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL
          >
class qrd <TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_TRUE, CASC_OUT_TRUE> : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_TRUE, CASC_OUT_TRUE>{


   private:

   public:

      // Constructor
      qrd() : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_TRUE, CASC_OUT_TRUE> (){};


      // Register Kernel Class

      static void registerKernelClass() { REGISTER_FUNCTION(qrd::qrd_main); }

      // Main function
      void qrd_main(input_buffer<TT_DATA>& __restrict inWindowA,
                    input_cascade<TT_DATA>* __restrict inCascade,
                    output_buffer<TT_DATA>& __restrict outWindowQ,
                    output_buffer<TT_DATA>& __restrict outWindowR,
                    output_cascade<TT_DATA>* __restrict outCascade
                                    );   
};


template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL
          >
class qrd <TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_TRUE, CASC_OUT_FALSE> : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_TRUE, CASC_OUT_FALSE>{


   private:

   public:

      // Constructor
      qrd() : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_TRUE, CASC_OUT_FALSE> (){};


      // Register Kernel Class

      static void registerKernelClass() { REGISTER_FUNCTION(qrd::qrd_main); }

      // Main function
      void qrd_main(input_buffer<TT_DATA>& __restrict inWindowA,
                    input_cascade<TT_DATA>* __restrict inCascade,
                    output_buffer<TT_DATA>& __restrict outWindowQ,
                    output_buffer<TT_DATA>& __restrict outWindowR
                                    );  
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_COLS_DIST,
          unsigned int TP_DIM_COLS_TOTAL
          >
          
class qrd <TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_FALSE, CASC_OUT_TRUE> : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_FALSE, CASC_OUT_TRUE>{


   private:

   public:

      // Constructor
      qrd() : qrd_kernel<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, TP_DIM_COLS_DIST, TP_DIM_COLS_TOTAL, CASC_IN_FALSE, CASC_OUT_TRUE> (){};


      // Register Kernel Class

      static void registerKernelClass() { REGISTER_FUNCTION(qrd::qrd_main); }

      // Main function
      void qrd_main(input_buffer<TT_DATA>& __restrict inWindowA,
                    output_buffer<TT_DATA>& __restrict outWindowQ,
                    output_buffer<TT_DATA>& __restrict outWindowR,
                    output_cascade<TT_DATA>* __restrict outCascade             
                  );   
};



}
}
}
}

#endif // _SOLVERLIB_QRD_HPP_
