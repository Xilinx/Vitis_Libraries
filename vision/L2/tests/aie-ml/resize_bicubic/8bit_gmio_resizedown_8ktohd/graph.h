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
    float y, Wy1, Wy2, Wy3, Wy4;
    float Wy1t, Wy2t, Wy3t, Wy4t;
    uint8_t Wy1_i, Wy2_i, Wy3_i, Wy4_i;
    uint32_t arr_elem;
    const float A = -0.75f;
    //	FILE *fp=fopen("wts.txt","w");
    constexpr WtsArray() : arr() {
        for (int r = 0; r < depth; r++) {
            y = (1.0 * r) / (depth - 1);
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
            Wy1t = (((A * (y + 1) - 5 * A) * (y + 1) + 8 * A) * (y + 1) - 4 * A) * (depth / 2);
            Wy1 = roundf(Wy1t);
            // std::cout <<"Testing round" << " 0.5 = " << roundf(0.5) << " 0.49 = " << roundf(0.49) << " 0.51 = " <<
            // roundf(0.51) << std::endl;
            // std::cout <<"Testing round" << " -0.5 = " << roundf(-0.5) << " -0.49 = " << roundf(-0.49) << " -0.51 = "
            // << roundf(-0.51) << std::endl;
            Wy2t = (((A + 2) * y - (A + 3)) * y * y + 1) * (depth / 2);
            Wy2 = roundf(Wy2t);
            Wy3t = (((A + 2) * (1 - y) - (A + 3)) * (1 - y) * (1 - y) + 1) * (depth / 2);
            if (Wy2 > Wy2t) {
                Wy3 = Wy3t;
            } else {
                Wy3 = roundf(Wy3t);
            }
            // Wy3 = roundf(Wy3t);
            // Wy3 = Wy3t;
            // Wy4 = ((depth/2 - 1) - Wy1 - Wy2 - Wy3)+1.5+0.5;
            Wy1_i = (uint8_t)Wy1;
            // Wy2_i = (uint8_t)Wy2;
            // Wy3_i = (uint8_t)Wy3;
            // Wy4_i = (uint8_t)Wy4;
            // Wy4_i = (uint8_t)(depth/2 - Wy1 - Wy2 - Wy3 + 1.0);
            Wy2_i = ((Wy2) >= depth / 2) ? (depth / 2 - 1) : (uint8_t)(Wy2);
            Wy3_i = ((Wy3) >= depth / 2) ? (depth / 2 - 1) : (uint8_t)(Wy3);
            Wy4_i = (uint8_t)(depth / 2 - Wy1_i - Wy2_i - Wy3_i);
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
            // fprintf(fp, "%f %f %f %f\n", y, (((A*(y + 1) - 5*A)*(y + 1) + 8*A)*(y + 1) - 4*A), (((A + 2)*y - (A +
            // 3))*y*y + 1), (((A + 2)*(1 - y) - (A + 3))*(1 - y)*(1 - y) + 1));
            // fprintf(fp, "%d %d %d %d %d\n", r,  Wy1_i, Wy2_i, Wy3_i, Wy4_i);
            arr_elem = (Wy1_i << 24) | (Wy2_i << 16) | (Wy3_i << 8) | Wy4_i;
            arr[r] = arr_elem;
        }
    }
};

const auto wtsY = WtsArray<LUT_DEPTH>();

class resizeGraph : public adf::graph {
   private:
    kernel k;
    kernel k1;

   public:
    input_gmio in1;
    output_gmio out1;
    output_gmio out2;

    port<input> channels;
    port<input> scalex;
    port<input> scaley;
    port<input> scaley_f;
    port<input> img_h_in;
    port<input> img_w_in;
    port<input> line_stride_in;
    port<input> img_h_out;
    port<input> img_w_out;
    port<input> tile_h_out;
    port<input> tile_w_out;
    port<input> outputStride;

    shared_buffer<DATA_TYPE> mtx_in1;
    resizeGraph() {
        k = kernel::create_object<ResizeRunner>(wtsY.arr);
        k1 = kernel::create(transpose_api);

        in1 = input_gmio::create("gmioIn", 256, 1000);
        out1 = output_gmio::create("gmioOut", 256, 1000);

        // resize
        /*         connect<>(in1.out[0], k.in[0]);
                connect<>(k.out[0], out1.in[0]);
                        adf::dimensions(k.in[0]) = {TILE_WINDOW_SIZE_IN};
                        adf::dimensions(k.out[0]) = {TILE_WINDOW_SIZE_OUT};
         */
        mtx_in1 = shared_buffer<DATA_TYPE>::create({TILE_WINDOW_SIZE_OUT}, 1, 2);
        num_buffers(mtx_in1) = 2;
        connect<>(in1.out[0], k.in[0]);
        connect<>(k.out[0], mtx_in1.in[0]);
        adf::dimensions(k.in[0]) = {TILE_WINDOW_SIZE_IN};
        adf::dimensions(k.out[0]) = {TILE_WINDOW_SIZE_OUT};

        write_access(mtx_in1.in[0]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT) / 4, 0, {1}, {});
        read_access(mtx_in1.out[0]) = buffer_descriptor(METADATA_SIZE / 4, 0, {1}, {});
        read_access(mtx_in1.out[1]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT - METADATA_SIZE) / 4, METADATA_SIZE / 4,
                                                        {1, TILE_WIDTH_OUT * CHANNELS / 4, 1}, {1, TILE_HEIGHT_OUT});

        connect<>(mtx_in1.out[0], k1.in[0]);
        adf::dimensions(k1.in[0]) = {METADATA_SIZE};
        connect<>(mtx_in1.out[1], k1.in[1]);
        adf::dimensions(k1.in[1]) = {TILE_ELEMENTS_OUT};

        connect<>(k1.out[0], out1.in[0]);
        adf::dimensions(k1.out[0]) = {TILE_WINDOW_SIZE_OUT};

        connect<parameter>(channels, async(k.in[1]));
        connect<parameter>(scalex, async(k.in[2]));
        connect<parameter>(scaley, async(k.in[3]));
        connect<parameter>(img_h_in, async(k.in[4]));
        connect<parameter>(img_h_out, async(k.in[5]));
        connect<parameter>(tile_h_out, async(k.in[6]));
        connect<parameter>(tile_w_out, async(k.in[7]));
        connect<parameter>(line_stride_in, async(k.in[8]));
        connect<parameter>(img_w_out, async(k.in[9]));
        connect<parameter>(scaley_f, async(k.in[10]));

        connect<parameter>(outputStride, async(k1.in[2]));
        // specify kernel sources
        source(k) = "xf_resize.cc";
        source(k1) = "xf_transpose.cc";

        runtime<ratio>(k) = 0.5;
        runtime<ratio>(k1) = 0.5;
    }

    void updateInputOutputSize(int width_in, int height_in, int width_out, int height_out) {
        uint32_t scale_x_fix = compute_scalefactor<16>(width_in, width_out);
        uint32_t scale_y_fix = compute_scalefactor<16>(height_in, height_out);
        float scale_y_f1 = compute_scalefactor_f<16>(height_in, height_out);

        update(channels, CHANNELS);
        update(scalex, scale_x_fix);
        update(scaley, scale_y_fix);
        update(img_h_in, height_in);
        update(img_h_out, height_out);
        update(tile_h_out, TILE_HEIGHT_OUT);
        update(tile_w_out, TILE_WIDTH_OUT);
        update(line_stride_in, width_in);
        update(img_w_out, width_out);
        update(scaley_f, scale_y_f1);
    }
};
class resizeGraph2 : public adf::graph {
   private:
    kernel k;
    kernel k1;

   public:
    input_gmio in1;
    output_gmio out1;
    output_gmio out2;

    port<input> channels;
    port<input> scalex;
    port<input> scaley;
    port<input> scale_y_f2;
    port<input> img_h_in;
    port<input> img_w_in;
    port<input> line_stride_in;
    port<input> img_h_out;
    port<input> img_w_out;
    port<input> tile_h_out;
    port<input> tile_w_out;
    port<input> outputStride;

    shared_buffer<DATA_TYPE> mtx_in1;
    resizeGraph2() {
        k = kernel::create_object<ResizeRunner>(wtsY.arr);
        k1 = kernel::create(transpose_api);

        in1 = input_gmio::create("gmioIn1", 256, 1000);
        out1 = output_gmio::create("gmioOut1", 256, 1000);
        // resize
        /*          connect<>(in1.out[0], k.in[0]);
                connect<>(k.out[0], out1.in[0]);
                        adf::dimensions(k.in[0]) = {TILE_WINDOW_SIZE_IN};
                        adf::dimensions(k.out[0]) = {TILE_WINDOW_SIZE_OUT};
         */
        mtx_in1 = shared_buffer<DATA_TYPE>::create({TILE_WINDOW_SIZE_OUT2}, 1, 2);
        num_buffers(mtx_in1) = 2;
        connect<>(in1.out[0], k.in[0]);
        connect<>(k.out[0], mtx_in1.in[0]);
        adf::dimensions(k.in[0]) = {TILE_WINDOW_SIZE_IN2};
        adf::dimensions(k.out[0]) = {TILE_WINDOW_SIZE_OUT2};

        write_access(mtx_in1.in[0]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT2) / 4, 0, {1}, {});
        read_access(mtx_in1.out[0]) = buffer_descriptor(METADATA_SIZE / 4, 0, {1}, {});
        read_access(mtx_in1.out[1]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT2 - METADATA_SIZE) / 4, METADATA_SIZE / 4,
                                                        {1, TILE_WIDTH_OUT2 * CHANNELS / 4, 1}, {1, TILE_HEIGHT_OUT2});

        connect<>(mtx_in1.out[0], k1.in[0]);
        adf::dimensions(k1.in[0]) = {METADATA_SIZE};
        connect<>(mtx_in1.out[1], k1.in[1]);
        adf::dimensions(k1.in[1]) = {TILE_ELEMENTS_OUT2};

        connect<>(k1.out[0], out1.in[0]);
        adf::dimensions(k1.out[0]) = {TILE_WINDOW_SIZE_OUT2};

        connect<parameter>(channels, async(k.in[1]));
        connect<parameter>(scalex, async(k.in[2]));
        connect<parameter>(scaley, async(k.in[3]));
        connect<parameter>(img_h_in, async(k.in[4]));
        connect<parameter>(img_h_out, async(k.in[5]));
        connect<parameter>(tile_h_out, async(k.in[6]));
        connect<parameter>(tile_w_out, async(k.in[7]));
        connect<parameter>(line_stride_in, async(k.in[8]));
        connect<parameter>(img_w_out, async(k.in[9]));
        connect<parameter>(scale_y_f2, async(k.in[10]));

        connect<parameter>(outputStride, async(k1.in[2]));
        // specify kernel sources
        source(k) = "xf_resize.cc";
        source(k1) = "xf_transpose.cc";

        runtime<ratio>(k) = 0.5;
        runtime<ratio>(k1) = 0.5;
        // location constraints
        //-------------------------------
        // location<kernel>(k)    = tile(26, 0);
        // location<stack>(k)     = address(26, 0, 2048);

        /*		location<parameter>(lut[0])  = address(2, 3, 0x0);
                        location<parameter>(lut[1])  = address(2, 3, 0x4000);
                        location<parameter>(lut[2])  = address(2, 3, 0x8000);

                        location<buffer>(k.in[0] ) = {address(26, 0, 16384*3-1024*3-640), address(2, 3,
           16384*4-1024*3-640)};
                        location<buffer>(k.out[0]) = {address(26, 0, 16384*3-1024-640),   address(2, 3,
           16384*4-1024-640)};
         */
    }

    void updateInputOutputSize(int width_in, int height_in, int width_out, int height_out) {
        uint32_t scale_x_fix = compute_scalefactor<16>(width_in, width_out);
        uint32_t scale_y_fix = compute_scalefactor<16>(height_in, height_out);
        uint32_t scale_y_fix2 = compute_scalefactor<16>(height_in, height_out);
        update(channels, CHANNELS);
        update(scalex, scale_x_fix);
        update(scaley, scale_y_fix);
        update(img_h_in, height_in);
        update(img_h_out, height_out);
        update(tile_h_out, TILE_HEIGHT_OUT);
        update(tile_w_out, TILE_WIDTH_OUT);
        update(line_stride_in, width_in);
        update(img_w_out, width_out);
        update(scale_y_f2, scale_y_fix2);
    }
};
#endif
