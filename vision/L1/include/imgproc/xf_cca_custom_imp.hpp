/*
 * Copyright 2020 Xilinx, Inc.
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

#ifndef __XF_CCA_CUSTOM_HPP__
#define __XF_CCA_CUSTOM_HPP__

#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

namespace xf {
namespace cv {

template <int WIDTH>
void process_row(uint8_t* in_ptr, uint8_t* tmp_out_ptr, bool* lab_arr, int& obj_pix, int width) {
// clang-format off
#pragma HLS INLINE
    // clang-format on

    bool a, b, c, d;
    a = 1;
    b = lab_arr[0];
    c = lab_arr[1];
    d = 1;

PROC_ROW_LOOP:
    for (int j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
#pragma HLS PIPELINE II=1
        // clang-format on
        bool lab;
        unsigned char out;

        if (in_ptr[j] != 0) {
            if ((a || b || c || d) == 1) {
                lab = 1;
                out = 0;
            } else {
                lab = 0;
                out = 255;
            }
        } else {
            lab = 0;
            out = 0;
            obj_pix++;
        }

        lab_arr[j] = lab;
        tmp_out_ptr[j] = out;
        a = b;
        b = c;
        c = lab_arr[j + 2];
        d = lab;
    }
}
template <int SRC_T, int HEIGHT, int WIDTH, int NPC, int XFCVDEPTH_OUT>
void process_mat_row(xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& _src,
                     xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& tmp_out_ptr,
                     bool* lab_arr,
                     int& obj_pix,
                     int offset,
                     int width) {
// clang-format off
#pragma HLS INLINE
    // clang-format on

    bool a, b, c, d;
    a = 1;
    b = lab_arr[0];
    c = lab_arr[1];
    d = 1;

PROC_ROW_LOOP:
    for (int j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
#pragma HLS PIPELINE II=1
        // clang-format on
        bool lab;
        unsigned char out;

        if (_src.read(offset + j) != 0) {
            if ((a || b || c || d) == 1) {
                lab = 1;
                out = 0;
            } else {
                lab = 0;
                out = 255;
            }
        } else {
            lab = 0;
            out = 0;
            obj_pix++;
        }

        lab_arr[j] = lab;
        tmp_out_ptr.write(offset + j, out);
        a = b;
        b = c;
        c = lab_arr[j + 2];
        d = lab;
    }
}

template <int WIDTH>
void read_row_to_ram(uint8_t* _fw_pass, uint8_t* ram, int width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    for (int j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
#pragma HLS PIPELINE II=1
        // clang-format on
        ram[j] = _fw_pass[j];
    }
}

template <int SRC_T, int HEIGHT, int WIDTH, int NPC, int XFCVDEPTH_OUT>
void read_row_mat_to_ram(xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& _rev_pass,
                         uint8_t* ram,
                         int rd_offset,
                         int width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    for (int j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
#pragma HLS PIPELINE II=1
        // clang-format on
        ram[j] = _rev_pass.read(rd_offset + j);
    }
}

template <int SRC_T, int HEIGHT, int WIDTH, int NPC, int XFCVDEPTH_OUT>
void write_row_to_mem(uint8_t* ram,
                      xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& tmp_out_mat,
                      int wr_offset,
                      int width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    for (int j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH		
#pragma HLS PIPELINE II=1
        // clang-format on
        tmp_out_mat.write(wr_offset + j, ram[j]);
        //_dst[j] = ram[j];
    }
}
template <int WIDTH>
void proc_write_to_out(
    uint8_t* fw_linebuffer, uint8_t* rev_linebuffer, uint8_t* out_ptr, int& def_pix, int wrt_offset, int width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    for (int j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
#pragma HLS PIPELINE II=1
        // clang-format on
        uint8_t tmp = fw_linebuffer[j] & rev_linebuffer[j];
        out_ptr[wrt_offset + j] = tmp;
        if (tmp != 0) def_pix++;
    }
}

template <int SRC_T, int HEIGHT, int WIDTH, int NPC, int XFCVDEPTH_OUT>
void fw_cca(xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& _src,
            xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& tmp_out_ptr,
            int& obj_pix,
            int height,
            int width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    obj_pix = 0;
    int offset = 0;
    bool lab_arr[WIDTH + 2];
    for (int i = 0; i < width + 2; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
#pragma HLS PIPELINE II=1
        // clang-format on
        lab_arr[i] = 1;
    }

    for (int i = 0; i < height; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT
        // clang-format on
        process_mat_row<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>(_src, tmp_out_ptr, lab_arr, obj_pix, offset, width);
        offset += width;
    }
}

template <int SRC_T, int HEIGHT, int WIDTH, int NPC, int XFCVDEPTH_OUT>
void rev_cca(uint8_t* in_ptr,
             xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& tmp_out_mat,
             int height,
             int width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    bool lab_arr[WIDTH + 2], flag = 0;
    int obj_pix;

    for (int i = 0; i < width + 2; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
#pragma HLS PIPELINE II=1
        // clang-format on
        lab_arr[i] = 1;
    }

    uint8_t rd_linebuff1[WIDTH], rd_linebuff2[WIDTH];
    uint8_t wrt_linebuff1[WIDTH], wrt_linebuff2[WIDTH];

    int rd_offset = (height * width);
    int wrt_offset = (height * width);

    rd_offset -= width;
    read_row_to_ram<WIDTH>(in_ptr + rd_offset, rd_linebuff1, width);

    rd_offset -= width;
    read_row_to_ram<WIDTH>(in_ptr + rd_offset, rd_linebuff2, width);
    process_row<WIDTH>(rd_linebuff1, wrt_linebuff1, lab_arr, obj_pix, width);

REV_ROW_LOOP:
    for (int i = 0; i < height - 2; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT
        // clang-format on
        rd_offset -= width;
        wrt_offset -= width;

        if (flag == 0) {
            read_row_to_ram<WIDTH>(in_ptr + rd_offset, rd_linebuff1, width);
            process_row<WIDTH>(rd_linebuff2, wrt_linebuff2, lab_arr, obj_pix, width);
            write_row_to_mem<SRC_T, HEIGHT, WIDTH, NPC>(wrt_linebuff1, tmp_out_mat, wrt_offset, width);
            flag = 1;
        } else {
            read_row_to_ram<WIDTH>(in_ptr + rd_offset, rd_linebuff2, width);
            process_row<WIDTH>(rd_linebuff1, wrt_linebuff1, lab_arr, obj_pix, width);
            write_row_to_mem<SRC_T, HEIGHT, WIDTH, NPC>(wrt_linebuff2, tmp_out_mat, wrt_offset, width);
            flag = 0;
        }
    }

    wrt_offset -= width;

    if (flag == 0) {
        process_row<WIDTH>(rd_linebuff2, wrt_linebuff2, lab_arr, obj_pix, width);
        write_row_to_mem<SRC_T, HEIGHT, WIDTH, NPC>(wrt_linebuff1, tmp_out_mat, wrt_offset, width);
        flag = 1;
    } else {
        process_row<WIDTH>(rd_linebuff1, wrt_linebuff1, lab_arr, obj_pix, width);
        write_row_to_mem<SRC_T, HEIGHT, WIDTH, NPC>(wrt_linebuff2, tmp_out_mat, wrt_offset, width);
        flag = 0;
    }

    wrt_offset -= width;

    if (flag == 0)
        write_row_to_mem<SRC_T, HEIGHT, WIDTH, NPC>(wrt_linebuff1, tmp_out_mat, wrt_offset, width);
    else
        write_row_to_mem<SRC_T, HEIGHT, WIDTH, NPC>(wrt_linebuff2, tmp_out_mat, wrt_offset, width);
}

/*template <int HEIGHT, int WIDTH>
void pass_1(uint8_t* in_ptr1,
            //uint8_t* in_ptr2,
            uint8_t* tmp_out_ptr1,
            //uint8_t* tmp_out_ptr2,
            int& obj_pix,
            int height,
            int width) {
// clang-format off
#pragma HLS INLINE OFF
// clang-format on

        fw_cca<HEIGHT,WIDTH>(in_ptr1,tmp_out_ptr1,obj_pix,height,width);
        //rev_cca<HEIGHT,WIDTH>(in_ptr2,tmp_out_ptr2,height,width);
}*/

template <int SRC_T, int HEIGHT, int WIDTH, int NPC, int XFCVDEPTH_OUT>
void pass_2(uint8_t* fwd_in_ptr,
            xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& tmp_out_mat,
            uint8_t* out_ptr,
            int& def_pix,
            int height,
            int width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    int idx = 0;
    def_pix = 0;
    bool flag = 0;
    /*    for (int i = 0; i < height; i++) {
    // clang-format off
    #pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT
    #pragma HLS LOOP_FLATTEN
            // clang-format on
            for (int j = 0; j < width; j++) {
    // clang-format off
    #pragma HLS LOOP_TRIPCOUNT min=1 max=WIDTH
    #pragma HLS PIPELINE II=1
                // clang-format on
                uint8_t tmp = fwd_in_ptr[idx] & tmp_out_ptr[idx];
                out_ptr[idx++] = tmp;
                if (tmp != 0) def_pix++;
            }
        }*/

    uint8_t fwd_linebuff1[WIDTH], rev_linebuff1[WIDTH];
    uint8_t fwd_linebuff2[WIDTH], rev_linebuff2[WIDTH];

    int rd_offset = (height * width);
    int wrt_offset = (height * width);

    rd_offset -= width;
    read_row_to_ram<WIDTH>(fwd_in_ptr + rd_offset, fwd_linebuff1, width);
    read_row_mat_to_ram<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>(tmp_out_mat, rev_linebuff1, rd_offset, width);

PASS2_ROW_LOOP:
    for (int i = 0; i < height - 1; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT
        // clang-format on
        rd_offset -= width;
        wrt_offset -= width;

        if (flag == 0) {
            read_row_to_ram<WIDTH>(fwd_in_ptr + rd_offset, fwd_linebuff2, width);
            read_row_mat_to_ram<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>(tmp_out_mat, rev_linebuff2, rd_offset, width);
            proc_write_to_out<WIDTH>(fwd_linebuff1, rev_linebuff1, out_ptr, def_pix, wrt_offset, width);
            flag = 1;
        } else {
            read_row_to_ram<WIDTH>(fwd_in_ptr + rd_offset, fwd_linebuff1, width);
            read_row_mat_to_ram<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>(tmp_out_mat, rev_linebuff1, rd_offset, width);
            proc_write_to_out<WIDTH>(fwd_linebuff2, rev_linebuff2, out_ptr, def_pix, wrt_offset, width);
            flag = 0;
        }
    }

    wrt_offset -= width;

    if (flag == 0)
        proc_write_to_out<WIDTH>(fwd_linebuff1, rev_linebuff1, out_ptr, def_pix, wrt_offset, width);
    else
        proc_write_to_out<WIDTH>(fwd_linebuff2, rev_linebuff2, out_ptr, def_pix, wrt_offset, width);
}

template <int SRC_T, int HEIGHT, int WIDTH, int NPC>
void ccaCustom(uint8_t* fwd_in_ptr, // input image pointer for forward pass
               uint8_t* in_ptr,     // input image pinter for the parallel
                                    // computation of reverse pass
               // uint8_t* tmp_out_ptr1, // pointer to store and read from the
               // temporary buffer in DDR for the forward
               // pass
               xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC>& tmp_out, // pointer to store and read from the
                                                                // temporary buffer in DDR for the reverse
                                                                // pass
               uint8_t* out_ptr,                                // output defects image
               // int& obj_pix,          // output - object pixels without defects
               int& def_pix, // output - defect pixels
               int height,
               int width) {
// clang-format off
#pragma HLS INLINE OFF
// clang-format on

/*for (int i = 0; i < 2; i++) {
    if (i == 0)
        pass_1<HEIGHT, WIDTH>(in_ptr1, in_ptr2, tmp_out_ptr1, tmp_out_ptr2, obj_pix, height, width);
    else
        pass_2<HEIGHT, WIDTH>(tmp_out_ptr1, tmp_out_ptr2, out_ptr, def_pix, height, width);
}*/
// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    // rev_cca<TYPE, HEIGHT, WIDTH, NPC>(in_ptr,tmp_out, height,width);
    // pass_2<TYPE, HEIGHT, WIDTH, NPC>(fwd_in_ptr, tmp_out, out_ptr, def_pix, height, width);

    return;
}
// ======================================================================================

} // end of cv
} // end of xf

#endif // end of __XF_CCA_CUSTOM_HPP__
