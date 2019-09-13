#define TEST_2D_FFT_
#ifdef TEST_2D_FFT_

#include <math.h>
#include <string>
#include <assert.h>
#include <stdio.h>
#include "top_2d_fft_test.hpp"
#include "mVerificationUtlityFunctions.hpp"
#include "xf_fft/hls_ssr_fft_2d_modeling_utilities.hpp"
#include "xf_fft.hpp"

int main(int argc, char** argv) {
    // 2d input matrix
    T_elemType l_inMat[k_fftKernelSize][k_fftKernelSize];
    T_elemType l_outMat[k_fftKernelSize][k_fftKernelSize];
    T_elemType l_data2d_golden[k_fftKernelSize][k_fftKernelSize];

    // init input matrix with imaginary part only  impulse
    for (int r = 0; r < k_fftKernelSize; ++r) {
        for (int c = 0; c < k_fftKernelSize; ++c) {
            if (r == 0 && c == 0)
                l_inMat[r][c] = T_compleFloat(0, 1);
            else
                l_inMat[r][c] = T_compleFloat(0, 0);
        }
    }
    // Wide Stream for reading and streaming a 2-d matrix
    MemWideIFStreamTypeIn l_matToStream("matrixToStreaming");
    MemWideIFStreamTypeOut fftOutputStream("fftOutputStream");
    // Pass same data stream multiple times to measure the II correctly
    for (int runs = 0; runs < 5; ++runs) {
        stream2DMatrix<k_fftKernelSize, k_fftKernelSize, k_memWidth, T_elemType>(l_inMat, l_matToStream);
        top_fft2d(l_matToStream, fftOutputStream);

        printMatStream<k_fftKernelSize, k_fftKernelSize, k_memWidth>(fftOutputStream, "2D FFT Output Natural Order...");
        streamToMatrix<k_fftKernelSize, k_fftKernelSize, k_memWidth, T_elemType>(fftOutputStream, l_outMat);
    } // runs loop

    T_elemType golden_result = T_elemType(0, 1);
    for (int r = 0; r < k_fftKernelSize; ++r) {
        for (int c = 0; c < k_fftKernelSize; ++c) {
            if (golden_result != l_outMat[r][c]) return 1;
        }
    }

    std::cout << "================================================================" << std::endl;
    std::cout << "---------------------Impulse test Passed Successfully." << std::endl;
    std::cout << "================================================================" << std::endl;
    return 0;
}
#endif
