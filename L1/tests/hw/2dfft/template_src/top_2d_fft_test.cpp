#include "top_2d_fft_test.hpp"

void top_fft2d(MemWideIFStreamTypeIn& p_inStream, MemWideIFStreamTypeOut& p_outStream) {
#pragma HLS INLINE
#pragma HLS DATA_PACK variable = p_inStream
#pragma HLS DATA_PACK variable = p_outStream
#pragma HLS interface ap_ctrl_none port = return
    FFT2d<k_memWidth, k_fftKernelSize, k_fftKernelSize, k_numOfKernels, FFTParams, FFTParams2, k_rowInstanceIDOffset,
          k_colInstanceIDOffset, T_elemType>
        l_fft2d_obj;
    l_fft2d_obj.fft2dProc(p_inStream, p_outStream);
}
