#include "device_defs.h"
#include "castKernel.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "kernel_api_utils.hpp"
#include <cstring>

#ifndef NOINLINE_DECL
#define NOINLINE_DECL __attribute__((noinline))
#endif
//#define __CASTKERNEL_DEBUG__
namespace xf::solver::aie::kernel_cast {

using namespace adf;

template <unsigned int TP_DIM_SIZE, unsigned int TP_NUM_FRAMES>
NOINLINE_DECL void castKernel<TP_DIM_SIZE, TP_NUM_FRAMES>::cast_main(input_buffer<cfloat>& __restrict in,
                                                                     output_buffer<float>& __restrict out) {
    typedef float TT_DATA;

    constexpr unsigned int kNumCfloats = TP_DIM_SIZE * TP_DIM_SIZE * TP_NUM_FRAMES;
    constexpr unsigned int m_kvecSampleNum = __ALIGN_BYTE_SIZE__ / sizeof(TT_DATA);
    constexpr unsigned int kNumMemTrans = kNumCfloats / m_kvecSampleNum;

    using vect_t = ::aie::vector<TT_DATA, m_kvecSampleNum>;
    using halfvect_t = ::aie::vector<TT_DATA, m_kvecSampleNum / 2>;
    TT_DATA* inSampPtr = (TT_DATA*)in.data();
    TT_DATA* outSampPtr = (TT_DATA*)out.data();

    vect_t* inPtr = (vect_t*)in.data();
    halfvect_t* outPtr = (halfvect_t*)out.data();
    vect_t inVec;
    halfvect_t outRealVec, outImagVec;

    for (int row = 0; row < TP_DIM_SIZE; row++) chess_prepare_for_pipelining chess_loop_range(TP_DIM_SIZE, ) {
            for (int colvect = 0; colvect < TP_DIM_SIZE * 2 / m_kvecSampleNum;
                 colvect++) //*2 because input is interpreted as float, not cfloat
                chess_prepare_for_pipelining chess_loop_range(TP_DIM_SIZE * 2 / m_kvecSampleNum, ) {
                    inVec = *inPtr++;
                    outRealVec = ::aie::filter_even<vect_t>(inVec);
                    outImagVec = ::aie::filter_odd<vect_t>(inVec);
                    outPtr[colvect + row * 2 * TP_DIM_SIZE * 2 / m_kvecSampleNum] = outRealVec;
                    outPtr[colvect + (row * 2 + 1) * TP_DIM_SIZE * 2 / m_kvecSampleNum] = outImagVec;
                }
        }
    /*
        //this code simply performs a vectorized copy of the input to the output - functionally a memcpy.
        for (int i = 0; i < kNumMemTrans; i++)
          chess_prepare_for_pipelining chess_loop_range(kNumMemTrans,) {
          *outPtr++ = *inPtr++;
        }
    */
}

} // namespace xf::solver::aie::kernel_cast
