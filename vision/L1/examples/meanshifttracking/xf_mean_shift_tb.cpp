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
#include "xf_mean_shift_tb_config.h"

#define _DISPLAY_TRACKING_ 0

float range_[] = {0, 180};
const float* range[] = {range_};
int histSize[] = {180};
int channels[] = {0};
cv::TermCriteria term_crit(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 10, 1);

struct _Box {
    float ymin = 0.0f;
    float ymax = 0.0f;
    float xmin = 0.0f;
    float xmax = 0.0f;
    float score = 0.0f;
};

class MST_REF {
   public:
    int no_of_obj_to_track;
    MST_REF(int num) { no_of_obj_to_track = num; }
    void setup_mst(cv::Mat& frame, std::vector<cv::Mat>& roi_hist, std::vector<cv::Rect>& track_window) {
        // set up the ROI for tracking
        cv::Mat roi, hsv_roi, mask;
        for (int i = 0; i < no_of_obj_to_track; i++) {
            roi = frame(track_window[i]);
            cv::cvtColor(roi, hsv_roi, cv::COLOR_BGR2HSV);
            cv::inRange(hsv_roi, cv::Scalar(0, 60, 32), cv::Scalar(180, 255, 255), mask);
            cv::calcHist(&hsv_roi, 1, channels, mask, roi_hist[i], 1, histSize, range);
            cv::normalize(roi_hist[i], roi_hist[i], 0, 255, cv::NORM_MINMAX);
        }
    }

    void track_mst(cv::Mat& frame, std::vector<cv::Mat>& roi_hist, std::vector<cv::Rect>& track_window) {
        cv::Mat hsv, dst;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
        for (int i = 0; i < no_of_obj_to_track; i++) {
            cv::calcBackProject(&hsv, 1, channels, roi_hist[i], dst, range);
            // apply meanshift to get the new location
            cv::meanShift(dst, track_window[i], term_crit);
        }
    }
};

float compIOU(_Box box_i, _Box box_j) {
    float area_i = (box_i.ymax - box_i.ymin) * (box_i.xmax - box_i.xmin);
    float area_j = (box_j.ymax - box_j.ymin) * (box_j.xmax - box_j.xmin);
    if (area_i <= 0 || area_j <= 0) return 0.0;
    float intersection_ymin = std::max<float>(box_i.ymin, box_j.ymin);
    float intersection_xmin = std::max<float>(box_i.xmin, box_j.xmin);
    float intersection_ymax = std::min<float>(box_i.ymax, box_j.ymax);
    float intersection_xmax = std::min<float>(box_i.xmax, box_j.xmax);
    float intersection_area = std::max<float>(intersection_ymax - intersection_ymin, 0.0) *
                              std::max<float>(intersection_xmax - intersection_xmin, 0.0);
    return (intersection_area / (area_i + area_j - intersection_area));
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr,
                "Missed input arguments. Usage: <executable> <path to input video file or image path> <Number of "
                "objects to be tracked>\n");
        return -1;
    }

    char* path = argv[1];

#if VIDEO_INPUT
    cv::VideoCapture cap(path);
    if (!cap.isOpened()) // check if we succeeded
    {
        fprintf(stderr, "ERROR: Cannot open the video file");
        return -1;
    }
#endif
    uint8_t no_objects = atoi(argv[2]);

    uint16_t* c_x = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* c_y = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* h_x = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* h_y = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* tlx = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* tly = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* brx = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* bry = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* track = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* obj_width = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* obj_height = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* dx = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));
    uint16_t* dy = (uint16_t*)malloc(XF_MAX_OBJECTS * sizeof(uint16_t));

    for (int i = 0; i < no_objects; i++) {
        track[i] = 0;
        dx[i] = 0;
        dy[i] = 0;
    }

#if !VIDEO_INPUT
    MST_REF mst(no_objects);
    std::vector<cv::Mat> roi_hist(no_objects);
    std::vector<cv::Rect> track_window(no_objects);
#endif

    // object loop, for reading input to the object
    for (uint16_t i = 0; i < no_objects; i++) {
        h_x[i] = WIDTH_MST[i] / 2;
        h_y[i] = HEIGHT_MST[i] / 2;
        c_x[i] = X1[i] + h_x[i];
        c_y[i] = Y1[i] + h_y[i];

        obj_width[i] = WIDTH_MST[i];
        obj_height[i] = HEIGHT_MST[i];

        tlx[i] = X1[i];
        tly[i] = Y1[i];
        brx[i] = c_x[i] + h_x[i];
        bry[i] = c_y[i] + h_y[i];
        track[i] = 1;

#if !VIDEO_INPUT
        // OCV
        track_window[i].x = X1[i];
        track_window[i].y = Y1[i];
        track_window[i].height = HEIGHT_MST[i];
        track_window[i].width = WIDTH_MST[i];
#endif
    }

    cv::Mat frame, image;
    int no_of_frames = TOTAL_FRAMES;
    char nm[1000];

#if !VIDEO_INPUT
    sprintf(nm, "%s/img%d.png", path, 1);
    frame = cv::imread(nm, 1);
    mst.setup_mst(frame, roi_hist, track_window);
#endif

    for (int f_no = 1; f_no <= no_of_frames; f_no++) {
#if VIDEO_INPUT
        cap >> frame;
#else
        sprintf(nm, "%s/img%d.png", path, f_no);
        frame = cv::imread(nm, 1);
        // ocv tracking
        mst.track_mst(frame, roi_hist, track_window);
#endif

        image.create(frame.size(), frame.type());

        if (frame.empty()) {
            fprintf(stderr, "no image!\n ");
            break;
        }

        // convert to four channels with a dummy alpha channel for 32-bit data
        // transfer
        cvtColor(frame, image, cv::COLOR_BGR2RGBA);

        uint16_t rows = image.rows;
        uint16_t cols = image.cols;

        // set the status of the frame, set as '0' for the first frame
        uint8_t frame_status = 1;
        if (f_no - 1 == 0) frame_status = 0;

        uint8_t no_of_iterations = 4;

        printf("starting the kernel...\n");

        mean_shift_accel((ap_uint<INPUT_PTR_WIDTH>*)image.data, tlx, tly, obj_height, obj_width, dx, dy, track,
                         frame_status, no_objects, no_of_iterations, rows, cols);

        printf("end of kernel...\n");

        std::cout << "frame " << f_no << ":" << std::endl;
        for (int k = 0; k < no_objects; k++) {
            c_x[k] = dx[k];
            c_y[k] = dy[k];
            tlx[k] = c_x[k] - h_x[k];
            tly[k] = c_y[k] - h_y[k];
            brx[k] = c_x[k] + h_x[k];
            bry[k] = c_y[k] + h_y[k];

            std::cout << "box " << k + 1 << ":\n"
                      << "hls: " << c_x[k] << " " << c_y[k] << std::endl;
#if !VIDEO_INPUT
            std::cout << "ref: " << (track_window[k].x + track_window[k].width / 2) << " "
                      << (track_window[k].y + track_window[k].height / 2) << std::endl;
            _Box box_t;
            box_t.xmin = tlx[k];
            box_t.xmax = brx[k];
            box_t.ymin = tly[k];
            box_t.ymax = bry[k];
            _Box box_r;
            box_r.xmin = track_window[k].x;
            box_r.xmax = track_window[k].x + track_window[k].width;
            box_r.ymin = track_window[k].y;
            box_r.ymax = track_window[k].y + track_window[k].height;

            // test validity with the reference
            float iou_val = compIOU(box_t, box_r);
            if (iou_val > 0.8f)
                std::cout << "Test Passed .... !!!" << std::endl;
            else {
                fprintf(stderr, "ERROR: Test Failed.\n ");
                return -1;
            }
#endif
        }

        std::cout << std::endl;

#if _DISPLAY_TRACKING_
        // bounding box in the image for the object track representation
        if (track[0]) rectangle(frame, cvPoint(tlx[0], tly[0]), cvPoint(brx[0], bry[0]), cv::Scalar(0, 0, 255), 2);
        if (track[1]) rectangle(frame, cvPoint(tlx[1], tly[1]), cvPoint(brx[1], bry[1]), cv::Scalar(0, 255, 0), 2);
        if (track[2]) rectangle(frame, cvPoint(tlx[2], tly[2]), cvPoint(brx[2], bry[2]), cv::Scalar(255, 0, 0), 2);
        if (track[3]) rectangle(frame, cvPoint(tlx[3], tly[3]), cvPoint(brx[3], bry[3]), cv::Scalar(255, 255, 0), 2);
        if (track[4]) rectangle(frame, cvPoint(tlx[4], tly[4]), cvPoint(brx[4], bry[4]), cv::Scalar(255, 0, 255), 2);
        if (track[5]) rectangle(frame, cvPoint(tlx[5], tly[5]), cvPoint(brx[5], bry[5]), cv::Scalar(0, 255, 255), 2);
        if (track[6]) rectangle(frame, cvPoint(tlx[6], tly[6]), cvPoint(brx[6], bry[6]), cv::Scalar(128, 0, 128), 2);
        if (track[7]) rectangle(frame, cvPoint(tlx[7], tly[7]), cvPoint(brx[7], bry[7]), cv::Scalar(128, 128, 128), 2);
        if (track[8]) rectangle(frame, cvPoint(tlx[8], tly[8]), cvPoint(brx[8], bry[8]), cv::Scalar(255, 255, 255), 2);
        if (track[9]) rectangle(frame, cvPoint(tlx[9], tly[9]), cvPoint(brx[9], bry[9]), cv::Scalar(0, 0, 0), 2);

        cv::namedWindow("tracking demo", 0);
        imshow("tracking demo", frame);

        char c = (char)cv::waitKey(20);
        if (c == 27) // ESC button
            break;
#endif
    }

    return 0;
}
