/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_stereo_pipeline_ref.h"
#include "xf_stereo_pipeline_ref_core.hpp"

void ref_init_undistort_rectify_map_inverse(float* cameraMatrix,
                                            float* distCoeffs,
                                            float* ir,
                                            int dist_coeff_count,
                                            int rows,
                                            int cols,
                                            cv::Mat& mapx,
                                            cv::Mat& mapy) {
    ref_core::init_undistort_rectify_map_inverse(cameraMatrix, distCoeffs, ir, dist_coeff_count, rows, cols, mapx,
                                                 mapy);
}

void ref_remap_bilinear_scaled_map(const cv::Mat& src, cv::Mat& dst, const cv::Mat& mapx, const cv::Mat& mapy) {
    ref_core::remap_bilinear_scaled_map(src, dst, mapx, mapy);
}

void ref_stereo_block_matching(const cv::Mat& left_remapped,
                               const cv::Mat& right_remapped,
                               cv::Mat& disp_u16,
                               const int* bm_state_arr) {
    ref_core::stereo_block_matching(left_remapped, right_remapped, disp_u16, ref_core::bm_state_from_arr(bm_state_arr));
}

void stereopipeline_ref(const cv::Mat& left_img,
                        const cv::Mat& right_img,
                        cv::Mat& disp_out,
                        float* cameraMA_l,
                        float* cameraMA_r,
                        float* distC_l,
                        float* distC_r,
                        float* irA_l,
                        float* irA_r,
                        int dist_coeff_count,
                        const int* bm_state_arr) {
    const cv::Mat left8 = ref_core::to_grayscale_u8(left_img);
    const cv::Mat right8 = ref_core::to_grayscale_u8(right_img);
    ref_core::stereopipeline(left8, right8, disp_out, cameraMA_l, cameraMA_r, distC_l, distC_r, irA_l, irA_r,
                             dist_coeff_count, bm_state_arr);
}
