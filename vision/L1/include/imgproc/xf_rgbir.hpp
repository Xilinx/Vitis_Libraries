/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef __XF_RGBIRBAYER_HPP__
#define __XF_RGBIRBAYER_HPP__

#include "ap_int.h"
#include "common/xf_common.hpp"
#include "hls_stream.h"
#include "imgproc/xf_rgbir_bilinear.hpp"
#include "imgproc/xf_duplicateimage.hpp"

namespace xf {
namespace cv {

template <int TYPE, int NPPC, int F_ROWS = 5, int F_COLS = 5>
void apply_filter(XF_DTUNAME(TYPE, NPPC) patch[F_ROWS][F_COLS],
                  ap_int<4> wgt[F_ROWS][F_COLS],
                  XF_DTUNAME(TYPE, NPPC) & pix) {
#pragma HLS INLINE OFF

    // Weights used for applying the filter are the values by which the pixel values are to be shifted
    int partial_sum[F_ROWS] = {0};
    int sum = 0;

    XF_DTUNAME(TYPE, NPPC) tempVal = 0, NtempVal = 0;

apply_row:
    for (int fr = 0; fr < F_ROWS; fr++) {
// clang-format off
#pragma HLS PIPELINE II=1
    // clang-format on
    apply_col:
        for (int fc = 0; fc < F_COLS; fc++) {
            //				#pragma HLS UNROLL
            if (wgt[fr][fc] > 0) {
                if (wgt[fr][fc] == 7) { // wgt is -1
                    partial_sum[fr] -= patch[fr][fc];
                } else if (wgt[fr][fc] == 6) { // wgt is 0
                    partial_sum[fr] += 0;
                } else {
                    partial_sum[fr] += patch[fr][fc] >> (__ABS((char)wgt[fr][fc]));
                }
            } else if (wgt[fr][fc] < 0) {
                partial_sum[fr] -= patch[fr][fc] >> (__ABS((char)wgt[fr][fc]));
            }
        }
    }

apply_sum:
    for (int fsum = 0; fsum < F_ROWS; fsum++) {
#pragma HLS UNROLL
        sum += partial_sum[fsum];
    }

    pix = xf::cv::xf_satcast<XF_PIXELWIDTH(TYPE, NPPC)>(sum);

    return;
}

template <int BSIZE1 = 5, int BSIZE2 = 3, int TYPE, int ROWS, int COLS, int NPPC = 1>
void coreProcess(XF_DTUNAME(TYPE, NPPC) imgblock[5][BSIZE1],
                 XF_DTUNAME(TYPE, NPPC) IR_imgblock[3][BSIZE2],
                 short int i,
                 short int j,
                 ap_int<4> B_at_R_wgts[5][5],
                 ap_int<4> R_IR_C1_wgts[5][5],
                 ap_int<4> R_IR_C2_wgts[5][5],
                 ap_int<4> IR_at_B_wgts[3][3],
                 ap_int<4> IR_at_R_wgts[3][3],
                 XF_DTUNAME(TYPE, NPPC) & out_pix,
                 XF_DTUNAME(TYPE, NPPC) & out_pix_ir) {
    /*	if (((((i - 2) % 4) == 0) && ((j % 4) == 0)) ||
                            (((i % 4) == 0) && (((j - 2) % 4) == 0))) {
                    apply_filter<TYPE, NPPC>(imgblock, B_at_R_wgts, out_pix); // B at R
            } else if ((i & 0x0001) == 1) { // BG Mode - This is odd row, IR location. Compute R here with 5x5 filter
                    if (((j - 1) % 4) == 0) {
                            apply_filter<TYPE, NPPC>(imgblock, R_IR_C2_wgts, out_pix); // B at IR - Constellation-1 (Red
       on the top left)
                    } else if (((j + 1) % 4) == 0) {
                            apply_filter<TYPE, NPPC>(imgblock, R_IR_C1_wgts, out_pix); // B at IR - Constellation-2
       (Blue on the top left)
                    }
            }
            if ((((i % 4) == 0) && ((j % 4) == 0)) || // BG Mode - B location, apply 3x3 IR filter
                            ((((i - 2) % 4) == 0) && (((j - 2) % 4) == 0))) {
                    apply_filter<TYPE, NPPC,3, 3>(IR_imgblock, IR_at_B_wgts, out_pix_ir); // IR at B location
            } else if (((((i - 2) % 4) == 0) && ((j % 4) == 0)) ||
                            (((i % 4) == 0) && (((j - 2) % 4) == 0))) { // BG Mode - R location, apply 3x3 IR filter

                    apply_filter<TYPE, NPPC,3, 3>(IR_imgblock, IR_at_R_wgts, out_pix_ir); // IR at R location
            }*/

    if (((((i - 2) % 4) == 0) && ((j % 4) == 0)) || (((i % 4) == 0) && (((j - 2) % 4) == 0))) {
        apply_filter<TYPE, NPPC>(imgblock, B_at_R_wgts, out_pix); // B at R
    } else if (((i - 1) % 4) == 0) { // BG Mode - This is odd row, IR location. Compute R here with 5x5 filter
        if (((j - 1) % 4) == 0) {
            apply_filter<TYPE, NPPC>(imgblock, R_IR_C2_wgts,
                                     out_pix); // B at IR - Constellation-1 (Red on the top left)
        } else if (((j + 1) % 4) == 0) {
            apply_filter<TYPE, NPPC>(imgblock, R_IR_C1_wgts,
                                     out_pix); // B at IR - Constellation-2 (Blue on the top left)
        }
    } else if (((i + 1) % 4) == 0) { // BG Mode - This is odd row, IR location. Compute R here with 5x5 filter
        if (((j - 1) % 4) == 0) {
            apply_filter<TYPE, NPPC>(imgblock, R_IR_C1_wgts,
                                     out_pix); // B at IR - Constellation-1 (Red on the top left)
        } else if (((j + 1) % 4) == 0) {
            apply_filter<TYPE, NPPC>(imgblock, R_IR_C2_wgts,
                                     out_pix); // B at IR - Constellation-2 (Blue on the top left)
        }
    }
    if ((((i % 4) == 0) && ((j % 4) == 0)) || // BG Mode - B location, apply 3x3 IR filter
        ((((i - 2) % 4) == 0) && (((j - 2) % 4) == 0))) {
        apply_filter<TYPE, NPPC, 3, 3>(IR_imgblock, IR_at_B_wgts, out_pix_ir); // IR at B location
    } else if (((((i - 2) % 4) == 0) && ((j % 4) == 0)) ||
               (((i % 4) == 0) && (((j - 2) % 4) == 0))) { // BG Mode - R location, apply 3x3 IR filter

        apply_filter<TYPE, NPPC, 3, 3>(IR_imgblock, IR_at_R_wgts, out_pix_ir); // IR at R location
    }
}

template <int FSIZE1 = 5,
          int FSIZE2 = 3,
          int BFORMAT = 0,
          int TYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_0 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int BORDER_T = XF_BORDER_CONSTANT,
          int USE_URAM = 0>
void xf_form_mosaic(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN>& _src,
                    char R_IR_C1_wgts[FSIZE1 * FSIZE1],
                    char R_IR_C2_wgts[FSIZE1 * FSIZE1],
                    char B_at_R_wgts[FSIZE1 * FSIZE1],
                    char IR_at_R_wgts[FSIZE2 * FSIZE2],
                    char IR_at_B_wgts[FSIZE2 * FSIZE2],
                    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_0>& _dst_rggb,
                    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1>& _half_ir) {
#ifndef __SYNTHESIS__
    assert(((BFORMAT == XF_BAYER_BG) || (BFORMAT == XF_BAYER_GB) || (BFORMAT == XF_BAYER_GR) ||
            (BFORMAT == XF_BAYER_RG)) &&
           ("Unsupported Bayer pattern. Use anyone among: "
            "XF_BAYER_BG;XF_BAYER_GB;XF_BAYER_GR;XF_BAYER_RG"));
    assert(((_src.rows <= ROWS) && (_src.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((NPPC == 1) || (NPPC == 2) || (NPPC == 4)) && "Only 1, 2 and 4 pixel-parallelism are supported");
    assert(((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_16UC1)) &&
           "Only 8, 10, 12 and 16 bit, single channel images are supported");
#endif

    static constexpr int __BWIDTH = XF_NPIXPERCYCLE(NPPC) + (FSIZE1 - 1);
    static constexpr int __IR_BWIDTH = XF_NPIXPERCYCLE(NPPC) + (FSIZE2 - 1);
    const int _ECPR = (((FSIZE1 >> 1) + (NPPC - 1)) / NPPC);

    ap_int<4> R_IR_C1_wgts_loc[FSIZE1][FSIZE1], R_IR_C2_wgts_loc[FSIZE1][FSIZE1], B_at_R_wgts_loc[FSIZE1][FSIZE1];
    ap_int<4> IR_at_R_wgts_loc[FSIZE2][FSIZE2], IR_at_B_wgts_loc[FSIZE2][FSIZE2];

// clang-format off
#pragma HLS ARRAY_PARTITION variable=R_IR_C1_wgts_loc complete dim=0
#pragma HLS ARRAY_PARTITION variable=R_IR_C2_wgts_loc complete dim=0
#pragma HLS ARRAY_PARTITION variable=B_at_R_wgts_loc complete dim=0
#pragma HLS ARRAY_PARTITION variable=IR_at_R_wgts_loc complete dim=0
#pragma HLS ARRAY_PARTITION variable=IR_at_B_wgts_loc complete dim=0
// clang-format on

FILTER1_ROW:
    for (ap_int<4> i = 0; i < FSIZE1; i++) {
    FILTER1_COL:
        for (ap_int<4> j = 0; j < FSIZE1; j++) {
            R_IR_C1_wgts_loc[i][j] = (ap_int<4>)R_IR_C1_wgts[i * FSIZE1 + j];
            R_IR_C2_wgts_loc[i][j] = (ap_int<4>)R_IR_C2_wgts[i * FSIZE1 + j];
            B_at_R_wgts_loc[i][j] = (ap_int<4>)B_at_R_wgts[i * FSIZE1 + j];
        }
    }

FILTER2_ROW:
    for (ap_int<4> k = 0; k < FSIZE2; k++) {
    FILTER2_COL:
        for (ap_int<4> l = 0; l < FSIZE2; l++) {
            IR_at_R_wgts_loc[k][l] = (ap_int<4>)IR_at_R_wgts[k * FSIZE2 + l];
            IR_at_B_wgts_loc[k][l] = (ap_int<4>)IR_at_B_wgts[k * FSIZE2 + l];
        }
    }

#pragma HLS INLINE OFF

    XF_TNAME(TYPE, NPPC) linebuffer[FSIZE1 - 1][COLS >> XF_BITSHIFT(NPPC)];
    if (USE_URAM) {
// clang-format off
#pragma HLS bind_storage variable=linebuffer type=RAM_T2P impl=URAM
#pragma HLS array_reshape variable=linebuffer dim=1 factor=4 cyclic
        // clang-format on
    } else {
// clang-format off
#pragma HLS bind_storage variable=linebuffer type=RAM_T2P impl=BRAM
#pragma HLS array_partition variable=linebuffer complete dim=1
        // clang-format on
    }
    XF_CTUNAME(TYPE, NPPC) imgblock[FSIZE1][__BWIDTH];
    XF_CTUNAME(TYPE, NPPC) IR_imgblock[FSIZE2][__IR_BWIDTH];
    /*
const int pre_read_count = (2 / NPPC) + ((NPPC * NPPC) >> 2);  // 2-2-4
const int post_read_count = pre_read_count + 2;             // 4-4-6
const int end_read_count = ((NPPC << 1) >> (NPPC * NPPC)) + 1; // 2-1-1
     */
    const int pre_read_count = NPPC; // 2-2-4

// clang-format off
#pragma HLS array_partition variable=imgblock complete dim=0
    // clang-format on

    int lineStore = 3, read_index = 0, write_index = 0, write_index_ir = 0;
LineBuffer:
    for (int i = 0; i < 2; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=2 max=2
        // clang-format on
        for (int j = 0; j<_src.cols>> XF_BITSHIFT(NPPC); j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPPC max=COLS/NPPC
#pragma HLS pipeline ii=1
            // clang-format on
            XF_TNAME(TYPE, NPPC) tmp = _src.read(read_index++);
            linebuffer[i][j] = 0;
            linebuffer[i + 2][j] = tmp;
        }
    }
    ap_uint<3> line0 = 3, line1 = 0, line2 = 1, line3 = 2;
    int step = XF_DTPIXELDEPTH(TYPE, NPPC);
    XF_TNAME(TYPE, NPPC) tmp = 0;

Row_Loop:
    for (int i = 0; i < _src.rows; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
        // clang-format on
        int bram_read_count = 0;
        lineStore++;
        if (lineStore > 3) {
            lineStore = 0;
        }
        if (line0 == 0) {
            line0 = 1;
            line1 = 2;
            line2 = 3;
            line3 = 0;
        } else if (line0 == 1) {
            line0 = 2;
            line1 = 3;
            line2 = 0;
            line3 = 1;
        } else if (line0 == 2) {
            line0 = 3;
            line1 = 0;
            line2 = 1;
            line3 = 2;
        } else {
            line0 = 0;
            line1 = 1;
            line2 = 2;
            line3 = 3;
        }

    /*Image left corner case */
    Zero:
        for (int p = 0; p < FSIZE1; ++p) {
// clang-format off
#pragma HLS PIPELINE II=1
            // clang-format on
            for (int k = 0; k < FSIZE1 - 1 + NPPC; k++) {
                imgblock[p][k] = 0;
            }
        }

    /*Filling the data in the first four rows of 5x5/5x6/5x10 window from
     * linebuffer */
    Datafill:
        for (int n = 0, w = 0, v = 0; n < pre_read_count; ++n, ++v) {
#pragma HLS UNROLL

            imgblock[0][FSIZE1 - 1 + n] = linebuffer[line0][w].range((step + step * v) - 1, step * v);
            imgblock[1][FSIZE1 - 1 + n] = linebuffer[line1][w].range((step + step * v) - 1, step * v);
            imgblock[2][FSIZE1 - 1 + n] = linebuffer[line2][w].range((step + step * v) - 1, step * v);
            imgblock[3][FSIZE1 - 1 + n] = linebuffer[line3][w].range((step + step * v) - 1, step * v);
        }
    //        bram_read_count++ ;

    Col_Loop:
        for (int j = 0; j < ((_src.cols) >> XF_BITSHIFT(NPPC)) + _ECPR; j++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS dependence variable=linebuffer inter false
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPPC max=COLS/NPPC
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on

            unsigned short candidateRow = i;
            unsigned short candidateCol = 0;

            if ((i < _src.rows - 2) && j < ((_src.cols) >> XF_BITSHIFT(NPPC))) {
                tmp = _src.read(read_index++); // Reading 5th row element
            } else {
                tmp = 0;
            }

            // Fill 4th row last element
            for (int z = 0; z < NPPC; ++z) {
                imgblock[4][FSIZE1 - 1 + z] = tmp.range((step + step * z) - 1, step * z);
            }

            // Extract 3x3 IR bloc from 5x5 block
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    IR_imgblock[r][c] = imgblock[r + 1][c + 1];
                }
            }

            // Calculate the resultant intensities at each pixel
            XF_TNAME(TYPE, NPPC) packed_res_pixel = 0, ir_packed_res_pixel = 0;
            int pstep = XF_PIXELWIDTH(TYPE, NPPC);

            XF_DTUNAME(TYPE, NPPC) out_pix = imgblock[FSIZE1 >> 1][FSIZE1 >> 1];
            XF_DTUNAME(TYPE, NPPC) ir_out_pix = IR_imgblock[FSIZE2 >> 1][FSIZE2 >> 1];
            //            int a =0;

            if (j >= _ECPR) {
                for (int loop = 0; loop < NPPC; loop++) {
                    candidateCol = j * NPPC - 2 + loop;

                    if (BFORMAT == XF_BAYER_GR) {
                        candidateRow = i + 1;
                        candidateCol = j * NPPC + loop;
                    }
                    /*		            if(candidateCol == 1923){
                                                    a = 1;
                                                }*/

                    coreProcess<__BWIDTH, __IR_BWIDTH, TYPE, ROWS, COLS, NPPC>(
                        imgblock, IR_imgblock, candidateRow, candidateCol, B_at_R_wgts_loc, R_IR_C1_wgts_loc,
                        R_IR_C2_wgts_loc, IR_at_B_wgts_loc, IR_at_R_wgts_loc, out_pix, ir_out_pix);
                }

                for (int ploop = 0; ploop < NPPC; ploop++) {
                    packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = out_pix;
                    ir_packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = ir_out_pix;
                }

                // Write the data out to DDR
                // .........................

                _dst_rggb.write(write_index++, out_pix);

                _half_ir.write(write_index_ir++, ir_out_pix);
            }

            // Left-shift the elements in imgblock by NPPC
            for (int k = 0; k < 5; k++) {
                for (int m = 0; m < NPPC; ++m) {
                    for (int l = 0; l < (__BWIDTH - 1); l++) {
                        imgblock[k][l] = imgblock[k][l + 1];
                    }
                }
            }

            bram_read_count++;

            XF_TNAME(TYPE, NPPC)
            packed_read1 = 0, packed_read2 = 0, packed_read3 = 0, packed_read4 = 0, packed_store = 0;

            if (j < ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) { // for each element being processed that is
                                                              // not at borders
                packed_read1 = linebuffer[line0][bram_read_count];
                packed_read2 = linebuffer[line1][bram_read_count];
                packed_read3 = linebuffer[line2][bram_read_count];
                packed_read4 = linebuffer[line3][bram_read_count];

                for (int q = 0; q < NPPC; ++q) {
                    imgblock[0][FSIZE1 - 1 + q] = packed_read1.range((step + step * q) - 1, step * q);
                    imgblock[1][FSIZE1 - 1 + q] = packed_read2.range((step + step * q) - 1, step * q);
                    imgblock[2][FSIZE1 - 1 + q] = packed_read3.range((step + step * q) - 1, step * q);
                    imgblock[3][FSIZE1 - 1 + q] = packed_read4.range((step + step * q) - 1, step * q);
                    // imgblock[4][FSIZE1-1+q] = tmp.range((step + step * q) - 1, step * q);
                    packed_store.range((step + step * q) - 1, step * q) =
                        imgblock[4][FSIZE1 - 2 + q]; // Write back to linebuffer
                }
                linebuffer[lineStore][j] = packed_store;

            } else { // For processing elements at the end of the line.
                for (int r = 0; r < NPPC; ++r) {
                    if (j == ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) {
                        linebuffer[lineStore][j].range((step + step * r) - 1, step * r) = imgblock[4][FSIZE1 - 2 + r];
                    }

                    imgblock[0][FSIZE1 - 1 + r] = 0;
                    imgblock[1][FSIZE1 - 1 + r] = 0;
                    imgblock[2][FSIZE1 - 1 + r] = 0;
                    imgblock[3][FSIZE1 - 1 + r] = 0;
                    imgblock[4][FSIZE1 - 1 + r] = 0;
                }
            }

            //			bram_read_count++;

        } // end COL loop
    }     // end ROW loop
}

template <int FSIZE1 = 5,
          int FSIZE2 = 3,
          int BFORMAT = 0,
          int TYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int BORDER_T = XF_BORDER_CONSTANT,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_0 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_2 = _XFCVDEPTH_DEFAULT>
void rgbir2bayer(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN>& _src,
                 char R_IR_C1_wgts[FSIZE1 * FSIZE1],
                 char R_IR_C2_wgts[FSIZE1 * FSIZE1],
                 char B_at_R_wgts[FSIZE1 * FSIZE1],
                 char IR_at_R_wgts[FSIZE2 * FSIZE2],
                 char IR_at_B_wgts[FSIZE2 * FSIZE2],
                 char sub_wgts[4],
                 xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_0>& _dst_rggb,
                 xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1>& _dst_ir) {
#ifndef __SYNTHESIS__
    assert(((BFORMAT == XF_BAYER_BG) || (BFORMAT == XF_BAYER_GB) || (BFORMAT == XF_BAYER_GR) ||
            (BFORMAT == XF_BAYER_RG)) &&
           ("Unsupported Bayer pattern. Use anyone among: "
            "XF_BAYER_BG;XF_BAYER_GB;XF_BAYER_GR;XF_BAYER_RG"));
    assert(((_src.rows <= ROWS) && (_src.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((NPPC == 1) || (NPPC == 2) || (NPPC == 4)) && "Only 1, 2 and 4 pixel-parallelism are supported");
    assert(((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_16UC1)) &&
           "Only 8, 10, 12 and 16 bit, single channel images are supported");
#endif

#pragma HLS DATAFLOW
    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_2> rggbOutput(_src.rows, _src.cols);
    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1> halfIrOutput(_src.rows, _src.cols);
    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1> fullIrOutput(_src.rows, _src.cols);
    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1> fullIrOutput_copy1(_src.rows, _src.cols);

    xf_form_mosaic<FSIZE1, FSIZE2, BFORMAT, TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN, XFCVDEPTH_OUT_2, XFCVDEPTH_OUT_1,
                   BORDER_T, USE_URAM>(_src, R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts,
                                       rggbOutput, halfIrOutput);
    xf::cv::xf_ir_bilinear<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1, XFCVDEPTH_OUT_1, BFORMAT, USE_URAM>(halfIrOutput,
                                                                                                        fullIrOutput);
    xf::cv::duplicateMat(fullIrOutput, fullIrOutput_copy1, _dst_ir);
    xf::cv::weightedSub<BFORMAT, TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_2, XFCVDEPTH_OUT_1, XFCVDEPTH_OUT_0>(
        sub_wgts, rggbOutput, fullIrOutput_copy1, _dst_rggb);
}

} // namespace cv
}; // namespace xf
#endif
