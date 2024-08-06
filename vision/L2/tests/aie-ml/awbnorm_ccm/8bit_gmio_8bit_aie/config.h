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

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <common/xf_aie_const.hpp>

// tile dimensions are normally computed by tiler but we need to
// hardcode these values to set the graph window sizes
using T = uint8_t;

#define XF_CCM_TYPE 0

static constexpr int TILE_WIDTH = 480;
static constexpr int TILE_HEIGHT = 16;

static constexpr int TILE_ELEMENTS_IN = (TILE_WIDTH * TILE_HEIGHT * 4);
static constexpr int TILE_ELEMENTS_OUT = (TILE_WIDTH * TILE_HEIGHT * 4);

static constexpr int TILE_WINDOW_SIZE_IN = ((TILE_ELEMENTS_IN * sizeof(T)) + xf::cv::aie::METADATA_SIZE);
static constexpr int ELEM_WITH_METADATA_IN = TILE_ELEMENTS_IN + (xf::cv::aie::METADATA_SIZE / sizeof(T));

static constexpr int TILE_WINDOW_SIZE_OUT = ((TILE_ELEMENTS_OUT * sizeof(T)) + xf::cv::aie::METADATA_SIZE);
static constexpr int ELEM_WITH_METADATA_OUT = TILE_ELEMENTS_OUT + (xf::cv::aie::METADATA_SIZE / sizeof(T));
static constexpr int __X86_DEVICE__ = 0;

/* Graph specific configuration */
static constexpr int VECTORIZATION_FACTOR = 32;

#define SAT_U8(x) std::max(0, std::min(255, (static_cast<int>(x))))

int16_t float2fixed(const float x, const uint8_t fractional_bits) {
    return int16_t(round(x * (1 << fractional_bits)));
}
const float bt2020_bt709_arr[3][3] = {
    {1.6605, -0.5876, -0.0728}, {-0.1246, 1.1329, -0.0083}, {-0.0182, -0.1006, 1.1187}};

const float bt2020_bt709_off[3] = {0.0, 0.0, 0.0};

const float bt709_bt2020_arr[3][3] = {{0.627, 0.329, 0.0433}, {0.0691, 0.92, 0.0113}, {0.0164, 0.088, 0.896}};

const float bt709_bt2020_off[3] = {0.0, 0.0, 0.0};

const float rgb_yuv_601_arr[3][3] = {{0.257, 0.504, 0.098}, {-0.148, -0.291, 0.439}, {0.439, -0.368, -0.071}};

const float rgb_yuv_601_off[3] = {0.0625, 0.500, 0.500};

const float rgb_yuv_709_arr[3][3] = {{0.183, 0.614, 0.062}, {-0.101, -0.338, 0.439}, {0.439, -0.399, -0.040}};

const float rgb_yuv_709_off[3] = {0.0625, 0.500, 0.500};

const float rgb_yuv_2020_arr[3][3] = {
    {0.225613, 0.582282, 0.050928}, {-0.119918, -0.309494, 0.429412}, {0.429412, -0.394875, -0.034537}};

const float rgb_yuv_2020_off[3] = {0.062745, 0.500, 0.500};

const float yuv_rgb_601_arr[3][3] = {{1.164, 0.000, 1.596}, {1.164, -0.813, -0.391}, {1.164, 2.018, 0.000}};

const float yuv_rgb_601_off[3] = {-0.87075, 0.52925, -1.08175};

const float yuv_rgb_709_arr[3][3] = {{1.164, 0.000, 1.793}, {1.164, -0.213, -0.534}, {1.164, 2.115, 0.000}};

const float yuv_rgb_709_off[3] = {-0.96925, 0.30075, -1.13025};

const float yuv_rgb_2020_arr[3][3] = {
    {1.164384, 0.000000, 1.717000}, {1.164384, -0.191603, -0.665274}, {1.164384, 2.190671, 0.000000}};

const float yuv_rgb_2020_off[3] = {-0.931559, 0.355379, -1.168395};

const float full_to_16_235_arr[3][3] = {
    {0.856305, 0.000000, 0.000000}, {0.000000, 0.856305, 0.000000}, {0.000000, 0.000000, 0.856305}};

const float full_to_16_235_off[3] = {0.0625, 0.0625, 0.0625};

const float full_from_16_235_arr[3][3] = {
    {1.167808, 0.000000, 0.000000}, {0.000000, 1.167808, 0.000000}, {0.000000, 0.000000, 1.167808}};

const float full_from_16_235_off[3] = {-0.0729880, -0.0729880, -0.0729880};

void compute_awb_params(uint16_t* coeff_awb, int* min, int* max) {
    int out_min = 0, out_max = 255;
    float coeff_fl_r = (out_max - out_min) / (max[0] - min[0]);
    float coeff_fl_g = (out_max - out_min) / (max[1] - min[1]);
    float coeff_fl_b = (out_max - out_min) / (max[2] - min[2]);

    coeff_awb[2] = (coeff_fl_r * min[0] + out_min);
    coeff_awb[5] = (coeff_fl_g * min[1] + out_min);
    coeff_awb[8] = (coeff_fl_b * min[2] + out_min);
    // fixing fp vals to 8
    coeff_awb[1] = 8;
    coeff_awb[4] = 8;
    coeff_awb[7] = 8;
    coeff_awb[0] = (uint16_t)(coeff_fl_r * pow(2, coeff_awb[1]));
    coeff_awb[3] = (uint16_t)(coeff_fl_g * pow(2, coeff_awb[4]));
    coeff_awb[6] = (uint16_t)(coeff_fl_b * pow(2, coeff_awb[7]));
}

template <typename T>
void awbnorm_colorcorrectionmatrix(T* _src,
                                   T* _dst,
                                   uint16_t* coeff_awb,
                                   int16_t* coeff_fix,
                                   int img_width = TILE_WIDTH,
                                   int img_height = TILE_HEIGHT) {
    float ccm_matrix[3][3];
    float offsetarray[3];

    switch (XF_CCM_TYPE) {
        case 0:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = bt2020_bt709_arr[i][j];
                }
                offsetarray[i] = bt2020_bt709_off[i];
            }

            break;
        case 1:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = bt709_bt2020_arr[i][j];
                }
                offsetarray[i] = bt709_bt2020_off[i];
            }

            break;
        case 2:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = rgb_yuv_601_arr[i][j];
                }
                offsetarray[i] = rgb_yuv_601_off[i];
            }

            break;
        case 3:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = rgb_yuv_709_arr[i][j];
                }
                offsetarray[i] = rgb_yuv_709_off[i];
            }

            break;
        case 4:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = rgb_yuv_2020_arr[i][j];
                }
                offsetarray[i] = rgb_yuv_2020_off[i];
            }

            break;
        case 5:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = yuv_rgb_601_arr[i][j];
                }
                offsetarray[i] = yuv_rgb_601_off[i];
            }

            break;
        case 6:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = yuv_rgb_709_arr[i][j];
                }
                offsetarray[i] = yuv_rgb_709_off[i];
            }

            break;
        case 7:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = yuv_rgb_2020_arr[i][j];
                }
                offsetarray[i] = yuv_rgb_2020_off[i];
            }

            break;
        case 8:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = full_to_16_235_arr[i][j];
                }
                offsetarray[i] = full_to_16_235_off[i];
            }

            break;
        case 9:
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    ccm_matrix[i][j] = full_from_16_235_arr[i][j];
                }
                offsetarray[i] = full_from_16_235_off[i];
            }

            break;
        default:
            break;
    }

    for (int i = 0; i < img_height; i++) {
        for (int j = 0, k = 0; j < img_width; j++, k += 4) {
            float _src_r = _src[(i * img_width * 4) + k];
            float _src_g = _src[(i * img_width * 4) + k + 1];
            float _src_b = _src[(i * img_width * 4) + k + 2];
            _src_r = SAT_U8(((_src_r * coeff_awb[0]) / pow(2, coeff_awb[1])) - coeff_awb[2]);
            _src_g = SAT_U8(((_src_g * coeff_awb[3]) / pow(2, coeff_awb[4])) - coeff_awb[5]);
            _src_b = SAT_U8(((_src_b * coeff_awb[6]) / pow(2, coeff_awb[7])) - coeff_awb[8]);

            float value1 = (_src_r * ccm_matrix[0][0]);
            float value2 = (_src_g * ccm_matrix[0][1]);
            float value3 = (_src_b * ccm_matrix[0][2]);
            float value4 = (_src_r * ccm_matrix[1][0]);
            float value5 = (_src_g * ccm_matrix[1][1]);
            float value6 = (_src_b * ccm_matrix[1][2]);
            float value7 = (_src_r * ccm_matrix[2][0]);
            float value8 = (_src_g * ccm_matrix[2][1]);
            float value9 = (_src_b * ccm_matrix[2][2]);
            int value_r = (int)(value1 + value2 + value3 + offsetarray[0]);
            int value_g = (int)(value4 + value5 + value6 + offsetarray[1]);
            int value_b = (int)(value7 + value8 + value9 + offsetarray[2]);
            /*             if(i==0 && j<32){
                         std::cout<<_src_r<<" "<<_src_g<<" "<<_src_b<<std::endl;
                         std::cout<<ccm_matrix[1][0]<<" "<<ccm_matrix[1][1]<<" "<<ccm_matrix[1][2]<<std::endl;
                        }
            */
            if (value_r > 255) {
                value_r = 255;
            }
            if (value_g > 255) {
                value_g = 255;
            }
            if (value_b > 255) {
                value_b = 255;
            }
            if (value_r < 0) {
                value_r = 0;
            }
            if (value_g < 0) {
                value_g = 0;
            }
            if (value_b < 0) {
                value_b = 0;
            }

            _dst[(i * img_width * 4) + k] = static_cast<T>(value_r);     // value_r;
            _dst[(i * img_width * 4) + k + 1] = static_cast<T>(value_g); // value_g;
            _dst[(i * img_width * 4) + k + 2] = static_cast<T>(value_b); // value_b;
            _dst[(i * img_width * 4) + k + 3] = 0;
        }
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            coeff_fix[(i * 3) + j] = float2fixed(ccm_matrix[i][j], 12);
        }
    }
    for (int j = 0; j < 3; j++) {
        coeff_fix[j + 9] = float2fixed(offsetarray[0], 12);
    }
}

#endif //__CONFIG_H_
