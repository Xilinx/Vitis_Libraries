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
#define CV_PII 3.1415926535897932384626433832795

static inline void interpolateLanczos4( float x, float* coeffs )
{
    static const double s45 = 0.70710678118654752440084436210485;
    static const double cs[][2]=
    {{1, 0}, {-s45, -s45}, {0, 1}, {s45, -s45}, {-1, 0}, {s45, s45}, {0, -1}, {-s45, s45}};

    float sum = 0;
    double y0=-(x+3)*CV_PII*0.25, s0 = std::sin(y0), c0= std::cos(y0);
    for(int i = 0; i < 8; i++ )
    {
        float y0_ = (x+3-i);
        if (fabs(y0_) >= 1e-6f)
        {
            double y = -y0_*CV_PII*0.25;
            coeffs[i] = (float)((cs[i][0]*s0 + cs[i][1]*c0)/(y*y));
        }
        else
        {
            // special handling for 'x' values:
            // - ~0.0: 0 0 0 1 0 0 0 0
            // - ~1.0: 0 0 0 0 1 0 0 0
            coeffs[i] = 1e30f;
        }
        sum += coeffs[i];
    }

    sum = 1.f/sum;
    for(int i = 0; i < 8; i++ )
        coeffs[i] *= sum;
}
void convert2fix(float* coeffs, uint16_t* coeff_fix, int shift)
{
 float coeffs_round[8];
 for(int i=0; i<8; i++)
 {

   coeffs_round[i] = roundf((coeffs[i] *(1<<shift)));
   coeff_fix[i]=(uint16_t)coeffs_round[i];
 }
}
template <int depth = 512>
class WtsArray {
   public:
    std::array<uint32_t, depth> arr;
    uint32_t arr_elem;
    uint32_t arr_elem1, arr_elem2, arr_elem3;
    float x;
    float cbuf[8];
    uint16_t cbuf_fix[8];
    int shift=15;
    FILE *fp=fopen("wts.txt","w");
    constexpr WtsArray() : arr() {
        for (int r = 0; r < 256; r++) {
            x = (1.0 * r) / (256-1);
	    interpolateLanczos4(x, cbuf);
	    convert2fix(cbuf, cbuf_fix, shift);
            /*arr_elem  = (cbuf_fix[0] << 24) | (cbuf_fix[1] << 16) | (cbuf_fix[2] << 8) | cbuf_fix[3];
            arr_elem1 = (cbuf_fix[4] << 24) | (cbuf_fix[5] << 16) | (cbuf_fix[6] << 8) | cbuf_fix[7];
            arr[r*2] = arr_elem;
            arr[r*2+1] = arr_elem1;*/
            arr_elem  =  (cbuf_fix[0] << 16) | cbuf_fix[1];
            arr_elem1 =  (cbuf_fix[2] << 16) | cbuf_fix[3];
            arr_elem2 =  (cbuf_fix[4] << 16) | cbuf_fix[5]; 
	        arr_elem3 =	 (cbuf_fix[6] << 16) | cbuf_fix[7];
            arr[r*4] =  arr_elem;
            arr[r*4+1] = arr_elem1;
            arr[r*4+2] = arr_elem2;
            arr[r*4+3] = arr_elem3;
	    /*if(r==1536){
		cout<< "x=" << x << endl;
		interpolateLanczos4(0.375, cbuf);
            fprintf(fp, "%f %f %f %f %f %f %f %f\n", cbuf[0],  cbuf[1], cbuf[2], cbuf[3],  cbuf[4], cbuf[5], cbuf[6], cbuf[7]);
		interpolateLanczos4(-0.375, cbuf);
           fprintf(fp, "%f %f %f %f %f %f %f %f\n", cbuf[0],  cbuf[1], cbuf[2], cbuf[3],  cbuf[4], cbuf[5], cbuf[6], cbuf[7]);
        */
            fprintf(fp, "%d %d %d %d %d %d %d %d\n", cbuf_fix[0],  cbuf_fix[1], cbuf_fix[2], cbuf_fix[3],  cbuf_fix[4], cbuf_fix[5], cbuf_fix[6], cbuf_fix[7]);
        //    fprintf(fp, "%d %d \n", arr_elem,  arr_elem1);
	    //fprintf(fp, "%d %d \n", arr[r*2],  arr[r*2+1]);
		/*cout<< "arr_elem=" << arr_elem<< endl;
		cout<< "arr_elem1=" << arr_elem1<< endl;
		cout<< "arr_elem2=" << arr_elem2<< endl;
		cout<< "arr_elem3=" << arr_elem3<< endl;
	   }*/
        }
    }
};

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
        write_access(mtx_in1.in[0]) = buffer_descriptor((TILE_WINDOW_SIZE_IN * NO_CORES) / 4, 0, {1}, {});
        connect<stream>(mtx_out1.out[0], out1.in[0]);
        read_access(mtx_out1.out[0]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT * NO_CORES) / 4, 0, {1}, {});

        for (int i = 0; i < NO_CORES; i++) {
            k[i] = kernel::create_object<ResizeRunner>(wtsY.arr);

            connect<>(mtx_in1.out[i], k[i].in[0]);
            adf::dimensions(k[i].in[0]) = {TILE_WINDOW_SIZE_IN};
            read_access(mtx_in1.out[i]) =
                buffer_descriptor(((TILE_WINDOW_SIZE_IN) / 4), (i * (TILE_WINDOW_SIZE_IN / 4)), {1}, {});

            connect<>(k[i].out[0], mtx_out1.in[i * 2]);
            adf::dimensions(k[i].out[0]) = {METADATA_SIZE};
            write_access(mtx_out1.in[i * 2]) =
                buffer_descriptor(METADATA_SIZE / 4, (i * TILE_WINDOW_SIZE_OUT / 4), {1}, {});

            connect<>(k[i].out[1], mtx_out1.in[i * 2 + 1]);
            adf::dimensions(k[i].out[1]) = {TILE_ELEMENTS_OUT};
            write_access(mtx_out1.in[i * 2 + 1]) = buffer_descriptor(
                (TILE_WINDOW_SIZE_OUT - METADATA_SIZE) / 4, ((i * TILE_WINDOW_SIZE_OUT / 4) + (METADATA_SIZE / 4)),
                {1, TILE_HEIGHT_OUT * CHANNELS / 4, 1}, {1, TILE_WIDTH_OUT});
            // rtps
            connect<parameter>(channels[i], async(k[i].in[1]));
            connect<parameter>(scaley[i], async(k[i].in[2]));
            connect<parameter>(img_h_in[i], async(k[i].in[3]));
            connect<parameter>(img_h_out[i], async(k[i].in[4]));
            connect<parameter>(scaley_f[i], async(k[i].in[5]));
            // specify kernel sources
            source(k[i]) = "xf_resize.cc";
            location<kernel>(k[i]) = tile(tile_col, i);

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
        write_access(mtx_in1.in[0]) = buffer_descriptor((TILE_WINDOW_SIZE_IN2 * NO_CORES) / 4, 0, {1}, {});

        connect<stream>(mtx_out1.out[0], out1.in[0]);
        read_access(mtx_out1.out[0]) = buffer_descriptor((TILE_WINDOW_SIZE_OUT2 * NO_CORES) / 4, 0, {1}, {});

        for (int i = 0; i < NO_CORES; i++) {
            k[i] = kernel::create_object<ResizeRunner>(wtsY.arr);

            connect<>(mtx_in1.out[i], k[i].in[0]);
            adf::dimensions(k[i].in[0]) = {TILE_WINDOW_SIZE_IN2};
            read_access(mtx_in1.out[i]) =
                buffer_descriptor(((TILE_WINDOW_SIZE_IN2) / 4), (i * (TILE_WINDOW_SIZE_IN2 / 4)), {1}, {});

            connect<>(k[i].out[0], mtx_out1.in[i * 2]);
            adf::dimensions(k[i].out[0]) = {METADATA_SIZE};
            write_access(mtx_out1.in[i * 2]) =
                buffer_descriptor(METADATA_SIZE / 4, (i * TILE_WINDOW_SIZE_OUT2 / 4), {1}, {});

            connect<>(k[i].out[1], mtx_out1.in[i * 2 + 1]);
            adf::dimensions(k[i].out[1]) = {TILE_ELEMENTS_OUT2};
            write_access(mtx_out1.in[i * 2 + 1]) = buffer_descriptor(
                (TILE_WINDOW_SIZE_OUT2 - METADATA_SIZE) / 4, ((i * TILE_WINDOW_SIZE_OUT2 / 4) + (METADATA_SIZE / 4)),
                {1, TILE_HEIGHT_OUT2 * CHANNELS / 4, 1}, {1, TILE_WIDTH_OUT2});
            // rtps
            connect<parameter>(channels[i], async(k[i].in[1]));
            connect<parameter>(scaley[i], async(k[i].in[2]));
            connect<parameter>(img_h_in[i], async(k[i].in[3]));
            connect<parameter>(img_h_out[i], async(k[i].in[4]));
            connect<parameter>(scaley_f[i], async(k[i].in[5]));
            // specify kernel sources
            source(k[i]) = "xf_resize.cc";
            location<kernel>(k[i]) = tile(tile_col, i + 3);

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
