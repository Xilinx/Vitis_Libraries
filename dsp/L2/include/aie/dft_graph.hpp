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
#ifndef _DSPLIB_DFT_GRAPH_HPP_
#define _DSPLIB_DFT_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the DFT function library element.
*/
/**
 * @file dft_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
#include "dft.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

/**
 * @defgroup dft_graph Discrete Fourier Transform (DFT)
 *
 * DFT
**/

/**
  * @cond NOCOMMENTS
  */

// Start of recursive kernel creation for cascaded dft kernels
// This is the specialization for kernels in the middle of the cascade.
template <int cascPos,
          typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class create_casc_kernel_recur {
   public:
    //             casc
    //        [0] - [1] - [2]       [cascPos + ssrPos*TP_CASC_LEN]
    //  SSR   [3] - [4] - [5]
    //        [6] - [7] - [8]
    static void create(kernel (&dftKernels)[TP_CASC_LEN * TP_SSR],
                       const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = cascPos - 1;
        for (int ssrPos = 0; ssrPos < TP_SSR; ssrPos++) {
            dftKernels[cascPos - 1 + ssrPos * TP_CASC_LEN] =
                kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                          TP_SSR, TP_NUM_FRAMES, TP_KERNEL_POSITION, true, true, TP_RND, TP_SAT> >(
                    coeffs[cascPos - 1 + ssrPos * TP_CASC_LEN]);
        }
        create_casc_kernel_recur<cascPos - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_SSR, TP_NUM_FRAMES, TP_RND, TP_SAT>::create(dftKernels, coeffs);
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
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_TWIDDLE,
                               TP_POINT_SIZE,
                               TP_FFT_NIFFT,
                               TP_SHIFT,
                               TP_CASC_LEN,
                               TP_SSR,
                               TP_NUM_FRAMES,
                               TP_RND,
                               TP_SAT> {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN * TP_SSR],
                       const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        for (int ssrPos = 0; ssrPos < TP_SSR; ssrPos++) {
            dftKernels[0 + ssrPos * TP_CASC_LEN] =
                kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                          TP_SSR, TP_NUM_FRAMES, TP_KERNEL_POSITION, false, true, TP_RND, TP_SAT> >(
                    coeffs[0 + ssrPos * TP_CASC_LEN]);
        }
    }
};

// dft Kernel creation, entry to recursion, also end of cascade.
template <int cascPos,
          typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class create_casc_kernel {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN * TP_SSR],
                       const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = cascPos - 1;
        for (int ssrPos = 0; ssrPos < TP_SSR; ssrPos++) {
            dftKernels[cascPos - 1 + ssrPos * TP_CASC_LEN] =
                kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                          TP_SSR, TP_NUM_FRAMES, TP_KERNEL_POSITION, true, false, TP_RND, TP_SAT> >(
                    coeffs[cascPos - 1 + ssrPos * TP_CASC_LEN]);
        }

        create_casc_kernel_recur<cascPos - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_SSR, TP_NUM_FRAMES, TP_RND, TP_SAT>::create(dftKernels, coeffs);
    }
};

// dft Kernel creation, Specialization for CASC_LEN=1
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class create_casc_kernel<1,
                         TT_DATA,
                         TT_TWIDDLE,
                         TP_POINT_SIZE,
                         TP_FFT_NIFFT,
                         TP_SHIFT,
                         TP_CASC_LEN,
                         TP_SSR,
                         TP_NUM_FRAMES,
                         TP_RND,
                         TP_SAT> {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN * TP_SSR],
                       const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        for (int ssrPos = 0; ssrPos < TP_SSR; ssrPos++) {
            dftKernels[ssrPos] =
                kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                          TP_SSR, TP_NUM_FRAMES, TP_KERNEL_POSITION, false, false, TP_RND, TP_SAT> >(
                    coeffs[ssrPos]);
        }
    }
};
/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// dft_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief dft performs the Discrete Fourier Transform on a set of data samples
 *
 * @ingroup dft_graph
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         cint16, cint32, cfloat.
 * @tparam TT_TWIDDLE describes the type of twiddle factors used in the transform. \n
 *         It must be one of the following: cint16, cfloat
 *         and must also satisfy the following rules:
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
 *         - TT_TWIDDLE must be an integer type if TT_DATA is an integer type
 *         - TT_TWIDDLE must be cfloat type if TT_DATA is a float type.
 * @tparam TP_POINT_SIZE describes the number of samples in the frame to be windowed.
 * @tparam TP_FFT_NIFFT selects whether the transform to perform is an forward (1) or reverse (0) transform.
 * @tparam TP_SHIFT selects the power of 2 to scale the result by prior to output.
 * @tparam TP_CASC_LEN selects the number of kernels the DFT will be split over in series
 *         to improve throughput
 * @tparam TP_NUM_FRAMES describes the number of frames of input data samples that occur
 *         within each input window of data.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although, TP_RND accepts unsigned integer values descriptive macros are recommended where
 *         - rnd_floor      = Truncate LSB, always round down (towards negative infinity).
 *         - rnd_ceil       = Always round up (towards positive infinity).
 *         - rnd_sym_floor  = Truncate LSB, always round towards 0.
 *         - rnd_sym_ceil   = Always round up towards infinity.
 *         - rnd_pos_inf    = Round halfway towards positive infinity.
 *         - rnd_neg_inf    = Round halfway towards negative infinity.
 *         - rnd_sym_inf    = Round halfway towards infinity (away from zero).
 *         - rnd_sym_zero   = Round halfway towards zero (away from infinity).
 *         - rnd_conv_even  = Round halfway towards nearest even number.
 *         - rnd_conv_odd   = Round halfway towards nearest odd number. \n
 *         No rounding is performed on ceil or floor mode variants. \n
 *         Other modes round to the nearest integer. They differ only in how
 *         they round for values of 0.5. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_SSR selects number cascade chains that will operate in parallel.
 *         The kernels in each SSR rank will receive the same input data as the kernels in all other SSR ranks. \n
 *         The internally calculated twiddles are split by across each SSR rank.
 *         There will be TP_SSR output ports that should be interleaved together to give the output of the DFT. \n
 **/

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_SSR>
class dft_graph : public graph {
   public:
    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     * There will be ``TP_SSR`` number of parallel cascade chains of length ``TP_CASC_LEN``.
     **/
    kernel m_dftKernels[TP_CASC_LEN * TP_SSR];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return m_dftKernels; };

    /**
     * The input data to each of the kernels in the function.
     * This input is a window of samples of ``TT_DATA`` type.
     * The number of samples in the window to each kernel is derived by ``(TP_POINT_SIZE / TP_CASC_LEN) *
     *TP_NUM_FRAMES``.
     * A data frame of size ``TP_POINT_SIZE`` may require zero-padding so that after it has been divided by TP_CASC_LEN,
     *so that it is aligned to the vector read size
     * (for AIE this would 4, 8, and 8 samples for cint16, cint32, and cfloat respectively, and for AIE-Ml this would be
     *8 samples for all data types).
     * Each kernel in a cascaded design will receive an equal share of the input data.
     * Further zero-padding of the input data to each kernel in cascade may be required to ensure this is possible.
     * More information about the required padding of the input data can be found in the documentation for the DFT.
     *
     **/
    port<input> in[TP_SSR * TP_CASC_LEN];

    /**
     * The output data of the function. For cascaded designs, this is located at the end of the cascaded kernel chain.
     * This input will be a complex ``TT_DATA`` type, and there will be ``TP_SSR`` output ports.
     * The number of samples in each SSR output window is derived by ``(TP_POINT_SIZE / TP_SSR) * TP_NUM_FRAMES``.
     **/
    port<output> out[TP_SSR];

    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

    double inv = (TP_FFT_NIFFT == 1) ? -1.0 : 1.0;

    /**
     * @brief This is the constructor function for the Discrete Fourier Transform graph.
     **/
    dft_graph() {
        static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value) ||
                          (std::is_same<TT_DATA, cfloat>::value),
                      "ERROR: TT_DATA is not supported");
#ifdef __SUPPORTS_ACC64__
        static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value),
                      "ERROR: TT_DATA is not supported");
#endif //__SUPPORTS_ACC64__

// Number of samples in output vector, and twiddle vector. Depends on type of output data, and twiddle
#ifdef __SUPPORTS_ACC64__
        constexpr int kSamplesInVectData = 8;
#else
        constexpr int kSamplesInVectData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__
        constexpr int kCoeffVectSize = kSamplesInVectData;
        constexpr int paddedDataSize = CEIL(TP_POINT_SIZE, kSamplesInVectData);

        constexpr int paddedCoeffSize = CEIL(TP_POINT_SIZE, (TP_SSR * kCoeffVectSize));

        constexpr int outWindowSize = paddedCoeffSize * TP_NUM_FRAMES / TP_SSR;

        constexpr int paddedFrameSize = CEIL(paddedDataSize, (kSamplesInVectData * TP_CASC_LEN));
        constexpr int paddedWindowSize = TP_NUM_FRAMES * paddedFrameSize;
        constexpr int cascWindowSize = paddedWindowSize / TP_CASC_LEN;
        constexpr int cascFrameSize = paddedFrameSize / TP_CASC_LEN;
        constexpr int kVecInCoeff = paddedCoeffSize / kCoeffVectSize;

        cfloat tmpCoeff[TP_POINT_SIZE][paddedCoeffSize];
        TT_TWIDDLE masterCoeff[TP_POINT_SIZE][paddedCoeffSize];
        std::vector<std::vector<TT_TWIDDLE> > cascCoeffs(TP_CASC_LEN * TP_SSR);

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
                    // printf("%d %d,  ", masterCoeff[n][k].real, masterCoeff[n][k].imag);
                }
            }
            // printf("\n");
        }

        int cascRank = 0;
        int ssrRank = 0;
        int ssrVsize = TP_SSR * kCoeffVectSize;
        int pairOrSingle;
        // int doubleLoad = ()
        for (int vectorPair = 0; vectorPair < paddedCoeffSize / ssrVsize; vectorPair += 2) {
            for (int n = 0; n < TP_POINT_SIZE; n++) {
                pairOrSingle = ((paddedCoeffSize / ssrVsize) - vectorPair > 1) + 1;
                for (int i = 0; i < pairOrSingle; i++) {
                    for (int k = (vectorPair + i) * (ssrVsize); k < (vectorPair + i + 1) * (ssrVsize); k++) {
                        cascRank = n % TP_CASC_LEN;
                        ssrRank = k % TP_SSR;
                        // printf("n %d k %d -> cascRank %d ssrRank %d on Kernel %d\n", n, k, cascRank, ssrRank,
                        // cascRank+(TP_CASC_LEN*ssrRank));
                        cascCoeffs[cascRank + (TP_CASC_LEN * ssrRank)].push_back(masterCoeff[n][k]);
                    }
                }
            }
        }

        // Create kernel classes
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN, TP_SSR,
                           TP_NUM_FRAMES, TP_RND, TP_SAT>::create(m_dftKernels, cascCoeffs);

        for (int ssr = 0; ssr < TP_SSR; ssr++) { // create object for each kernel in cascade
            for (int cascNum = 0; cascNum < TP_CASC_LEN; cascNum++) {
                int kernelIdx = (ssr * TP_CASC_LEN) + cascNum;
                // connect cascaded kernels
                if (cascNum >= 1 && TP_CASC_LEN > 1) {
                    connect<cascade>(m_dftKernels[kernelIdx - 1].out[0], m_dftKernels[kernelIdx].in[1]);
                }
                // // connect input data to each kernel
                connect(in[kernelIdx], m_dftKernels[kernelIdx].in[0]);
                dimensions(m_dftKernels[kernelIdx].in[0]) = {cascWindowSize};

                // Specify mapping constraints
                runtime<ratio>(m_dftKernels[kernelIdx]) = 0.8;
                // Source files
                source(m_dftKernels[kernelIdx]) = "dft.cpp";
                headers(m_dftKernels[kernelIdx]) = {"dft.hpp"};
            }
            // connect final kernel output to output of the graph
            connect(m_dftKernels[((ssr * TP_CASC_LEN) + TP_CASC_LEN - 1)].out[0], out[ssr]);
            dimensions(m_dftKernels[((ssr * TP_CASC_LEN) + TP_CASC_LEN - 1)].out[0]) = {outWindowSize};
        }
    };
};

} // namespace dft
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_DFT_GRAPH_HPP_
