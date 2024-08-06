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
#include "maskgen_ref.hpp"
#include "maskgen_track_ref.hpp"
#include "maskgen_util.h"

// instantiate adf dataflow graph
maskGenTrackingGraph maskGenTrack;
maskGenGraph maskGen;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
#include <common/xf_aie_utils.hpp>
int main(int argc, char** argv) {
    int BLOCK_SIZE_in_Bytes = (TILE_WINDOW_SIZE_IN);
    int BLOCK_SIZE_out_Bytes = (TILE_WINDOW_SIZE_IN);
    int BLOCK_SIZE_out_Bytes_track = TILE_WINDOW_SIZE_OUT;

    uint8_t* inputDataPredDepth = (uint8_t*)GMIO::malloc(NUM_TILES * BLOCK_SIZE_in_Bytes);
    uint8_t* inputDataPredSeg = (uint8_t*)GMIO::malloc(NUM_TILES * BLOCK_SIZE_in_Bytes);
    uint8_t* outputDataTrack = (uint8_t*)GMIO::malloc(BLOCK_SIZE_out_Bytes_track);
    uint8_t* outputData = (uint8_t*)GMIO::malloc(NUM_TILES * BLOCK_SIZE_out_Bytes);
    uint8_t* maskRef = (uint8_t*)malloc(NUM_TILES * TILE_HEIGHT_IN * TILE_WIDTH_IN);

    memset(inputDataPredDepth, 0, NUM_TILES * BLOCK_SIZE_in_Bytes);
    memset(inputDataPredSeg, 0, NUM_TILES * BLOCK_SIZE_in_Bytes);
    xf::cv::aie::xfSetTileWidth(inputDataPredDepth, TILE_WIDTH_IN);
    xf::cv::aie::xfSetTileHeight(inputDataPredDepth, TILE_HEIGHT_IN);
    xf::cv::aie::xfSetTileWidth(inputDataPredSeg, TILE_WIDTH_IN);
    xf::cv::aie::xfSetTileHeight(inputDataPredSeg, TILE_HEIGHT_IN);

    uint8_t* dataIn0 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputDataPredDepth);
    uint8_t* dataIn1 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(inputDataPredSeg);
    int temp = 0;
    for (int i = 0; i < (TILE_HEIGHT_IN * TILE_WIDTH_IN * NUM_TILES); i++) {
        dataIn0[i] = rand();
        dataIn1[i] = rand();
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
    maskGenTrack.in1.gm2aie_nb(inputDataPredDepth, BLOCK_SIZE_in_Bytes);
    maskGenTrack.in2.gm2aie_nb(inputDataPredSeg, BLOCK_SIZE_in_Bytes);
    maskGenTrack.out1.aie2gm_nb(outputDataTrack, BLOCK_SIZE_out_Bytes_track);
    maskGenTrack.out1.wait();
    maskGenTrack.end();

    uint32_t sum_r, non_zero_r;
    uint8_t depth_or_ref;
    uint8_t fg_thresh_track_r, bg_thresh_track_r;
    maskgen_track_param_ref(dataIn0, dataIn1, (uint8)_MIN, (uint8)_MAX, (uint8)_FGTH, (uint8)_BGTH,
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
    maskGen.in1.gm2aie_nb(inputDataPredDepth, BLOCK_SIZE_in_Bytes);
    maskGen.out1.aie2gm_nb(outputData, BLOCK_SIZE_out_Bytes);
    maskGen.out1.wait();
    maskGen.end();

    maskgen_ref(dataIn0, maskRef, (bool)MASKGEN_TRACKING, (uint8)_MIN, (uint8)_MAX, (uint8)_FGTH, (uint8)_BGTH,
                fg_thresh_track_r, bg_thresh_track_r, (int)(TILE_HEIGHT_IN * NUM_TILES), (int)TILE_WIDTH_IN);

    // Compare the results
    int acceptableError = 0;
    int errCount = 0;
    uint8_t* dataOut = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(outputData);
    std::ofstream fref("reference.txt");
    std::ofstream aie_fref("aie_reference.txt");

    int tmp_idx = 0;
    for (int i = 0; i < (TILE_HEIGHT_IN * TILE_WIDTH_IN * NUM_TILES); i++) {
        fref << unsigned(maskRef[i]);
        aie_fref << unsigned(dataOut[i]);

        if (abs(dataOut[i] - maskRef[i]) > acceptableError) errCount++;
        tmp_idx++;
        if (tmp_idx == 16) {
            tmp_idx = 0;
            fref << std::endl;
            aie_fref << std::endl;
        } else {
            fref << " ";
            aie_fref << " ";
        }
    }

    if (errCount) {
        std::cout << "Test failed!" << std::endl;
        exit(-1);
    }
    std::cout << "Test passed!" << std::endl;

    return 0;
}
#endif
