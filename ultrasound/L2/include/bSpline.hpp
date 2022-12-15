/*
 * Copyright 2022 Xilinx, Inc.
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
          unsigned int POINTS_PER_ITERATION_ = POINTS_PER_ITERATION,
          unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
          unsigned int WIN_SIZE_INTERPOLATOR_ = WIN_SIZE_INTERPOLATOR,
          unsigned int FIFO_DEPTH_ = FIFO_DEPTH>
class bSpline : public adf::graph {
   public:
    // interpolator
    adf::port<input> P1;
    adf::port<input> P2;
    adf::port<input> P3;
    adf::port<input> P4;
    adf::port<input> P5;
    adf::port<input> P6;
    adf::port<output> C;

    bSpline() {
        // KERNELS
        // interpolator
        interpolator_A1_diffSVKernel = adf::kernel::create(us::L1::diffOneLin<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A1_mulVSCRStreamIn1Kernel =
            adf::kernel::create(us::L1::mulVSCRSWindow<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A1_mulVSCRStreamIn2Kernel =
            adf::kernel::create(us::L1::mulLinSCRStreamIn<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A1_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_A2_diffSVKernel = adf::kernel::create(us::L1::diffTwoLin<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A2_mulVSCRStreamIn1Kernel =
            adf::kernel::create(us::L1::mulVSCRSWindow<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A2_mulVSCRStreamIn2Kernel =
            adf::kernel::create(us::L1::mulVSCRSWindow<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A2_diffVSKernel = adf::kernel::create(us::L1::diffLinOne<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A2_sumVVKernel =
            adf::kernel::create(us::L1::sumVVStreamOut<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A3_diffSVKernel = adf::kernel::create(us::L1::diffThreeLin<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A3_mulVSCRStreamIn1Kernel =
            adf::kernel::create(us::L1::mulVSCRSWindow<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A3_mulVSCRStreamIn2Kernel =
            adf::kernel::create(us::L1::mulVSCRSWindow<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A3_diffVSKernel = adf::kernel::create(us::L1::diffLinTwo<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_A3_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_B1_diffSVKernel =
            adf::kernel::create(us::L1::diffTwoLin<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>); // diffOneLin
        interpolator_B1_mulVSCRKernel = adf::kernel::create(us::L1::mulVHalfInt<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B1_mulVSCRStreamKernel =
            adf::kernel::create(us::L1::mulLinHalf<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B1_mulVV1Kernel = adf::kernel::create(us::L1::mulVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_B1_mulVV2Kernel = adf::kernel::create(us::L1::mulVVStream<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B1_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_B2_diffSVKernel = adf::kernel::create(us::L1::diffThreeLin<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B2_mulVSCR1Kernel =
            adf::kernel::create(us::L1::mulVHalfInt<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B2_mulVSCR2Kernel =
            adf::kernel::create(us::L1::mulVHalfInt<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B2_diffVSKernel = adf::kernel::create(us::L1::diffLinOne<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B2_mulVV1Kernel = adf::kernel::create(us::L1::mulVVStream<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_B2_mulVV2Kernel = adf::kernel::create(us::L1::mulVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_B2_sumVVKernel = adf::kernel::create(us::L1::sumVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_C_diffSVKernel = adf::kernel::create(us::L1::diffTwoLin<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_C_mulVV1Kernel = adf::kernel::create(us::L1::mulVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_C_diffVSKernel = adf::kernel::create(us::L1::diffLinOne<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        interpolator_C_mulVV2Kernel = adf::kernel::create(us::L1::mulVV<T, POINTS_PER_ITERATION_, 1, SIMD_DEPTH_>);
        interpolator_C_sumVVKernel = adf::kernel::create(us::L1::sumVVStreamOut<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        dataMoverKernel1 = adf::kernel::create(us::L1::dataMover<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        dataMoverKernel2 = adf::kernel::create(us::L1::dataMover<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        dataMoverKernel3 = adf::kernel::create(us::L1::dataMover<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        dataMoverKernel4 = adf::kernel::create(us::L1::dataMover<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        dataMoverKernel5 = adf::kernel::create(us::L1::dataMover<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);
        dataMoverKernel6 = adf::kernel::create(us::L1::dataMover<T, POINTS_PER_ITERATION_, SIMD_DEPTH_>);

        ///////////// CONNECTIONS /////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        adf::connect<adf::stream> points1(P1, dataMoverKernel1.in[0]);
        adf::connect<adf::stream> points2(P2, dataMoverKernel2.in[0]);
        adf::connect<adf::stream> points3(P3, dataMoverKernel3.in[0]);
        adf::connect<adf::stream> points4(P4, dataMoverKernel4.in[0]);
        adf::connect<adf::stream> points5(P5, dataMoverKernel5.in[0]);
        adf::connect<adf::stream> points6(P6, dataMoverKernel6.in[0]);
        //	//		diffSV
        //			adf::connect< adf::window < SIMD_DEPTH*POINTS_PER_ITERATION > > (t,
        // interpolator_A1_diffSVKernel.in[0]); 		A1_mulVSCRStreamIn1
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A1_diffSVKernel.out[0],
                                                           interpolator_A1_mulVSCRStreamIn1Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(dataMoverKernel1.out[0],
                                                           interpolator_A1_mulVSCRStreamIn1Kernel.in[1]);
        //		A1_mulVSCRStreamIn2
        //			adf::connect< adf::window< WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_A1_mulVSCRStreamIn2Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(dataMoverKernel2.out[0],
                                                           interpolator_A1_mulVSCRStreamIn2Kernel.in[0]);
        //		sumVV
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A1_mulVSCRStreamIn1Kernel.out[0],
                                                           interpolator_A1_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A1_mulVSCRStreamIn2Kernel.out[0],
                                                           interpolator_A1_sumVVKernel.in[1]);

        //			adf::connect< adf::stream > (interpolator_A1_sumVVKernel.out[0], C1);

        //	//		A2
        //
        //	//		diffSV
        //			adf::connect< adf::window < WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_A2_diffSVKernel.in[0]); 		mulVS
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A2_diffSVKernel.out[0],
                                                           interpolator_A2_mulVSCRStreamIn1Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> > A2_P1_stream(dataMoverKernel3.out[0],
                                                                        interpolator_A2_mulVSCRStreamIn1Kernel.in[1]);
        //		diffVS
        //			adf::connect< adf::window < WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_A2_diffVSKernel.in[0]); 		A2_mulVSCRStreamIn2
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A2_diffVSKernel.out[0],
                                                           interpolator_A2_mulVSCRStreamIn2Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> > A2_P2_stream(dataMoverKernel4.out[0],
                                                                        interpolator_A2_mulVSCRStreamIn2Kernel.in[1]);
        //		sumVV
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A2_mulVSCRStreamIn1Kernel.out[0],
                                                           interpolator_A2_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A2_mulVSCRStreamIn2Kernel.out[0],
                                                           interpolator_A2_sumVVKernel.in[1]);
        //
        //			adf::connect< adf::stream, adf::window< 64 > > (interpolator_A2_sumVVKernel.out[0], C2);

        //	//		A3
        //
        //	//		diffSV
        //			adf::connect< adf::window < WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_A3_diffSVKernel.in[0]); 		mulVS
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A3_diffSVKernel.out[0],
                                                           interpolator_A3_mulVSCRStreamIn1Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> > A3_P2_stream(dataMoverKernel5.out[0],
                                                                        interpolator_A3_mulVSCRStreamIn1Kernel.in[1]);
        //		diffVS
        //			adf::connect< adf::window < WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_A3_diffVSKernel.in[0]); 		A3_mulVSCRStreamIn2
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A3_diffVSKernel.out[0],
                                                           interpolator_A3_mulVSCRStreamIn2Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> > A3_P3_stream(dataMoverKernel6.out[0],
                                                                        interpolator_A3_mulVSCRStreamIn2Kernel.in[1]);
        //		sumVV
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A3_mulVSCRStreamIn1Kernel.out[0],
                                                           interpolator_A3_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A3_mulVSCRStreamIn2Kernel.out[0],
                                                           interpolator_A3_sumVVKernel.in[1]);
        //
        //			adf::connect< adf::window< 64 > > (interpolator_A3_sumVVKernel.out[0], C2);

        //	//		B1
        //			adf::connect< adf::window < WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_B1_diffSVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B1_diffSVKernel.out[0],
                                                           interpolator_B1_mulVSCRKernel.in[0]);
        //			adf::connect< adf::window< WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_B1_mulVSCRStreamKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B1_mulVSCRKernel.out[0],
                                                           interpolator_B1_mulVV1Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A1_sumVVKernel.out[0],
                                                           interpolator_B1_mulVV1Kernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B1_mulVSCRStreamKernel.out[0],
                                                           interpolator_B1_mulVV2Kernel.in[0]);
        adf::connect<adf::stream> B1_stream(interpolator_A2_sumVVKernel.out[0], interpolator_B1_mulVV2Kernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B1_mulVV1Kernel.out[0],
                                                           interpolator_B1_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B1_mulVV2Kernel.out[0],
                                                           interpolator_B1_sumVVKernel.in[1]);

        //			adf::connect< adf::window< 64 > > (interpolator_B1_sumVVKernel.out[0], C1);

        //
        //	//		B2
        //			adf::connect< adf::window< WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_B2_diffSVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B2_diffSVKernel.out[0],
                                                           interpolator_B2_mulVSCR1Kernel.in[0]);
        //			adf::connect< adf::window< WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_B2_diffVSKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B2_mulVSCR1Kernel.out[0],
                                                           interpolator_B2_mulVV1Kernel.in[0]);
        adf::connect<adf::stream> B2_stream(interpolator_A2_sumVVKernel.out[0], interpolator_B2_mulVV1Kernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B2_diffVSKernel.out[0],
                                                           interpolator_B2_mulVSCR2Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B2_mulVSCR2Kernel.out[0],
                                                           interpolator_B2_mulVV2Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_A3_sumVVKernel.out[0],
                                                           interpolator_B2_mulVV2Kernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B2_mulVV1Kernel.out[0],
                                                           interpolator_B2_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B2_mulVV2Kernel.out[0],
                                                           interpolator_B2_sumVVKernel.in[1]);

        //			adf::connect< adf::window< 64 > > (interpolator_B2_sumVVKernel.out[0], C2);
        //
        //	//		C
        //			adf::connect< adf::window< WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_C_diffSVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_C_diffSVKernel.out[0],
                                                           interpolator_C_mulVV1Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B1_sumVVKernel.out[0],
                                                           interpolator_C_mulVV1Kernel.in[1]);
        //			adf::connect< adf::window< WIN_SIZE_INTERPOLATOR_ > > (t,
        // interpolator_C_diffVSKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_C_diffVSKernel.out[0],
                                                           interpolator_C_mulVV2Kernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_B2_sumVVKernel.out[0],
                                                           interpolator_C_mulVV2Kernel.in[1]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_C_mulVV1Kernel.out[0],
                                                           interpolator_C_sumVVKernel.in[0]);
        adf::connect<adf::window<WIN_SIZE_INTERPOLATOR_> >(interpolator_C_mulVV2Kernel.out[0],
                                                           interpolator_C_sumVVKernel.in[1]);

        //		RESULTS
        adf::connect<adf::stream> c_out(interpolator_C_sumVVKernel.out[0], C);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //			interpolator
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

        //			interpolator
        adf::runtime<adf::ratio>(interpolator_A1_diffSVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A1_mulVSCRStreamIn1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A1_mulVSCRStreamIn2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A1_sumVVKernel) = KERNEL_RATIO;

        adf::runtime<adf::ratio>(interpolator_A2_diffSVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A2_mulVSCRStreamIn1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A2_diffVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A2_mulVSCRStreamIn2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A2_sumVVKernel) = KERNEL_RATIO;

        adf::runtime<adf::ratio>(interpolator_A3_diffSVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A3_mulVSCRStreamIn1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A3_diffVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A3_mulVSCRStreamIn2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_A3_sumVVKernel) = KERNEL_RATIO;

        adf::runtime<adf::ratio>(interpolator_B1_diffSVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B1_mulVSCRKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B1_mulVSCRStreamKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B1_mulVV1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B1_mulVV2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B1_sumVVKernel) = KERNEL_RATIO;

        adf::runtime<adf::ratio>(interpolator_B2_diffSVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B2_mulVSCR1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B2_mulVSCR2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B2_diffVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B2_mulVV1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B2_mulVV2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_B2_sumVVKernel) = KERNEL_RATIO;

        adf::runtime<adf::ratio>(interpolator_C_diffSVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_C_mulVV1Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_C_diffVSKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_C_mulVV2Kernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(interpolator_C_sumVVKernel) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(dataMoverKernel1) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(dataMoverKernel2) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(dataMoverKernel3) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(dataMoverKernel4) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(dataMoverKernel5) = KERNEL_RATIO;
        adf::runtime<adf::ratio>(dataMoverKernel6) = KERNEL_RATIO;

        // interpolator
        adf::fifo_depth(points1) = FIFO_DEPTH_;
        adf::fifo_depth(points2) = FIFO_DEPTH_;
        adf::fifo_depth(points3) = FIFO_DEPTH_;
        adf::fifo_depth(points4) = FIFO_DEPTH_;
        adf::fifo_depth(points5) = FIFO_DEPTH_;
        adf::fifo_depth(points6) = FIFO_DEPTH_;
        adf::fifo_depth(B1_stream) = FIFO_DEPTH_;
        adf::fifo_depth(B2_stream) = FIFO_DEPTH_;
        adf::fifo_depth(c_out) = FIFO_DEPTH_;
    }

   private:
    //		interpolator
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