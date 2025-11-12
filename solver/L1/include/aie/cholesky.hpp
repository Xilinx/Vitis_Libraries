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
#ifndef _DSPLIB_CHOLESKY_HPP_
#define _DSPLIB_CHOLESKY_HPP_

/*
Cholesky Kernel.
This file exists to capture the definition of the Cholesky kernel class.
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
#include "device_defs.h"
#include "cholesky_traits.hpp"

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {

//-----------------------------------------------------------------------------------------------------
// Cholesky kernel base class - windows (io buffer) interface
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
class cholesky {
   private:
   public:
      static constexpr unsigned int kVecSampleNum = fnVecSampleNum<TT_DATA>();
      static constexpr unsigned int kNumVecsPerDim = TP_DIM / kVecSampleNum;
      static constexpr unsigned int kNumPriorKernels = TP_X;
      TT_DATA (&diagColBuffer)[TP_DIM];    // holds diagonal col data from other tiles
      TT_DATA (&diagRowBuffer)[TP_DIM];    // holds diagonal row data from other tiles

#if (__STREAMS_PER_TILE__ == 1)
      using inputPortLeft_t = input_cascade<TT_DATA>;
      using inputPortUp_t = input_stream<TT_DATA>;
      using outputPortRight_t = output_cascade<TT_DATA>;
      using outputPortDown_t = output_stream<TT_DATA>;
#elif (__STREAMS_PER_TILE__ == 2)
      using inputPortLeft_t = input_stream<TT_DATA>;
      using inputPortUp_t = input_stream<TT_DATA>;
      using outputPortRight_t = output_stream<TT_DATA>;
      using outputPortDown_t = output_stream<TT_DATA>;
#endif   // No "else" clause since we want to explicitly only support AIE and AIE-ML

      struct T_communicationIF {
         inputPortLeft_t* inLeft;
         inputPortUp_t* inUp;
         outputPortRight_t* outRight;
         outputPortDown_t* outDown;
      };

      // Constructor
      cholesky(
         TT_DATA (&m_diagColBuffer)[TP_DIM], 
         TT_DATA (&m_diagRowBuffer)[TP_DIM]):   diagColBuffer(m_diagColBuffer),
                                                diagRowBuffer(m_diagRowBuffer){}

      // Register Kernel Class
      static void registerKernelClass() {
         if (TP_GRID_DIM == 1) { // if there is no paralellism
            REGISTER_FUNCTION(cholesky::cholesky_main);
         }

         else if (TP_X == TP_Y) {
            if (TP_X == 0) {
               REGISTER_FUNCTION(cholesky::cholesky_diagKernel_topLeft);
            }
            else if (TP_X < TP_GRID_DIM-1) {
               REGISTER_FUNCTION(cholesky::cholesky_diagKernel_middle);
            }
            else if (TP_X == TP_GRID_DIM-1) {
               REGISTER_FUNCTION(cholesky::cholesky_diagKernel_botRight);
            }
         }
         else if (TP_X < TP_Y) {
            if ((TP_Y < TP_GRID_DIM-1) && (TP_X == 0)) {
               REGISTER_FUNCTION(cholesky::cholesky_lowerKernel_leftEdge);
            }
            else if ((TP_Y == TP_GRID_DIM-1) && (TP_X == 0)) {
               REGISTER_FUNCTION(cholesky::cholesky_lowerKernel_botLeft);
            }
            else if ((TP_Y == TP_GRID_DIM-1) && (TP_X > 0)) {
               REGISTER_FUNCTION(cholesky::cholesky_lowerKernel_botEdge);
            }
            else {
               REGISTER_FUNCTION(cholesky::cholesky_lowerKernel_nonEdge);
            }
         }
         REGISTER_PARAMETER(diagColBuffer);
         REGISTER_PARAMETER(diagRowBuffer);

      }

      // Main (Single Tile) Function
      void cholesky_main(input_buffer<TT_DATA>& __restrict inWindow,
                           output_buffer<TT_DATA>& __restrict outWindow);

      // ************** Function Body Kernels **************
      void cholesky_diagKernel(input_buffer<TT_DATA>& __restrict inWindow,
                              output_buffer<TT_DATA>& __restrict outWindow,
                              T_communicationIF ports);

      void cholesky_lowerKernel(input_buffer<TT_DATA>& __restrict inWindow,
                              output_buffer<TT_DATA>& __restrict outWindow,
                              T_communicationIF ports);

                           

      // ************** Diagonal Entry Functions **************
      // Top Left Kernel
      void cholesky_diagKernel_topLeft(input_buffer<TT_DATA>& __restrict inWindow,
                                    output_buffer<TT_DATA>& __restrict outWindow,
                                    outputPortDown_t* outDown);

      // Kernels along diagonal
      void cholesky_diagKernel_middle(input_buffer<TT_DATA>& __restrict inWindow,
                                    output_buffer<TT_DATA>& __restrict outWindow,
                                    inputPortLeft_t* inLeft,
                                    outputPortDown_t* outDown);

      // Bottom Right Kernel
      void cholesky_diagKernel_botRight(input_buffer<TT_DATA>& __restrict inWindow,
                                    output_buffer<TT_DATA>& __restrict outWindow,
                                    inputPortLeft_t* inLeft);

      // ************** Lower Entry Functions *****************
      // Kernels along Left Edge
      void cholesky_lowerKernel_leftEdge(input_buffer<TT_DATA>& __restrict inWindow,
                                    output_buffer<TT_DATA>& __restrict outWindow,
                                    inputPortUp_t* inUp,
                                    outputPortRight_t* outRight,
                                    outputPortDown_t* outDown);

      // Bottom Left Kernel
      void cholesky_lowerKernel_botLeft(input_buffer<TT_DATA>& __restrict inWindow,
                                    output_buffer<TT_DATA>& __restrict outWindow,
                                    inputPortUp_t* inUp,
                                    outputPortRight_t* outRight);


      // Kernels along Bottom Edge
      void cholesky_lowerKernel_botEdge(input_buffer<TT_DATA>& __restrict inWindow,
                                    output_buffer<TT_DATA>& __restrict outWindow,
                                    inputPortLeft_t* inLeft,
                                    inputPortUp_t* inUp,
                                    outputPortRight_t* outRight);

      // Non-Edge Lower Kernels
      void cholesky_lowerKernel_nonEdge(input_buffer<TT_DATA>& __restrict inWindow,
                                    output_buffer<TT_DATA>& __restrict outWindow,
                                    inputPortLeft_t* inLeft,
                                    inputPortUp_t* inUp,
                                    outputPortRight_t* outRight,
                                    outputPortDown_t* outDown);

};

}
}
}
}

#endif // _DSPLIB_CHOLESKY_HPP_
