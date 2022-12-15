/*
 * Copyright 2022 Xilinx, Inc.
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

/////////////////INPUTS
// IMAGE POINTS
adf::PLIO* start_positions = new adf::PLIO("start_positions", adf::plio_32_bits, "data/start_positions.txt");
adf::PLIO* directions = new adf::PLIO("directions", adf::plio_32_bits, "data/directions.txt");
adf::PLIO* samples_arange = new adf::PLIO("samples_arange", adf::plio_32_bits, "data/samples_arange.txt");
// DELAY
adf::PLIO* image_points_from_PL = new adf::PLIO("image_points_from_PL", adf::plio_32_bits, "data/image_points.txt");
adf::PLIO* image_points_from_PL_2 = new adf::PLIO("image_points_from_PL_2", adf::plio_32_bits, "data/image_points.txt");
adf::PLIO* tx_def_delay_distance =
    new adf::PLIO("tx_def_delay_distance", adf::plio_32_bits, "data/tx_def_delay_distance.txt");
adf::PLIO* tx_def_delay_distance_2 =
    new adf::PLIO("tx_def_delay_distance_2", adf::plio_32_bits, "data/tx_def_delay_distance.txt");
adf::PLIO* tx_def_ref_point = new adf::PLIO("tx_def_ref_point", adf::plio_32_bits, "data/tx_def_ref_point.txt");
adf::PLIO* tx_def_focal_point = new adf::PLIO("tx_def_focal_point", adf::plio_32_bits, "data/tx_def_focal_point.txt");
adf::PLIO* t_start = new adf::PLIO("t_start", adf::plio_32_bits, "data/t_start.txt");
// FOCUSING
adf::PLIO* apo_ref_0 = new adf::PLIO("apo_ref_0", adf::plio_32_bits, "data/apo_ref_0.txt");
adf::PLIO* xdc_def_0 = new adf::PLIO("xdc_def_0", adf::plio_32_bits, "data/xdc_def_0.txt");
adf::PLIO* apo_ref_1 = new adf::PLIO("apo_ref_1", adf::plio_32_bits, "data/apo_ref_1.txt");
adf::PLIO* xdc_def_1 = new adf::PLIO("xdc_def_1", adf::plio_32_bits, "data/xdc_def_1.txt");
////SAMPLES
adf::PLIO* image_points_from_PL_3 = new adf::PLIO("image_points_from_PL_3", adf::plio_32_bits, "data/image_points.txt");
adf::PLIO* delay = new adf::PLIO("delay_from_PL", adf::plio_32_bits, "data/delay_from_PL.txt");
adf::PLIO* xdc_def_positions = new adf::PLIO("xdc_def_positions", adf::plio_32_bits, "data/xdc_def_positions.txt");
adf::PLIO* sampling_frequency = new adf::PLIO("sampling_frequency", adf::plio_32_bits, "data/sampling_frequency.txt");
////APODIZATION
adf::PLIO* image_points_apo = new adf::PLIO("image_points_from_PL_4", adf::plio_32_bits, "data/image_points.txt");
adf::PLIO* apodization_reference =
    new adf::PLIO("apodization_reference", adf::plio_32_bits, "data/apodization_reference.txt");
adf::PLIO* apo_distance_k = new adf::PLIO("apo_distance_k", adf::plio_32_bits, "data/apo_distance_k.txt");
adf::PLIO* F_number = new adf::PLIO("F_number", adf::plio_32_bits, "data/F_number.txt");
////INTERPOLATOR
adf::PLIO* P1 = new adf::PLIO("P1", adf::plio_32_bits, "data/P1.txt");
adf::PLIO* P2 = new adf::PLIO("P2", adf::plio_32_bits, "data/P2.txt");
adf::PLIO* P3 = new adf::PLIO("P3", adf::plio_32_bits, "data/P3.txt");
adf::PLIO* P4 = new adf::PLIO("P4", adf::plio_32_bits, "data/P4.txt");
adf::PLIO* P5 = new adf::PLIO("P5", adf::plio_32_bits, "data/P5.txt");
adf::PLIO* P6 = new adf::PLIO("P6", adf::plio_32_bits, "data/P6.txt");

/////////////////OUTPUTS
// IMAGE POINTS
adf::PLIO* image_points_output = new adf::PLIO("image_points", adf::plio_32_bits, "data/image_points.txt");
// DELAY
adf::PLIO* delay_to_PL = new adf::PLIO("delay_to_PL", adf::plio_32_bits, "data/delay_to_PL.txt");
// FOCUSING
adf::PLIO* focusing_output = new adf::PLIO("focusing_output", adf::plio_32_bits, "data/focusing_output.txt");
////SAMPLES
adf::PLIO* samples_to_PL = new adf::PLIO("samples_to_PL", adf::plio_32_bits, "data/samples_to_PL.txt");
////APODIZATION
adf::PLIO* apodization_output = new adf::PLIO("apodization", adf::plio_32_bits, "data/apodization.txt");
////INTERPOLATOR
adf::PLIO* C = new adf::PLIO("C", adf::plio_32_bits, "data/C.txt");

// GRAPH DECLARATIONS
us::L2::imagePoints<> img;
us::L2::delay<> d;
us::L2::focusing<> foc;
us::L2::samples<> sam;
us::L2::apodization<> apo;
us::L2::bSpline<> interp;

// GRAPH I/O DECLARATION
adf::simulation::platform<28, 6> platform(
    // INPUTS
    // IMAGE POINTS
    start_positions,
    directions,
    samples_arange,
    // DELAY
    image_points_from_PL,
    image_points_from_PL_2,
    tx_def_delay_distance,
    tx_def_delay_distance_2,
    tx_def_focal_point,
    tx_def_ref_point,
    t_start,
    //										//FOCUSING
    apo_ref_0,
    xdc_def_0,
    apo_ref_1,
    xdc_def_1,
    //										//SAMPLES
    image_points_from_PL_3,
    delay,
    xdc_def_positions,
    sampling_frequency,
    ////									//APODIZATION
    image_points_apo,
    apodization_reference,
    apo_distance_k,
    F_number,
    ////									//INTERPOLATOR
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,

    // OUTPUTS
    // IMAGE POINTS
    image_points_output,
    //										//DELAY
    delay_to_PL,
    //										//FOCUSING
    focusing_output,
    ////									//SAMPLES
    samples_to_PL,
    ////									//APODIZATION
    apodization_output,
    ////									//INTERPOLATOR
    C);

// GRAPH I/O SPECIFICATION
// INPUT
// IMAGE POINTS
adf::connect<> src_start_positions(platform.src[0], img.start_positions);
adf::connect<> src_directions(platform.src[1], img.directions);
adf::connect<> src_samples_arange(platform.src[2], img.samples_arange);
////DELAY
adf::connect<> src_image_points_delay(platform.src[3], d.image_points_from_PL);
adf::connect<> src_delay_ref_1(platform.src[4], d.image_points_from_PL_);
adf::connect<> src_delay_ref_2(platform.src[5], d.tx_def_delay_distance);
adf::connect<> src_delay_ref_3(platform.src[6], d.tx_def_delay_distance_);
adf::connect<> src_delay_ref_4(platform.src[7], d.tx_def_focal_point);
adf::connect<> src_delay_ref_5(platform.src[8], d.tx_def_reference_point);
adf::connect<> src_t_start(platform.src[9], d.t_start);
////FOCUSING
adf::connect<> input_apo_0(platform.src[10], foc.apo_ref_0);
adf::connect<> input_xdc_0(platform.src[11], foc.xdc_def_0);
adf::connect<> input_apo_1(platform.src[12], foc.apo_ref_1);
adf::connect<> input_xdc_1(platform.src[13], foc.xdc_def_1);
//////SAMPLES
adf::connect<> src_image_points_2(platform.src[14], sam.image_points_from_PL_2);
adf::connect<> delay_in(platform.src[15], sam.delay_from_PL);
adf::connect<> xdc_def_pos_in(platform.src[16], sam.xdc_def_positions);
adf::connect<> sampl_freq(platform.src[17], sam.samplingFrequency);
//////APODIZATION
adf::connect<> src_image_points_apo(platform.src[18], apo.image_points);
adf::connect<> src_apodization_reference(platform.src[19], apo.apodization_reference_i);
adf::connect<> src_apo_distance_k(platform.src[20], apo.apo_distance_k);
adf::connect<> src_F_number(platform.src[21], apo.F_number);
//////INTERPOLATOR
adf::connect<> in_P1(platform.src[22], interp.P1);
adf::connect<> in_P2(platform.src[23], interp.P2);
adf::connect<> in_P3(platform.src[24], interp.P3);
adf::connect<> in_P4(platform.src[25], interp.P4);
adf::connect<> in_P5(platform.src[26], interp.P5);
adf::connect<> in_P6(platform.src[27], interp.P6);

// OUTPUT
// IMAGE POINTS
adf::connect<> out_image_points(img.image_points, platform.sink[0]);
////DELAY
adf::connect<> out_delay(d.delay_to_PL, platform.sink[1]);
////FOCUSING
adf::connect<> output_vector(foc.focusing_output, platform.sink[2]);
//////SAMPLES
adf::connect<> out_samples(sam.samples_to_PL, platform.sink[3]);
//////APODIZATION
adf::connect<> out_apodization_output(apo.apodization_output, platform.sink[4]);
//////INTERPOLATOR
adf::connect<> res_C(interp.C, platform.sink[5]);

// TEST MAIN (EMULATION AIE)
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    // INIT ALL GRAPHS
    img.init();
    d.init();
    foc.init();
    sam.init();
    apo.init();
    interp.init();

    // run all graphs
    img.run(1);
    d.run(1);
    foc.run(1);
    sam.run(1);
    apo.run(1);
    interp.run(1);

    // ENDING GRAPHS (CANNOT BE RE-INSTANTIATED)
    img.end();
    d.end();
    foc.end();
    sam.end();
    apo.end();
    interp.end();

    return 0;
}
#endif