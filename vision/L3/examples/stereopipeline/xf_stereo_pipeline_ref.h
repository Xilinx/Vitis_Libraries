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

#ifndef _XF_STEREO_PIPELINE_REF_H_
#define _XF_STEREO_PIPELINE_REF_H_

#include <opencv2/core.hpp>

// InitUndistortRectifyMapInverse: float undistort math, maps written as scaled int32 (CV_32SC1).
void ref_init_undistort_rectify_map_inverse(float* cameraMatrix,
                                            float* distCoeffs,
                                            float* ir,
                                            int dist_coeff_count,
                                            int rows,
                                            int cols,
                                            cv::Mat& mapx,
                                            cv::Mat& mapy);

// Bilinear remap: xFRemapLI line-buffer port (plain C++, HLS map format).
void ref_remap_bilinear_scaled_map(const cv::Mat& src, cv::Mat& dst, const cv::Mat& mapx, const cv::Mat& mapy);

// Stereo block matching: cv::StereoBM on remapped images.
void ref_stereo_block_matching(const cv::Mat& left_remapped,
                               const cv::Mat& right_remapped,
                               cv::Mat& disp_u16,
                               const int* bm_state_arr);

// Full software pipeline: undistort maps -> remap -> stereo BM (plain C++/OpenCV).
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
                        const int* bm_state_arr);

#endif // _XF_STEREO_PIPELINE_REF_H_
