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

#include "common/xf_headers.hpp"
#include "xf_pyr_dense_optical_flow_tb_config.h"

#include "opencv2/video.hpp"
#include <time.h>

// #include "xcl2.hpp"
/* Color Coding */
// kernel returns this type. Packed strcuts on axi ned to be powers-of-2.
typedef struct __rgba {
    TYPE r, g, b;
    TYPE a; // can be unused
} rgba_t;
typedef struct __rgb { TYPE r, g, b; } rgb_t;

typedef cv::Vec<unsigned short, 3> Vec3u;
typedef cv::Vec<TYPE, 3> Vec3ucpt;

const float powTwo15 = pow(2, 15);
#define THRESHOLD 3.0
#define THRESHOLD_R 3.0
/* color coding */
#define NORM_FAC 10
// custom, hopefully, low cost colorizer.
void getPseudoColorInt(TYPE pix, float fx, float fy, rgba_t& rgba) {
    // TODO get the normFac from the host as cmdline arg
    const int normFac = 10;

    int y = 127 + (int)(fy * normFac);
    int x = 127 + (int)(fx * normFac);
    if (y > 255) y = 255;
    if (y < 0) y = 0;
    if (x > 255) x = 255;
    if (x < 0) x = 0;

    rgb_t rgb;
    if (x > 127) {
        if (y < 128) {
            // 1 quad
            rgb.r = x - 127 + (127 - y) / 2;
            rgb.g = (127 - y) / 2;
            rgb.b = 0;
        } else {
            // 4 quad
            rgb.r = x - 127;
            rgb.g = 0;
            rgb.b = y - 127;
        }
    } else {
        if (y < 128) {
            // 2 quad
            rgb.r = (127 - y) / 2;
            rgb.g = 127 - x + (127 - y) / 2;
            rgb.b = 0;
        } else {
            // 3 quad
            rgb.r = 0;
            rgb.g = 128 - x;
            rgb.b = y - 127;
        }
    }

    rgba.r = pix * 1 / 2 + rgb.r * 1 / 2;
    rgba.g = pix * 1 / 2 + rgb.g * 1 / 2;
    rgba.b = pix * 1 / 2 + rgb.b * 1 / 2;
    rgba.a = 0;
}

void pyrof_hw(cv::Mat im0,
              cv::Mat im1,
              cv::Mat flowUmat,
              cv::Mat flowVmat,
              xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX>& flow,
              xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX>& flow_iter,
              xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX> mat_imagepyr1[NUM_LEVELS],
              xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX> mat_imagepyr2[NUM_LEVELS],
              int pyr_h[NUM_LEVELS],
              int pyr_w[NUM_LEVELS]) {
    for (int l = 0; l < NUM_LEVELS; l++) {
        mat_imagepyr1[l].rows = pyr_h[l];
        mat_imagepyr1[l].cols = pyr_w[l];
        mat_imagepyr1[l].size = pyr_h[l] * pyr_w[l];
        mat_imagepyr2[l].rows = pyr_h[l];
        mat_imagepyr2[l].cols = pyr_w[l];
        mat_imagepyr2[l].size = pyr_h[l] * pyr_w[l];
    }

    // mat_imagepyr1[0].copyTo(im0.data);
    // mat_imagepyr2[0].copyTo(im1.data);

    for (int i = 0; i < pyr_h[0]; i++) {
        for (int j = 0; j < pyr_w[0]; j++) {
            mat_imagepyr1[0].write(i * pyr_w[0] + j, im0.data[i * pyr_w[0] + j]);
            mat_imagepyr2[0].write(i * pyr_w[0] + j, im1.data[i * pyr_w[0] + j]);
        }
    }
    // creating image pyramid
    for (int pyr_comp = 0; pyr_comp < NUM_LEVELS - 1; pyr_comp++) {
        pyr_dense_optical_flow_pyr_down_accel((ap_uint<INPUT_PTR_WIDTH>*)mat_imagepyr1[pyr_comp].data,
                                              (ap_uint<OUTPUT_PTR_WIDTH>*)mat_imagepyr1[pyr_comp + 1].data,
                                              mat_imagepyr1[pyr_comp].rows, mat_imagepyr1[pyr_comp].cols,
                                              mat_imagepyr1[pyr_comp + 1].rows, mat_imagepyr1[pyr_comp + 1].cols);
        pyr_dense_optical_flow_pyr_down_accel((ap_uint<INPUT_PTR_WIDTH>*)mat_imagepyr2[pyr_comp].data,
                                              (ap_uint<OUTPUT_PTR_WIDTH>*)mat_imagepyr2[pyr_comp + 1].data,
                                              mat_imagepyr2[pyr_comp].rows, mat_imagepyr2[pyr_comp].cols,
                                              mat_imagepyr2[pyr_comp + 1].rows, mat_imagepyr2[pyr_comp + 1].cols);
    }

    bool flag_flowin = 1;
    flow.rows = pyr_h[NUM_LEVELS - 1];
    flow.cols = pyr_w[NUM_LEVELS - 1];
    flow.size = pyr_h[NUM_LEVELS - 1] * pyr_w[NUM_LEVELS - 1];
    flow_iter.rows = pyr_h[NUM_LEVELS - 1];
    flow_iter.cols = pyr_w[NUM_LEVELS - 1];
    flow_iter.size = pyr_h[NUM_LEVELS - 1] * pyr_w[NUM_LEVELS - 1];

    for (int l = NUM_LEVELS - 1; l >= 0; l--) {
        // compute current level height
        int curr_height = pyr_h[l];
        int curr_width = pyr_w[l];

        // compute the flow vectors for the current pyramid level iteratively
        for (int iterations = 0; iterations < NUM_ITERATIONS; iterations++) {
            bool scale_up_flag = (iterations == 0) && (l != NUM_LEVELS - 1);
            int next_height = (scale_up_flag == 1) ? pyr_h[l + 1] : pyr_h[l];
            int next_width = (scale_up_flag == 1) ? pyr_w[l + 1] : pyr_w[l];
            float scale_in = (next_height - 1) * 1.0 / (curr_height - 1);
            ap_uint<1> init_flag = ((iterations == 0) && (l == NUM_LEVELS - 1)) ? 1 : 0;
            if (flag_flowin) {
                flow.rows = pyr_h[l];
                flow.cols = pyr_w[l];
                flow.size = pyr_h[l] * pyr_w[l];
                pyr_dense_optical_flow_accel(
                    (ap_uint<INPUT_PTR_WIDTH>*)mat_imagepyr1[l].data, (ap_uint<INPUT_PTR_WIDTH>*)mat_imagepyr2[l].data,
                    (ap_uint<OUTPUT_PTR_WIDTH>*)flow_iter.data, (ap_uint<OUTPUT_PTR_WIDTH>*)flow.data, l, scale_up_flag,
                    scale_in, init_flag, mat_imagepyr1[l].rows, mat_imagepyr1[l].cols, mat_imagepyr2[l].rows,
                    mat_imagepyr2[l].cols, flow_iter.rows, flow_iter.cols, flow.rows, flow.cols);
                flag_flowin = 0;
            } else {
                flow_iter.rows = pyr_h[l];
                flow_iter.cols = pyr_w[l];
                flow_iter.size = pyr_h[l] * pyr_w[l];
                pyr_dense_optical_flow_accel(
                    (ap_uint<INPUT_PTR_WIDTH>*)mat_imagepyr1[l].data, (ap_uint<INPUT_PTR_WIDTH>*)mat_imagepyr2[l].data,
                    (ap_uint<OUTPUT_PTR_WIDTH>*)flow.data, (ap_uint<OUTPUT_PTR_WIDTH>*)flow_iter.data, l, scale_up_flag,
                    scale_in, init_flag, mat_imagepyr1[l].rows, mat_imagepyr1[l].cols, mat_imagepyr2[l].rows,
                    mat_imagepyr2[l].cols, flow.rows, flow.cols, flow_iter.rows, flow_iter.cols);
                flag_flowin = 1;
            }
        } // end iterative coptical flow computation
    }     // end pyramidal iterative optical flow HLS computation

    // write output flow vectors to Mat after splitting the bits.
    for (int i = 0; i < pyr_h[0]; i++) {
        for (int j = 0; j < pyr_w[0]; j++) {
            unsigned int tempcopy = 0;
            if (flag_flowin) {
                // tempcopy = *(flow_iter.data + i*pyr_w[0] + j);
                tempcopy = flow_iter.read(i * pyr_w[0] + j);
            } else {
                // tempcopy = *(flow.data + i*pyr_w[0] + j);
                tempcopy = flow.read(i * pyr_w[0] + j);
            }

            short splittemp1 = (tempcopy >> 16);
            short splittemp2 = (0x0000FFFF & tempcopy);

            TYPE_FLOW_TYPE* uflow = (TYPE_FLOW_TYPE*)&splittemp1;
            TYPE_FLOW_TYPE* vflow = (TYPE_FLOW_TYPE*)&splittemp2;

            flowUmat.at<float>(i, j) = (float)*uflow;
            flowVmat.at<float>(i, j) = (float)*vflow;
        }
    }
    return;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage incorrect! Correct usage: ./exe <current image> <next image>\n");
        return -1;
    }
    // allocating memory spaces for all the hardware operations
    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX> imagepyr1[NUM_LEVELS];
    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX> imagepyr2[NUM_LEVELS];
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX> flow;
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX> flow_iter;

    for (int i = 0; i < NUM_LEVELS; i++) {
        imagepyr1[i].init(HEIGHT, WIDTH);
        imagepyr2[i].init(HEIGHT, WIDTH);
    }
    flow.init(HEIGHT, WIDTH);
    flow_iter.init(HEIGHT, WIDTH);

    // initializing flow pointers to 0
    // initializing flow vector with 0s
    cv::Mat init_mat = cv::Mat::zeros(HEIGHT, WIDTH, CV_32SC1);
    flow_iter.copyTo((XF_PTSNAME(OUT_TYPE, NPPCX)*)init_mat.data);
    flow.copyTo((XF_PTSNAME(OUT_TYPE, NPPCX)*)init_mat.data);
    init_mat.release();

    cv::Mat im0, im1;

    // Read the file
    im0 = cv::imread(argv[1], 0);
    im1 = cv::imread(argv[2], 0);
    if (im0.empty()) {
        fprintf(stderr, "Loading image 1 failed, exiting!!\n");
        return -1;
    } else if (im1.empty()) {
        fprintf(stderr, "Loading image 2 failed, exiting!!\n");
        return -1;
    }
    std::cout << "Input image height : " << im0.rows << std::endl;
    std::cout << "Input image width  : " << im0.cols << std::endl;

    // OpenCV Implementation
    cv::Mat glx_cv = cv::Mat::zeros(im0.size(), CV_32F);
    cv::Mat gly_cv = cv::Mat::zeros(im0.size(), CV_32F);
    int rows = im0.rows, cols = im0.cols;

    std::vector<cv::Point2f> pt0, pt1;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) pt0.push_back(cv::Point2f(j, i));
    const cv::Size winSize(WINSIZE_OFLOW, WINSIZE_OFLOW);
    std::vector<uchar> status;
    std::vector<float> err;

    // TIMER START CODE
    struct timespec begin_hw, end_hw;
    clock_gettime(CLOCK_REALTIME, &begin_hw);

    cv::calcOpticalFlowPyrLK(im0, im1, pt0, pt1, status, err, winSize, NUM_LEVELS);

    // TIMER END CODE
    clock_gettime(CLOCK_REALTIME, &end_hw);
    long seconds, nanoseconds;
    double hw_time;
    seconds = end_hw.tv_sec - begin_hw.tv_sec;
    nanoseconds = end_hw.tv_nsec - begin_hw.tv_nsec;
    hw_time = seconds + nanoseconds * 1e-9;
    hw_time = hw_time * 1e3;

    for (int i = 0; i < pt0.size(); i++) {
        if (status[i]) {
            glx_cv.at<float>((int)pt0[i].y, (int)pt0[i].x) = pt1[i].x - pt0[i].x;
            gly_cv.at<float>((int)pt0[i].y, (int)pt0[i].x) = pt1[i].y - pt0[i].y;
        }
    }

    // Auviz Hardware implementation
    cv::Mat glx(im0.size(), CV_32F, cv::Scalar::all(0)); // flow at each level is updated in this variable
    cv::Mat gly(im0.size(), CV_32F, cv::Scalar::all(0));
    /***********************************************************************************/
    // Setting image sizes for each pyramid level
    int pyr_w[NUM_LEVELS], pyr_h[NUM_LEVELS];
    pyr_h[0] = im0.rows;
    pyr_w[0] = im0.cols;
    for (int lvls = 1; lvls < NUM_LEVELS; lvls++) {
        pyr_w[lvls] = (pyr_w[lvls - 1] + 1) >> 1;
        pyr_h[lvls] = (pyr_h[lvls - 1] + 1) >> 1;
    }

    std::cout << "Input Image Bit Depth:" << XF_DTPIXELDEPTH(IN_TYPE, NPPCX) << std::endl;
    std::cout << "Input Image Channels:" << XF_CHANNELS(IN_TYPE, NPPCX) << std::endl;
    std::cout << "NPPC:" << NPPCX << std::endl;

    // call the hls optical flow implementation
    pyrof_hw(im0, im1, glx, gly, flow, flow_iter, imagepyr1, imagepyr2, pyr_h, pyr_w);

    // output file names for the current case
    char colorout_filename[20] = "flow_image.png";
    char colorout_filename_cv[20] = "flow_image_cv.png";

    // Color code the flow vectors on original image
    Vec3ucpt color_px;

    cv::Mat color_code_img;
    color_code_img.create(im0.size(), CV_8UC3);
    for (int rc = 0; rc < im0.rows; rc++) {
        for (int cc = 0; cc < im0.cols; cc++) {
            rgba_t colorcodedpx;
            getPseudoColorInt(im0.at<unsigned char>(rc, cc), glx.at<float>(rc, cc), gly.at<float>(rc, cc),
                              colorcodedpx);
            color_px = Vec3ucpt(colorcodedpx.b, colorcodedpx.g, colorcodedpx.r);
            color_code_img.at<Vec3ucpt>(rc, cc) = color_px;
        }
    }
    cv::imwrite(colorout_filename, color_code_img);
    color_code_img.release();

    cv::Mat color_code_img_cv;
    color_code_img_cv.create(im0.size(), CV_8UC3);
    for (int rc = 0; rc < im0.rows; rc++) {
        for (int cc = 0; cc < im0.cols; cc++) {
            rgba_t colorcodedpx;
            getPseudoColorInt(im0.at<unsigned char>(rc, cc), glx_cv.at<float>(rc, cc), gly_cv.at<float>(rc, cc),
                              colorcodedpx);
            color_px = Vec3ucpt(colorcodedpx.b, colorcodedpx.g, colorcodedpx.r);
            color_code_img_cv.at<Vec3ucpt>(rc, cc) = color_px;
        }
    }
    cv::imwrite(colorout_filename_cv, color_code_img_cv);
    color_code_img_cv.release();
// end color coding

#if __XF_BENCHMARK == 0
    const int ERROR_THRESH = 5;
    cv::Mat glx_diff, gly_diff, glx_diff_thresh, gly_diff_thresh;
    cv::absdiff(glx, glx_cv, glx_diff);
    cv::absdiff(gly, gly_cv, gly_diff);

    double u_min, u_max, v_min, v_max, min, max;
    cv::minMaxLoc(glx_diff, &u_min, &u_max);
    cv::minMaxLoc(gly_diff, &v_min, &v_max);
    min = (u_min < v_min) ? u_min : v_min;
    max = (u_max > v_max) ? u_max : v_max;

    glx_diff = glx_diff * NORM_FAC;
    gly_diff = gly_diff * NORM_FAC;
    threshold(glx_diff, glx_diff_thresh, ERROR_THRESH, 1, cv::THRESH_BINARY_INV);
    threshold(gly_diff, gly_diff_thresh, ERROR_THRESH, 1, cv::THRESH_BINARY_INV);
    cv::Mat valid_out = glx_diff_thresh.mul(gly_diff_thresh);

    int pixels_under_thresh = cv::countNonZero(valid_out);
    int total_pixels = im0.rows * im0.cols;
    int pixels_above_thresh = total_pixels - pixels_under_thresh;
    float pixels_above_thresh_per = ((float)pixels_above_thresh) * 100.0 / total_pixels;

    std::cout << "        Minimum error in intensity = " << min << std::endl;
    std::cout << "        Maximum error in intensity = " << max << std::endl;
    std::cout << "        Percentage of pixels above error threshold = " << pixels_above_thresh_per << std::endl;
    if (pixels_above_thresh_per > 20) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
        return -1;
    } else
        std::cout << "Test Passed " << std::endl;
#else
    std::cout.precision(3);
    std::cout << std::fixed;
    std::cout << "Latency for CPU function is " << hw_time << "ms" << std::endl;
#endif

    // releaseing mats and pointers created inside the main for loop
    glx.release();
    gly.release();
    glx_cv.release();
    gly_cv.release();
    im0.release();
    im1.release();
    return 0;
}
