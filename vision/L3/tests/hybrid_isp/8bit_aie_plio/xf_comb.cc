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

#include "kernels.h"
#include "imgproc/xf_blacklevel_aie2.hpp"
#include "imgproc/xf_gaincontrol_aie2.hpp"
#include "imgproc/xf_demosaicing_impl_aie2.hpp"
#include "imgproc/xf_awbnorm_ccm.hpp"

static constexpr int INPUT_TILE_WIDTH = 128; // Assumes input tile is always 64 elements wide
static constexpr int INPUT_TILE_HEIGHT = (TILE_ELEMENTS / 128);
static constexpr int INTERLEAVE_TILE_HEIGHT = (INPUT_TILE_HEIGHT / 2) + 2;
static constexpr int INTERLEAVE_TILE_ELEMENTS = (INPUT_TILE_WIDTH * INTERLEAVE_TILE_HEIGHT);

template <typename T, unsigned int N, int code>
void comb(adf::input_buffer<uint8_t>& input,
          const int16_t (&coeff)[25],
          const uint8_t& black_level,
          const uint16_t& mul_fact,
          const uint8_t& rgain,
          const uint8_t& bgain,
          const uint8_t& ggain,
          adf::output_buffer<uint8_t>& outputDem,
          adf::output_buffer<uint8_t>& output) {
    uint8_t mInEven[INTERLEAVE_TILE_ELEMENTS];
    uint8_t mInOdd[INTERLEAVE_TILE_ELEMENTS];
    uint8_t mRChannel[TILE_ELEMENTS];
    uint8_t mGChannel[TILE_ELEMENTS];
    uint8_t mBChannel[TILE_ELEMENTS];

    uint8_t* img_in_ptr = (uint8_t*)::aie::begin(input);
    uint8_t* img_out_ptr = (uint8_t*)::aie::begin(outputDem);
    uint8_t* img_out_ptr2 = (uint8_t*)::aie::begin(output);

    const int img_width = xf::cv::aie::xfGetTileWidth(img_in_ptr);
    const int img_height = xf::cv::aie::xfGetTileHeight(img_in_ptr);

    xf::cv::aie::xfCopyMetaData(img_in_ptr, img_out_ptr);
    xf::cv::aie::xfUnsignedSaturation(img_out_ptr);
    xf::cv::aie::xfCopyMetaData(img_in_ptr, img_out_ptr2);
    xf::cv::aie::xfUnsignedSaturation(img_out_ptr2);

    const uint16_t posH = xf::cv::aie::xfGetTilePosH(img_in_ptr);
    const uint16_t posV = xf::cv::aie::xfGetTilePosV(img_in_ptr);

    uint8_t* in_ptr = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_in_ptr);
    uint8_t* out_ptr = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_out_ptr);
    uint8_t* out_ptr2 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_out_ptr2);
    uint8_t* blc_in = (uint8_t*)in_ptr;
    uint8_t* blc_out = (uint8_t*)out_ptr;
    uint8_t* gc_in = (uint8_t*)out_ptr;
    uint8_t* gc_out = (uint8_t*)in_ptr;
    uint8_t* dem_in = (uint8_t*)img_in_ptr;
    uint8_t* dem_out = (uint8_t*)img_out_ptr;
    uint8_t* awbccm_in = (uint8_t*)img_out_ptr;
    uint8_t* awbccm_out = (uint8_t*)img_out_ptr2;

    ::aie::vector<uint8_t, 32> coeff0;
    ::aie::vector<uint8_t, 32> coeff1;

    if (posH % 2 == 0) {
        if (posV % 2 == 0) {
            xf::cv::aie::ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(rgain, bgain, ggain, coeff0,
                                                                                         coeff1);
        } else {
            xf::cv::aie::ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(rgain, bgain, ggain, coeff1,
                                                                                         coeff0);
        }
    } else {
        if (posV % 2 == 0) {
            xf::cv::aie::ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(bgain, rgain, ggain, coeff0,
                                                                                         coeff1);
        } else {
            xf::cv::aie::ComputeGainVector<uint8_t, 32, code>::compute_gain_kernel_coeff(bgain, rgain, ggain, coeff1,
                                                                                         coeff0);
        }
    }

    xf::cv::aie::blackLevelCorrection<uint8_t, 32, uint8_t>(blc_in, blc_out, img_width, img_height, black_level,
                                                            mul_fact);
    xf::cv::aie::gaincontrol<uint8_t, 32, code, uint8_t>(gc_in, gc_out, img_width, img_height, coeff0, coeff1);
    xf::cv::aie::DemosaicRGBA<xf::cv::aie::BayerPattern::RGGB, TILE_ELEMENTS> demosaic(mInEven, mInOdd, mRChannel,
                                                                                       mGChannel, mBChannel);
    demosaic.runImpl_ptr(dem_in, dem_out);
    xf::cv::aie::awbNormCCMImpl1<T, N>(awbccm_in, awbccm_out, coeff, img_width, img_height);
};
