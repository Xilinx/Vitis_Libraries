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
template <int kPos,
          typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        dftKernels[kPos - 1] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, true, true, TP_RND, TP_SAT> >(
                coeffs[kPos - 1]);

        create_casc_kernel_recur<kPos - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_NUM_FRAMES, TP_RND, TP_SAT>::create(dftKernels, coeffs);
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
                               TP_NUM_FRAMES,
                               TP_RND,
                               TP_SAT> {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        dftKernels[0] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, false, true, TP_RND, TP_SAT> >(coeffs[0]);
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
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class create_casc_kernel {
   public:
    static void create(kernel (&dftKernels)[TP_CASC_LEN], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        dftKernels[kPos - 1] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, true, false, TP_RND, TP_SAT> >(
                coeffs[kPos - 1]);

        create_casc_kernel_recur<kPos - 1, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                 TP_NUM_FRAMES, TP_RND, TP_SAT>::create(dftKernels, coeffs);
    }
};

// dft Kernel creation, Specialization for CASC_LEN=1
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
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
                         TP_NUM_FRAMES,
                         TP_RND,
                         TP_SAT> {
   public:
    static void create(kernel (&dftKernels)[1], const std::vector<std::vector<TT_TWIDDLE> >& coeffs) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        dftKernels[0] =
            kernel::create_object<dft<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                                      TP_NUM_FRAMES, TP_KERNEL_POSITION, false, false, TP_RND, TP_SAT> >(coeffs[0]);
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
 * @tparam TT_DATA describes the type of individual data samples input to the function.
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
 *          within each input window of data.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. Although, TP_RND accepts unsigned integer values
 *         descriptive macros are recommended where
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
 * @tparam TP_SAT describes the selection of saturation to be applied during the
 *         shift down stage of processing. TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value in the range [- ( 2^(n-1) ) : +2^(n-1)
 *- 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds an n-bit signed value in the
 *range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 **/

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dft_graph : public graph {
   public:
    /**
     * The chain of kernels that will be created and mapped onto AIE tiles.
     * Number of kernels (``TP_CASC_LEN``) will be connected with each other in series via a cascade interface.
     **/
    kernel m_dftKernels[TP_CASC_LEN];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return m_dftKernels; };

    /**
     * The input data to each of the kernels in the function.
     * This input is a window of samples of TT_DATA type.
     * The number of samples in the window is
     * derived from the POINT_SIZE and NUM_FRAMES parameters. A data frame of size (``TP_POINT_SIZE``) may require
     * zero-padding so that the length is a multiple of the size of the vector being used to read this data.
     * Each kernel in a cascaded design will receive an equal share of the input data.
     * Further zero-padding of the input data to each kernel in cascade may be required to ensure this is possible.
     * More information about the required padding of the input data can be found in the documentation for the DFT.
     *
     **/
    port<input> in[TP_CASC_LEN];

    /**
     * The output data of the function. For cascaded designs, this is located at the end of the cascaded kernel chain.
     * This input will be a complex TT_DATA type.
     * The number of samples in the output window is derived from the POINT_SIZE and NUM_FRAMES parameters.
     * A data frame of size (``TP_POINT_SIZE``) may require zero-padding so that the length is a multiple of the size of
     *the vector being used to read this data.
     * Any additional padding added that was required for the input of a cascaded design will not be included in
     * the size of the output data window.
     *
     **/
    port<output> out[1];

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
#if __SUPPORTS_CFLOAT__ == 1
        static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value) ||
                          (std::is_same<TT_DATA, cfloat>::value),
                      "ERROR: TT_DATA is not supported");
#else
        // AIE variants that don't support cfloat should flag that.
        static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value),
                      "ERROR: TT_DATA is not supported");
#endif //__SUPPORTS_CFLOAT__ == 0

// Number of samples in output vector, and twiddle vector. Depends on type of output data, and twiddle
#ifdef __SUPPORTS_ACC64__
        constexpr int kSamplesInVectOutData = 8;
#else
        constexpr int kSamplesInVectOutData = 256 / 8 / sizeof(TT_DATA);
#endif //__SUPPORTS_ACC64__
        // constexpr int kSamplesInVectTwiddle =  256 / 8 / sizeof(TT_TWIDDLE);
        constexpr int kSamplesInVectTwiddle = kSamplesInVectOutData;
        constexpr int paddedDataSize = CEIL(TP_POINT_SIZE, kSamplesInVectOutData);
        constexpr int paddedCoeffSize = CEIL(TP_POINT_SIZE, kSamplesInVectTwiddle);
        constexpr int outWindowSize = paddedDataSize * TP_NUM_FRAMES;
        constexpr int paddedFrameSize = CEIL(paddedDataSize, (kSamplesInVectOutData * TP_CASC_LEN));
        constexpr int paddedWindowSize = TP_NUM_FRAMES * paddedFrameSize;
        constexpr int cascWindowSize = paddedWindowSize / TP_CASC_LEN;
        constexpr int cascFrameSize = paddedFrameSize / TP_CASC_LEN;
        constexpr int kVecInCoeff = paddedCoeffSize / kSamplesInVectTwiddle;

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
                    // printf("%d %d,  ", masterCoeff[n][k].real, masterCoeff[n][k].imag);
                }
            }
            // printf("\n");
        }

        // // Deal coefficients across kernels
        // for (int vectorSlice = 0; vectorSlice < paddedCoeffSize/kSamplesInVectTwiddle; vectorSlice++) {
        //   for (int row = 0; row < TP_POINT_SIZE; row++) {
        //     for (int col = (vectorSlice*kSamplesInVectTwiddle); col < (vectorSlice + 1)*kSamplesInVectTwiddle;
        //     col++){
        //       cascCoeffs[row % TP_CASC_LEN].push_back(masterCoeff[row][col]);
        //       // printf("Kernel %d: %d rows %d cols\n", row % TP_CASC_LEN, row, col);
        //     }
        //   }
        // }

        // Deal coefficients across kernels
        int pairOrSingle;
        for (int vectorPair = 0; vectorPair < kVecInCoeff; vectorPair += 2) {
            for (int row = 0; row < TP_POINT_SIZE; row++) {
                pairOrSingle = (kVecInCoeff - vectorPair > 1) + 1;
                for (int i = 0; i < pairOrSingle; i++) {
                    for (int col = ((vectorPair + i) * kSamplesInVectTwiddle);
                         col < (vectorPair + 1 + i) * kSamplesInVectTwiddle; col++) {
                        cascCoeffs[row % TP_CASC_LEN].push_back(masterCoeff[row][col]);
                        // printf("Kernel %d: %d rows %d cols\n", row % TP_CASC_LEN, row, col);
                    }
                }
            }
        }

        // Create kernel classes
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN,
                           TP_NUM_FRAMES, TP_RND, TP_SAT>::create(m_dftKernels, cascCoeffs);

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
