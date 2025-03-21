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

#include "graph.h"
#include <stdio.h>
#include <cstdio>
// Graph object
myGraph nv12resize_graph;
myGraph1 nv12resize_uvgraph;

void interleave_input(uint8_t* uv, uint8_t* u, uint8_t* v, int16_t height, int16_t width) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0, k = 0; j < 1920; j += 2, k++) {
            u[(i * 960) + k] = uv[(i * 1920) + j];
            v[(i * 960) + k] = uv[(i * 1920) + j + 1];
        }
    }
}
void merge_out(uint8_t* u, uint8_t* v, uint8_t* uv, int16_t height, int16_t width) {
    for (int j = 0, k = 0; j < 192; j++, k += 2) {
        uv[k] = u[j];
        uv[k + 1] = v[j];
    }
}

void run_ref(uint8_t* y, uint8_t* y_resized, int16_t height, int16_t width, int16_t debug = 0) {
    typedef uint8_t Pixel_t;
    int idx = 0;
    for (int i_r = 0; i_r < height; i_r += 5) {
        int sum = 0;
        uint8_t* r0 = (uint8_t*)(y);
        uint8_t* r1 = (uint8_t*)(y + width);
        uint8_t* r2 = (uint8_t*)(y + 2 * width);
        uint8_t* r3 = (uint8_t*)(y + 3 * width);
        uint8_t* r4 = (uint8_t*)(y + 4 * width);

        for (int i = 0; i < width; i += 5) {
            sum += r0[i] + r0[i + 1] + r0[i + 2] + r0[i + 3] + r0[i + 4];
            sum += r1[i] + r1[i + 1] + r1[i + 2] + r1[i + 3] + r1[i + 4];
            sum += r2[i] + r2[i + 1] + r2[i + 2] + r2[i + 3] + r2[i + 4];
            sum += r3[i] + r3[i + 1] + r3[i + 2] + r3[i + 3] + r3[i + 4];
            sum += r4[i] + r4[i + 1] + r4[i + 2] + r4[i + 3] + r4[i + 4];

            double temp = (double)sum / 25;
            sum = std::round(temp);
            y_resized[idx] = (uint8_t)sum; // value_r;
            idx++;
            sum = 0;
        }
    }
    return;
}

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    uint8_t* y_in = (uint8_t*)GMIO::malloc(ELEM_WITH_METADATA * sizeof(uint8_t));
    uint8_t* y_out = (uint8_t*)GMIO::malloc(ELEM_OUT_WITH_METADATA * sizeof(uint8_t));
    uint8_t* yref_out = (uint8_t*)GMIO::malloc(Y_OUT_TILE_ELEMENTS * sizeof(uint8_t));

    uint8_t* uv_in = (uint8_t*)GMIO::malloc(UV_ELEM_WITH_METADATA * sizeof(uint8_t));
    uint8_t* uv_out = (uint8_t*)GMIO::malloc(UV_ELEM_OUT_WITH_METADATA * sizeof(uint8_t));
    uint8_t* uvref_out = (uint8_t*)GMIO::malloc(UV_OUT_TILE_ELEMENTS * sizeof(uint8_t));

    memset(y_in, 0, ELEM_WITH_METADATA * sizeof(uint8_t));
    memset(uv_in, 0, UV_ELEM_WITH_METADATA * sizeof(uint8_t));

    xf::cv::aie::xfSetTileWidth(y_in, Y_IN_TILE_WIDTH);
    xf::cv::aie::xfSetTileHeight(y_in, Y_IN_TILE_HEIGHT);
    xf::cv::aie::xfSetTileWidth(uv_in, (UV_IN_TILE_WIDTH / 2));
    xf::cv::aie::xfSetTileHeight(uv_in, UV_IN_TILE_HEIGHT);

    uint8_t* dataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(y_in);
    uint8_t* UVdataIn = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(uv_in);

    //    uint8_t* dataRefOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(yref_out);
    uint8_t* dataOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(y_out);
    uint8_t* UVdataOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(uv_out);

    for (int i = 0; i < Y_IN_TILE_ELEMENTS; i++) {
        dataIn[i] = rand() % 256;
    }
    for (int i = 0; i < UV_IN_TILE_ELEMENTS; i++) {
        UVdataIn[i] = rand() % 256;
    }
    FILE* fpin = fopen("in.txt", "w");
    for (int i = 0; i < UV_IN_TILE_ELEMENTS; i++) {
        fprintf(fpin, "%d\n", UVdataIn[i]);
    }
    fclose(fpin);

    nv12resize_graph.init();
    nv12resize_graph.run(1);
    nv12resize_graph.yinptr.gm2aie_nb(y_in, ELEM_WITH_METADATA * sizeof(uint8_t));
    nv12resize_graph.youtptr.aie2gm_nb(y_out, ELEM_OUT_WITH_METADATA * sizeof(uint8_t));
    nv12resize_graph.youtptr.wait();
    nv12resize_graph.end();
    printf("after y graph wait\n");

    nv12resize_uvgraph.init();
    nv12resize_uvgraph.run(1);
    nv12resize_uvgraph.uvinptr.gm2aie_nb(uv_in, UV_ELEM_WITH_METADATA * sizeof(uint8_t));
    nv12resize_uvgraph.uvoutptr.aie2gm_nb(uv_out, UV_ELEM_OUT_WITH_METADATA * sizeof(uint8_t));
    nv12resize_uvgraph.uvoutptr.wait();
    nv12resize_uvgraph.end();
    printf("after uv graph wait\n");

    // compare the resized Y results
    run_ref(dataIn, yref_out, Y_IN_TILE_HEIGHT, Y_IN_TILE_WIDTH);
    uint8_t u_b[4800];
    uint8_t v_b[4800];
    uint8_t outu_b[192];
    uint8_t outv_b[192];

    interleave_input(UVdataIn, u_b, v_b, UV_IN_TILE_HEIGHT, UV_IN_TILE_WIDTH);
    run_ref(u_b, outu_b, UV_IN_TILE_HEIGHT, (UV_IN_TILE_WIDTH / 2));
    run_ref(v_b, outv_b, UV_IN_TILE_HEIGHT, (UV_IN_TILE_WIDTH / 2), 1);
    merge_out(outu_b, outv_b, uvref_out, UV_OUT_TILE_HEIGHT, UV_OUT_TILE_WIDTH);

    int acceptableError = 1;
    int errCount = 0;

    for (int i = 0; i < Y_OUT_TILE_ELEMENTS; i++) {
        if (abs(yref_out[i] - dataOut[i]) > acceptableError) {
            std::cout << "err at : i=" << i << " err=" << abs(yref_out[i] - dataOut[i]) << "=" << yref_out[i] << "-"
                      << dataOut[i] << std::endl;
            errCount++;
        }
    }
    FILE* fp = fopen("ref.txt", "w");
    FILE* fp1 = fopen("aie.txt", "w");
    for (int i = 0; i < UV_OUT_TILE_ELEMENTS; i++) {
        if (abs(uvref_out[i] - UVdataOut[i]) > acceptableError) {
            std::cout << "err at : i=" << i << " err=" << abs(uvref_out[i] - UVdataOut[i]) << "=" << uvref_out[i] << "-"
                      << UVdataOut[i] << std::endl;
            errCount++;
        }
        int ref = uvref_out[i];
        fprintf(fp, "%d\n", ref);
        int aie = UVdataOut[i];
        fprintf(fp1, "%d\n", aie);
    }

    fclose(fp);
    fclose(fp1);

    if (errCount) {
        std::cerr << "Test failed." << std::endl;
        exit(-1);
    }

    std::cout << "Test passed!" << std::endl;
    printf("after Test passed!\n");

    GMIO::free(y_in);
    GMIO::free(y_out);
    GMIO::free(yref_out);
    GMIO::free(uv_in);
    GMIO::free(uv_out);
    GMIO::free(uvref_out);

    return 0;
}
#endif
