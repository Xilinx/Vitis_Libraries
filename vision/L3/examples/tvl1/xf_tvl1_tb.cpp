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
#include "xf_config_params.h"
#include "video/xf_tvl1_wrapper.hpp"
#include "opencv2/optflow.hpp"

using namespace std;
using namespace cv;
using namespace std::chrono;

typedef unsigned char pix_t;
typedef unsigned char IN_TYPE2;

typedef struct __rgba {
    IN_TYPE2 r, g, b;
    IN_TYPE2 a; // can be unused
} rgba_t;
typedef struct __rgb { IN_TYPE2 r, g, b; } rgb_t;

static void getPseudoColorInt(pix_t pix, float fx, float fy, rgba_t& rgba) {
    // normalization factor is key for good visualization. Make this auto-ranging
    // or controllable from the host TODO
    // const int normFac = 127/2;
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

    rgba.r = pix / 4 + 3 * rgb.r / 4;
    rgba.g = pix / 4 + 3 * rgb.g / 4;
    rgba.b = pix / 4 + 3 * rgb.b / 4;
    rgba.a = 255;
    // rgba.r = rgb.r;
    // rgba.g = rgb.g;
    // rgba.b = rgb.b ;
}

void writeOpticalFlowToFile(const Mat_<Point2f>& flow, const string& fileName) {
    const char FLO_TAG_STRING[] = "PIEH"; // use this when WRITING the file
    ofstream file(fileName.c_str(), ios_base::binary);

    file << FLO_TAG_STRING;

    file.write((const char*)&flow.cols, sizeof(int));
    file.write((const char*)&flow.rows, sizeof(int));

    for (int i = 0; i < flow.rows; ++i) {
        for (int j = 0; j < flow.cols; ++j) {
            const Point2f u = flow(i, j);

            file.write((const char*)&u.x, sizeof(float));
            file.write((const char*)&u.y, sizeof(float));
        }
    }
}

void Write_FLowimage(cv::Mat frame1, Mat_<Point2f> flow, bool hwout_en) {
    const string gold_flow_path = "tvl1_flow.flo";
    writeOpticalFlowToFile(flow, gold_flow_path);

    rgba_t pix;
    Point2f u;
    cv::Mat frame_out(frame1.size(), CV_8UC4);

    for (int r = 0; r < frame1.rows; r++) {
        for (int c = 0; c < frame1.cols; c++) {
            u = flow(r, c);

            getPseudoColorInt(frame1.at<pix_t>(r, c), u.x, u.y, pix);

            frame_out.at<unsigned int>(r, c) = ((unsigned int)pix.a << 24 | (unsigned int)pix.b << 16 |
                                                (unsigned int)pix.g << 8 | (unsigned int)pix.r);
        }
    }

    string out_img;
    if (hwout_en == 0)
        out_img = "./flow_out_sw_tvl1.png";
    else
        out_img = "./flow_out_hw_tvl1.png";

    cv::imwrite(out_img, frame_out);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Current_frame> <Next_frame> \n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat current_frame = cv::imread(argv[1], 0);
    cv::Mat next_frame = cv::imread(argv[2], 0);

    std::cout << "Run with SW TVL1 optical flow" << std::endl;
    Mat_<Point2f> flow;
    Ptr<cv::optflow::DualTVL1OpticalFlow> tvl1 = cv::optflow::DualTVL1OpticalFlow::create();

    auto t1_sw = std::chrono::system_clock::now();
    // SW TVL1 call- Load two frame and create image pyaramid. TVL1 processsing on current two frames.
    tvl1->calc(current_frame, next_frame, flow);
    auto t2_sw = std::chrono::system_clock::now();
    auto value_t1_sw = std::chrono::duration_cast<std::chrono::microseconds>(t2_sw - t1_sw);
    std::cout << "SW TVL1 latency: " << (float)(value_t1_sw.count() * 1000) << " msec\n";

    // Generate output image using "flow" matrix
    Write_FLowimage(next_frame, flow, 0);

    std::cout << "Run with HW accelerator:TVL1 optical flow" << std::endl;
    Mat_<Point2f> flow_hw;
    Ptr<xf::cv::DualTVL1OpticalFlow> tvl1_xfcv = xf::cv::DualTVL1OpticalFlow::create();

    // HW TVL1 init
    tvl1_xfcv->init(current_frame.rows, current_frame.cols);

    // HW TVL1 call- Load first two frames and create image pyaramid. No TVL1 processsing
    tvl1_xfcv->calc(current_frame, flow_hw);
    tvl1_xfcv->calc(next_frame, flow_hw);

    auto t1_hw = std::chrono::system_clock::now();
    // HW TVL1 call- Load next frame and create image pyaramid. TVL1 processsing on previous two frames.
    tvl1_xfcv->calc(next_frame, flow_hw);
    auto t2_hw = std::chrono::system_clock::now();
    auto value_t1_hw = std::chrono::duration_cast<std::chrono::microseconds>(t2_hw - t1_hw);
    std::cout << "HW TVL1 latency: " << (float)(value_t1_hw.count() * 1000) << " msec\n";

    // Generate output image using "flow_hw" matrix
    Write_FLowimage(next_frame, flow_hw, 1);

    return 0;
}