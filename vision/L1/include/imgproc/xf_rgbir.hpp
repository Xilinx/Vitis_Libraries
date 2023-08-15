/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

/*template <int SIZE>
void printImgBlock(int FSIZE, int __BWIDTH, ap_uint<16> imgblock[][SIZE]){
    for(int i=0; i< FSIZE; i++){
        for(int j=0; j< __BWIDTH; j++){
                std::cout << imgblock[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}*/

template <int TYPE, int NPPC, int BWIDTH, int F_ROWS = 5, int F_COLS = 5>
void apply_filter(XF_DTUNAME(TYPE, NPPC) patch[F_ROWS][BWIDTH],
                  ap_int<4> wgt[F_ROWS][F_COLS],
                  int loop,
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
            if (wgt[fr][fc] >= 0) {
                if (wgt[fr][fc] == 7) { // wgt is -1
                    partial_sum[fr] -= patch[fr][fc + loop];
                } else if (wgt[fr][fc] == 6) { // wgt is 0
                    partial_sum[fr] += 0;
                } else {
                    partial_sum[fr] += patch[fr][fc + loop] >> (__ABS((char)wgt[fr][fc]));
                }
            } else if (wgt[fr][fc] < 0) {
                partial_sum[fr] -= patch[fr][fc + loop] >> (__ABS((char)wgt[fr][fc]));
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

template <int BSIZE1, int BSIZE2, int TYPE, int ROWS, int COLS, int NPPC = 1>
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
                 XF_DTUNAME(TYPE, NPPC) & out_pix_ir,
                 int loop) {
    if (((((i - 2) % 4) == 0) && ((j % 4) == 0)) || (((i % 4) == 0) && (((j - 2) % 4) == 0))) {
        apply_filter<TYPE, NPPC, BSIZE1>(imgblock, B_at_R_wgts, loop, out_pix); // B at R

    } else if (((i - 1) % 4) == 0) { // BG Mode - This is odd row, IR location. Compute R here with 5x5 filter

        if (((j - 1) % 4) == 0) {
            apply_filter<TYPE, NPPC, BSIZE1>(imgblock, R_IR_C2_wgts, loop,
                                             out_pix); // B at IR - Constellation-1 (Red on the top left)
        } else if (((j + 1) % 4) == 0) {
            apply_filter<TYPE, NPPC, BSIZE1>(imgblock, R_IR_C1_wgts, loop,
                                             out_pix); // B at IR - Constellation-2 (Blue on the top left)
        }
    } else if (((i + 1) % 4) == 0) { // BG Mode - This is odd row, IR location. Compute R here with 5x5 filter

        if (((j - 1) % 4) == 0) {
            apply_filter<TYPE, NPPC, BSIZE1>(imgblock, R_IR_C1_wgts, loop,
                                             out_pix); // B at IR - Constellation-1 (Red on the top left)
        } else if (((j + 1) % 4) == 0) {
            apply_filter<TYPE, NPPC, BSIZE1>(imgblock, R_IR_C2_wgts, loop,
                                             out_pix); // B at IR - Constellation-2 (Blue on the top left)
        }
    }
    if ((((i % 4) == 0) && ((j % 4) == 0)) || // BG Mode - B location, apply 3x3 IR filter
        ((((i - 2) % 4) == 0) && (((j - 2) % 4) == 0))) {
        apply_filter<TYPE, NPPC, BSIZE2, 3, 3>(IR_imgblock, IR_at_B_wgts, loop, out_pix_ir); // IR at B location
    } else if (((((i - 2) % 4) == 0) && ((j % 4) == 0)) ||
               (((i % 4) == 0) && (((j - 2) % 4) == 0))) { // BG Mode - R location, apply 3x3 IR filter

        apply_filter<TYPE, NPPC, BSIZE2, 3, 3>(IR_imgblock, IR_at_R_wgts, loop, out_pix_ir); // IR at R location
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
    assert(
        ((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_14UC1) || (TYPE == XF_16UC1)) &&
        "Only 8, 10, 12, 14 and 16 bit, single channel images are supported");
#endif

    static constexpr int __BWIDTH = __MAX(FSIZE1, ((FSIZE1 >> 1) + (2 * XF_NPIXPERCYCLE(NPPC))));
    ;
    static constexpr int __IR_BWIDTH = __MAX(FSIZE2, ((FSIZE1 >> 1) + (2 * XF_NPIXPERCYCLE(NPPC))));
    ;
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

    /*Filling the data from linebuffer in the first four rows of data at the
     * data fill start index :(__BWIDTH - NPPC) */
    Datafill:
        for (int n = 0, w = 0, v = 0; n < pre_read_count; ++n, ++v) {
#pragma HLS UNROLL

            imgblock[0][__BWIDTH - NPPC + n] = linebuffer[line0][w].range((step + step * v) - 1, step * v);
            imgblock[1][__BWIDTH - NPPC + n] = linebuffer[line1][w].range((step + step * v) - 1, step * v);
            imgblock[2][__BWIDTH - NPPC + n] = linebuffer[line2][w].range((step + step * v) - 1, step * v);
            imgblock[3][__BWIDTH - NPPC + n] = linebuffer[line3][w].range((step + step * v) - 1, step * v);
        }

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
                imgblock[4][__BWIDTH - NPPC + z] = tmp.range((step + step * z) - 1, step * z);
            }

            // Extract 3x3 IR bloc from 5x5 block
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3 + NPPC - 1; c++) {
                    IR_imgblock[r][c] = imgblock[r + 1][c + 1];
                }
            }

            // Calculate the resultant intensities at each pixel
            XF_TNAME(TYPE, NPPC) packed_res_pixel = 0, ir_packed_res_pixel = 0;
            int pstep = XF_PIXELWIDTH(TYPE, NPPC);

            XF_DTUNAME(TYPE, NPPC) comb_out_pixs[NPPC] = {0}, comb_ir_out_pixs[NPPC] = {0};
            //            int a =0;

            if (j >= _ECPR) {
                for (int loop = 0; loop < NPPC; loop++) {
                    candidateCol = j * NPPC - (_ECPR * NPPC) + loop;

                    if (BFORMAT == XF_BAYER_GR) {
                        candidateRow = i + 1;
                        candidateCol = j * NPPC - (_ECPR * NPPC) + 2 + loop;
                    }

                    XF_DTUNAME(TYPE, NPPC) out_pix = imgblock[FSIZE1 >> 1][(FSIZE1 >> 1) + loop];
                    XF_DTUNAME(TYPE, NPPC) ir_out_pix = IR_imgblock[FSIZE2 >> 1][(FSIZE2 >> 1) + loop];

                    coreProcess<__BWIDTH, __IR_BWIDTH, TYPE, ROWS, COLS, NPPC>(
                        imgblock, IR_imgblock, candidateRow, candidateCol, B_at_R_wgts_loc, R_IR_C1_wgts_loc,
                        R_IR_C2_wgts_loc, IR_at_B_wgts_loc, IR_at_R_wgts_loc, out_pix, ir_out_pix, loop);

                    comb_out_pixs[loop] = out_pix;
                    comb_ir_out_pixs[loop] = ir_out_pix;
                }

                for (int ploop = 0; ploop < NPPC; ploop++) {
                    packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = comb_out_pixs[ploop];
                    ir_packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = comb_ir_out_pixs[ploop];
                }

                // Write the data out to DDR
                // .........................

                _dst_rggb.write(write_index++, packed_res_pixel);

                _half_ir.write(write_index_ir++, ir_packed_res_pixel);
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
                    imgblock[0][__BWIDTH - NPPC + q] = packed_read1.range((step + step * q) - 1, step * q);
                    imgblock[1][__BWIDTH - NPPC + q] = packed_read2.range((step + step * q) - 1, step * q);
                    imgblock[2][__BWIDTH - NPPC + q] = packed_read3.range((step + step * q) - 1, step * q);
                    imgblock[3][__BWIDTH - NPPC + q] = packed_read4.range((step + step * q) - 1, step * q);

                    packed_store.range((step + step * q) - 1, step * q) =
                        imgblock[4][__BWIDTH - NPPC - NPPC + q]; // Write back to linebuffer at location:
                                                                 // Data fill start index (__BWIDTH - NPPC)
                    // minus NPC
                }
                linebuffer[lineStore][j] = packed_store;

            } else { // For processing elements at the end of the line.
                for (int r = 0; r < NPPC; ++r) {
                    if (j == ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) {
                        linebuffer[lineStore][j].range((step + step * r) - 1, step * r) =
                            imgblock[4][__BWIDTH - NPPC - NPPC + r];
                    }

                    imgblock[0][__BWIDTH - NPPC + r] = 0;
                    imgblock[1][__BWIDTH - NPPC + r] = 0;
                    imgblock[2][__BWIDTH - NPPC + r] = 0;
                    imgblock[3][__BWIDTH - NPPC + r] = 0;
                    imgblock[4][__BWIDTH - NPPC + r] = 0;
                }
            }

        } // end COL loop
    }     // end ROW loop
}

template <int FSIZE1 = 5,
          int FSIZE2 = 3,
          int TYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_0 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int BORDER_T = XF_BORDER_CONSTANT,
          int USE_URAM = 0,
          int STREAMS = 2,
          int SLICES = 2>
void xf_form_mosaic_multi(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN>& _src,
                          char R_IR_C1_wgts[FSIZE1 * FSIZE1],
                          char R_IR_C2_wgts[FSIZE1 * FSIZE1],
                          char B_at_R_wgts[FSIZE1 * FSIZE1],
                          char IR_at_R_wgts[FSIZE2 * FSIZE2],
                          char IR_at_B_wgts[FSIZE2 * FSIZE2],
                          unsigned short bformat,
                          int slice_id,
                          int stream_id,
                          xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_0>& _dst_rggb,
                          xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1>& _half_ir,
                          XF_TNAME(TYPE, NPPC) rgbir_buffs[STREAMS][4][COLS >> XF_BITSHIFT(NPPC)],
                          uint16_t slice_rows) {
#ifndef __SYNTHESIS__

    assert(((bformat == XF_BAYER_BG) || (bformat == XF_BAYER_GR)) && ("Unsupported Bayer pattern. Use one from: "
                                                                      "XF_BAYER_BG;XF_BAYER_GR"));
    assert(((_src.rows <= ROWS) && (_src.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((NPPC == 1) || (NPPC == 2) || (NPPC == 4)) && "Only 1, 2 and 4 pixel-parallelism are supported");
    assert(
        ((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_14UC1) || (TYPE == XF_16UC1)) &&
        "Only 8, 10, 12, 14 and 16 bit, single channel images are supported");
#endif

    static constexpr int __BWIDTH = __MAX(FSIZE1, ((FSIZE1 >> 1) + (2 * XF_NPIXPERCYCLE(NPPC))));
    static constexpr int __IR_BWIDTH = __MAX(FSIZE2, ((FSIZE1 >> 1) + (2 * XF_NPIXPERCYCLE(NPPC))));
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

    const int pre_read_count = NPPC; // 2-2-4

// clang-format off
#pragma HLS array_partition variable=imgblock complete dim=0
    // clang-format on

    int lineStore = 3, read_index = 0, write_index = 0, write_index_ir = 0;
    int demo_row_cnt_rd = 0, demo_row_cnt_wr = 0, demo_col_cnt_rd = 0, demo_col_cnt_wr = 0;

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
            if (slice_id == 0) { // The beginning rows of full image
                XF_TNAME(TYPE, NPPC) tmp = _src.read(read_index++);
                linebuffer[i][j] = 0;
                linebuffer[i + 2][j] = tmp;
            }
            // Condition when last 4 rows need to be read from previous slice
            else {
                linebuffer[i][j] = rgbir_buffs[stream_id][i][j];
                linebuffer[i + 2][j] = rgbir_buffs[stream_id][i + 2][j];
            }
        }
    }
    ap_uint<3> line0 = 3, line1 = 0, line2 = 1, line3 = 2;
    int step = XF_DTPIXELDEPTH(TYPE, NPPC);
    XF_TNAME(TYPE, NPPC) tmp = 0;

    // Last slice has to process left over rows accumulated from previous slices
    int strm_rows = 0;
    if (SLICES > 1 && slice_id == SLICES - 1) {
        strm_rows = _src.rows + 2;
    } else {
        strm_rows = _src.rows;
    }

Row_Loop:
    for (int i = 0; i < strm_rows; i++) {
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

    /*Filling the data from linebuffer in the first four rows of data at the
     * data fill start index :(__BWIDTH - NPPC) */
    Datafill:
        for (int n = 0, w = 0, v = 0; n < pre_read_count; ++n, ++v) {
#pragma HLS UNROLL

            imgblock[0][__BWIDTH - NPPC + n] = linebuffer[line0][w].range((step + step * v) - 1, step * v);
            imgblock[1][__BWIDTH - NPPC + n] = linebuffer[line1][w].range((step + step * v) - 1, step * v);
            imgblock[2][__BWIDTH - NPPC + n] = linebuffer[line2][w].range((step + step * v) - 1, step * v);
            imgblock[3][__BWIDTH - NPPC + n] = linebuffer[line3][w].range((step + step * v) - 1, step * v);
        }

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

            if (slice_id == 0) { // First slice
                if ((i < _src.rows - 2) && (j < ((_src.cols) >> XF_BITSHIFT(NPPC)))) {
                    tmp = _src.read(read_index++); // Reading 5th row element
                    // Condition when last 4 rows need to be stored for next slice

                    if ((i >= _src.rows - 6) && (j < ((_src.cols) >> XF_BITSHIFT(NPPC)))) {
                        rgbir_buffs[stream_id][demo_row_cnt_wr][demo_col_cnt_wr++] = tmp;

                        if (demo_col_cnt_wr == ((_src.cols) >> XF_BITSHIFT(NPPC))) {
                            demo_col_cnt_wr = 0;
                            demo_row_cnt_wr++;
                        }
                    }
                } else {
                    tmp = 0;
                }
            } else if (slice_id != SLICES - 1) { // Other than first and last slices
                if (j < ((_src.cols) >> XF_BITSHIFT(NPPC))) {
                    tmp = _src.read(read_index++);
                    // Condition when last 4 rows need to be stored for next slice
                    if (i >= _src.rows - 4) {
                        rgbir_buffs[stream_id][demo_row_cnt_wr][demo_col_cnt_wr++] = tmp;

                        if (demo_col_cnt_wr == ((_src.cols) >> XF_BITSHIFT(NPPC))) {
                            demo_col_cnt_wr = 0;
                            demo_row_cnt_wr++;
                        }
                    }
                } else {
                    tmp = 0;
                }
            } else { //	Last slice
                if ((i < _src.rows) && (j < ((_src.cols) >> XF_BITSHIFT(NPPC)))) {
                    tmp = _src.read(read_index++);

                } else {
                    tmp = 0;
                }
            }

            // Fill 4th row last element
            for (int z = 0; z < NPPC; ++z) {
                imgblock[4][__BWIDTH - NPPC + z] = tmp.range((step + step * z) - 1, step * z);
            }

            // Extract 3x3 IR bloc from 5x5 block
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3 + NPPC - 1; c++) {
                    IR_imgblock[r][c] = imgblock[r + 1][c + 1];
                }
            }

            // Calculate the resultant intensities at each pixel
            XF_TNAME(TYPE, NPPC) packed_res_pixel = 0, ir_packed_res_pixel = 0;
            int pstep = XF_PIXELWIDTH(TYPE, NPPC);

            XF_DTUNAME(TYPE, NPPC) comb_out_pixs[NPPC] = {0}, comb_ir_out_pixs[NPPC] = {0};

            if (j >= _ECPR) {
                // Prohibit writing last two rows o/p in first slice
                if ((SLICES == 1) || (!((slice_id == 0) && (i > (_src.rows - 3))))) {
                    for (int loop = 0; loop < NPPC; loop++) {
                        candidateCol = j * NPPC - (_ECPR * NPPC) + loop;

                        XF_DTUNAME(TYPE, NPPC) out_pix = imgblock[FSIZE1 >> 1][(FSIZE1 >> 1) + loop];
                        XF_DTUNAME(TYPE, NPPC) ir_out_pix = IR_imgblock[FSIZE2 >> 1][(FSIZE2 >> 1) + loop];

                        if ((SLICES > 1) && (slice_id != 0)) {
                            candidateRow = slice_rows - (FSIZE1 >> 1) + i;
                        }
                        if (bformat == XF_BAYER_GR) {
                            candidateRow = candidateRow + 1;

                            candidateCol = j * NPPC - (_ECPR * NPPC) + 2 + loop;
                        }
                        coreProcess<__BWIDTH, __IR_BWIDTH, TYPE, ROWS, COLS, NPPC>(
                            imgblock, IR_imgblock, candidateRow, candidateCol, B_at_R_wgts_loc, R_IR_C1_wgts_loc,
                            R_IR_C2_wgts_loc, IR_at_B_wgts_loc, IR_at_R_wgts_loc, out_pix, ir_out_pix, loop);

                        comb_out_pixs[loop] = out_pix;
                        comb_ir_out_pixs[loop] = ir_out_pix;
                    }

                    for (int ploop = 0; ploop < NPPC; ploop++) {
                        packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = comb_out_pixs[ploop];
                        ir_packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = comb_ir_out_pixs[ploop];
                    }

                    // Write the data out to DDR
                    // .........................

                    _dst_rggb.write(write_index++, packed_res_pixel);

                    _half_ir.write(write_index_ir++, ir_packed_res_pixel);
                }
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
                    imgblock[0][__BWIDTH - NPPC + q] = packed_read1.range((step + step * q) - 1, step * q);
                    imgblock[1][__BWIDTH - NPPC + q] = packed_read2.range((step + step * q) - 1, step * q);
                    imgblock[2][__BWIDTH - NPPC + q] = packed_read3.range((step + step * q) - 1, step * q);
                    imgblock[3][__BWIDTH - NPPC + q] = packed_read4.range((step + step * q) - 1, step * q);
                    // imgblock[4][FSIZE1-1+q] = tmp.range((step + step * q) - 1, step * q);
                    packed_store.range((step + step * q) - 1, step * q) =
                        imgblock[4][__BWIDTH - NPPC - NPPC + q]; // Write back to linebuffer at location:
                                                                 // Data fill start index (__BWIDTH - NPPC)
                    // minus NPC
                }
                linebuffer[lineStore][j] = packed_store;

                //                printImgBlock<__BWIDTH>(FSIZE1, __BWIDTH, imgblock);

            } else { // For processing elements at the end of the line.
                for (int r = 0; r < NPPC; ++r) {
                    if (j == ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) {
                        linebuffer[lineStore][j].range((step + step * r) - 1, step * r) =
                            imgblock[4][__BWIDTH - NPPC - NPPC + r];
                    }

                    imgblock[0][__BWIDTH - NPPC + r] = 0;
                    imgblock[1][__BWIDTH - NPPC + r] = 0;
                    imgblock[2][__BWIDTH - NPPC + r] = 0;
                    imgblock[3][__BWIDTH - NPPC + r] = 0;
                    imgblock[4][__BWIDTH - NPPC + r] = 0;
                }
            }

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
    assert(
        ((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_14UC1) || (TYPE == XF_16UC1)) &&
        "Only 8, 10, 12, 14 and 16 bit, single channel images are supported");
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

//////
template <int FSIZE1 = 5,
          int FSIZE2 = 3,

          int TYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int BORDER_T = XF_BORDER_CONSTANT,
          int USE_URAM = 0,
          int STREAMS = 2,
          int SLICES = 2,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_0 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_2 = _XFCVDEPTH_DEFAULT>
void rgbir2bayer_multi(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN>& _src,
                       char R_IR_C1_wgts[FSIZE1 * FSIZE1],
                       char R_IR_C2_wgts[FSIZE1 * FSIZE1],
                       char B_at_R_wgts[FSIZE1 * FSIZE1],
                       char IR_at_R_wgts[FSIZE2 * FSIZE2],
                       char IR_at_B_wgts[FSIZE2 * FSIZE2],
                       char sub_wgts[4],
                       unsigned short bformat,
                       int slice_id,
                       int stream_id,
                       xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_0>& _dst_rggb,
                       xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1>& _dst_ir,
                       XF_TNAME(TYPE, NPPC) rgbir_buffs[STREAMS][4][COLS >> XF_BITSHIFT(NPPC)],
                       XF_TNAME(TYPE, NPPC) rgbir_ir_buffs[STREAMS][2][COLS >> XF_BITSHIFT(NPPC)],
                       XF_TNAME(TYPE, NPPC) rgbir_wgt_buffs[STREAMS][COLS >> XF_BITSHIFT(NPPC)],
                       uint16_t slice_rows) {
#ifndef __SYNTHESIS__

    assert(((bformat == XF_BAYER_BG) || (bformat == XF_BAYER_GR)) && ("Unsupported Bayer pattern. Use one from: "
                                                                      "XF_BAYER_BG;XF_BAYER_GR"));
    assert(((_src.rows <= ROWS) && (_src.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((NPPC == 1) || (NPPC == 2) || (NPPC == 4)) && "Only 1, 2 and 4 pixel-parallelism are supported");
    assert(
        ((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_14UC1) || (TYPE == XF_16UC1)) &&
        "Only 8, 10, 12, 14 and 16 bit, single channel images are supported");
#endif

#pragma HLS DATAFLOW

    unsigned short rggbout_height, halfirout_height, fullirout_height;

    if (SLICES > 1) {
        if (slice_id == 0) {
            rggbout_height = _src.rows - 2;
            halfirout_height = _src.rows - 2;
            fullirout_height = halfirout_height - 1;

        } else if (slice_id == SLICES - 1) {
            rggbout_height = _src.rows + 2;
            halfirout_height = _src.rows + 2;
            fullirout_height = halfirout_height + 1;
        }
    }
    if (((SLICES > 1) && (slice_id != 0) && (slice_id != SLICES - 1)) || (SLICES == 1)) {
        rggbout_height = _src.rows;
        halfirout_height = _src.rows;
        fullirout_height = _src.rows;
    }

    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_2> rggbOutput(rggbout_height, _src.cols);
    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1> halfIrOutput(halfirout_height, _src.cols);
    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1> fullIrOutput(fullirout_height, _src.cols);
    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1> fullIrOutput_copy1(fullirout_height, _src.cols);

    xf_form_mosaic_multi<FSIZE1, FSIZE2, TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN, XFCVDEPTH_OUT_2, XFCVDEPTH_OUT_1,
                         BORDER_T, USE_URAM, STREAMS, SLICES>(_src, R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts,
                                                              IR_at_R_wgts, IR_at_B_wgts, bformat, slice_id, stream_id,
                                                              rggbOutput, halfIrOutput, rgbir_buffs, slice_rows);

    xf::cv::xf_ir_bilinear_multi<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1, XFCVDEPTH_OUT_1, USE_URAM, STREAMS, SLICES>(
        halfIrOutput, fullIrOutput, bformat, slice_id, stream_id, rgbir_ir_buffs, slice_rows);

    xf::cv::duplicateMat(fullIrOutput, fullIrOutput_copy1, _dst_ir);

    xf::cv::weightedSub_multi<TYPE, ROWS, COLS, NPPC, STREAMS, SLICES, XFCVDEPTH_OUT_2, XFCVDEPTH_OUT_1,
                              XFCVDEPTH_OUT_0>(sub_wgts, rggbOutput, fullIrOutput_copy1, _dst_rggb, rgbir_wgt_buffs,
                                               rggbout_height, bformat, slice_id, stream_id, slice_rows);
}

template <int FSIZE1 = 5,
          int FSIZE2 = 3,

          int TYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int BORDER_T = XF_BORDER_CONSTANT,
          int USE_URAM = 0,
          int STREAMS = 2,
          int SLICES = 2,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_0 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_2 = _XFCVDEPTH_DEFAULT>
void rgbir2bayer_multi_wrap(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN>& _src,
                            char R_IR_C1_wgts[STREAMS][FSIZE1 * FSIZE1],
                            char R_IR_C2_wgts[STREAMS][FSIZE1 * FSIZE1],
                            char B_at_R_wgts[STREAMS][FSIZE1 * FSIZE1],
                            char IR_at_R_wgts[STREAMS][FSIZE2 * FSIZE2],
                            char IR_at_B_wgts[STREAMS][FSIZE2 * FSIZE2],
                            char sub_wgts[STREAMS][4],
                            unsigned short bformat[STREAMS],
                            xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_0>& _dst_rggb,
                            xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT_1>& _dst_ir,
                            XF_TNAME(TYPE, NPPC) rgbir_buffs[STREAMS][4][COLS >> XF_BITSHIFT(NPPC)],
                            XF_TNAME(TYPE, NPPC) rgbir_ir_buffs[STREAMS][2][COLS >> XF_BITSHIFT(NPPC)],
                            XF_TNAME(TYPE, NPPC) rgbir_wgt_buffs[STREAMS][COLS >> XF_BITSHIFT(NPPC)],
                            int strm_id,
                            int slice_id,
                            uint16_t slice_rows) {
// clang-format off
#pragma HLS ARRAY_PARTITION variable=R_IR_C1_wgts dim=1 complete 
#pragma HLS ARRAY_PARTITION variable=R_IR_C2_wgts dim=1 complete   
   
#pragma HLS ARRAY_PARTITION variable=B_at_R_wgts dim=1 complete 

#pragma HLS ARRAY_PARTITION variable=IR_at_R_wgts dim=1 complete
#pragma HLS ARRAY_PARTITION variable=IR_at_B_wgts dim=1 complete 

#pragma HLS ARRAY_PARTITION variable=sub_wgts dim=1 complete
#pragma HLS ARRAY_PARTITION variable=bformat dim=1 complete

   // clang-format on 
  
   rgbir2bayer_multi<FSIZE1, FSIZE2, TYPE, ROWS, COLS, NPPC, BORDER_T, USE_URAM, STREAMS, SLICES, XFCVDEPTH_IN, XFCVDEPTH_OUT_0, 
         XFCVDEPTH_OUT_1, XFCVDEPTH_OUT_2>(_src, R_IR_C1_wgts[strm_id], R_IR_C2_wgts[strm_id], B_at_R_wgts[strm_id], 
            IR_at_R_wgts[strm_id], IR_at_B_wgts[strm_id], sub_wgts[strm_id], bformat[strm_id], slice_id, strm_id, 
            _dst_rggb, _dst_ir, rgbir_buffs, rgbir_ir_buffs, rgbir_wgt_buffs, slice_rows);
         
   
}         

} // namespace cv
}; // namespace xf
#endif
