/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include <adf.h>
#include <array>
#include <type_traits>

#include "kernels.h"

// extern uint64_t wts[256];

using namespace adf;

/*
 * computing scalefactors
 */
template <int FBITS>
uint32_t compute_scalefactor(int M, int N) {
    float x_scale = (float)M / (float)N;
    float scale = x_scale * (1 << FBITS);
    return (uint32_t)(std::roundf(scale));
}
template <int FBITS>
float compute_scalefactor_f(int M, int N) {
    float x_scale = (float)M / (float)N;
    return (x_scale);
}
template <int depth = 256>
class WtsArray {
   public:
    std::array<uint32_t, depth> arr;
    // int  WtsArray()
    // {
    // int depth = 256;
    float y, Wy1, Wy2, Wy3, Wy4;
    float Wy1t, Wy2t, Wy3t, Wy4t;
    uint16_t Wy1_i, Wy2_i, Wy3_i, Wy4_i;
    uint32_t arr_elem;
    const float A = -0.75f;
    FILE* fp = fopen("wts.txt", "w");
    constexpr WtsArray() : arr() {
        for (int r = 0; r < 4096; r++) {
            y = (1.0 * r) / (4096 - 1);
            // Wy1 = (((A*(y + 1) - 5*A)*(y + 1) + 8*A)*(y + 1) - 4*A)*(depth/2 - 1)+0.5;
            // Wy2 = (((A + 2)*y - (A + 3))*y*y + 1)*(depth/2 - 1)+0.5;
            // Wy3 = (((A + 2)*(1 - y) - (A + 3))*(1 - y)*(1 - y) + 1)*(depth/2 - 1)+0.5;
            // Wy4 = ((depth/2 - 1) - Wy1 - Wy2 - Wy3)+1.5+0.5;
            // Wy1 = (((A*(y + 1) - 5*A)*(y + 1) + 8*A)*(y + 1) - 4*A)*(depth/2);
            // Wy2 = (((A + 2)*y - (A + 3))*y*y + 1)*(depth/2 - 1);
            // Wy3 = (((A + 2)*(1 - y) - (A + 3))*(1 - y)*(1 - y) + 1)*(depth/2);
            // Wy4 = ((depth/2) - Wy1 - Wy2 - Wy3);
            // Wy1 = (((A*(y + 1) - 5*A)*(y + 1) + 8*A)*(y + 1) - 4*A)*(depth/2);
            // Wy2 = (((A + 2)*y - (A + 3))*y*y + 1)*(depth/2-1);
            // Wy3 = (((A + 2)*(1 - y) - (A + 3))*(1 - y)*(1 - y) + 1)*(depth/2);
            // Wy4 = ((depth/2 - 1) - Wy1 - Wy2 - Wy3);
            Wy1t = (((A * (y + 1) - 5 * A) * (y + 1) + 8 * A) * (y + 1) - 4 * A) * (32768);
            Wy1 = roundf(Wy1t);
            // std::cout <<"Testing round" << " 0.5 = " << roundf(0.5) << " 0.49 = " << roundf(0.49) << " 0.51 = " <<
            // roundf(0.51) << std::endl;
            // std::cout <<"Testing round" << " -0.5 = " << roundf(-0.5) << " -0.49 = " << roundf(-0.49) << " -0.51 = "
            // << roundf(-0.51) << std::endl;
            Wy2t = (((A + 2) * y - (A + 3)) * y * y + 1) * (32768);
            Wy2 = roundf(Wy2t);
            Wy3t = (((A + 2) * (1 - y) - (A + 3)) * (1 - y) * (1 - y) + 1) * (32768);
            if (Wy2 > Wy2t) {
                Wy3 = Wy3t;
            } else {
                Wy3 = roundf(Wy3t);
            }
            // Wy3 = roundf(Wy3t);
            // Wy3 = Wy3t;
            // Wy4 = ((depth/2 - 1) - Wy1 - Wy2 - Wy3)+1.5+0.5;
            Wy1_i = (uint16_t)Wy1;
            // Wy2_i = (uint8_t)Wy2;
            // Wy3_i = (uint8_t)Wy3;
            // Wy4_i = (uint8_t)Wy4;
            // Wy4_i = (uint8_t)(depth/2 - Wy1 - Wy2 - Wy3 + 1.0);
            Wy2_i = ((Wy2) >= 32768) ? (32768 - 1) : (uint16_t)(Wy2);
            Wy3_i = ((Wy3) >= 32768) ? (32768 - 1) : (uint16_t)(Wy3);
            Wy4_i = (uint16_t)(32768 - Wy1_i - Wy2_i - Wy3_i);
            // uint8_t Wy4_it;
            // Wy4_it = (uint8_t)(depth/2 - Wy1_i - Wy2_i - Wy3_i);
            // Wy4_i = (((Wy2) >= depth/2) |((Wy3) >= depth/2)) ? 0 : Wy4_it;
            // Wy4_i = (uint8_t)(roundf(depth/2 - Wy1t - Wy2t - Wy3t));
            // std::cout << y << ": " << Wy1t << "\t" << Wy2t << "\t" << Wy3t << "\t" << Wy4t << "\t" << std::endl;
            // std::cout << y << ": " << (int)Wy1_i << "\t" << (int)Wy2_i << "\t" << (int)Wy3_i << "\t" << (int)Wy4_i <<
            // "\t" << std::endl;
            // std::cout << "Wy1: " << Wy1 << std::endl;
            // std::cout << "Wy2: " << Wy2 << std::endl;
            // std::cout << "Wy3: " << Wy3 << std::endl;
            // std::cout << "Wy4: " << Wy4 << std::endl;
            // std::cout << "Wy1_i: " << Wy1_i << std::endl;
            // std::cout << "Wy2_i: " << Wy2_i << std::endl;
            // std::cout << "Wy3_i: " << Wy3_i << std::endl;
            // std::cout << "Wy4_i: " << Wy4_i << std::endl;
            fprintf(fp, "%f %f %f %f\n", y, (((A * (y + 1) - 5 * A) * (y + 1) + 8 * A) * (y + 1) - 4 * A),
                    (((A + 2) * y - (A + 3)) * y * y + 1), (((A + 2) * (1 - y) - (A + 3)) * (1 - y) * (1 - y) + 1));
            fprintf(fp, "%d %d %d %d %d\n", r, Wy1_i, Wy2_i, Wy3_i, Wy4_i);
            arr_elem = (Wy1_i << 16) | Wy2_i;
            arr[r * 2] = arr_elem;
            arr_elem = (Wy3_i << 16) | Wy4_i;
            arr[r * 2 + 1] = arr_elem;
        }
        //		return 0;
        //	}
    }
};

// int temp_val = WtsArray();
// std::array<unsigned long int, 256> wts;
const auto wtsY = WtsArray<LUT_DEPTH>();

class resizeGraph : public adf::graph {
   private:
    kernel k[NO_CORES];

   public:
    input_gmio in1;
    output_gmio out1;

    port<input> channels[NO_CORES];
    port<input> scaley[NO_CORES];
    port<input> scaley_f[NO_CORES];
    port<input> img_h_in[NO_CORES];
    port<input> img_h_out[NO_CORES];

    shared_buffer<DATA_TYPE> mtx_in1;
    shared_buffer<DATA_TYPE> mtx_out1;

    resizeGraph(int tile_col, int tile_row, int CORE_IDX) {
        int mt_col = tile_col;

        std::stringstream ssi;
        ssi << "DataIn" << (0 + CORE_IDX);
        in1 = input_gmio::create(ssi.str().c_str(), 256, 1000);

        std::stringstream sso;
        sso << "DataOut" << (0 + CORE_IDX);
        out1 = output_gmio::create(sso.str().c_str(), 256, 1000);

        // Create Memtile blocks
        mtx_in1 = shared_buffer<DATA_TYPE>::create({TILE_WINDOW_SIZE_IN * NO_CORES}, 1, NO_CORES);
        mtx_out1 = shared_buffer<DATA_TYPE>::create({TILE_WINDOW_SIZE_OUT * NO_CORES}, (2 * NO_CORES), 1);
        num_buffers(mtx_in1) = 2;
        num_buffers(mtx_out1) = 2;
        location<buffer>(mtx_in1) = {address(mt_col, 0, 0), address(mt_col, 0, (TILE_WINDOW_SIZE_IN * NO_CORES * 4))};
        location<buffer>(mtx_out1) = {address(mt_col, 0, (TILE_WINDOW_SIZE_IN * NO_CORES * 8)),
                                      address(mt_col, 0, (TILE_WINDOW_SIZE_IN * NO_CORES * 12))};
        // plio ports to mem-tile1
        connect<stream>(in1.out[0], mtx_in1.in[0]);
        write_access(mtx_in1.in[0]) = buffer_descriptor((TILE_WINDOW_SIZE_IN * NO_CORES) / 2, 0, {1}, {});
        connect<stream>(mtx_out1.out[0], out1.in[0]);
        read_access(mtx_out1.out[0]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT * NO_CORES) / 2, 0, {1}, {});

        for (int i = 0; i < NO_CORES; i++) {
            k[i] = kernel::create_object<ResizeRunner>(wtsY.arr);

            connect<>(mtx_in1.out[i], k[i].in[0]);
            adf::dimensions(k[i].in[0]) = {TILE_WINDOW_SIZE_IN};
            read_access(mtx_in1.out[i]) =
                buffer_descriptor(((TILE_WINDOW_SIZE_IN) / 2), (i * (TILE_WINDOW_SIZE_IN / 2)), {1}, {});

            connect<>(k[i].out[0], mtx_out1.in[i * 2]);
            adf::dimensions(k[i].out[0]) = {METADATA_SIZE};
            write_access(mtx_out1.in[i * 2]) =
                buffer_descriptor(METADATA_SIZE / 2, (i * TILE_WINDOW_SIZE_OUT / 2), {1}, {});

            connect<>(k[i].out[1], mtx_out1.in[i * 2 + 1]);
            adf::dimensions(k[i].out[1]) = {TILE_ELEMENTS_OUT};
            write_access(mtx_out1.in[i * 2 + 1]) = buffer_descriptor(
                (TILE_WINDOW_SIZE_OUT - METADATA_SIZE) / 2, ((i * TILE_WINDOW_SIZE_OUT / 2) + (METADATA_SIZE / 2)),
                {1, TILE_HEIGHT_OUT * CHANNELS / 2, 2}, {2, TILE_WIDTH_OUT});
            // rtps
            connect<parameter>(channels[i], async(k[i].in[1]));
            connect<parameter>(scaley[i], async(k[i].in[2]));
            connect<parameter>(img_h_in[i], async(k[i].in[3]));
            connect<parameter>(img_h_out[i], async(k[i].in[4]));
            connect<parameter>(scaley_f[i], async(k[i].in[5]));
            // lut
            // auto wts_buff = parameter::array(wts);
            // connect<>(wts_buff, k[i]);

            // specify kernel sources
            source(k[i]) = "xf_resize.cc";
            location<kernel>(k[i]) = tile(tile_col, i);
            // location<buffer>(k[i].in[0] ) = {address(tile_col, i, 0), address(tile_col, i, 16384)};
            // location<buffer>(k[i].out[0] ) = {address(tile_col, i, (TILE_WINDOW_SIZE_IN*2)), address(tile_col,
            // i,(16384+(TILE_WINDOW_SIZE_IN*2)))};
            // location<buffer>(k[i].out[1] ) = {address(tile_col, i, 16384*2), address(tile_col, i, 16384*3)};

            runtime<ratio>(k[i]) = 0.5;
        } // cores
    }

    void updateInputOutputSize(int width_in, int height_in, int width_out, int height_out) {
        uint32_t scale_x_fix = compute_scalefactor<16>(width_in, width_out);
        uint32_t scale_y_fix = compute_scalefactor<16>(height_in, height_out);
        float scale_y_f1 = compute_scalefactor_f<16>(height_in, height_out);

        for (int i = 0; i < NO_CORES; i++) {
            update(channels[i], CHANNELS);
            update(scaley[i], scale_y_fix);
            update(img_h_in[i], height_in);
            update(img_h_out[i], height_out);
            update(scaley_f[i], scale_y_f1);
        }
    }
};
class resizeGraph2 : public adf::graph {
   private:
    kernel k[NO_CORES];

   public:
    input_gmio in1;
    output_gmio out1;

    port<input> channels[NO_CORES];
    port<input> scaley[NO_CORES];
    port<input> scaley_f[NO_CORES];
    port<input> img_h_in[NO_CORES];
    port<input> img_h_out[NO_CORES];

    shared_buffer<DATA_TYPE> mtx_in1;
    shared_buffer<DATA_TYPE> mtx_out1;

    resizeGraph2(int tile_col, int tile_row, int CORE_IDX) {
        int mt_col = tile_col;
        std::stringstream ssi;
        ssi << "DataIn" << (0 + CORE_IDX);
        in1 = input_gmio::create(ssi.str().c_str(), 256, 1000);

        std::stringstream sso;
        sso << "DataOut" << (0 + CORE_IDX);
        out1 = output_gmio::create(sso.str().c_str(), 256, 1000);

        // Create Memtile blocks
        mtx_in1 = shared_buffer<DATA_TYPE>::create({TILE_WINDOW_SIZE_IN2 * NO_CORES}, 1, NO_CORES);
        mtx_out1 = shared_buffer<DATA_TYPE>::create({TILE_WINDOW_SIZE_OUT2 * NO_CORES}, (2 * NO_CORES), 1);
        num_buffers(mtx_in1) = 2;
        num_buffers(mtx_out1) = 2;

        location<buffer>(mtx_in1) = {address(mt_col, 1, 0), address(mt_col, 1, (TILE_WINDOW_SIZE_IN * NO_CORES * 4))};
        location<buffer>(mtx_out1) = {address(mt_col, 1, (TILE_WINDOW_SIZE_IN * NO_CORES * 8)),
                                      address(mt_col, 1, (TILE_WINDOW_SIZE_IN * NO_CORES * 12))};

        // plio ports to mem-tile1
        connect<stream>(in1.out[0], mtx_in1.in[0]);
        write_access(mtx_in1.in[0]) = buffer_descriptor((TILE_WINDOW_SIZE_IN2 * NO_CORES) / 2, 0, {1}, {});

        connect<stream>(mtx_out1.out[0], out1.in[0]);
        read_access(mtx_out1.out[0]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT2 * NO_CORES) / 2, 0, {1}, {});

        for (int i = 0; i < NO_CORES; i++) {
            k[i] = kernel::create_object<ResizeRunner>(wtsY.arr);

            connect<>(mtx_in1.out[i], k[i].in[0]);
            adf::dimensions(k[i].in[0]) = {TILE_WINDOW_SIZE_IN2};
            read_access(mtx_in1.out[i]) =
                buffer_descriptor(((TILE_WINDOW_SIZE_IN2) / 2), (i * (TILE_WINDOW_SIZE_IN2 / 2)), {1}, {});

            connect<>(k[i].out[0], mtx_out1.in[i * 2]);
            adf::dimensions(k[i].out[0]) = {METADATA_SIZE};
            write_access(mtx_out1.in[i * 2]) =
                buffer_descriptor(METADATA_SIZE / 2, (i * TILE_WINDOW_SIZE_OUT2 / 2), {1}, {});

            connect<>(k[i].out[1], mtx_out1.in[i * 2 + 1]);
            adf::dimensions(k[i].out[1]) = {TILE_ELEMENTS_OUT2};
            write_access(mtx_out1.in[i * 2 + 1]) = buffer_descriptor(
                (TILE_WINDOW_SIZE_OUT2 - METADATA_SIZE) / 2, ((i * TILE_WINDOW_SIZE_OUT2 / 2) + (METADATA_SIZE / 2)),
                {1, TILE_HEIGHT_OUT2 * CHANNELS / 2, 2}, {2, TILE_WIDTH_OUT2});
            // rtps
            connect<parameter>(channels[i], async(k[i].in[1]));
            connect<parameter>(scaley[i], async(k[i].in[2]));
            connect<parameter>(img_h_in[i], async(k[i].in[3]));
            connect<parameter>(img_h_out[i], async(k[i].in[4]));
            connect<parameter>(scaley_f[i], async(k[i].in[5]));
            // lut
            // auto wts_buff = parameter::array(wts);
            // connect<>(wts_buff, k[i]);

            // specify kernel sources
            source(k[i]) = "xf_resize.cc";
            location<kernel>(k[i]) = tile(tile_col, i + 3);
            // location<buffer>(k[i].in[0] ) = {address(tile_col, i+3, 0), address(tile_col, i+3, 16384)};
            // location<buffer>(k[i].out[0] ) = {address(tile_col, i+3, (TILE_WINDOW_SIZE_IN2*2)), address(tile_col,
            // i+3,(16384+(TILE_WINDOW_SIZE_IN2*2)))};
            // location<buffer>(k[i].out[1] ) = {address(tile_col, i+3, 16384*2), address(tile_col, i+3, 16384*3)};

            runtime<ratio>(k[i]) = 0.5;
        } // cores
    }

    void updateInputOutputSize(int width_in, int height_in, int width_out, int height_out) {
        uint32_t scale_x_fix = compute_scalefactor<16>(width_in, width_out);
        uint32_t scale_y_fix = compute_scalefactor<16>(height_in, height_out);
        float scale_y_f1 = compute_scalefactor_f<16>(height_in, height_out);

        for (int i = 0; i < NO_CORES; i++) {
            update(channels[i], CHANNELS);
            update(scaley[i], scale_y_fix);
            update(img_h_in[i], height_in);
            update(img_h_out[i], height_out);
            update(scaley_f[i], scale_y_f1);
        }
    }
};
#endif
