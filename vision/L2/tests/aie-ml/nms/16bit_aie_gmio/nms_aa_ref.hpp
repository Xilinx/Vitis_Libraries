#ifndef _NMS_REF_FL_HPP
#define _NMS_REF_FL_HPP

#include <cmath>
#include <math.h>

using namespace std;

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;

struct _Box {
    float ymin = 0.0f;
    float ymax = 0.0f;
    float xmin = 0.0f;
    float xmax = 0.0f;
    float score = 0.0f;
};

// This function computes intersection between two boxes
float compIOU(_Box box_i, _Box box_j) {
    float area_i = (box_i.ymax - box_i.ymin) * (box_i.xmax - box_i.xmin);
    float area_j = (box_j.ymax - box_j.ymin) * (box_j.xmax - box_j.xmin);
    if (area_i <= 0 || area_j <= 0) return 0.0f;
    float intersection_ymin = std::max<float>(box_i.ymin, box_j.ymin);
    float intersection_xmin = std::max<float>(box_i.xmin, box_j.xmin);
    float intersection_ymax = std::min<float>(box_i.ymax, box_j.ymax);
    float intersection_xmax = std::min<float>(box_i.xmax, box_j.xmax);
    float intersection_area = std::max<float>(intersection_ymax - intersection_ymin, 0.0f) *
                              std::max<float>(intersection_xmax - intersection_xmin, 0.0f);
    return (intersection_area / (area_i + area_j - intersection_area));
}

int IOU(_Box a[], int N, float IOU_thresh, _Box out[]) {
    std::vector<uint8_t> active_box_candidate(N, 1);
    int total_valid = 0;

    int num_active_candidate = N;

    for (int i = 0; i < N; ++i) {
        if (num_active_candidate == 0) break;

        if (active_box_candidate[i] == 1) {
            out[total_valid] = a[i];
            total_valid++;
            active_box_candidate[i] = 0;
            num_active_candidate--;
        } else {
            continue;
        }
        for (int j = i + 1; j < N; ++j) {
            if (active_box_candidate[j] == 1) {
                float intersection_over_union = compIOU(a[i], a[j]);
                if (intersection_over_union > IOU_thresh) {
                    active_box_candidate[j] = 0;
                    num_active_candidate--;
                }
            }
        }
    }
    return total_valid;
}

// Inputs: ymin, xmin, ymax, xmax
// Outputs: outBoxes
template <int _MAX_VALID_>
void nms_ref(int16_t* _ymin,
             int16_t* _xmin,
             int16_t* _ymax,
             int16_t* _xmax,
             int16_t* _out,
             float iou_threshold,
             int max_det,
             int total_valid_box) {
    _Box dv_box[_MAX_VALID_];
    _Box out[_MAX_VALID_];

    for (int i = 0; i < _MAX_VALID_; i++) {
        dv_box[i].ymin = (float)_ymin[i];
        dv_box[i].xmin = (float)_xmin[i];
        dv_box[i].ymax = (float)_ymax[i];
        dv_box[i].xmax = (float)_xmax[i];
    }

    int total_det_box = IOU(dv_box, total_valid_box, iou_threshold, out);
    total_det_box = (total_det_box > (int)max_det) ? max_det : total_det_box;

    _out[0] = (uint16_t)total_det_box;
    for (int i = 0; i < total_det_box; i++) {
        _out[i * 4 + 1] = static_cast<uint16_t>(round(out[i].ymin));
        _out[i * 4 + 2] = static_cast<uint16_t>(round(out[i].xmin));
        _out[i * 4 + 3] = static_cast<uint16_t>(round(out[i].ymax));
        _out[i * 4 + 4] = static_cast<uint16_t>(round(out[i].xmax));
    }
}

#endif
