/*
 * Copyright 2019 Xilinx, Inc.
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

#include "common/xf_headers.hpp"
#include "xf_warp_transform_config.h"

// changing transformation matrix dimensions with transform Affine 2x3, Perspecitve 3x3
#if TRANSFORM_TYPE == 1
#define TRMAT_DIM2 3
#define TRMAT_DIM1 3
#else
#define TRMAT_DIM2 3
#define TRMAT_DIM1 2
#endif

// Random Number generator limits
#define M_NUMI1 1
#define M_NUMI2 20

// image operations and transformation matrix input format
typedef float image_oper;

int main(int argc, char* argv[]) {
    cv::RNG rng;
    cv::Mat diff_img, image_input, image_output, opencv_image;
    image_oper R[9];
    cv::Mat _transformation_matrix(TRMAT_DIM1, TRMAT_DIM2, CV_32FC1);
    cv::Mat _transformation_matrix_2(TRMAT_DIM1, TRMAT_DIM2, CV_32FC1);
#if TRANSFORM_TYPE == 1
    cv::Point2f src_p[4];
    cv::Point2f dst_p[4];
    src_p[0] = cv::Point2f(0.0f, 0.0f);
    src_p[1] = cv::Point2f(WIDTH - 1, 0.0f);
    src_p[2] = cv::Point2f(WIDTH - 1, HEIGHT - 1);
    src_p[3] = cv::Point2f(0.0f, HEIGHT - 1);
    //	  to points
    dst_p[0] = cv::Point2f(rng.uniform(int(M_NUMI1), int(M_NUMI2)), rng.uniform(int(M_NUMI1), int(M_NUMI2)));
    dst_p[1] = cv::Point2f(WIDTH - rng.uniform(int(M_NUMI1), int(M_NUMI2)), rng.uniform(int(M_NUMI1), int(M_NUMI2)));
    dst_p[2] =
        cv::Point2f(WIDTH - rng.uniform(int(M_NUMI1), int(M_NUMI2)), HEIGHT - rng.uniform(int(M_NUMI1), int(M_NUMI2)));
    dst_p[3] = cv::Point2f(rng.uniform(int(M_NUMI1), int(M_NUMI2)), HEIGHT - rng.uniform(int(M_NUMI1), int(M_NUMI2)));

    _transformation_matrix = cv::getPerspectiveTransform(dst_p, src_p);
    cv::Mat transform_mat = _transformation_matrix;
#else
    cv::Point2f src_p[3];
    cv::Point2f dst_p[3];
    src_p[0] = cv::Point2f(0.0f, 0.0f);
    src_p[1] = cv::Point2f(WIDTH - 1, 0.0f);
    src_p[2] = cv::Point2f(0.0f, HEIGHT - 1);
    //	  to points
    dst_p[0] = cv::Point2f(rng.uniform(int(M_NUMI1), int(M_NUMI2)), rng.uniform(int(M_NUMI1), int(M_NUMI2)));
    dst_p[1] = cv::Point2f(WIDTH - rng.uniform(int(M_NUMI1), int(M_NUMI2)), rng.uniform(int(M_NUMI1), int(M_NUMI2)));
    dst_p[2] = cv::Point2f(rng.uniform(int(M_NUMI1), int(M_NUMI2)), HEIGHT - rng.uniform(int(M_NUMI1), int(M_NUMI2)));

    _transformation_matrix = cv::getAffineTransform(dst_p, src_p);
    cv::Mat transform_mat = _transformation_matrix;
#endif
    int i = 0, j = 0;

    std::cout << "Transformation Matrix \n";
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
#if TRANSFORM_TYPE == 1
            R[i * 3 + j] = image_oper(transform_mat.at<double>(i, j));
            _transformation_matrix_2.at<image_oper>(i, j) = image_oper(transform_mat.at<double>(i, j));
#else
            if (i == 2) {
                R[i * 3 + j] = 0;
            } else {
                R[i * 3 + j] = image_oper(transform_mat.at<double>(i, j));
                _transformation_matrix_2.at<image_oper>(i, j) = image_oper(transform_mat.at<double>(i, j));
            }
#endif
            std::cout << R[i * 3 + j] << " ";
        }
        std::cout << "\n";
    }

#if RGBA
    image_input = cv::imread(argv[1], 1);
    image_output.create(image_input.rows, image_input.cols, CV_8UC3);
    diff_img.create(image_input.rows, image_input.cols, CV_8UC3);
    opencv_image = cv::Mat::zeros(image_input.rows, image_input.cols, CV_8UC3);
#else
    image_input = cv::imread(argv[1], 0);
    image_output.create(image_input.rows, image_input.cols, CV_8UC1);
    diff_img.create(image_input.rows, image_input.cols, CV_8UC1);
    opencv_image = cv::Mat::zeros(image_input.rows, image_input.cols, CV_8UC1);
#endif

    if (image_input.data == NULL) {
        printf("Failed to load the image ... %s\n!", argv[1]);
        return -1;
    }

#if TRANSFORM_TYPE == 1
#if INTERPOLATION == 1
    cv::warpPerspective(image_input, opencv_image, _transformation_matrix_2,
                        cv::Size(image_input.cols, image_input.rows), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP,
                        cv::BORDER_TRANSPARENT, 80);
#else
    cv::warpPerspective(image_input, opencv_image, _transformation_matrix_2,
                        cv::Size(image_input.cols, image_input.rows), cv::INTER_NEAREST + cv::WARP_INVERSE_MAP,
                        cv::BORDER_TRANSPARENT, 80);
#endif
#else
#if INTERPOLATION == 1
    cv::warpAffine(image_input, opencv_image, _transformation_matrix_2, cv::Size(image_input.cols, image_input.rows),
                   cv::INTER_LINEAR + cv::WARP_INVERSE_MAP, cv::BORDER_TRANSPARENT, 80);
#else
    cv::warpAffine(image_input, opencv_image, _transformation_matrix_2, cv::Size(image_input.cols, image_input.rows),
                   cv::INTER_NEAREST + cv::WARP_INVERSE_MAP, cv::BORDER_TRANSPARENT, 80);
#endif
#endif

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> _src(image_input.rows, image_input.cols);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> _dst(image_input.rows, image_input.cols);

    _src.copyTo(image_input.data);

    warp_transform_accel(_src, _dst, R);

    image_output.data = _dst.copyFrom();
    cv::imwrite("hls_output.png", image_output);
    char output_opencv[] = "opencv_output.png";
    cv::imwrite(output_opencv, opencv_image);

    ap_uint8_t temp_px1 = 0, temp_px2 = 0, max_err = 0, min_err = 255;
    int num_errs = 0, num_errs1 = 0;
    float err_per = 0;
    cv::absdiff(opencv_image, image_output, diff_img);
    xf::cv::analyzeDiff(diff_img, 1, err_per);

    if (err_per > 0.05) {
        return -1;
    } else {
        return 0;
    }
}
