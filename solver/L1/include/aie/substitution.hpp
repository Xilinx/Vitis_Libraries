#ifndef _SOLVERLIB_SUBSTITUTION_HPP_
#define _SOLVERLIB_SUBSTITUTION_HPP_

#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "device_defs.h"
#include "substitution_traits.hpp"

using namespace adf;

//#define __SUBST_DEBUG__

#ifndef NOINLINE_DECL
#define NOINLINE_DECL __attribute__((noinline))
#endif

namespace xf::solver::aie::substitution {


// enum class subst_type { FWD, BWD };

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV>
class substitution_base {
private:
    static constexpr unsigned int m_kvecSampleNum = kMaxReadInBytes / sizeof(TT_DATA);

public:
  alignas(__ALIGN_BYTE_SIZE__) tFbsubBaseType<TT_DATA> m_invX[TP_DIM_SIZE];
  static constexpr int m_kSwapSize = (__MAX_BD_DSIZE_TPOSE__ == 4) && //i.e. AIE1
    (std::is_same<TT_DATA,cfloat>::value) &&                          // and cfloat
    ((TP_L_LEADING == 0 && TP_SUBST_TYPE == 0) || (TP_L_LEADING == 1 && TP_SUBST_TYPE == 1))                                               // and transpose required
    ? TP_DIM_SIZE*2 : 0;
  alignas(__ALIGN_BYTE_SIZE__) TT_DATA inSwap[m_kSwapSize]; //scratch pad for fixing transpose on AIE1 cfloat

  //In a multi kernel configuration, X values calculated in a diagonal kernel are passed down a column of kernels
  //but they may be needed for more than one lane of processing, so they are read from stream for the first lane (vector of rows) so that
  //they can be retrieved for later lanes, rather than have the diagonal kernel transmit them several times which could lead to stall delays.
  alignas(__ALIGN_BYTE_SIZE__) TT_DATA xStoreSamp[TP_GRID_DIM>1? TP_DIM_SIZE: 0]; //This stores the X values from a kernel at the top of a column. Not required for single kernel operation.
  void substitution_main(
                         fbsub_input_params<TT_DATA>& input_params,
                         fbsub_output_params<TT_DATA>& output_params);
};


template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM, //Kernel should not need to know this.
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV,
          typename TT_IO_DATA>
class substitution : public substitution_base<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV> {
   private:
     static constexpr unsigned int m_kvecSampleNum = kMaxReadInBytes / sizeof(TT_DATA);

   public:
     // Constructor
     substitution() = default;

    // Main kernel function
     void substitution_single(input_buffer<TT_IO_DATA>& __restrict in_L,
                       input_buffer<TT_DATA>& __restrict in_y,
                       output_buffer<TT_DATA>& __restrict out_x);
     void substitution_top(input_buffer<TT_IO_DATA>& __restrict in_L,
                    input_buffer<TT_DATA>& __restrict in_y,
                    output_buffer<TT_DATA>& __restrict out_x,
                    output_stream<TT_DATA>* __restrict out_x_strm);
     void substitution_first(input_buffer<TT_IO_DATA>& __restrict in_L,
                      input_buffer<TT_DATA>& __restrict in_y,
                      input_stream<TT_DATA>* __restrict in_x,
                      output_cascade<accTypeFbsub<TT_DATA>>* __restrict out_pp);
     void substitution_mid(input_buffer<TT_IO_DATA>& __restrict in_L,
                    input_buffer<TT_DATA>& __restrict in_y,
                    input_stream<TT_DATA>* __restrict in_x,
                    input_cascade<accTypeFbsub<TT_DATA>>* __restrict in_pp,
                    output_cascade<accTypeFbsub<TT_DATA>>* __restrict out_pp);
     void substitution_last(input_buffer<TT_IO_DATA>& __restrict in_L,
                     input_buffer<TT_DATA>& __restrict in_y,
                     input_cascade<accTypeFbsub<TT_DATA>>* __restrict in_pp,
                     output_buffer<TT_DATA>& __restrict out_x,
                     output_stream<TT_DATA>* __restrict out_x_strm);
     void substitution_last_bottom(input_buffer<TT_IO_DATA>& __restrict in_L,
                                   input_buffer<TT_DATA>& __restrict in_y,
                                   input_cascade<accTypeFbsub<TT_DATA>>* __restrict in_pp,
                                   output_buffer<TT_DATA>& __restrict out_x);
     // Register Kernel Class
     static void registerKernelClass() {
       if (TP_GRID_DIM == 1 ) {
         REGISTER_FUNCTION(substitution::substitution_single);
       } else if ( TP_X == 0 && TP_Y == 0) {
         REGISTER_FUNCTION(substitution::substitution_top);
       } else if (TP_X == 0) {
         REGISTER_FUNCTION(substitution::substitution_first);
       } else if ((TP_X == TP_Y) && (TP_X == TP_GRID_DIM-1)) {
         REGISTER_FUNCTION(substitution::substitution_last_bottom);
       } else if (TP_X == TP_Y) {
         REGISTER_FUNCTION(substitution::substitution_last);
       } else {
         REGISTER_FUNCTION(substitution::substitution_mid);
       }
     }

};

} // namespace xf::solver::aie::substitution




#endif // _SOLVERLIB_SUBSTITUTION_HPP_
