//---------------------------------------------------------------------------------------------------
// create_casc_kernel_recur
// Where the FFT function is split over multiple processors to increase throughput, recursion
// is used to generate the multiple kernels, rather than a for loop due to constraints of
// c++ template handling.
// For each such kernel, only a splice of the full array of coefficients is processed.
// One complication comes from the fact that the cfloat implementation works on Radix2, whereas
// integer types are able to use Radix4. The Radix4 optimization constrains the function to split the
// overall function into stages since a split can only occur at the IO of an R4 stage, not at any R2
// stage. That is why the integer and float implementations of the recursion entry are specialized.
// Another complication comes from the fact that for cint16 data, cint32 data type is used between
// stages for better precision. This requires the internal type to be resolved at the recursion entry
// point and for the kernels to have different types for input and output type.
//---------------------------------------------------------------------------------------------------

// This file is included in the fft hpp. The following 2 includes should be superfluous, but harmless due to include
// guard
#include <adf.h>
#include <vector>

// Recursive kernel creation, static coefficients
template <int dim,
          typename TT_DATA,
          typename TT_INT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_END_RANK,
          unsigned int TP_RANKS_PER_KERNEL>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int kRawStartRank = TP_END_RANK - TP_RANKS_PER_KERNEL;
        static constexpr unsigned int TP_START_RANK = kRawStartRank < 0 ? 0 : kRawStartRank;

        fftKernels[dim - 1] =
            kernel::create_object<fft_ifft_dit_1ch<TT_INT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
                                                   TP_SHIFT, TP_START_RANK, TP_END_RANK> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                 TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL>::create(fftKernels);
    }
};
// Recursive kernel creation, static coefficients
template <typename TT_DATA,
          typename TT_INT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN,
          unsigned int TP_END_RANK,
          unsigned int TP_RANKS_PER_KERNEL>
class create_casc_kernel_recur<1,
                               TT_DATA,
                               TT_INT_DATA,
                               TT_TWIDDLE,
                               TP_POINT_SIZE,
                               TP_FFT_NIFFT,
                               TP_SHIFT,
                               TP_CASC_LEN,
                               TP_END_RANK,
                               TP_RANKS_PER_KERNEL> {
   public:
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_START_RANK = 0;
        fftKernels[0] = kernel::create_object<fft_ifft_dit_1ch<TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE,
                                                               TP_FFT_NIFFT, TP_SHIFT, TP_START_RANK, TP_END_RANK> >();
    }
};
// Kernel creation, entry to recursion, also end of cascade. For integer types
template <int dim,
          typename TT_DATA, // type for I/O
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN>
class create_casc_kernel {
   public:
    typedef typename std::conditional<std::is_same<TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType;
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_END_RANK =
            (TP_POINT_SIZE == 4096)
                ? 12
                : (TP_POINT_SIZE == 2048)
                      ? 11
                      : (TP_POINT_SIZE == 1024)
                            ? 10
                            : (TP_POINT_SIZE == 512)
                                  ? 9
                                  : (TP_POINT_SIZE == 256)
                                        ? 8
                                        : (TP_POINT_SIZE == 128)
                                              ? 7
                                              : (TP_POINT_SIZE == 64)
                                                    ? 6
                                                    : (TP_POINT_SIZE == 32)
                                                          ? 5
                                                          : (TP_POINT_SIZE == 16) ? 4
                                                                                  : 0; // 0 is an error trap effectively
        static constexpr unsigned int kRawRanksPerKernel = (TP_END_RANK / TP_CASC_LEN / 2) * 2;
        static constexpr unsigned int TP_RANKS_PER_KERNEL = kRawRanksPerKernel < 2 ? 2 : kRawRanksPerKernel;
        static constexpr unsigned int TP_START_RANK = TP_END_RANK - TP_RANKS_PER_KERNEL;
        fftKernels[dim - 1] =
            kernel::create_object<fft_ifft_dit_1ch<T_internalDataType, TT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
                                                   TP_SHIFT, TP_START_RANK, TP_END_RANK> >();
        create_casc_kernel_recur<dim - 1, TT_DATA, T_internalDataType, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT,
                                 TP_SHIFT, TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL>::create(fftKernels);
    }
};
// Kernel creation, entry to recursion, also end of cascade. For cfloat types
template <int dim,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_CASC_LEN>
class create_casc_kernel<dim, cfloat, cfloat, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN> {
   public:
    typedef cfloat T_internalDataType;
    static_assert((TP_CASC_LEN > 0) && (TP_CASC_LEN < kMaxPointLog - 4),
                  "ERROR : TP_CASC_LEN is not in supported range");
    static void create(kernel (&fftKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_END_RANK =
            (TP_POINT_SIZE == 4096)
                ? 12
                : (TP_POINT_SIZE == 2048)
                      ? 11
                      : (TP_POINT_SIZE == 1024)
                            ? 10
                            : (TP_POINT_SIZE == 512)
                                  ? 9
                                  : (TP_POINT_SIZE == 256)
                                        ? 8
                                        : (TP_POINT_SIZE == 128)
                                              ? 7
                                              : (TP_POINT_SIZE == 64)
                                                    ? 6
                                                    : (TP_POINT_SIZE == 32)
                                                          ? 5
                                                          : (TP_POINT_SIZE == 16) ? 4
                                                                                  : 0; // 0 is an error trap effectively
        static constexpr unsigned int kRawRanksPerKernel = (TP_END_RANK / TP_CASC_LEN);
        static constexpr unsigned int TP_RANKS_PER_KERNEL = kRawRanksPerKernel < 1 ? 1 : kRawRanksPerKernel;
        static constexpr unsigned int TP_START_RANK = TP_END_RANK - TP_RANKS_PER_KERNEL;
        fftKernels[dim - 1] =
            kernel::create_object<fft_ifft_dit_1ch<T_internalDataType, cfloat, cfloat, TP_POINT_SIZE, TP_FFT_NIFFT,
                                                   TP_SHIFT, TP_START_RANK, TP_END_RANK> >();
        create_casc_kernel_recur<dim - 1, cfloat, T_internalDataType, cfloat, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT,
                                 TP_CASC_LEN, TP_START_RANK, TP_RANKS_PER_KERNEL>::create(fftKernels);
    }
};

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
