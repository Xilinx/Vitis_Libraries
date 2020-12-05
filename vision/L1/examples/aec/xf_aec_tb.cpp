
#include "common/xf_headers.hpp"
#include "xf_aec_config.h"
#include <math.h>

// OpenCV reference function:
void AEC_ref(cv::Mat& _src, cv::Mat& _dst) {
    // Temporary matrices for processing

    cv::Mat mask1, subimg;

    cv::Mat yuvimage, yuvimageop, finalop;

    cv::cvtColor(_src, yuvimage, CV_BGR2HSV);

    cv::Mat yuvchannels[3];

    split(yuvimage, yuvchannels);

    cv::equalizeHist(yuvchannels[2], yuvchannels[2]);

    cv::merge(yuvchannels, 3, yuvimageop);

    cv::cvtColor(yuvimageop, _dst, CV_HSV2BGR);
}

int main(int argc, char** argv) {
    cv::Mat in_img, out_img_hls, diff, img_rgba, out_img, out_img1, ocv_ref;

    in_img = cv::imread(argv[1], 1);
    if (!in_img.data) {
        return -1;
    }

    imwrite("input_3.jpg", in_img);

    int height = in_img.rows;
    int width = in_img.cols;

    out_img.create(in_img.rows, in_img.cols, CV_8UC3);
    out_img_hls.create(in_img.rows, in_img.cols, CV_8UC3);
    diff.create(in_img.rows, in_img.cols, CV_8UC3);

    AEC_ref(in_img, out_img);

    for (int i = 0; i < 2; i++) {
        aec_accel((ap_uint<INPUT_PTR_WIDTH>*)in_img.data, (ap_uint<OUTPUT_PTR_WIDTH>*)out_img_hls.data, height, width);
    }

    // Write output image
    cv::imwrite("hls_out.jpg", out_img_hls);
    cv::imwrite("ocv_out.jpg", out_img);

    // Compute absolute difference image
    cv::absdiff(out_img_hls, out_img, diff);
    // Save the difference image for debugging purpose:
    cv::imwrite("error.png", diff);
    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);

    if (err_per > 0.0f) {
        return 1;
    }
    return 0;
}
