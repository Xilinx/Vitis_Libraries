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

#include "imagePoints.hpp"
#include "delay.hpp"
#include "focusing.hpp"
#include "samples.hpp"
#include "apodization.hpp"
#include "bSpline.hpp"

// #define DIM_VECTOR LENGTH
// #define DIM_MATRIX (LENGTH * SPACE_DIMENSION)
#define KERNEL_RATIO_SCANLINE 1.0

namespace us {
namespace L3 {

class scanline : public adf::graph {
   public:
    // IMAGE POINTS
    adf::input_plio start_positions;
    adf::input_plio directions;
    adf::input_plio samples_arange;
    adf::output_plio image_points_output;
    // // DELAY
    adf::input_plio image_points_from_PL;
    adf::input_plio image_points_from_PL_2;
    adf::input_plio tx_def_ref_point;
    adf::input_plio tx_def_delay_distance;
    adf::input_plio tx_def_delay_distance_2;
    adf::input_plio tx_def_focal_point;
    adf::input_plio t_start;
    adf::output_plio delay_to_PL;
    // FOCUSING
    adf::input_plio apo_ref_0;
    adf::input_plio xdc_def_0;
    adf::input_plio apo_ref_1;
    adf::input_plio xdc_def_1;
    adf::output_plio focusing_output;
    // SAMPLES
    adf::input_plio image_points_from_PL_3;
    adf::input_plio xdc_def_positions;
    adf::input_plio sampling_frequency;
    adf::input_plio delay_from_PL;
    adf::output_plio samples_to_PL;
    // APODIZATION
    adf::input_plio image_points_apo;
    adf::input_plio apodization_reference;
    adf::input_plio apo_distance_k;
    adf::input_plio F_number;
    adf::output_plio apodization_output;
    // INTERPOLATOR
    adf::input_plio P1;
    adf::input_plio P2;
    adf::input_plio P3;
    adf::input_plio P4;
    adf::input_plio P5;
    adf::input_plio P6;
    adf::output_plio C;

    us::L2::imagePoints_graph<> img = us::L2::imagePoints_graph<>(KERNEL_RATIO_SCANLINE);
    us::L2::delay_graph<> d = us::L2::delay_graph<>(KERNEL_RATIO_SCANLINE);
    us::L2::focusing_graph<> foc = us::L2::focusing_graph<>(KERNEL_RATIO_SCANLINE);
    us::L2::samples_graph<> sam = us::L2::samples_graph<>(KERNEL_RATIO_SCANLINE);
    us::L2::apodization_graph<> apo = us::L2::apodization_graph<>(KERNEL_RATIO_SCANLINE);
    us::L2::bSpline_graph<> interp = us::L2::bSpline_graph<>(KERNEL_RATIO_SCANLINE);

    scanline() {
        // IMAGEPOINTS
        start_positions = adf::input_plio::create("start_positions", adf::plio_32_bits, "data/start_positions.txt");
        directions = adf::input_plio::create("directions", adf::plio_32_bits, "data/directions.txt");
        samples_arange = adf::input_plio::create("samples_arange", adf::plio_32_bits, "data/samples_arange.txt");
        image_points_output = adf::output_plio::create("image_points", adf::plio_32_bits, "data/image_points.txt");

        adf::connect<>(start_positions.out[0], img.start_positions);
        adf::connect<>(samples_arange.out[0], img.samples_arange);
        adf::connect<>(directions.out[0], img.directions);
        adf::connect<>(img.image_points, image_points_output.in[0]);

        // DELAY
        image_points_from_PL =
            adf::input_plio::create("image_points_from_PL", adf::plio_32_bits, "data/image_points.txt");
        image_points_from_PL_2 =
            adf::input_plio::create("image_points_from_PL_2", adf::plio_32_bits, "data/image_points.txt");
        tx_def_ref_point = adf::input_plio::create("tx_def_ref_point", adf::plio_32_bits, "data/tx_def_ref_point.txt");
        tx_def_delay_distance =
            adf::input_plio::create("tx_def_delay_distance", adf::plio_32_bits, "data/tx_def_delay_distance.txt");
        tx_def_delay_distance_2 =
            adf::input_plio::create("tx_def_delay_distance_2", adf::plio_32_bits, "data/tx_def_delay_distance.txt");
        tx_def_focal_point =
            adf::input_plio::create("tx_def_focal_point", adf::plio_32_bits, "data/tx_def_focal_point.txt");
        t_start = adf::input_plio::create("t_start", adf::plio_32_bits, "data/t_start.txt");
        delay_to_PL = adf::output_plio::create("delay_to_PL", adf::plio_32_bits, "data/delay_to_PL.txt");

        // connections
        adf::connect<>(image_points_from_PL.out[0], d.image_points_from_PL);
        adf::connect<>(image_points_from_PL_2.out[0], d.image_points_from_PL_);
        adf::connect<>(tx_def_ref_point.out[0], d.tx_def_ref_point);
        adf::connect<>(tx_def_delay_distance.out[0], d.tx_def_delay_distance);
        adf::connect<>(tx_def_delay_distance_2.out[0], d.tx_def_delay_distance2);
        adf::connect<>(tx_def_focal_point.out[0], d.tx_def_focal_point);
        adf::connect<>(t_start.out[0], d.t_start);
        adf::connect<>(d.delay_to_PL, delay_to_PL.in[0]);

        // FOCUSING
        apo_ref_0 = adf::input_plio::create("apo_ref_0", adf::plio_32_bits, "data/apo_ref_0.txt");
        xdc_def_0 = adf::input_plio::create("xdc_def_0", adf::plio_32_bits, "data/xdc_def_0.txt");
        apo_ref_1 = adf::input_plio::create("apo_ref_1", adf::plio_32_bits, "data/apo_ref_1.txt");
        xdc_def_1 = adf::input_plio::create("xdc_def_1", adf::plio_32_bits, "data/xdc_def_1.txt");
        focusing_output = adf::output_plio::create("focusing_output", adf::plio_32_bits, "data/focusing_output.txt");

        adf::connect<>(apo_ref_0.out[0], foc.apo_ref_0);
        adf::connect<>(xdc_def_0.out[0], foc.xdc_def_0);
        adf::connect<>(apo_ref_1.out[0], foc.apo_ref_1);
        adf::connect<>(xdc_def_1.out[0], foc.xdc_def_1);
        adf::connect<>(foc.focusing_output, focusing_output.in[0]);

        // SAMPLES
        image_points_from_PL_3 =
            adf::input_plio::create("image_points_from_PL_3", adf::plio_32_bits, "data/image_points.txt");
        delay_from_PL = adf::input_plio::create("delay_from_PL", adf::plio_32_bits, "data/delay_from_PL.txt");
        xdc_def_positions =
            adf::input_plio::create("xdc_def_positions", adf::plio_32_bits, "data/xdc_def_positions.txt");
        sampling_frequency =
            adf::input_plio::create("sampling_frequency", adf::plio_32_bits, "data/sampling_frequency.txt");
        samples_to_PL = adf::output_plio::create("samples_to_PL", adf::plio_32_bits, "data/samples_to_PL.txt");

        adf::connect<>(image_points_from_PL_3.out[0], sam.image_points_from_PL);
        adf::connect<>(xdc_def_positions.out[0], sam.xdc_def_positions);
        adf::connect<>(sampling_frequency.out[0], sam.sampling_frequency);
        adf::connect<>(delay_from_PL.out[0], sam.delay_from_PL);
        adf::connect<>(sam.samples_to_PL, samples_to_PL.in[0]);

        // APODIZATION
        image_points_apo =
            adf::input_plio::create("image_points_from_PL_4", adf::plio_32_bits, "data/image_points.txt");
        apodization_reference =
            adf::input_plio::create("apodization_reference", adf::plio_32_bits, "data/apodization_reference.txt");
        apo_distance_k = adf::input_plio::create("apo_distance_k", adf::plio_32_bits, "data/apo_distance_k.txt");
        F_number = adf::input_plio::create("F_number", adf::plio_32_bits, "data/F_number.txt");
        apodization_output = adf::output_plio::create("apodization", adf::plio_32_bits, "data/apodization.txt");

        adf::connect<>(image_points_apo.out[0], apo.image_points);
        adf::connect<>(apodization_reference.out[0], apo.apodization_reference_i);
        adf::connect<>(apo_distance_k.out[0], apo.apo_distance_k);
        adf::connect<>(F_number.out[0], apo.F_number);
        adf::connect<>(apo.apodization_output, apodization_output.in[0]);

        // INTERPOLATOR
        P1 = adf::input_plio::create("P1", adf::plio_32_bits, "data/P1.txt");
        P2 = adf::input_plio::create("P2", adf::plio_32_bits, "data/P2.txt");
        P3 = adf::input_plio::create("P3", adf::plio_32_bits, "data/P3.txt");
        P4 = adf::input_plio::create("P4", adf::plio_32_bits, "data/P4.txt");
        P5 = adf::input_plio::create("P5", adf::plio_32_bits, "data/P5.txt");
        P6 = adf::input_plio::create("P6", adf::plio_32_bits, "data/P6.txt");
        C = adf::output_plio::create("C", adf::plio_32_bits, "data/C.txt");

        adf::connect<>(P1.out[0], interp.P1);
        adf::connect<>(P2.out[0], interp.P2);
        adf::connect<>(P3.out[0], interp.P3);
        adf::connect<>(P4.out[0], interp.P4);
        adf::connect<>(P5.out[0], interp.P5);
        adf::connect<>(P6.out[0], interp.P6);
        adf::connect<>(interp.C, C.in[0]);
    }
};

} // namespace L3
} // namespace us

us::L3::scanline scan;

// TEST MAIN (EMULATION AIE)
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    scan.init();

    scan.run(1);

    scan.end();

    return 0;
}
#endif