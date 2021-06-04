#include "matrix_mult_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

// This file holds the body of the Asymmetric Interpolation FIR reference model kernel class
template <typename TT_DATA_A,
          typename TT_DATA_B,
          size_t TP_DIM_A,
          size_t TP_DIM_AB,
          size_t TP_DIM_B,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_DIM_A_LEADING, // = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING, // = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_INPUT_WINDOW_VSIZE_A, // = TP_DIM_A*TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B  // = TP_DIM_B*TP_DIM_AB
          >
void matrix_mult_ref<TT_DATA_A,
                     TT_DATA_B,
                     TP_DIM_A,
                     TP_DIM_AB,
                     TP_DIM_B,
                     TP_SHIFT,
                     TP_RND,
                     TP_DIM_A_LEADING,
                     TP_DIM_B_LEADING,
                     TP_DIM_OUT_LEADING,
                     TP_INPUT_WINDOW_VSIZE_A,
                     TP_INPUT_WINDOW_VSIZE_B>::mmult(input_window<TT_DATA_A>* inWindowA,
                                                     input_window<TT_DATA_B>* inWindowB,
                                                     output_window<outType_t<TT_DATA_A, TT_DATA_B> >* outWindow) {
    // typename TT_OUT = outType_t<TT_DATA_A,TT_DATA_B>;
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_OUT> accum;
    TT_DATA_A dA_in[TP_DIM_AB];
    TT_DATA_B dB_in[TP_DIM_AB];
    TT_OUT accum_srs;

    /*
    Most Ideal Case                  |   Good for A
     Index for RowMaj A x ColMaj B.  |    Index for RowMaj A x RowMaj B.
      <-AB-->   B B                  |     <-AB-->   B B
    A 1 2 3 4   1 5                  |   A 1 2 3 4   1 2
    A 5 6 7 8   2 6                  |   A 5 6 7 8   3 4
                3 7                  |               5 6
                4 8                  |               7 8

    Good for B                       |   Least Ideal Case
     Index for ColMaj A x ColMaj B.  |    Index for ColMaj A x RowMaj B.
      <-AB-->   B B                  |     <-AB-->   B B
    A 1 3 5 7   1 5                  |   A 1 3 5 7   1 2
    A 2 4 6 8   2 6                  |   A 2 4 6 8   3 4
                3 7                  |               5 6
                4 8                  |               7 8

    */
    // const int TP_DIM_A_LEADING =1;
    // const int TP_DIM_B_LEADING =0;
    // const int ROW_MAJOR =0;
    // const int COL_MAJOR =1;

    const int ABElementDistA = (TP_DIM_A_LEADING == ROW_MAJOR) ? 1 : TP_DIM_A;
    const int ABElementDistB = (TP_DIM_B_LEADING == ROW_MAJOR) ? TP_DIM_B : 1;
    const int AElementDist = (TP_DIM_A_LEADING == ROW_MAJOR) ? TP_DIM_AB : 1;
    const int BElementDist = (TP_DIM_B_LEADING == ROW_MAJOR) ? 1 : TP_DIM_AB;

    const int matALoadIncr = ABElementDistA;                  // Incremental go down Arow  (ABelement)
    const int matBLoadIncr = ABElementDistB;                  // Get to the next Bcol element (AB element)
    const int matAPostABDimDecr = (TP_DIM_AB * matALoadIncr); // Go back to the start of Arow
    const int matBPostABDimDecr = ((TP_DIM_AB * matBLoadIncr) -
                                   BElementDist); // go back to the start, then go forward by one B element direction.
    const int matAPostBDimIncr = AElementDist;    // Move one Arow/BCol forward
    const int matBPostBDimIncr = -((int)(TP_DIM_B * BElementDist)); // Move back to the start of Brow

    printf("Ref model params:\n");
    printf("TP_SHIFT = %lu\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_VSIZE_A = %d\n", TP_INPUT_WINDOW_VSIZE_A);
    printf("TP_INPUT_WINDOW_VSIZE_B = %d\n", TP_INPUT_WINDOW_VSIZE_B);

    constexpr unsigned int num_matrix_A = (TP_INPUT_WINDOW_VSIZE_A / (TP_DIM_A * TP_DIM_AB));
    constexpr unsigned int num_matrix_B = (TP_INPUT_WINDOW_VSIZE_B / (TP_DIM_B * TP_DIM_AB));
    static_assert(
        (TP_INPUT_WINDOW_VSIZE_A >= (TP_DIM_A * TP_DIM_AB)) && (TP_INPUT_WINDOW_VSIZE_B >= (TP_DIM_B * TP_DIM_AB)),
        "Window Size is smaller than matrix dimensions. ");
    static_assert(!((num_matrix_A > 1) && (num_matrix_B > 1)), "We don't support these window sizes. ");

    unsigned int num_iterations = std::max(num_matrix_A, num_matrix_B);

    for (unsigned int iter = 0; iter < num_iterations; ++iter) {
        for (unsigned int elemA = 0; elemA < TP_DIM_A; elemA++) {
            for (unsigned int elemB = 0; elemB < TP_DIM_B; elemB++) {
                // The vector that gets multiplied
                // printf("A: ");
                for (unsigned int j = 0; j < TP_DIM_AB; ++j) {
                    dA_in[j] = window_read(inWindowA);    // read input data
                    window_incr(inWindowA, matALoadIncr); // read from the next column.
                    // printf("%d ", dA_in[j].real);
                }
                // printf("\n");
                // printf("B: ");
                for (unsigned int j = 0; j < TP_DIM_AB; ++j) {
                    dB_in[j] = window_read(inWindowB);    // read input data
                    window_incr(inWindowB, matBLoadIncr); // read from the next row.
                    // printf("%d ", dB_in[j].real);
                }
                // printf("\n");

                accum = null_accRef<TT_OUT>(); // reset accumulator at the start of the mult-add for each output sample
                for (unsigned int j = 0; j < TP_DIM_AB; ++j) {
                    multiplyAcc(accum, dA_in[j], dB_in[j]);
                }
                // prior to output, the final accumulated value must be downsized to the same type
                // as was input. To do this, the final result is rounded, saturated and shifted down
                roundAcc(TP_RND, shift, accum);
                saturateAcc(accum);
                accum_srs = castAcc(accum);
                window_writeincr((output_window<TT_OUT>*)outWindow, accum_srs);
                // Revert data pointer by the amount we've incremented
                window_decr(inWindowB, (matBPostABDimDecr));
                // revert A back to start of row
                window_decr(inWindowA, matAPostABDimDecr);
            }
            // Point A one row forward
            window_incr(inWindowA, matAPostBDimIncr);
            window_incr(inWindowB, matBPostBDimIncr);
        }
        if (num_matrix_A < num_matrix_B && num_matrix_A == 1) {
            // Fixed_A - Move it back to the start
            window_decr(inWindowA, (TP_DIM_A - 1) * TP_DIM_AB);
        }
        // if fixed_B, all the pointers in the right place
        // if only one matrix for both, we're done
        // if both more than one, then we've already got a static assert to cover this.
    }
};
}
}
}
}
}

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
