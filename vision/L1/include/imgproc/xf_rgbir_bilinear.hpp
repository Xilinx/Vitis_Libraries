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
#include <iostream>
using namespace std;
namespace xf {
namespace cv {

#define __MAX(A, B) A > B ? A : B

template <int SIZE>
void printImgBlock(int FSIZE, int __BWIDTH, ap_uint<16> imgblock[][SIZE]) {
    for (int i = 0; i < FSIZE; i++) {
        for (int j = 0; j < __BWIDTH; j++) {
            std::cout << imgblock[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template <int BWIDTH, int TYPE, int NPPC>
void apply_interp(XF_CTUNAME(TYPE, NPPC) imgblock[3][BWIDTH], XF_DTUNAME(TYPE, NPPC) & pix, short int loop) {
    XF_DTUNAME(TYPE, NPPC) partial_sum_0 = 0, partial_sum_1 = 0;
    XF_DTUNAME(TYPE, NPPC) res = 0;

    res = (imgblock[0][1 + loop] + imgblock[1][0 + loop] + imgblock[1][2 + loop] + imgblock[2][1 + loop]) >> 2;

    pix = res;
    return;
}

template <int BWIDTH, int TYPE, int ROWS, int COLS, int NPPC, int BFORMAT>
void bilinearProcess(XF_CTUNAME(TYPE, NPPC) imgblock[3][BWIDTH],
                     short int row,
                     short int col,
                     XF_DTUNAME(TYPE, NPPC) & pix,
                     short int loop) {
    if ((((row & 0x0001) == 1) && ((col & 0x0001) == 0)) || // GB, GR Mode - This is odd row, even column
        (((row & 0x0001) == 0) && ((col & 0x0001) == 1)))   // even row, odd column
    {
        apply_interp<BWIDTH, TYPE, NPPC>(imgblock, pix, loop);
    } else {
        pix = imgblock[1][1 + loop];
    }
}
/////
template <int BWIDTH, int TYPE, int ROWS, int COLS, int NPPC>
void bilinearProcess_multi(XF_CTUNAME(TYPE, NPPC) imgblock[3][BWIDTH],
                           short int row,
                           short int col,
                           XF_DTUNAME(TYPE, NPPC) & pix,
                           short int loop,
                           unsigned short bformat) {
    if ((((row & 0x0001) == 1) && ((col & 0x0001) == 0)) || // GB, GR Mode - This is odd row, even column
        (((row & 0x0001) == 0) && ((col & 0x0001) == 1)))   // even row, odd column
    {
        apply_interp<BWIDTH, TYPE, NPPC>(imgblock, pix, loop);
    } else {
        pix = imgblock[1][1 + loop];
    }
}

template <int TYPE,
          int ROWS,
          int COLS,
          int NPPC,
          int XFCVDEPTH_IN_0 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int BFORMAT,
          int USE_URAM>
void xf_ir_bilinear(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_0>& _src,
                    xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_1>& _full_ir) {
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

#pragma HLS INLINE OFF

    static constexpr int FSIZE = 3;
    //	static constexpr int __IR_BWIDTH = XF_NPIXPERCYCLE(NPPC) + (FSIZE - 1);
    static constexpr int __IR_BWIDTH = __MAX(FSIZE, ((FSIZE >> 1) + (2 * XF_NPIXPERCYCLE(NPPC))));
    const int _ECPR = ((((FSIZE >> 1) + (NPPC - 1)) / NPPC));

    XF_TNAME(TYPE, NPPC) linebuffer[FSIZE - 1][COLS >> XF_BITSHIFT(NPPC)];
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
    XF_CTUNAME(TYPE, NPPC) imgblock[FSIZE][__IR_BWIDTH];

// clang-format off
#pragma HLS array_partition variable=imgblock complete dim=0
    // clang-format on

    int lineStore = 1, read_index = 0, write_index = 0;
LineBuffer:
    for (int i = 0; i < FSIZE - 1; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=3 max=3
        // clang-format on
        for (int j = 0; j<_src.cols>> XF_BITSHIFT(NPPC); j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPPC max=COLS/NPPC
#pragma HLS pipeline ii=1
            // clang-format on
            XF_TNAME(TYPE, NPPC) tmp = 0;
            if (i == 0) {
                tmp = 0;
            } else {
                tmp = _src.read(read_index++);
            }
            linebuffer[i][j] = tmp;
        }
    }
    ap_uint<3> line0 = 1, line1 = 0;
    int step = XF_DTPIXELDEPTH(TYPE, NPPC);
    int out_step = XF_DTPIXELDEPTH(TYPE, NPPC);
    XF_TNAME(TYPE, NPPC) tmp = 0;
    unsigned short col_wo_npc = 0;

Row_Loop:
    for (int i = 0; i < _src.rows; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
        // clang-format on
        int bram_read_count = 0;
        lineStore++;
        if (lineStore > 1) {
            lineStore = 0;
        }
        if (line0 == 0) {
            line0 = 1;
            line1 = 0;
        } else if (line0 == 1) {
            line0 = 0;
            line1 = 1;
        }

    /* Image left corner case
     * Fill left 2x2 in 3x3 with zeroes
     */
    Zero:
        for (int p = 0; p < FSIZE; ++p) {
// clang-format off
#pragma HLS PIPELINE II=1
            // clang-format on
            for (int k = 0; k < NPPC + FSIZE - 1; k++) {
                imgblock[p][k] = 0;
            }
        }

    /*Filling the data in the first two rows of 3x3/3x5/3x9 window from
     * linebuffer in the farthest columns */
    Datafill:
        for (int n = 0; n < NPPC; ++n) {
#pragma HLS UNROLL

            imgblock[0][__IR_BWIDTH - NPPC + n] = linebuffer[line0][0].range((step + step * n) - 1, step * n);
            imgblock[1][__IR_BWIDTH - NPPC + n] = linebuffer[line1][0].range((step + step * n) - 1, step * n);
        }

    //        printImgBlock<__IR_BWIDTH>(FSIZE, __IR_BWIDTH, imgblock);
    Col_Loop:
        for (int j = 0; j < ((_src.cols) >> XF_BITSHIFT(NPPC)) + _ECPR; j++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS dependence variable=linebuffer inter false
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPPC max=COLS/NPPC
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on

            if ((i < _src.rows - 1) && j < ((_src.cols) >> XF_BITSHIFT(NPPC))) {
                tmp = _src.read(read_index++); // Reading 5th row element
            } else {
                tmp = 0;
            }

            // Fill last row last element(s)
            for (int z = 0; z < NPPC; ++z) {
                imgblock[2][__IR_BWIDTH - NPPC + z] = tmp.range((step + step * z) - 1, step * z);
            }

            //            printImgBlock<__IR_BWIDTH>(FSIZE, __IR_BWIDTH, imgblock);

            // Calculate the resultant intensities at each pixel
            XF_DTUNAME(TYPE, NPPC) out_pix = 0;
            XF_TNAME(TYPE, NPPC) packed_res_pixel = 0;
            unsigned short candidateCol = 0, candidateRow = i;
            XF_DTUNAME(TYPE, NPPC) comb_out_pix[NPPC] = {0};
            //            int a=0;

            short int pstep = XF_PIXELWIDTH(TYPE, NPPC);
            if (j >= _ECPR) {
                for (int loop = 0; loop < NPPC; loop++) {
                    //					col_wo_npc = j * NPPC + loop;
                    candidateCol = j * NPPC - (_ECPR * NPPC) + loop;

                    if (BFORMAT == XF_BAYER_GR) {
                        candidateRow = i + 1;
                        candidateCol = j * NPPC - (_ECPR * NPPC) + 2 + loop;
                    }

                    /*		            if(col_wo_npc == 1923){
                                a = 1;
                            }*/

                    bilinearProcess<__IR_BWIDTH, TYPE, ROWS, COLS, NPPC, BFORMAT>(imgblock, candidateRow, candidateCol,
                                                                                  out_pix, loop);
                    comb_out_pix[loop] = out_pix;
                }

                for (int ploop = 0; ploop < NPPC; ploop++) {
                    packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = comb_out_pix[ploop];
                }

                // Write the data out to DDR
                // .........................

                _full_ir.write(write_index++, packed_res_pixel);
            }

            // Left-shift the elements in imgblock by NPPC
            for (int k = 0; k < FSIZE; k++) {
                for (int m = 0; m < NPPC; ++m) {
                    for (int l = 0; l < (__IR_BWIDTH - 1); l++) {
                        imgblock[k][l] = imgblock[k][l + 1];
                    }
                }
            }
            //            printImgBlock<__IR_BWIDTH>(FSIZE, __IR_BWIDTH, imgblock);
            bram_read_count++;

            XF_TNAME(TYPE, NPPC)
            packed_read1 = 0, packed_read2 = 0, packed_store = 0;

            if (j < ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) { // for each element being processed that is
                // not at borders
                packed_read1 = linebuffer[line0][bram_read_count];
                packed_read2 = linebuffer[line1][bram_read_count];

                for (int q = 0; q < NPPC; ++q) {
                    imgblock[0][__IR_BWIDTH - NPPC + q] = packed_read1.range((step + step * q) - 1, step * q);
                    imgblock[1][__IR_BWIDTH - NPPC + q] = packed_read2.range((step + step * q) - 1, step * q);
                    //                    imgblock[2][FSIZE-1+q] = packed_read3.range((step + step * q) - 1, step * q);
                    // imgblock[4][FSIZE-1+q] = tmp.range((step + step * q) - 1, step * q);
                    packed_store.range((step + step * q) - 1, step * q) =
                        //							imgblock[2][FSIZE - NPPC-1 + q]; //
                        // Write
                        // back
                        // to
                        // linebuffer
                        imgblock[2][(__IR_BWIDTH - NPPC) - NPPC + q]; // Write back to linebuffer
                }
                //	            printImgBlock<__IR_BWIDTH>(FSIZE, __IR_BWIDTH, imgblock);
                linebuffer[lineStore][j] = packed_store;

            } else { // For processing elements at the end of the line.
                for (int r = 0; r < NPPC; ++r) {
                    if (j == ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) {
                        linebuffer[lineStore][j].range((step + step * r) - 1, step * r) =
                            imgblock[2][(__IR_BWIDTH - NPPC) - NPPC + r];
                    }

                    imgblock[0][__IR_BWIDTH - NPPC + r] = 0;
                    imgblock[1][__IR_BWIDTH - NPPC + r] = 0;
                    imgblock[2][__IR_BWIDTH - NPPC + r] = 0;
                }
            }
        }
    }
}

template <int TYPE,
          int ROWS,
          int COLS,
          int NPPC,
          int XFCVDEPTH_IN_0 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int USE_URAM = 0,
          int STREAMS = 2,
          int SLICES = 2>
void xf_ir_bilinear_multi(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_0>& _src,
                          xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_1>& _full_ir,
                          unsigned short bformat,
                          int slice_id,
                          int stream_id,
                          XF_TNAME(TYPE, NPPC) rgbir_ir_buffs[STREAMS][2][COLS >> XF_BITSHIFT(NPPC)],
                          uint16_t slice_rows) {
#ifndef __SYNTHESIS__

    assert(((bformat == XF_BAYER_BG) || (bformat == XF_BAYER_GR)) && ("Unsupported Bayer pattern. Use one from:"
                                                                      "XF_BAYER_BG;XF_BAYER_GR"));
    assert(((_src.rows <= ROWS) && (_src.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((NPPC == 1) || (NPPC == 2) || (NPPC == 4)) && "Only 1, 2 and 4 pixel-parallelism are supported");
    assert(
        ((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_14UC1) || (TYPE == XF_16UC1)) &&
        "Only 8, 10, 12, 14 and 16 bit, single channel images are supported");

#endif

#pragma HLS INLINE OFF

    static constexpr int FSIZE = 3;
    static constexpr int __IR_BWIDTH = __MAX(FSIZE, ((FSIZE >> 1) + (2 * XF_NPIXPERCYCLE(NPPC))));
    const int _ECPR = ((((FSIZE >> 1) + (NPPC - 1)) / NPPC));

    XF_TNAME(TYPE, NPPC) linebuffer[FSIZE - 1][COLS >> XF_BITSHIFT(NPPC)];
    if (USE_URAM) {
// clang-format off
#pragma HLS bind_storage variable=linebuffer type=RAM_T2P impl=URAM
#pragma HLS array_reshape variable=linebuffer dim=1 factor=3 cyclic
        // clang-format on
    } else {
// clang-format off
#pragma HLS bind_storage variable=linebuffer type=RAM_T2P impl=BRAM
#pragma HLS array_partition variable=linebuffer complete dim=1
        // clang-format on
    }
    XF_CTUNAME(TYPE, NPPC) imgblock[FSIZE][__IR_BWIDTH];

// clang-format off
#pragma HLS array_partition variable=imgblock complete dim=0
    // clang-format on

    int lineStore = 1, read_index = 0, write_index = 0;
    int demo_row_cnt_rd = 0, demo_row_cnt_wr = 0, demo_col_cnt_rd = 0, demo_col_cnt_wr = 0;
LineBuffer:
    for (int i = 0; i < FSIZE - 1; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=3 max=3
        // clang-format on
        for (int j = 0; j<_src.cols>> XF_BITSHIFT(NPPC); j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPPC max=COLS/NPPC
#pragma HLS pipeline ii=1
            // clang-format on
            if (slice_id == 0) { // The beginning rows of full image
                XF_TNAME(TYPE, NPPC) tmp = 0;
                if (i == 0) {
                    tmp = 0;
                } else {
                    tmp = _src.read(read_index++);
                }
                linebuffer[i][j] = tmp;
            }
            // Condition when last 2 rows need to be read from previous slice
            else {
                linebuffer[i][j] = rgbir_ir_buffs[stream_id][i][j];
            }
        }
    }
    ap_uint<3> line0 = 1, line1 = 0;
    int step = XF_DTPIXELDEPTH(TYPE, NPPC);
    int out_step = XF_DTPIXELDEPTH(TYPE, NPPC);
    XF_TNAME(TYPE, NPPC) tmp = 0;
    unsigned short col_wo_npc = 0;

    // Last slice has to process left over rows accumulated from previous slices
    int strm_rows = 0;
    if (SLICES > 1 && slice_id == SLICES - 1) {
        strm_rows = _src.rows + 1;
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
        if (lineStore > 1) {
            lineStore = 0;
        }
        if (line0 == 0) {
            line0 = 1;
            line1 = 0;
        } else if (line0 == 1) {
            line0 = 0;
            line1 = 1;
        }

    /* Image left corner case
     * Fill left 2x2 in 3x3 with zeroes
     */
    Zero:
        for (int p = 0; p < FSIZE; ++p) {
// clang-format off
#pragma HLS PIPELINE II=1
            // clang-format on
            for (int k = 0; k < NPPC + FSIZE - 1; k++) {
                imgblock[p][k] = 0;
            }
        }

    /*Filling the data in the first two rows of 3x3/3x5/3x9 window from
     * linebuffer in the farthest columns */
    Datafill:
        for (int n = 0; n < NPPC; ++n) {
#pragma HLS UNROLL

            imgblock[0][__IR_BWIDTH - NPPC + n] = linebuffer[line0][0].range((step + step * n) - 1, step * n);
            imgblock[1][__IR_BWIDTH - NPPC + n] = linebuffer[line1][0].range((step + step * n) - 1, step * n);
        }

    Col_Loop:
        for (int j = 0; j < ((_src.cols) >> XF_BITSHIFT(NPPC)) + _ECPR; j++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS dependence variable=linebuffer inter false
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPPC max=COLS/NPPC
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on

            if (slice_id == 0) { // First slice
                if ((i < _src.rows - 1) && (j < ((_src.cols) >> XF_BITSHIFT(NPPC)))) {
                    tmp = _src.read(read_index++); // Reading 5th row element
                    // Condition when last 2 rows need to be stored for next slice
                    if ((i > _src.rows - 4) && (j < ((_src.cols) >> XF_BITSHIFT(NPPC)))) {
                        rgbir_ir_buffs[stream_id][demo_row_cnt_wr][demo_col_cnt_wr++] = tmp;
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
                    // Condition when last 2 rows need to be stored for next slice
                    if (i >= _src.rows - 2) {
                        rgbir_ir_buffs[stream_id][demo_row_cnt_wr][demo_col_cnt_wr++] = tmp;
                        if (demo_col_cnt_wr == ((_src.cols) >> XF_BITSHIFT(NPPC))) {
                            demo_col_cnt_wr = 0;
                            demo_row_cnt_wr++;
                        }
                    }
                }
            } else { //	Last slice
                if ((i < _src.rows) && (j < ((_src.cols) >> XF_BITSHIFT(NPPC)))) {
                    tmp = _src.read(read_index++);
                } else {
                    tmp = 0;
                }
            }

            // Fill last row last element(s)
            for (int z = 0; z < NPPC; ++z) {
                imgblock[2][__IR_BWIDTH - NPPC + z] = tmp.range((step + step * z) - 1, step * z);
            }

            // Calculate the resultant intensities at each pixel
            XF_DTUNAME(TYPE, NPPC) out_pix = 0;
            XF_TNAME(TYPE, NPPC) packed_res_pixel = 0;
            unsigned short candidateCol = 0, candidateRow = i;
            XF_DTUNAME(TYPE, NPPC) comb_out_pix[NPPC] = {0};
            //            int a=0;

            short int pstep = XF_PIXELWIDTH(TYPE, NPPC);
            if (j >= _ECPR) {
                // Prohibit writing last row's o/p in first slice
                if ((SLICES == 1) || (!((slice_id == 0) && (i > (_src.rows - 2))))) {
                    for (int loop = 0; loop < NPPC; loop++) {
                        candidateCol = j * NPPC - (_ECPR * NPPC) + loop;

                        if ((SLICES > 1) && (slice_id != 0)) {
                            candidateRow = slice_rows - FSIZE + i;
                        }
                        if (bformat == XF_BAYER_GR) {
                            candidateRow = candidateRow + 1;
                            candidateCol = j * NPPC - (_ECPR * NPPC) + 2 + loop;
                        }

                        bilinearProcess_multi<__IR_BWIDTH, TYPE, ROWS, COLS, NPPC>(imgblock, candidateRow, candidateCol,
                                                                                   out_pix, loop, bformat);
                        comb_out_pix[loop] = out_pix;
                    }

                    for (int ploop = 0; ploop < NPPC; ploop++) {
                        packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = comb_out_pix[ploop];
                    }

                    // Write the data out to DDR
                    // .........................

                    _full_ir.write(write_index++, packed_res_pixel);
                }
            }

            // Left-shift the elements in imgblock by NPPC
            for (int k = 0; k < FSIZE; k++) {
                for (int m = 0; m < NPPC; ++m) {
                    for (int l = 0; l < (__IR_BWIDTH - 1); l++) {
                        imgblock[k][l] = imgblock[k][l + 1];
                    }
                }
            }
            //            printImgBlock<__IR_BWIDTH>(FSIZE, __IR_BWIDTH, imgblock);
            bram_read_count++;

            XF_TNAME(TYPE, NPPC)
            packed_read1 = 0, packed_read2 = 0, packed_store = 0;

            if (j < ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) { // for each element being processed that is
                // not at borders
                packed_read1 = linebuffer[line0][bram_read_count];
                packed_read2 = linebuffer[line1][bram_read_count];

                for (int q = 0; q < NPPC; ++q) {
                    imgblock[0][__IR_BWIDTH - NPPC + q] = packed_read1.range((step + step * q) - 1, step * q);
                    imgblock[1][__IR_BWIDTH - NPPC + q] = packed_read2.range((step + step * q) - 1, step * q);

                    packed_store.range((step + step * q) - 1, step * q) =

                        imgblock[2][(__IR_BWIDTH - NPPC) - NPPC + q]; // Write back to linebuffer
                }

                linebuffer[lineStore][j] = packed_store;

            } else { // For processing elements at the end of the line.
                for (int r = 0; r < NPPC; ++r) {
                    if (j == ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) {
                        linebuffer[lineStore][j].range((step + step * r) - 1, step * r) =
                            imgblock[2][(__IR_BWIDTH - NPPC) - NPPC + r];
                    }

                    imgblock[0][__IR_BWIDTH - NPPC + r] = 0;
                    imgblock[1][__IR_BWIDTH - NPPC + r] = 0;
                    imgblock[2][__IR_BWIDTH - NPPC + r] = 0;
                }
            }
        }
    }
}

template <int BFORMAT = 0,
          int INTYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_IN_2 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void weightedSub(const char weights[4],
                 xf::cv::Mat<INTYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_1>& _src1,
                 xf::cv::Mat<INTYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_2>& _src2,
                 xf::cv::Mat<INTYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT>& _dst) {
    ap_uint<4> wgts[4] = {0};
    for (int i = 0; i < 4; i++) {
        wgts[i] = weights[i];
    }
#pragma HLS INLINE OFF
    int rd_index = 0, wr_index = 0;
//    int a =0;
ROW_LOOP_COPYR:
    for (unsigned short row = 0; row < _src1.rows; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
#pragma HLS LOOP_FLATTEN OFF
    // clang-format on
    COL_LOOP_COPYR:
        for (unsigned short col = 0, count = 0; col<_src1.cols>> XF_BITSHIFT(NPPC); col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
            // clang-format on
            XF_TNAME(INTYPE, NPPC) inVal1 = _src1.read(rd_index);
            XF_TNAME(INTYPE, NPPC) inVal2 = _src2.read(rd_index++);
            XF_TNAME(INTYPE, NPPC) outVal = 0;
            unsigned short tmp1 = 0;

            ap_int<17> tmp2 = 0;
            ap_uint<8> step = XF_DTPIXELDEPTH(INTYPE, NPPC);
            for (int iter = 0; iter < NPPC; iter++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = NPPC
#pragma HLS LOOP_FLATTEN OFF
                XF_CTUNAME(INTYPE, NPPC) extractVal2 = inVal2.range(step * iter + step - 1, step * iter);
                XF_CTUNAME(INTYPE, NPPC) extractVal1 = inVal1.range(step * iter + step - 1, step * iter);
                unsigned short col_wnpc = col * NPPC + iter;

                if (BFORMAT == XF_BAYER_GR) {
                    if ((((row & 0x0001) == 0) && ((col_wnpc & 0x0001) == 0)) ||
                        (((row & 0x0001) == 1) && ((col_wnpc & 0x0001) == 1))) { // G Pixel
                        tmp1 = extractVal2 >> wgts[0];                           // G has medium level of reduced weight
                    } else if ((((row & 0x0001) == 0) && ((col_wnpc & 0x0001) == 1))) { // R Pixel
                        tmp1 = extractVal2 >> wgts[1]; // R has lowest level of reduced weight
                    } else if (((((row - 1) % 4) == 0) && ((col_wnpc % 4) == 0)) ||
                               ((((row + 1) % 4) == 0) && (((col_wnpc - 2) % 4) == 0))) { // B Pixel
                        tmp1 = extractVal2 >> wgts[2]; // B has low level of reduced weight
                    } else if ((((((row - 1) % 4)) == 0) && (((col_wnpc - 2) % 4) == 0)) ||
                               (((((row + 1) % 4)) == 0) && (((col_wnpc) % 4) == 0))) { // Calculated B Pixel
                        tmp1 = extractVal2 >> wgts[3]; // B has highest level of reduced weight
                    }
                    if ((wgts[0] == 6) || (wgts[1] == 6) || (wgts[2] == 6) || (wgts[3] == 6)) {
                        tmp1 = 0;
                    }
                }
                if (BFORMAT == XF_BAYER_BG) {
                    if ((((row & 0x0001) == 0) && ((col_wnpc & 0x0001) == 1)) ||
                        (((row & 0x0001) == 1) && ((col_wnpc & 0x0001) == 0))) { // G Pixel
                        tmp1 = extractVal2 >> wgts[0];                           // G has medium level of reduced weight
                    } else if ((((row & 0x0001) == 1) && ((col_wnpc & 0x0001) == 1))) { // R Pixel
                        tmp1 = extractVal2 >> wgts[1]; // R has lowest level of reduced weight
                    } else if (((((row) % 4) == 0) && (((col_wnpc) % 4) == 0)) ||
                               ((((row - 2) % 4) == 0) && (((col_wnpc - 2) % 4) == 0))) { // B Pixel
                        tmp1 = extractVal2 >> wgts[2]; // B has low level of reduced weight
                    } else if ((((((row) % 4)) == 0) && (((col_wnpc - 2) % 4) == 0)) ||
                               (((((row - 2) % 4)) == 0) && (((col_wnpc) % 4) == 0))) { // Calculated B Pixel
                        tmp1 = extractVal2 >> wgts[3]; // B has highest level of reduced weight
                    }
                }
                tmp2 = extractVal1 - tmp1;

                if (tmp2 < 0) {
                    tmp2 = 0;
                }

                outVal.range(step * iter + step - 1, step * iter) = (XF_CTUNAME(INTYPE, NPPC))tmp2;
            }

            _dst.write(wr_index++, outVal);
        }
    }
}

//////////////////

template <int INTYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int STREAMS = 2,
          int SLICES = 2,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_IN_2 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void weightedSub_multi(const char weights[4],
                       xf::cv::Mat<INTYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_1>& _src1,
                       xf::cv::Mat<INTYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_2>& _src2,
                       xf::cv::Mat<INTYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT>& _dst,
                       XF_TNAME(INTYPE, NPPC) rgbir_wgt_buffs[STREAMS][COLS >> XF_BITSHIFT(NPPC)],
                       unsigned short height,
                       unsigned short bformat,
                       int slice_id,
                       int stream_id,
                       uint16_t slice_rows) {
    ap_uint<4> wgts[4] = {0};
    for (int i = 0; i < 4; i++) {
        wgts[i] = weights[i];
    }
#pragma HLS INLINE OFF
    int rd_index = 0, rd_index_str = 0, rd_index1 = 0, wr_index = 0;

    int strm_rows = 0;

    // Iterating 1 extra row for in-between
    // slices, as the 1st row of rggb is read from stored buffer
    // and last row to be stored back
    if (SLICES > 1 && slice_id != SLICES - 1 && slice_id != 0) {
        strm_rows = height + 1;

    }

    // Last slice will have rggb produce 1 extra row
    // and IR produce 2 extra rows which evens out
    // no.of rows diff in previous slices
    else if (SLICES > 1 && slice_id == SLICES - 1) {
        strm_rows = height + 1;
    } else {
        strm_rows = height;
    }

ROW_LOOP_COPYR:
    for (unsigned short row = 0; row < strm_rows; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
#pragma HLS LOOP_FLATTEN OFF

    // clang-format on
    COL_LOOP_COPYR:
        for (unsigned short col = 0, count = 0; col<_src1.cols>> XF_BITSHIFT(NPPC); col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
            // clang-format on

            // For all slices excpet last slice, store
            // last row of rggb output, as IR output has 1 less row

            if ((SLICES > 1) && (slice_id != SLICES - 1) && (row == strm_rows - 1)) {
                rgbir_wgt_buffs[stream_id][col] = _src1.read(rd_index_str++);

            } else {
                XF_TNAME(INTYPE, NPPC) inVal1 = 0;
                // Read first row from stored buffer for all
                // slices except first
                if ((slice_id != 0) && (row == 0)) {
                    inVal1 = rgbir_wgt_buffs[stream_id][col];

                } else {
                    inVal1 = _src1.read(rd_index1++);
                }

                XF_TNAME(INTYPE, NPPC) inVal2 = _src2.read(rd_index++);
                XF_TNAME(INTYPE, NPPC) outVal = 0;
                unsigned short tmp1 = 0;

                ap_int<17> tmp2 = 0;
                ap_uint<8> step = XF_DTPIXELDEPTH(INTYPE, NPPC);
                for (int iter = 0; iter < NPPC; iter++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = NPPC
#pragma HLS LOOP_FLATTEN OFF
                    XF_CTUNAME(INTYPE, NPPC) extractVal2 = inVal2.range(step * iter + step - 1, step * iter);
                    XF_CTUNAME(INTYPE, NPPC) extractVal1 = inVal1.range(step * iter + step - 1, step * iter);
                    unsigned short col_wnpc = col * NPPC + iter;
                    unsigned short candidateRow = row;

                    if ((SLICES > 1) && (slice_id != 0)) {
                        candidateRow = slice_rows - 3 + row;
                    }

                    if (bformat == XF_BAYER_GR) {
                        if ((((candidateRow & 0x0001) == 0) && ((col_wnpc & 0x0001) == 0)) ||
                            (((candidateRow & 0x0001) == 1) && ((col_wnpc & 0x0001) == 1))) { // G Pixel
                            tmp1 = extractVal2 >> wgts[0]; // G has medium level of reduced weight
                        } else if ((((candidateRow & 0x0001) == 0) && ((col_wnpc & 0x0001) == 1))) { // R Pixel
                            tmp1 = extractVal2 >> wgts[1]; // R has lowest level of reduced weight
                        } else if (((((candidateRow - 1) % 4) == 0) && ((col_wnpc % 4) == 0)) ||
                                   ((((candidateRow + 1) % 4) == 0) && (((col_wnpc - 2) % 4) == 0))) { // B Pixel
                            tmp1 = extractVal2 >> wgts[2]; // B has low level of reduced weight
                        } else if ((((((candidateRow - 1) % 4)) == 0) && (((col_wnpc - 2) % 4) == 0)) ||
                                   (((((candidateRow + 1) % 4)) == 0) &&
                                    (((col_wnpc) % 4) == 0))) { // Calculated B Pixel
                            tmp1 = extractVal2 >> wgts[3];      // B has highest level of reduced weight
                        }
                        if ((wgts[0] == 6) || (wgts[1] == 6) || (wgts[2] == 6) || (wgts[3] == 6)) {
                            tmp1 = 0;
                        }
                    }
                    if (bformat == XF_BAYER_BG) {
                        if ((((candidateRow & 0x0001) == 0) && ((col_wnpc & 0x0001) == 1)) ||
                            (((candidateRow & 0x0001) == 1) && ((col_wnpc & 0x0001) == 0))) { // G Pixel
                            tmp1 = extractVal2 >> wgts[0]; // G has medium level of reduced weight
                        } else if ((((candidateRow & 0x0001) == 1) && ((col_wnpc & 0x0001) == 1))) { // R Pixel
                            tmp1 = extractVal2 >> wgts[1]; // R has lowest level of reduced weight
                        } else if (((((candidateRow) % 4) == 0) && (((col_wnpc) % 4) == 0)) ||
                                   ((((candidateRow - 2) % 4) == 0) && (((col_wnpc - 2) % 4) == 0))) { // B Pixel
                            tmp1 = extractVal2 >> wgts[2]; // B has low level of reduced weight
                        } else if ((((((candidateRow) % 4)) == 0) && (((col_wnpc - 2) % 4) == 0)) ||
                                   (((((candidateRow - 2) % 4)) == 0) &&
                                    (((col_wnpc) % 4) == 0))) { // Calculated B Pixel
                            tmp1 = extractVal2 >> wgts[3];      // B has highest level of reduced weight
                        }
                    }
                    tmp2 = extractVal1 - tmp1;

                    if (tmp2 < 0) {
                        tmp2 = 0;
                    }

                    outVal.range(step * iter + step - 1, step * iter) = (XF_CTUNAME(INTYPE, NPPC))tmp2;
                }

                _dst.write(wr_index++, outVal);
            }
        }
    }
}

//===============================================================================

/* Function to retrieve original R pixel value and replace in final image */

//===============================================================================

template <int BFORMAT = 0,
          int INTYPE,
          int OUTTYPE,
          int ROWS,
          int COLS,
          int NPPC = 1,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_IN_2 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void copyRpixel(xf::cv::Mat<INTYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_1>& _src,
                xf::cv::Mat<OUTTYPE, ROWS, COLS, NPPC, XFCVDEPTH_IN_2>& _src2,
                xf::cv::Mat<OUTTYPE, ROWS, COLS, NPPC, XFCVDEPTH_OUT>& _dst) {
#pragma HLS INLINE OFF
    int rd_index = 0, wr_index = 0;
    unsigned short candidateRow = 0, candidateCol = 0;
ROW_LOOP_COPYR:
    for (int r = 0; r < _src.rows; r++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = ROWS
#pragma HLS LOOP_FLATTEN OFF
    COL_LOOP_COPYR:
        for (int c = 0, count = 0; c < _src.cols; c++, count++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = COLS
#pragma HLS PIPELINE II = 1
            XF_TNAME(OUTTYPE, NPPC) inVal = _src.read(rd_index);
            XF_TNAME(OUTTYPE, NPPC) dstVal = _src2.read(rd_index++);
            if (BFORMAT == XF_BAYER_GR) {
                candidateRow = r + 1;
                candidateCol = c + 2;
            }
            if (((((candidateRow - 2) % 4) == 0) && ((candidateCol % 4) == 0)) ||
                (((candidateRow % 4) == 0) &&
                 (((candidateCol - 2) % 4) == 0))) { // BG Mode - This is even row, R location.

                dstVal.range((XF_DTPIXELDEPTH(INTYPE, NPPC) * 3) - 1, XF_DTPIXELDEPTH(INTYPE, NPPC) * 2) = inVal;
                _dst.write(wr_index++, dstVal);

            } else {
                _dst.write(wr_index++, dstVal);
            }
        }
    }
}
}
}
