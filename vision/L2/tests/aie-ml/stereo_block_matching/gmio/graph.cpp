
/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
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

#include "graph.h"
using namespace std;
using namespace adf;

sbmGraph sbm_graph;
sobelGraph demo_sobel;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

int main(int argc, char** argv) {
    int left_vtile_size = (TILE_IN_HEIGHT_WITH_WINDOW)*64 + 64; // tile + Boder(winsize-1) + winsize (zeros)
    int right_vtile_size = (TILE_IN_HEIGHT_WITH_WINDOW) * (64 + NO_DISPARITIES) + 64;
    int out_aie_tile_size = (TILE_OUT_HEIGHT)*64;
    int gt_tile_size = (TILE_OUT_HEIGHT)*COLS_COMBINE;

    int NO_TILES = 1;

    uint8_t* in_vleft = (uint8_t*)GMIO::malloc(NO_TILES * left_vtile_size * sizeof(uint8_t)); // 2 tiles
    uint8_t* in_vright = (uint8_t*)GMIO::malloc(NO_TILES * right_vtile_size * sizeof(uint8_t));
    int16_t* out_minima_neigh = (int16_t*)GMIO::malloc(NO_TILES * out_aie_tile_size * sizeof(int16_t) + 64);
    int16_t* ground_truth_out = (int16_t*)GMIO::malloc(NO_TILES * gt_tile_size * sizeof(int16_t));

    int lctr = 0;
    int rctr = 0;
    {
        FILE* fpl = fopen("data/left_tile.txt", "r");
        FILE* fpr = fopen("data/right_tile.txt", "r");

        if (fpl == NULL) {
            printf("Failuer opening file ");
        } else {
            uint8_t temp;
            for (int i = 0; i < 1 * 64; i++, lctr++) { // meta-data
                in_vleft[lctr] = (uint8_t)0;
            }
            for (int i = 0; i < TILE_WINSIZE * 64; i++, lctr++) {
                in_vleft[lctr] = (uint8_t)31;
            }

            for (int i = 0; i < TILE_IN_HEIGHT * 64; i++, lctr++) {
                fscanf(fpl, "%hhd\n", &temp);
                in_vleft[lctr] = (uint8_t)temp;
            }
        }

        if (fpr == NULL) {
            printf("Failuer opening file ");
        } else {
            uint8_t temp;
            for (int i = 0; i < 64; i++, rctr++) {
                in_vright[rctr] = (uint8_t)0;
            }
            for (int i = 0; i < TILE_WINSIZE * (64 + NO_DISPARITIES); i++, rctr++) {
                in_vright[rctr] = (uint8_t)31;
            }
            for (int i = 0; i < TILE_IN_HEIGHT * (64 + NO_DISPARITIES); i++, rctr++) {
                fscanf(fpr, "%hhd\n", &temp);
                in_vright[rctr] = (uint8_t)temp;
            }
        }

        fclose(fpl);
        fclose(fpr);
    }

    int run_val = 1;

    sbm_graph.init();
    sbm_graph.run(run_val);

    sbm_graph.in_sobel_left_tile[0].gm2aie_nb(in_vleft, run_val * left_vtile_size * sizeof(uint8_t));
    sbm_graph.in_sobel_right_tile[0].gm2aie_nb(in_vright, run_val * right_vtile_size * sizeof(uint8_t));

    sbm_graph.out_interp[0].aie2gm(out_minima_neigh, run_val * (out_aie_tile_size * sizeof(int16_t) + 64));

    sbm_graph.out_interp[0].wait();

    FILE* fp_gt;
    fp_gt = fopen("data/ground_truth_out.txt", "r");
    for (int i = 0; i < gt_tile_size; i++) {
        fscanf(fp_gt, "%hd", &ground_truth_out[i]);
    }

    int error_cnt = 0;
    int16_t* aie_out_ptr = out_minima_neigh + 32;
    for (int row = 0; row < IMAGE_HEIGHT - (TILE_WINSIZE - 1); row++) {
        for (int col = 0; col < COLS_COMBINE; col++) {
            // cout << " GT = " << *ground_truth_out << " AIE = " << *aie_out_ptr<< endl;
            if (*ground_truth_out++ - *aie_out_ptr++ > 1) {
                error_cnt++;
            }
        }
        aie_out_ptr += 64 - COLS_COMBINE;
    }

    cout << "error_cnt " << error_cnt << "!" << endl;

    sbm_graph.end();
    return 0;
}

#endif