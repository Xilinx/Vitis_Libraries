/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include "kernels.h"
#include <adf.h>
#include <sstream>
#include <string.h>
#include <type_traits>
using namespace adf;

class sobelGraph : public adf::graph {
   private:
    kernel k1[NO_CORES_SOBEL];

   public:
    input_gmio in[NO_CORES_SOBEL];
    output_gmio out[NO_CORES_SOBEL];

    shared_buffer<uint8_t> mtx_out[NO_CORES_SOBEL];

    sobelGraph() {
        for (int CORE_IDX = 0; CORE_IDX < NO_CORES_SOBEL; CORE_IDX++) {
            k1[CORE_IDX] = kernel::create_object<SobelRunner<TILE_IN_HEIGHT_SOBEL, NO_DISPARITIES, TILE_WINSIZE> >();
            std::stringstream ssi2;
            ssi2 << "A" << CORE_IDX;
            in[CORE_IDX] = input_gmio::create(ssi2.str().c_str(), 64, 1000);
            std::stringstream ssi3;
            ssi3 << "B" << CORE_IDX;
            out[CORE_IDX] = output_gmio::create(ssi3.str().c_str(), 64, 1000);

            // k1[in]
            connect<>(in[CORE_IDX].out[0], k1[CORE_IDX].in[0]);
            adf::dimensions(k1[CORE_IDX].in[0]) = {(TILE_IN_WIDTH_SOBEL * (TILE_IN_HEIGHT_SOBEL + 2))};

            // k1 output1 to MemTile
            mtx_out[CORE_IDX] =
                shared_buffer<uint8_t>::create({TILE_OUT_WIDTH_SOBEL, (TILE_OUT_HEIGHT_SOBEL + 1)}, 2, 1);

            // k1.out[0] -> metadata
            connect<>(k1[CORE_IDX].out[0], mtx_out[CORE_IDX].in[0]);
            adf::dimensions(k1[CORE_IDX].out[0]) = {64};
            buffer_descriptor_parameters mtx_in0;
            mtx_in0.length = 64 / 4;
            mtx_in0.offset = (TILE_OUT_WIDTH_SOBEL - 64) / 4;
            mtx_in0.stepsize = {1};
            mtx_in0.wrap = {};
            write_access(mtx_out[CORE_IDX].in[0]) = buffer_descriptor(mtx_in0);

            // k1.out[1]
            connect<>(k1[CORE_IDX].out[1], mtx_out[CORE_IDX].in[1]);
            adf::dimensions(k1[CORE_IDX].out[1]) = {(TILE_IN_WIDTH_SOBEL * TILE_OUT_HEIGHT_SOBEL)};
            buffer_descriptor_parameters mtx_in1;
            mtx_in1.length = ((TILE_OUT_HEIGHT_SOBEL) * (TILE_IN_WIDTH_SOBEL)) / 4;
            mtx_in1.offset = TILE_OUT_WIDTH_SOBEL / 4;
            mtx_in1.stepsize = {1, TILE_OUT_WIDTH_SOBEL / 4};
            mtx_in1.wrap = {32 / 4};
            write_access(mtx_out[CORE_IDX].in[1]) = buffer_descriptor(mtx_in1);

            // MemTile to DDR
            connect<stream>(mtx_out[CORE_IDX].out[0], out[CORE_IDX].in[0]);
            buffer_descriptor_parameters mtx_out1;
            mtx_out1.length = (((TILE_OUT_HEIGHT_SOBEL)*TILE_OUT_WIDTH_SOBEL) + 64) / 4;
            mtx_out1.offset = (TILE_OUT_WIDTH_SOBEL - 64) / 4;
            mtx_out1.stepsize = {1};
            mtx_out1.wrap = {};
            read_access(mtx_out[CORE_IDX].out[0]) = buffer_descriptor(mtx_out1);

            // specify kernel sources
            source(k1[CORE_IDX]) = "xf_sobel.cc";
            // location constraints
            location<kernel>(k1[CORE_IDX]) = tile(1, CORE_IDX + 1);
            runtime<ratio>(k1[CORE_IDX]) = 0.1;
        }
    }
};

class sbmGraph : public adf::graph {
   private:
    kernel k1[NO_CORES];

   public:
    input_gmio in_sobel_left_tile[NO_CORES];
    input_gmio in_sobel_right_tile[NO_CORES];
    output_gmio out_interp[NO_CORES];

    shared_buffer<uint8_t> mtx_in_left[NO_CORES];
    shared_buffer<uint8_t> mtx_in_right[NO_CORES];
    shared_buffer<int16_t> mtx_out[NO_CORES];

    sbmGraph() {
        for (int CORE_IDX = 0; CORE_IDX < NO_CORES; CORE_IDX++) {
            k1[CORE_IDX] =
                kernel::create_object<SbmRunner<TILE_OUT_HEIGHT, NO_DISPARITIES, TILE_WINSIZE, UNIQUENESS_RATIO,
                                                TEXTURE_THRESHOLD, FILTERED, TILE_IN_HEIGHT> >();

            repetition_count(k1[CORE_IDX]) = TILE_IN_HEIGHT;

            std::stringstream ssi1;
            ssi1 << "Right" << CORE_IDX;
            in_sobel_right_tile[CORE_IDX] = input_gmio::create(ssi1.str().c_str(), 64, 500);

            std::stringstream ssi2;
            ssi2 << "Left" << CORE_IDX;
            in_sobel_left_tile[CORE_IDX] = input_gmio::create(ssi2.str().c_str(), 64, 500);

            std::stringstream ssi3;
            ssi3 << "Out1" << CORE_IDX;
            out_interp[CORE_IDX] = output_gmio::create(ssi3.str().c_str(), 64, 500);

            mtx_in_left[CORE_IDX] =
                shared_buffer<uint8_t>::create({(TILE_IN_HEIGHT_WITH_WINDOW + 1), (LEFT_TILE_IN_WIDTH)}, 1, 3);
            mtx_in_right[CORE_IDX] =
                shared_buffer<uint8_t>::create({(TILE_IN_HEIGHT_WITH_WINDOW + 1), (RIGHT_TILE_IN_WIDTH)}, 1, 2);
            mtx_out[CORE_IDX] = shared_buffer<int16_t>::create({TILE_IN_HEIGHT, COLS_COMBINE}, 1, 1);

            location<buffer>(mtx_in_left[CORE_IDX]) = {address(CORE_IDX + 3, 0, 0)};
            location<buffer>(mtx_in_right[CORE_IDX]) = {address(CORE_IDX + 3, 1, 0)};

            location<buffer>(mtx_out[CORE_IDX]) = {
                address(CORE_IDX + 3, 0, 2 * LEFT_TILE_IN_WIDTH * (TILE_IN_HEIGHT_WITH_WINDOW + 1))};

            // Read left and right images into MemTiles
            // left image
            connect<stream>(in_sobel_left_tile[CORE_IDX].out[0], mtx_in_left[CORE_IDX].in[0]);
            buffer_descriptor_parameters mtx_left_sub0;
            mtx_left_sub0.length = (((TILE_IN_HEIGHT_WITH_WINDOW) * (LEFT_TILE_IN_WIDTH)) + 64) / 4;
            mtx_left_sub0.offset = 0;
            mtx_left_sub0.stepsize = {1};
            mtx_left_sub0.wrap = {};
            write_access(mtx_in_left[CORE_IDX].in[0]) = buffer_descriptor(mtx_left_sub0);

            // right image
            connect<stream>(in_sobel_right_tile[CORE_IDX].out[0], mtx_in_right[CORE_IDX].in[0]);
            buffer_descriptor_parameters mtx_right_sub0;
            mtx_right_sub0.length = (((TILE_IN_HEIGHT_WITH_WINDOW) * (RIGHT_TILE_IN_WIDTH)) + 64) / 4;
            mtx_right_sub0.offset = 0;
            mtx_right_sub0.stepsize = {1};
            mtx_right_sub0.wrap = {};
            write_access(mtx_in_right[CORE_IDX].in[0]) = buffer_descriptor(mtx_right_sub0);

            // K1 - input1  left_sub

            connect<>(mtx_in_left[CORE_IDX].out[0], k1[CORE_IDX].in[0]);
            adf::dimensions(k1[CORE_IDX].in[0]) = {LEFT_TILE_IN_WIDTH};
            buffer_descriptor_parameters mtx_sub_left;
            mtx_sub_left.length = (LEFT_TILE_IN_WIDTH) * (TILE_IN_HEIGHT) / 4;
            mtx_sub_left.offset = 64 / 4;
            mtx_sub_left.stepsize = {1};
            mtx_sub_left.wrap = {};
            read_access(mtx_in_left[CORE_IDX].out[0]) = buffer_descriptor(mtx_sub_left);

            // K1 - input2 left_add

            connect<>(mtx_in_left[CORE_IDX].out[1], k1[CORE_IDX].in[1]);
            adf::dimensions(k1[CORE_IDX].in[1]) = {LEFT_TILE_IN_WIDTH};
            buffer_descriptor_parameters mtx_add_left;
            mtx_add_left.length = (LEFT_TILE_IN_WIDTH) * (TILE_IN_HEIGHT) / 4;
            mtx_add_left.offset = (64 + ((TILE_WINSIZE) * (LEFT_TILE_IN_WIDTH))) / 4;
            mtx_add_left.stepsize = {1};
            mtx_add_left.wrap = {};
            read_access(mtx_in_left[CORE_IDX].out[1]) = buffer_descriptor(mtx_add_left);

            // K1 - metadata

            connect<>(mtx_in_left[CORE_IDX].out[2], k1[CORE_IDX].in[4]);
            adf::dimensions(k1[CORE_IDX].in[4]) = {64};
            buffer_descriptor_parameters mtx_meta_data;
            mtx_meta_data.length = 64 * TILE_IN_HEIGHT / 4;
            mtx_meta_data.offset = 0;
            mtx_meta_data.stepsize = {1};
            mtx_meta_data.wrap = {};
            read_access(mtx_in_left[CORE_IDX].out[2]) = buffer_descriptor(mtx_meta_data);

            // K1 - input3 right_sub

            connect<>(mtx_in_right[CORE_IDX].out[0], k1[CORE_IDX].in[2]);
            adf::dimensions(k1[CORE_IDX].in[2]) = {RIGHT_TILE_IN_WIDTH};

            buffer_descriptor_parameters mtx_sub_right;
            mtx_sub_right.length = (RIGHT_TILE_IN_WIDTH) * (TILE_IN_HEIGHT) / 4;
            mtx_sub_right.offset = 64 / 4;
            mtx_sub_right.stepsize = {1};
            mtx_sub_right.wrap = {};
            read_access(mtx_in_right[CORE_IDX].out[0]) = buffer_descriptor(mtx_sub_right);

            // K1 - input4 right_add

            connect<>(mtx_in_right[CORE_IDX].out[1], k1[CORE_IDX].in[3]);
            adf::dimensions(k1[CORE_IDX].in[3]) = {RIGHT_TILE_IN_WIDTH};

            buffer_descriptor_parameters mtx_add_right;
            mtx_add_right.length = (RIGHT_TILE_IN_WIDTH) * (TILE_IN_HEIGHT) / 4;
            mtx_add_right.offset = (((TILE_WINSIZE) * (RIGHT_TILE_IN_WIDTH)) + 64) / 4;
            mtx_add_right.stepsize = {1};
            mtx_add_right.wrap = {};
            read_access(mtx_in_right[CORE_IDX].out[1]) = buffer_descriptor(mtx_add_right);

            // k1 output1 to MemTile
            connect<>(k1[CORE_IDX].out[0], mtx_out[CORE_IDX].in[0]);
            adf::dimensions(k1[CORE_IDX].out[0]) = {COLS_COMBINE * 2 / SIZEOF_INT16};
            buffer_descriptor_parameters mtx_out2;
            mtx_out2.length = (COLS_COMBINE * 2) * (TILE_IN_HEIGHT) / 4;
            mtx_out2.offset = 0;
            mtx_out2.stepsize = {1};
            mtx_out2.wrap = {};
            write_access(mtx_out[CORE_IDX].in[0]) = buffer_descriptor(mtx_out2);

            // MemTile to DDR
            connect<stream>(mtx_out[CORE_IDX].out[0], out_interp[CORE_IDX].in[0]);
            buffer_descriptor_parameters mtx_out1;
            mtx_out1.length = (((COLS_COMBINE * 2) * (TILE_OUT_HEIGHT)) + 64) / 4;
            mtx_out1.offset = (((COLS_COMBINE * 2) * (TILE_WINSIZE - 1)) - 64) / 4;
            mtx_out1.stepsize = {1};
            mtx_out1.wrap = {};
            read_access(mtx_out[CORE_IDX].out[0]) = buffer_descriptor(mtx_out1);

            // specify kernel sources
            source(k1[CORE_IDX]) = "xf_sbm.cc";

            runtime<ratio>(k1[CORE_IDX]) = 1;

            // Location constraints
            location<kernel>(k1[CORE_IDX]) = tile(CORE_IDX + 3, 2);
        }
    }
};

#endif
