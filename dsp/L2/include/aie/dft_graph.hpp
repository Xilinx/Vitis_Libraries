/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_DFT_GRAPH_HPP_
#define _DSPLIB_DFT_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
#include "dft.hpp"
#include "dft_fns.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

// ---------- start of recursive kernel creation code
// Recursive kernel creation for cascaded dft kernels
// This is the specialization for kernels in the middle of the cascade.
template <int kPos,
          typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        dftKernels[kPos - 1] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, true, true> >(coeffs[kPos - 1]);

        // create_casc_kernel_recur<dim - 1, TT_DATA, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
        // TP_SHIFT, TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL, TP_DYN_PT_SIZE, TP_IN_API,
        // TP_ORIG_PAR_POWER>::create(fftKernels);
        create_casc_kernel_recur<kPos - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_NUM_FRAMES>::create(dftKernels, coeffs);
    }
};

// Recursive fft kernel creation
// This is the specialization for the end of recursion (first kernel in cascade)
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_TWIDDLE,
                               TP_POINT_SIZE,
                               TP_FFT_NIFFT,
                               TP_SHIFT,
                               TP_CASC_LEN,
                               TP_NUM_FRAMES> {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        dftKernels[0] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, false, true> >(coeffs[0]);
    }
};

// dft Kernel creation, entry to recursion, also end of cascade.
template <int kPos,
          typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES>
class create_casc_kernel {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        dftKernels[kPos - 1] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, true, false> >(coeffs[kPos - 1]);

        // create_casc_kernel_recur<dim - 1, TT_DATA, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
        // TP_SHIFT, TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL, TP_DYN_PT_SIZE, TP_IN_API,
        // TP_ORIG_PAR_POWER>::create(fftKernels);
        create_casc_kernel_recur<kPos - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_NUM_FRAMES>::create(dftKernels, coeffs);
    }
};

// dft Kernel creation, Specialization for CASC_LEN=1
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES>
class create_casc_kernel<1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN, TP_NUM_FRAMES> {
   public:
    static void create(kernel (&dftKernels)[1], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        dftKernels[0] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, false, false> >(coeffs[0]);
    }
};

/**
 * @cond NOCOMMENTS
 */
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES>
class dft_graph : public graph {
   public:
    // declare DFT Kernel array
    kernel m_dftKernels[TP_CASC_LEN];
    kernel* getKernels() { return m_dftKernels; };

    port<input> in[TP_CASC_LEN];
    port<output> out[1];

    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

    double inv = (TP_FFT_NIFFT == 1) ? -1.0 : 1.0;

    // Constructor
    dft_graph() {
        // Number of samples in output vector, and twiddle vector. Depends on type of output data, and twiddle
        constexpr int kSamplesInVectOutData = 256 / 8 / sizeof(T_outDataType);
        // constexpr int kSamplesInVectTwiddle =  256 / 8 / sizeof(TT_TWIDDLE);
        constexpr int kSamplesInVectTwiddle = kSamplesInVectOutData;
        constexpr int paddedDataSize = CEIL(TP_POINT_SIZE, kSamplesInVectOutData);
        constexpr int paddedCoeffSize = CEIL(TP_POINT_SIZE, kSamplesInVectTwiddle);
        // int k
        constexpr int outWindowSize = paddedDataSize * TP_NUM_FRAMES;
        constexpr int paddedFrameSize = CEIL(paddedDataSize, (kSamplesInVectOutData * TP_CASC_LEN));
        constexpr int paddedWindowSize = TP_NUM_FRAMES * paddedFrameSize;
        constexpr int cascWindowSize = paddedWindowSize / TP_CASC_LEN;
        constexpr int cascFrameSize = paddedFrameSize / TP_CASC_LEN;

        printf("kSamplesInVectOutData = %d\n", kSamplesInVectOutData);
        printf("kSamplesInVectTwiddle = %d\n", kSamplesInVectTwiddle);
        printf("paddedDataSize = %d\n", paddedDataSize);
        printf("paddedCoeffSize = %d\n", paddedCoeffSize);
        printf("paddedFrameSize = %d\n", paddedFrameSize);
        printf("paddedWindowSize = %d\n", paddedWindowSize);
        printf("cascWindowSize = %d\n", cascWindowSize);
        printf("cascFrameSize = %d\n", cascFrameSize);

        cfloat tmpCoeff[TP_POINT_SIZE][paddedCoeffSize];
        TT_TWIDDLE masterCoeff[TP_POINT_SIZE][paddedCoeffSize];
        std::vector<std::vector<TT_TWIDDLE> > cascCoeffs(TP_CASC_LEN);

        // create master table of dft coefficients
        // n is matrix row, k is matrix column
        for (int n = 0; n < TP_POINT_SIZE; n++) {
            for (int k = 0; k < paddedCoeffSize; k++) {
                tmpCoeff[n][k].real = (cos(M_PI * 2.0 * (double)n * (double)k / (double)TP_POINT_SIZE));
                tmpCoeff[n][k].imag = (inv * sin(M_PI * 2.0 * (double)n * (double)k / (double)TP_POINT_SIZE));

                // Padding the twiddles on the row
                if (k >= TP_POINT_SIZE) {
                    tmpCoeff[n][k] = {0.0, 0.0};
                }

                if (std::is_same<TT_TWIDDLE, cfloat>::value) {
                    masterCoeff[n][k].real = tmpCoeff[n][k].real;
                    masterCoeff[n][k].imag = tmpCoeff[n][k].imag;
                } else {
                    tmpCoeff[n][k].real = (tmpCoeff[n][k].real * 32768.0);
                    tmpCoeff[n][k].imag = (tmpCoeff[n][k].imag * 32768.0);
                    if (tmpCoeff[n][k].real > 32767.0) {
                        tmpCoeff[n][k].real = 32767.0;
                    }
                    if (tmpCoeff[n][k].imag > 32767.0) {
                        tmpCoeff[n][k].imag = 32767.0;
                    }
                    masterCoeff[n][k].real = (int16)tmpCoeff[n][k].real;
                    masterCoeff[n][k].imag = (int16)tmpCoeff[n][k].imag;
                }
            }
        }

        // Deal coefficients across kernels
        for (int vectorSlice = 0; vectorSlice < paddedCoeffSize / kSamplesInVectTwiddle; vectorSlice++) {
            for (int row = 0; row < TP_POINT_SIZE; row++) {
                for (int col = (vectorSlice * kSamplesInVectTwiddle); col < (vectorSlice + 1) * kSamplesInVectTwiddle;
                     col++) {
                    cascCoeffs[row % TP_CASC_LEN].push_back(masterCoeff[row][col]);
                    // printf("Kernel %d: %d rows %d cols\n", row % TP_CASC_LEN, row, col);
                }
            }
        }

        // Create kernel classes
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                           TP_NUM_FRAMES>::create(m_dftKernels, cascCoeffs);

        // create object for each kernel in cascade
        for (int cascNum = 0; cascNum < TP_CASC_LEN; cascNum++) {
            // connect cascaded kernels
            if (cascNum >= 1 && TP_CASC_LEN > 1) {
                connect<cascade>(m_dftKernels[cascNum - 1].out[0], m_dftKernels[cascNum].in[1]);
            }
            // // connect input data to each kernel
            connect(in[cascNum], m_dftKernels[cascNum].in[0]);
            dimensions(m_dftKernels[cascNum].in[0]) = {cascWindowSize};

            // Specify mapping constraints
            runtime<ratio>(m_dftKernels[cascNum]) = 0.8;
            // Source files
            source(m_dftKernels[cascNum]) = "dft.cpp";
            headers(m_dftKernels[cascNum]) = {"dft.hpp"};
        }

        // connect final kernel output to output of the graph
        connect(m_dftKernels[(TP_CASC_LEN - 1)].out[0], out[0]);
        dimensions(m_dftKernels[(TP_CASC_LEN - 1)].out[0]) = {outWindowSize};
    };
};

} // namespace dft
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_DFT_GRAPH_HPP_
