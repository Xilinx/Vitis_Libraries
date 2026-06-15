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

#include <cstdint>
#include <cstring>
#include "common/xf_headers.hpp"
#include "common/xf_params.hpp"
#include "common/xf_axi.hpp"
#include "xf_preprocess_config.h"

#define ERROR_THRESHOLD 2
#define PARAM_SCALE 8388608.0f

#if _XF_RGBA_
#define CHANNELS 4
#else
#define CHANNELS 3
#endif

static uint32_t float_to_param(float value) {
    return (uint32_t)(value * PARAM_SCALE);
}

static void build_params_int(uint32_t params_int[2 * XF_CHANNELS(OUT_TYPE, NPPCX)]) {
    const float alpha[CHANNELS] = {TB_ALPHA0, TB_ALPHA1, TB_ALPHA2, TB_ALPHA3};
    const float beta[CHANNELS] = {TB_BETA0, TB_BETA1, TB_BETA2, TB_BETA3};

    for (int c = 0; c < XF_CHANNELS(OUT_TYPE, NPPCX); c++) {
        params_int[2 * c] = float_to_param(alpha[c % CHANNELS]);
        params_int[2 * c + 1] = float_to_param(beta[c % CHANNELS]);
    }
}

static cv::Mat reference_preprocess(const cv::Mat& resized) {
    const float alpha[CHANNELS] = {TB_ALPHA0, TB_ALPHA1, TB_ALPHA2, TB_ALPHA3};
    const float beta[CHANNELS] = {TB_BETA0, TB_BETA1, TB_BETA2, TB_BETA3};

    cv::Mat ref_out(resized.rows, resized.cols, XF_CV_TYPE);

    for (int i = 0; i < resized.rows; i++) {
        for (int j = 0; j < resized.cols; j++) {
#if _XF_RGBA_
            cv::Vec4b in_pix = resized.at<cv::Vec4b>(i, j);
            cv::Vec4f out_pix;
            for (int c = 0; c < CHANNELS; c++) {
                out_pix[c] = ((float)in_pix[c] - alpha[c]) * beta[c];
            }
            ref_out.at<cv::Vec4f>(i, j) = out_pix;
#else
            cv::Vec3b in_pix = resized.at<cv::Vec3b>(i, j);
            cv::Vec3f out_pix;
            for (int c = 0; c < CHANNELS; c++) {
                out_pix[c] = ((float)in_pix[c] - alpha[c]) * beta[c];
            }
            ref_out.at<cv::Vec3f>(i, j) = out_pix;
#endif
        }
    }

    return ref_out;
}

#if XF_AXI_GBR
static void apply_gbr_order(cv::Mat& img) {
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
#if _XF_RGBA_
            cv::Vec4f pix = img.at<cv::Vec4f>(i, j);
            img.at<cv::Vec4f>(i, j) = cv::Vec4f(pix[1], pix[0], pix[2], pix[3]);
#else
            cv::Vec3f pix = img.at<cv::Vec3f>(i, j);
            img.at<cv::Vec3f>(i, j) = cv::Vec3f(pix[1], pix[0], pix[2]);
#endif
        }
    }
}
#endif

static float bits_to_float(uint32_t raw) {
    float val;
    std::memcpy(&val, &raw, sizeof(float));
    return val;
}

static float half_bits_to_float(uint16_t half_bits) {
    half hf;
    std::memcpy(&hf, &half_bits, sizeof(uint16_t));
    return (float)hf;
}

static float bf16_bits_to_float(uint16_t bf16_bits) {
    uint32_t fbits = ((uint32_t)bf16_bits) << 16;
    return bits_to_float(fbits);
}

static float decode_channel(uint32_t raw, uint32_t datatype) {
    switch (datatype) {
        case data_types::INT8:
            return (float)((raw >> 24) & 0xFF);
        case data_types::FP16:
            return half_bits_to_float((uint16_t)((raw >> 16) & 0xFFFF));
        case data_types::BF16:
            return bf16_bits_to_float((uint16_t)((raw >> 16) & 0xFFFF));
        case data_types::FP32:
        default:
            return bits_to_float(raw);
    }
}

static cv::Mat read_axis_output(OutStream& strm, int rows, int cols, uint32_t datatype) {
    cv::Mat raw_mat(rows, cols, CV_32SC4);
    xf::cv::AXIvideo2cvMatxf<NPPCX, AXI_WIDTH_OUT>(strm, raw_mat);

    cv::Mat out(rows, cols, CV_32FC4);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cv::Vec4i raw_pix = raw_mat.at<cv::Vec4i>(i, j);
            cv::Vec4f out_pix;
            for (int c = 0; c < CHANNELS; c++) {
                out_pix[c] = decode_channel((uint32_t)raw_pix[c], datatype);
            }
            out.at<cv::Vec4f>(i, j) = out_pix;
        }
    }
    return out;
}

static float compute_error_percent(const cv::Mat& ref, const cv::Mat& hls, float threshold) {
    int cnt = 0;
    float min_err = 1e9f;
    float max_err = 0.0f;

    for (int i = 0; i < ref.rows; i++) {
        for (int j = 0; j < ref.cols; j++) {
            float max_diff = 0.0f;
#if _XF_RGBA_
            cv::Vec4f ref_pix = ref.at<cv::Vec4f>(i, j);
            cv::Vec4f hls_pix = hls.at<cv::Vec4f>(i, j);
            for (int c = 0; c < CHANNELS; c++) {
                max_diff = std::max(max_diff, std::abs(ref_pix[c] - hls_pix[c]));
            }
#else
            cv::Vec3f ref_pix = ref.at<cv::Vec3f>(i, j);
            cv::Vec3f hls_pix = hls.at<cv::Vec3f>(i, j);
            for (int c = 0; c < CHANNELS; c++) {
                max_diff = std::max(max_diff, std::abs(ref_pix[c] - hls_pix[c]));
            }
#endif
            if (max_diff > threshold) {
                cnt++;
            }
            min_err = std::min(min_err, max_diff);
            max_err = std::max(max_err, max_diff);
        }
    }

    std::cout << "\tMinimum error in intensity = " << min_err << std::endl;
    std::cout << "\tMaximum error in intensity = " << max_err << std::endl;
    std::cout << "\tPercentage of pixels above error threshold = " << (100.0f * cnt / (ref.rows * ref.cols))
              << std::endl;

    return 100.0f * cnt / (ref.rows * ref.cols);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input image>\n", argv[0]);
        return -1;
    }

    cv::Mat imgInput = cv::imread(argv[1], 1);
    if (!imgInput.data) {
        fprintf(stderr, "Can't open image %s\n", argv[1]);
        return -1;
    }

    const int in_width = imgInput.cols;
    const int in_height = imgInput.rows;
    const int resize_width = (TB_RESIZE_WIDTH > 0) ? TB_RESIZE_WIDTH : in_width;
    const int resize_height = (TB_RESIZE_HEIGHT > 0) ? TB_RESIZE_HEIGHT : in_height;
    const uint32_t datatype = TB_DATATYPE;

    std::cout << "Input image height : " << in_height << std::endl;
    std::cout << "Input image width  : " << in_width << std::endl;
    std::cout << "Resize height      : " << resize_height << std::endl;
    std::cout << "Resize width       : " << resize_width << std::endl;
    std::cout << "Datatype           : " << datatype << std::endl;

    cv::Mat ref_resize;
    cv::resize(imgInput, ref_resize, cv::Size(resize_width, resize_height), 0, 0, cv::INTER_LINEAR);
#if _XF_RGBA_
    cv::cvtColor(ref_resize, ref_resize, cv::COLOR_BGR2BGRA);
#endif

    cv::Mat ref_out = reference_preprocess(ref_resize);
#if XF_AXI_GBR
    apply_gbr_order(ref_out);
#endif

    InStream src_stream;
    OutStream dst_stream;
    xf::cv::cvMat2AXIvideoxf<NPPCX, AXI_WIDTH_IN>(imgInput, src_stream);

    uint32_t params_int[2 * XF_CHANNELS(OUT_TYPE, NPPCX)];
    build_params_int(params_int);

    preprocess_accel(src_stream, dst_stream, params_int, in_width, in_height, resize_width, resize_height, datatype);

    cv::Mat result_hls = read_axis_output(dst_stream, resize_height, resize_width, datatype);

    cv::Mat diff;
    cv::absdiff(ref_out, result_hls, diff);
    cv::imwrite("out_hls.jpg", result_hls);
    cv::imwrite("ref_out.jpg", ref_out);
    cv::imwrite("diff.jpg", diff);

    float err_per = compute_error_percent(ref_out, result_hls, ERROR_THRESHOLD);
    std::cout << "Error percent: " << err_per << std::endl;

    if (err_per > 0.0f) {
        fprintf(stderr, "ERROR: Test failed.\n");
        return 1;
    }

    std::cout << "Test Passed" << std::endl;
    return 0;
}
