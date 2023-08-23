/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _BSPLINE_HPP_
#define _BSPLINE_HPP_

#include "kernels.hpp"

namespace us {
namespace L2 {

template <typename T = float,
          unsigned int LENGTH_ = LENGTH,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)>
class bSpline_graph : public adf::graph {
   public:
    // interpolator
    adf::port<input> P1;
    adf::port<input> P2;
    adf::port<input> P3;
    adf::port<input> P4;
    adf::port<input> P5;
    adf::port<input> P6;
    adf::port<output> C;

    bSpline_graph(double kernel_ratio = KERNEL_RATIO) {
        // kernels
        interpolator_A1_diffSVKernel = adf::kernel::create(us::L1::diffOneLin<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A1_mulVSCRStreamIn1Kernel = adf::kernel::create(us::L1::mulVSCRS<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A1_mulVSCRStreamIn2Kernel =
            adf::kernel::create(us::L1::mulLinSCRStreamIn<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A1_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, LENGTH_, 1, SIMD_DEPTH_>);

        interpolator_A2_diffSVKernel = adf::kernel::create(us::L1::diffTwoLin<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A2_mulVSCRStreamIn1Kernel = adf::kernel::create(us::L1::mulVSCRS<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A2_mulVSCRStreamIn2Kernel = adf::kernel::create(us::L1::mulVSCRS<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A2_diffVSKernel = adf::kernel::create(us::L1::diffLinOne<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A2_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, LENGTH_, 1, SIMD_DEPTH_>);

        interpolator_A3_diffSVKernel = adf::kernel::create(us::L1::diffThreeLin<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A3_mulVSCRStreamIn1Kernel = adf::kernel::create(us::L1::mulVSCRS<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A3_mulVSCRStreamIn2Kernel = adf::kernel::create(us::L1::mulVSCRS<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A3_diffVSKernel = adf::kernel::create(us::L1::diffLinTwo<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_A3_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, LENGTH_, 1, SIMD_DEPTH_>);

        interpolator_B1_diffSVKernel = adf::kernel::create(us::L1::diffTwoLin<T, LENGTH_, SIMD_DEPTH_>); // diffOneLin
        interpolator_B1_mulVSCRKernel = adf::kernel::create(us::L1::mulVHalfInt<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_B1_mulVSCRStreamKernel = adf::kernel::create(us::L1::mulLinHalf<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_B1_mulVV1Kernel = adf::kernel::create(us::L1::mulVV<T, LENGTH_, 1, SIMD_DEPTH_>);
        interpolator_B1_mulVV2Kernel = adf::kernel::create(us::L1::mulVV<T, LENGTH_, 1, SIMD_DEPTH_>);
        interpolator_B1_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, LENGTH_, 1, SIMD_DEPTH_>);

        interpolator_B2_diffSVKernel = adf::kernel::create(us::L1::diffThreeLin<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_B2_mulVSCR1Kernel = adf::kernel::create(us::L1::mulVHalfInt<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_B2_mulVSCR2Kernel = adf::kernel::create(us::L1::mulVHalfInt<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_B2_diffVSKernel = adf::kernel::create(us::L1::diffLinOne<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_B2_mulVV1Kernel = adf::kernel::create(us::L1::mulVV<T, LENGTH_, 1, SIMD_DEPTH_>);
        interpolator_B2_mulVV2Kernel = adf::kernel::create(us::L1::mulVV<T, LENGTH_, 1, SIMD_DEPTH_>);
        interpolator_B2_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, LENGTH_, 1, SIMD_DEPTH_>);

        interpolator_C_diffSVKernel = adf::kernel::create(us::L1::diffTwoLin<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_C_mulVV1Kernel = adf::kernel::create(us::L1::mulVV<T, LENGTH_, 1, SIMD_DEPTH_>);
        interpolator_C_diffVSKernel = adf::kernel::create(us::L1::diffLinOne<T, LENGTH_, SIMD_DEPTH_>);
        interpolator_C_mulVV2Kernel = adf::kernel::create(us::L1::mulVV<T, LENGTH_, 1, SIMD_DEPTH_>);
        interpolator_C_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, LENGTH_, 1, SIMD_DEPTH_>);

        dataMoverKernel1 = adf::kernel::create(us::L1::dataMover<T, LENGTH_, SIMD_DEPTH_>);
        dataMoverKernel2 = adf::kernel::create(us::L1::dataMover<T, LENGTH_, SIMD_DEPTH_>);
        dataMoverKernel3 = adf::kernel::create(us::L1::dataMover<T, LENGTH_, SIMD_DEPTH_>);
        dataMoverKernel4 = adf::kernel::create(us::L1::dataMover<T, LENGTH_, SIMD_DEPTH_>);
        dataMoverKernel5 = adf::kernel::create(us::L1::dataMover<T, LENGTH_, SIMD_DEPTH_>);
        dataMoverKernel6 = adf::kernel::create(us::L1::dataMover<T, LENGTH_, SIMD_DEPTH_>);

        adf::source(interpolator_A1_diffSVKernel) = "diffSV/diffSV.cpp";
        adf::source(interpolator_A1_mulVSCRStreamIn1Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_A1_mulVSCRStreamIn2Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_A1_sumVVKernel) = "sumVV/sumVV.cpp";

        adf::source(interpolator_A2_diffSVKernel) = "diffSV/diffSV.cpp";
        adf::source(interpolator_A2_mulVSCRStreamIn1Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_A2_diffVSKernel) = "diffVS/diffVS.cpp";
        adf::source(interpolator_A2_mulVSCRStreamIn2Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_A2_sumVVKernel) = "sumVV/sumVV.cpp";

        adf::source(interpolator_A3_diffSVKernel) = "diffSV/diffSV.cpp";
        adf::source(interpolator_A3_mulVSCRStreamIn1Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_A3_diffVSKernel) = "diffVS/diffVS.cpp";
        adf::source(interpolator_A3_mulVSCRStreamIn2Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_A3_sumVVKernel) = "sumVV/sumVV.cpp";

        adf::source(interpolator_B1_diffSVKernel) = "diffSV/diffSV.cpp";
        adf::source(interpolator_B1_mulVSCRKernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_B1_mulVSCRStreamKernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_B1_mulVV1Kernel) = "mulVV/mulVV.cpp";
        adf::source(interpolator_B1_mulVV2Kernel) = "mulVV/mulVV.cpp";
        adf::source(interpolator_B1_sumVVKernel) = "sumVV/sumVV.cpp";

        adf::source(interpolator_B2_diffSVKernel) = "diffSV/diffSV.cpp";
        adf::source(interpolator_B2_mulVSCR1Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_B2_mulVSCR2Kernel) = "mulVS/mulVS.cpp";
        adf::source(interpolator_B2_diffVSKernel) = "diffVS/diffVS.cpp";
        adf::source(interpolator_B2_mulVV1Kernel) = "mulVV/mulVV.cpp";
        adf::source(interpolator_B2_mulVV2Kernel) = "mulVV/mulVV.cpp";
        adf::source(interpolator_B2_sumVVKernel) = "sumVV/sumVV.cpp";

        adf::source(interpolator_C_diffSVKernel) = "diffSV/diffSV.cpp";
        adf::source(interpolator_C_mulVV1Kernel) = "mulVV/mulVV.cpp";
        adf::source(interpolator_C_diffVSKernel) = "diffVS/diffVS.cpp";
        adf::source(interpolator_C_mulVV2Kernel) = "mulVV/mulVV.cpp";
        adf::source(interpolator_C_sumVVKernel) = "sumVV/sumVV.cpp";
        adf::source(dataMoverKernel1) = "dataMover/dataMover.cpp";
        adf::source(dataMoverKernel2) = "dataMover/dataMover.cpp";
        adf::source(dataMoverKernel3) = "dataMover/dataMover.cpp";
        adf::source(dataMoverKernel4) = "dataMover/dataMover.cpp";
        adf::source(dataMoverKernel5) = "dataMover/dataMover.cpp";
        adf::source(dataMoverKernel6) = "dataMover/dataMover.cpp";

        // dataMover
        adf::connect<>(P1, dataMoverKernel1.in[0]);
        adf::connect<>(P2, dataMoverKernel2.in[0]);
        adf::connect<>(P3, dataMoverKernel3.in[0]);
        adf::connect<>(P4, dataMoverKernel4.in[0]);
        adf::connect<>(P5, dataMoverKernel5.in[0]);
        adf::connect<>(P6, dataMoverKernel6.in[0]);

        adf::dimensions(dataMoverKernel1.in[0]) = {LENGTH_};
        adf::dimensions(dataMoverKernel2.in[0]) = {LENGTH_};
        adf::dimensions(dataMoverKernel3.in[0]) = {LENGTH_};
        adf::dimensions(dataMoverKernel4.in[0]) = {LENGTH_};
        adf::dimensions(dataMoverKernel5.in[0]) = {LENGTH_};
        adf::dimensions(dataMoverKernel6.in[0]) = {LENGTH_};
        adf::dimensions(dataMoverKernel1.out[0]) = {DIM_MATRIX_};
        adf::dimensions(dataMoverKernel2.out[0]) = {DIM_MATRIX_};
        adf::dimensions(dataMoverKernel3.out[0]) = {DIM_MATRIX_};
        adf::dimensions(dataMoverKernel4.out[0]) = {DIM_MATRIX_};
        adf::dimensions(dataMoverKernel5.out[0]) = {DIM_MATRIX_};
        adf::dimensions(dataMoverKernel6.out[0]) = {DIM_MATRIX_};

        //		diffSV
        adf::connect<>(interpolator_A1_diffSVKernel.out[0], interpolator_A1_mulVSCRStreamIn1Kernel.in[0]);
        adf::connect<>(dataMoverKernel1.out[0], interpolator_A1_mulVSCRStreamIn1Kernel.in[1]);
        //		A1_mulVSCRStreamIn2
        adf::connect<>(dataMoverKernel2.out[0], interpolator_A1_mulVSCRStreamIn2Kernel.in[0]);
        //		sumVV
        adf::connect<>(interpolator_A1_mulVSCRStreamIn1Kernel.out[0], interpolator_A1_sumVVKernel.in[0]);
        adf::connect<>(interpolator_A1_mulVSCRStreamIn2Kernel.out[0], interpolator_A1_sumVVKernel.in[1]);

        //	    A2
        adf::connect<>(interpolator_A2_diffSVKernel.out[0], interpolator_A2_mulVSCRStreamIn1Kernel.in[0]);
        adf::connect<>(dataMoverKernel3.out[0], interpolator_A2_mulVSCRStreamIn1Kernel.in[1]);
        //		diffVS
        adf::connect<>(interpolator_A2_diffVSKernel.out[0], interpolator_A2_mulVSCRStreamIn2Kernel.in[0]);
        adf::connect<>(dataMoverKernel4.out[0], interpolator_A2_mulVSCRStreamIn2Kernel.in[1]);
        //		sumVV
        adf::connect<>(interpolator_A2_mulVSCRStreamIn1Kernel.out[0], interpolator_A2_sumVVKernel.in[0]);
        adf::connect<>(interpolator_A2_mulVSCRStreamIn2Kernel.out[0], interpolator_A2_sumVVKernel.in[1]);

        //		A3
        adf::connect<>(interpolator_A3_diffSVKernel.out[0], interpolator_A3_mulVSCRStreamIn1Kernel.in[0]);
        adf::connect<>(dataMoverKernel5.out[0], interpolator_A3_mulVSCRStreamIn1Kernel.in[1]);
        //		diffVS
        adf::connect<>(interpolator_A3_diffVSKernel.out[0], interpolator_A3_mulVSCRStreamIn2Kernel.in[0]);
        adf::connect<>(dataMoverKernel6.out[0], interpolator_A3_mulVSCRStreamIn2Kernel.in[1]);
        //		sumVV
        adf::connect<>(interpolator_A3_mulVSCRStreamIn1Kernel.out[0], interpolator_A3_sumVVKernel.in[0]);
        adf::connect<>(interpolator_A3_mulVSCRStreamIn2Kernel.out[0], interpolator_A3_sumVVKernel.in[1]);

        //		B1
        adf::connect<>(interpolator_B1_diffSVKernel.out[0], interpolator_B1_mulVSCRKernel.in[0]);
        adf::connect<>(interpolator_B1_mulVSCRKernel.out[0], interpolator_B1_mulVV1Kernel.in[0]);
        adf::connect<>(interpolator_A1_sumVVKernel.out[0], interpolator_B1_mulVV1Kernel.in[1]);
        adf::connect<>(interpolator_B1_mulVSCRStreamKernel.out[0], interpolator_B1_mulVV2Kernel.in[0]);
        adf::connect<>(interpolator_A2_sumVVKernel.out[0], interpolator_B1_mulVV2Kernel.in[1]);
        adf::connect<>(interpolator_B1_mulVV1Kernel.out[0], interpolator_B1_sumVVKernel.in[0]);
        adf::connect<>(interpolator_B1_mulVV2Kernel.out[0], interpolator_B1_sumVVKernel.in[1]);

        //		B2
        adf::connect<>(interpolator_B2_diffSVKernel.out[0], interpolator_B2_mulVSCR1Kernel.in[0]);
        adf::connect<>(interpolator_B2_mulVSCR1Kernel.out[0], interpolator_B2_mulVV1Kernel.in[0]);
        adf::connect<>(interpolator_A2_sumVVKernel.out[0], interpolator_B2_mulVV1Kernel.in[1]);
        adf::connect<>(interpolator_B2_diffVSKernel.out[0], interpolator_B2_mulVSCR2Kernel.in[0]);
        adf::connect<>(interpolator_B2_mulVSCR2Kernel.out[0], interpolator_B2_mulVV2Kernel.in[0]);
        adf::connect<>(interpolator_A3_sumVVKernel.out[0], interpolator_B2_mulVV2Kernel.in[1]);
        adf::connect<>(interpolator_B2_mulVV1Kernel.out[0], interpolator_B2_sumVVKernel.in[0]);
        adf::connect<>(interpolator_B2_mulVV2Kernel.out[0], interpolator_B2_sumVVKernel.in[1]);

        //		C
        adf::connect<>(interpolator_C_diffSVKernel.out[0], interpolator_C_mulVV1Kernel.in[0]);
        adf::connect<>(interpolator_B1_sumVVKernel.out[0], interpolator_C_mulVV1Kernel.in[1]);
        adf::connect<>(interpolator_C_diffVSKernel.out[0], interpolator_C_mulVV2Kernel.in[0]);
        adf::connect<>(interpolator_B2_sumVVKernel.out[0], interpolator_C_mulVV2Kernel.in[1]);
        adf::connect<>(interpolator_C_mulVV1Kernel.out[0], interpolator_C_sumVVKernel.in[0]);
        adf::connect<>(interpolator_C_mulVV2Kernel.out[0], interpolator_C_sumVVKernel.in[1]);

        //		RESULTS
        adf::connect<>(interpolator_C_sumVVKernel.out[0], C);

        adf::dimensions(interpolator_A1_diffSVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_mulVSCRStreamIn1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_mulVSCRStreamIn1Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_mulVSCRStreamIn1Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_mulVSCRStreamIn2Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_mulVSCRStreamIn2Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_sumVVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_sumVVKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A1_sumVVKernel.out[0]) = {DIM_MATRIX_};

        adf::dimensions(interpolator_A2_diffSVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_diffVSKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_mulVSCRStreamIn1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_mulVSCRStreamIn1Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_mulVSCRStreamIn1Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_mulVSCRStreamIn2Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_mulVSCRStreamIn2Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_mulVSCRStreamIn2Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_sumVVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_sumVVKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A2_sumVVKernel.out[0]) = {DIM_MATRIX_};

        adf::dimensions(interpolator_A3_diffSVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_diffVSKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_mulVSCRStreamIn1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_mulVSCRStreamIn1Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_mulVSCRStreamIn1Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_mulVSCRStreamIn2Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_mulVSCRStreamIn2Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_mulVSCRStreamIn2Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_sumVVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_sumVVKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_A3_sumVVKernel.out[0]) = {DIM_MATRIX_};

        adf::dimensions(interpolator_B1_diffSVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVSCRKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVSCRKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVSCRStreamKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVV1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVV1Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVV1Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVV2Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVV2Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_mulVV2Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_sumVVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_sumVVKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B1_sumVVKernel.out[0]) = {DIM_MATRIX_};

        adf::dimensions(interpolator_B2_diffSVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVSCR1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVSCR1Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVSCR2Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVSCR2Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_diffVSKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVV1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVV1Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVV1Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVV2Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVV2Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_mulVV2Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_sumVVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_sumVVKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_B2_sumVVKernel.out[0]) = {DIM_MATRIX_};

        adf::dimensions(interpolator_C_diffSVKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_mulVV1Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_mulVV1Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_mulVV1Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_diffVSKernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_mulVV2Kernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_mulVV2Kernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_mulVV2Kernel.out[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_sumVVKernel.in[0]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_sumVVKernel.in[1]) = {DIM_MATRIX_};
        adf::dimensions(interpolator_C_sumVVKernel.out[0]) = {DIM_MATRIX_};

        // interpolator
        adf::runtime<adf::ratio>(interpolator_A1_diffSVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A1_mulVSCRStreamIn1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A1_mulVSCRStreamIn2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A1_sumVVKernel) = kernel_ratio;

        adf::runtime<adf::ratio>(interpolator_A2_diffSVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A2_mulVSCRStreamIn1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A2_diffVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A2_mulVSCRStreamIn2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A2_sumVVKernel) = kernel_ratio;

        adf::runtime<adf::ratio>(interpolator_A3_diffSVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A3_mulVSCRStreamIn1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A3_diffVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A3_mulVSCRStreamIn2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_A3_sumVVKernel) = kernel_ratio;

        adf::runtime<adf::ratio>(interpolator_B1_diffSVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B1_mulVSCRKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B1_mulVSCRStreamKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B1_mulVV1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B1_mulVV2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B1_sumVVKernel) = kernel_ratio;

        adf::runtime<adf::ratio>(interpolator_B2_diffSVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B2_mulVSCR1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B2_mulVSCR2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B2_diffVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B2_mulVV1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B2_mulVV2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_B2_sumVVKernel) = kernel_ratio;

        adf::runtime<adf::ratio>(interpolator_C_diffSVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_C_mulVV1Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_C_diffVSKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_C_mulVV2Kernel) = kernel_ratio;
        adf::runtime<adf::ratio>(interpolator_C_sumVVKernel) = kernel_ratio;
        adf::runtime<adf::ratio>(dataMoverKernel1) = kernel_ratio;
        adf::runtime<adf::ratio>(dataMoverKernel2) = kernel_ratio;
        adf::runtime<adf::ratio>(dataMoverKernel3) = kernel_ratio;
        adf::runtime<adf::ratio>(dataMoverKernel4) = kernel_ratio;
        adf::runtime<adf::ratio>(dataMoverKernel5) = kernel_ratio;
        adf::runtime<adf::ratio>(dataMoverKernel6) = kernel_ratio;
    }

   private:
    adf::kernel interpolator_A1_diffSVKernel;
    adf::kernel interpolator_A1_mulVSCRStreamIn1Kernel;
    adf::kernel interpolator_A1_mulVSCRStreamIn2Kernel;
    adf::kernel interpolator_A1_sumVVKernel;

    adf::kernel interpolator_A2_diffSVKernel;
    adf::kernel interpolator_A2_diffVSKernel;
    adf::kernel interpolator_A2_mulVSCRStreamIn1Kernel;
    adf::kernel interpolator_A2_mulVSCRStreamIn2Kernel;
    adf::kernel interpolator_A2_sumVVKernel;

    adf::kernel interpolator_A3_diffSVKernel;
    adf::kernel interpolator_A3_diffVSKernel;
    adf::kernel interpolator_A3_mulVSCRStreamIn1Kernel;
    adf::kernel interpolator_A3_mulVSCRStreamIn2Kernel;
    adf::kernel interpolator_A3_sumVVKernel;

    adf::kernel interpolator_B1_diffSVKernel;
    adf::kernel interpolator_B1_mulVSCRKernel;
    adf::kernel interpolator_B1_mulVSCRStreamKernel;
    adf::kernel interpolator_B1_mulVV1Kernel;
    adf::kernel interpolator_B1_mulVV2Kernel;
    adf::kernel interpolator_B1_sumVVKernel;

    adf::kernel interpolator_B2_diffSVKernel;
    adf::kernel interpolator_B2_mulVSCR1Kernel;
    adf::kernel interpolator_B2_mulVSCR2Kernel;
    adf::kernel interpolator_B2_diffVSKernel;
    adf::kernel interpolator_B2_mulVV1Kernel;
    adf::kernel interpolator_B2_mulVV2Kernel;
    adf::kernel interpolator_B2_sumVVKernel;

    adf::kernel interpolator_C_diffSVKernel;
    adf::kernel interpolator_C_mulVV1Kernel;
    adf::kernel interpolator_C_diffVSKernel;
    adf::kernel interpolator_C_mulVV2Kernel;
    adf::kernel interpolator_C_sumVVKernel;

    adf::kernel dataMoverKernel1;
    adf::kernel dataMoverKernel2;
    adf::kernel dataMoverKernel3;
    adf::kernel dataMoverKernel4;
    adf::kernel dataMoverKernel5;
    adf::kernel dataMoverKernel6;
};

} // namespace L2
} // namespace us

#endif
