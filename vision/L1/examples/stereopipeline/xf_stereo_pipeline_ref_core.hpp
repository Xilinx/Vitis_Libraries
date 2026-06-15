/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#ifndef _XF_STEREO_PIPELINE_REF_CORE_H_
#define _XF_STEREO_PIPELINE_REF_CORE_H_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

namespace ref_core {

static const int REF_CORE_MAP_XY_SCALE = 256;
static const int REF_INTER_BITS = 5;
static const int REF_INTER_TAB = 1 << REF_INTER_BITS;

static const int REF_WSIZE = 15;
static const int REF_NDISP = 48;

struct BMState {
    int preFilterCap;
    int textureThreshold;
    int uniquenessRatio;
    int sweepFactor;
};

inline BMState bm_state_from_arr(const int* arr) {
    BMState s;
    s.preFilterCap = arr[2];
    s.textureThreshold = arr[6];
    s.uniquenessRatio = arr[7];
    s.sweepFactor = arr[9];
    return s;
}

inline cv::Mat to_grayscale_u8(const cv::Mat& img) {
    cv::Mat gray;
    if (img.channels() == 3) {
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    } else if (img.type() == CV_8UC1) {
        gray = img;
    } else {
        img.convertTo(gray, CV_8UC1);
    }
    return gray;
}

inline void compute_undistort_coord(
    float* cameraMatrix, float* distCoeffs, int dist_coeff_count, float* ir, int i, int j, float& u, float& v) {
    const int N = dist_coeff_count;
    const float k1 = distCoeffs[0];
    const float k2 = distCoeffs[1];
    const float p1 = distCoeffs[2];
    const float p2 = distCoeffs[3];
    const float k3 = (N >= 5) ? distCoeffs[4] : 0.0f;
    const float k4 = (N >= 8) ? distCoeffs[5] : 0.0f;
    const float k5 = (N >= 8) ? distCoeffs[6] : 0.0f;
    const float k6 = (N >= 8) ? distCoeffs[7] : 0.0f;

    const float u0 = cameraMatrix[2];
    const float v0 = cameraMatrix[5];
    const float fx = cameraMatrix[0];
    const float fy = cameraMatrix[4];

    const float _x = (float)i * ir[1] + (float)j * ir[0] + ir[2];
    const float _y = (float)i * ir[4] + (float)j * ir[3] + ir[5];
    const float w = (float)i * ir[7] + (float)j * ir[6] + ir[8];
    const float winv = 1.0f / w;
    const float x = _x * winv;
    const float y = _y * winv;

    const float x2 = x * x;
    const float y2 = y * y;
    const float _2xy = 2.0f * x * y;
    const float r2 = x2 + y2;

    float kr = 1.0f + ((k3 * r2 + k2) * r2 + k1) * r2;
    if (N > 5) {
        const float krd = ((k6 * r2 + k5) * r2 + k4) * r2;
        kr = kr / (1.0f + krd);
    }

    u = fx * (x * kr + p1 * _2xy + p2 * (2.0f * x2 + r2)) + u0;
    v = fy * (y * kr + p1 * (r2 + 2.0f * y2) + p2 * _2xy) + v0;
}

inline void init_undistort_rectify_map_inverse(float* cameraMatrix,
                                               float* distCoeffs,
                                               float* ir,
                                               int dist_coeff_count,
                                               int rows,
                                               int cols,
                                               cv::Mat& mapx,
                                               cv::Mat& mapy) {
    mapx.create(rows, cols, CV_32SC1);
    mapy.create(rows, cols, CV_32SC1);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            float u, v;
            compute_undistort_coord(cameraMatrix, distCoeffs, dist_coeff_count, ir, i, j, u, v);
            mapx.at<int32_t>(i, j) = (int32_t)((uint32_t)(u * (float)REF_CORE_MAP_XY_SCALE));
            mapy.at<int32_t>(i, j) = (int32_t)((uint32_t)(v * (float)REF_CORE_MAP_XY_SCALE));
        }
    }
}

inline int positive_mod(int val, int mod) {
    int r = val % mod;
    return (r < 0) ? (r + mod) : r;
}

inline void decode_remap_coord(
    int32_t mx, int32_t my, int rows, int cols, int& x, int& y, int& x_frac, int& y_frac, bool& in_range) {
    const uint32_t xu = (uint32_t)mx;
    const uint32_t yu = (uint32_t)my;

    const int x_fix = (int)(xu >> (8 - REF_INTER_BITS));
    const int y_fix = (int)(yu >> (8 - REF_INTER_BITS));
    x = x_fix >> REF_INTER_BITS;
    y = y_fix >> REF_INTER_BITS;
    x_frac = x_fix & (REF_INTER_TAB - 1);
    y_frac = y_fix & (REF_INTER_TAB - 1);

    const int x_fl = (int)(xu >> 8);
    const int y_fl = (int)(yu >> 8);
    in_range = (y >= 0) && (y_fl <= (rows - 1)) && (x >= 0) && (x_fl <= (cols - 1));
}

inline uint8_t remap_pixel_line_buffer(
    const std::vector<std::vector<uint8_t> >& buf_bram, int win_row, int32_t mx, int32_t my, int rows, int cols) {
    int x = 0;
    int y = 0;
    int x_frac = 0;
    int y_frac = 0;
    bool in_range = false;
    decode_remap_coord(mx, my, rows, cols, x, y, x_frac, y_frac, in_range);
    if (!in_range) {
        return 0;
    }

    const int xa0_bram = x + 1;
    const int xa1_bram = x;

    uint8_t temp1[7][2];
    for (int m = 6; m >= 0; m--) {
        const int brow = positive_mod(y + (m - 3), win_row);
        temp1[m][0] = buf_bram[(size_t)brow][(size_t)xa1_bram];
        temp1[m][1] = buf_bram[(size_t)brow][(size_t)xa1_bram + 1];
    }

    int t_c1 = 0;
    int t_c2 = 0;
    if (xa1_bram == 0) {
        t_c1 = 1;
        t_c2 = 0;
    } else {
        t_c1 = xa0_bram % xa1_bram;
        t_c2 = xa1_bram % xa1_bram;
    }
    const int t_r1 = 3;
    const int t_r2 = 4;

    const int iu_fp = x_frac;
    const int iv_fp = y_frac;
    const int k00_fp = (32 - iv_fp) * (32 - iu_fp);
    const int k01_fp = iu_fp * (32 - iv_fp);
    const int k10_fp = iv_fp * (32 - iu_fp);
    const int k11_fp = iu_fp * iv_fp;

    uint8_t d00 = temp1[t_r1][t_c1];
    uint8_t d01 = temp1[t_r1][t_c2];
    uint8_t d10 = temp1[t_r2][t_c1];
    uint8_t d11 = temp1[t_r2][t_c2];
    std::swap(d00, d01);
    std::swap(d10, d11);

    const int val_fp = d00 * k00_fp + d01 * k01_fp + d10 * k10_fp + d11 * k11_fp;
    return (uint8_t)std::max(0, std::min(255, val_fp >> 10));
}

// Port of xf::cv::xFRemapLI (BRAM path, NPC=1) for plain-C++ reference validation.
inline void remap_bilinear_scaled_map(const cv::Mat& src, cv::Mat& dst, const cv::Mat& mapx, const cv::Mat& mapy) {
    const int rows = src.rows;
    const int cols = src.cols;
    const int WIN_ROW = 128;
    const int ISHIFT = (WIN_ROW >= rows) ? WIN_ROW : (WIN_ROW / 2);
    const int row_iteration = rows;
    const int col_iteration = cols;
    const int hls_remaped_rows = rows;
    const int hls_remaped_cols = cols;

    dst.create(rows, cols, CV_8UC1);
    dst.setTo(0);

    std::vector<std::vector<uint8_t> > buf_bram((size_t)WIN_ROW, std::vector<uint8_t>((size_t)cols + 2, 0));

    for (int i = 0; i < row_iteration + ISHIFT; i++) {
        for (int j = 0; j < col_iteration + 2; j++) {
            const int i_mod = positive_mod(i, WIN_ROW);
            if (i < rows && j < cols) {
                buf_bram[(size_t)i_mod][(size_t)j] = src.at<uint8_t>(i, j);
            }

            if (i >= ISHIFT && i < hls_remaped_rows + ISHIFT && j < hls_remaped_cols) {
                const int out_row = i - ISHIFT;
                const int out_col = j;
                const int32_t mx = mapx.at<int32_t>(out_row, out_col);
                const int32_t my = mapy.at<int32_t>(out_row, out_col);
                dst.at<uint8_t>(out_row, out_col) = remap_pixel_line_buffer(buf_bram, WIN_ROW, mx, my, rows, cols);
            }
        }
    }
}

inline void stereo_block_matching(const cv::Mat& left_remapped,
                                  const cv::Mat& right_remapped,
                                  cv::Mat& disp_u16,
                                  const BMState& state) {
    cv::Ptr<cv::StereoBM> stereo = cv::StereoBM::create(REF_NDISP, REF_WSIZE);
    stereo->setMinDisparity(0);
    stereo->setPreFilterCap(state.preFilterCap);
    stereo->setPreFilterType(cv::StereoBM::PREFILTER_XSOBEL);
    stereo->setTextureThreshold(state.textureThreshold);
    stereo->setUniquenessRatio(state.uniquenessRatio);
    stereo->setDisp12MaxDiff(-1);
    stereo->setSpeckleWindowSize(0);
    stereo->setSpeckleRange(0);

    cv::Mat disp_s16;
    stereo->compute(left_remapped, right_remapped, disp_s16);

    disp_u16.create(left_remapped.rows, left_remapped.cols, CV_16UC1);
    for (int r = 0; r < disp_u16.rows; r++) {
        for (int c = 0; c < disp_u16.cols; c++) {
            const short v = disp_s16.at<short>(r, c);
            disp_u16.at<uint16_t>(r, c) = (uint16_t)(v < 0 ? 0 : v);
        }
    }
}

inline void stereopipeline(const cv::Mat& left8,
                           const cv::Mat& right8,
                           cv::Mat& disp_out,
                           float* cameraMA_l,
                           float* cameraMA_r,
                           float* distC_l,
                           float* distC_r,
                           float* irA_l,
                           float* irA_r,
                           int dist_coeff_count,
                           const int* bm_state_arr) {
    const int rows = left8.rows;
    const int cols = left8.cols;
    const BMState bm = bm_state_from_arr(bm_state_arr);

    cv::Mat mapx_l, mapy_l, mapx_r, mapy_r;
    init_undistort_rectify_map_inverse(cameraMA_l, distC_l, irA_l, dist_coeff_count, rows, cols, mapx_l, mapy_l);
    init_undistort_rectify_map_inverse(cameraMA_r, distC_r, irA_r, dist_coeff_count, rows, cols, mapx_r, mapy_r);

    cv::Mat left_remapped, right_remapped;
    remap_bilinear_scaled_map(left8, left_remapped, mapx_l, mapy_l);
    remap_bilinear_scaled_map(right8, right_remapped, mapx_r, mapy_r);

    stereo_block_matching(left_remapped, right_remapped, disp_out, bm);
}

} // namespace ref_core

#endif // _XF_STEREO_PIPELINE_REF_CORE_H_
