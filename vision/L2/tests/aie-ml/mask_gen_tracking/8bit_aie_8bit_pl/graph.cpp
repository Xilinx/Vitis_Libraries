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
#include "maskgen_util.h"
#include "maskgen_ref.hpp"
#include "maskgen_track_ref.hpp"

// instantiate adf dataflow graph
maskGenTrackingGraph maskGenTrack;
maskGenGraph maskGen;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = (TILE_WIDTH_IN * TILE_HEIGHT_IN * sizeof(uint8_t));
    int BLOCK_SIZE_out_Bytes = (TILE_WIDTH_OUT * TILE_HEIGHT_OUT * sizeof(uint8_t));
    int BLOCK_SIZE_out_Bytes_track = 8;

    uint8_t* inputDataPredDepth = (uint8_t*)malloc(NUM_TILES * BLOCK_SIZE_in_Bytes);
    uint8_t* inputDataPredSeg = (uint8_t*)malloc(NUM_TILES * BLOCK_SIZE_in_Bytes);
    uint8_t* outputDataTrack = (uint8_t*)malloc(BLOCK_SIZE_out_Bytes_track);
    uint8_t* maskRef = (uint8_t*)malloc(NUM_TILES * BLOCK_SIZE_in_Bytes);

    std::ifstream pdfs("data/pred_depth_1920x4_ref.txt");
    std::ifstream psfs("data/pred_seg_1920x4_ref.txt");

    if (!pdfs) { // file couldn't be opened
        std::cerr << "Error: pred_depth_1080.txt file could not be opened" << std::endl;
    }
    if (!psfs) { // file couldn't be opened
        std::cerr << "Error: pred_seg_1080.txt file could not be opened" << std::endl;
    }

    int temp = 0;
    for (int i = 0; i < (TILE_HEIGHT_IN * TILE_WIDTH_IN * NUM_TILES); i++) {
        pdfs >> temp;
        inputDataPredDepth[i] = (uint8_t)temp;
        psfs >> temp;
        inputDataPredSeg[i] = (uint8_t)temp;
    }

    uint16_t fg_thresh, bg_thresh;
    scalar_comp_utility((uint8_t)_FGTH, (uint8_t)_BGTH, (uint8_t)_MIN, (uint8_t)_MAX, fg_thresh, bg_thresh);
    printf("fg_thresh:%d\nbg_thresh:%d\n", fg_thresh, bg_thresh);

    maskGenTrack.init();
    maskGenTrack.update(maskGenTrack.depth_min, (uint8_t)_MIN);
    maskGenTrack.update(maskGenTrack.depth_max, (uint8_t)_MAX);
    maskGenTrack.update(maskGenTrack.thres_f_new, fg_thresh);
    maskGenTrack.update(maskGenTrack.thres_b_new, bg_thresh);
    maskGenTrack.update(maskGenTrack.pred_seg_thresh, (uint8_t)PRED_SEG_THRESH);
    maskGenTrack.run(NUM_TILES);
    maskGenTrack.wait();
    maskGenTrack.end();

    uint32_t sum_r, non_zero_r;
    uint8_t depth_or_ref;
    uint8_t fg_thresh_track_r, bg_thresh_track_r;
    maskgen_track_param_ref(inputDataPredDepth, inputDataPredSeg, (uint8)_MIN, (uint8)_MAX, (uint8)_FGTH, (uint8)_BGTH,
                            (uint8)PRED_SEG_THRESH, (int)(TILE_HEIGHT_IN * NUM_TILES), (int)(TILE_WIDTH_IN),
                            depth_or_ref, sum_r, non_zero_r, fg_thresh_track_r, bg_thresh_track_r);

    printf("REF : non zero:%d\n sum:%d\n dep_or:%d\n", non_zero_r, sum_r, depth_or_ref);

    uint8_t dep_or = scalar_tracking_comp_utility((uint8_t)_FGTH, (uint8_t)_BGTH, (uint8_t)_MIN, (uint8_t)_MAX,
                                                  non_zero_r, sum_r, fg_thresh, bg_thresh);

    printf("AIE : dep_or:%d\n", dep_or);
    printf("fg_thresh:%d\nbg_thresh:%d\n", fg_thresh, bg_thresh);

    maskGen.init();
    maskGen.update(maskGen.depth_min, (uint8_t)_MIN);
    maskGen.update(maskGen.depth_max, (uint8_t)_MAX);
    maskGen.update(maskGen.thres_f_new, fg_thresh);
    maskGen.update(maskGen.thres_b_new, bg_thresh);
    maskGen.run(NUM_TILES);
    maskGen.wait();
    maskGen.end();

    maskgen_ref(inputDataPredDepth, maskRef, (bool)MASKGEN_TRACKING, (uint8)_MIN, (uint8)_MAX, (uint8)_FGTH,
                (uint8)_BGTH, fg_thresh_track_r, bg_thresh_track_r, (int)(TILE_HEIGHT_IN * NUM_TILES),
                (int)TILE_WIDTH_IN);

    std::ofstream fref("reference.txt");

    int tmp_idx = 0;
    for (int i = 0; i < (TILE_HEIGHT_IN * TILE_WIDTH_IN * NUM_TILES); i++) {
        fref << unsigned(maskRef[i]);

        tmp_idx++;
        if (tmp_idx == 16) {
            tmp_idx = 0;
            fref << std::endl;
        } else {
            fref << " ";
        }
    }

    return 0;
}
#endif
