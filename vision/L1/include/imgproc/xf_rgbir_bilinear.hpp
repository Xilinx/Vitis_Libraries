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

namespace xf {
namespace cv {

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
    if ((BFORMAT == XF_BAYER_BG)) {
        if ((((row & 0x0001) == 0) && ((col & 0x0001) == 1)) || // BG Mode: even row, odd column
            (((row & 0x0001) == 1) && ((col & 0x0001) == 0)))   // odd row, even column
        {
            apply_interp<BWIDTH, TYPE, NPPC>(imgblock, pix, loop);
        } else {
            pix = imgblock[1][1 + loop];
        }

    } else if ((BFORMAT == XF_BAYER_GR)) {
        if ((((row & 0x0001) == 0) && ((col & 0x0001) == 0)) || // GB, GR Mode - This is even row, even column
            (((row & 0x0001) == 1) && ((col & 0x0001) == 1)))   // odd row, odd column
        {
            apply_interp<BWIDTH, TYPE, NPPC>(imgblock, pix, loop);
        } else {
            pix = imgblock[1][1 + loop];
        }
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
    assert(((TYPE == XF_8UC1) || (TYPE == XF_10UC1) || (TYPE == XF_12UC1) || (TYPE == XF_16UC1)) &&
           "Only 8, 10, 12 and 16 bit, single channel images are supported");
#endif

#pragma HLS INLINE OFF

    static constexpr int FSIZE = 3;
    static constexpr int __IR_BWIDTH = XF_NPIXPERCYCLE(NPPC) + (FSIZE - 1);
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

    /*Filling the data in the first four rows of 5x5/5x6/5x10 window from
     * linebuffer */
    Datafill:
        for (int n = 0; n < NPPC; ++n) {
#pragma HLS UNROLL

            imgblock[0][FSIZE - 1 + n] = linebuffer[line0][0].range((step + step * n) - 1, step * n);
            imgblock[1][FSIZE - 1 + n] = linebuffer[line1][0].range((step + step * n) - 1, step * n);
        }
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
                imgblock[2][FSIZE - 1 + z] = tmp.range((step + step * z) - 1, step * z);
            }

            // Calculate the resultant intensities at each pixel
            XF_DTUNAME(TYPE, NPPC) out_pix = 0;
            XF_TNAME(TYPE, NPPC) packed_res_pixel = 0;
            //            int a=0;

            short int pstep = XF_PIXELWIDTH(TYPE, NPPC);
            if (j >= _ECPR) {
                for (int loop = 0; loop < NPPC; loop++) {
                    col_wo_npc = j * NPPC - 1 + loop;

                    /*		            if(col_wo_npc == 1923){
                                                    a = 1;
                                                }*/

                    bilinearProcess<__IR_BWIDTH, TYPE, ROWS, COLS, NPPC, BFORMAT>(imgblock, i, col_wo_npc, out_pix,
                                                                                  loop);
                }

                for (int ploop = 0; ploop < NPPC; ploop++) {
                    packed_res_pixel.range(pstep + pstep * ploop - 1, pstep * ploop) = out_pix;
                }

                // Write the data out to DDR
                // .........................

                _full_ir.write(write_index++, out_pix);
            }

            // Left-shift the elements in imgblock by NPPC
            for (int k = 0; k < FSIZE; k++) {
                for (int m = 0; m < NPPC; ++m) {
                    for (int l = 0; l < (__IR_BWIDTH - 1); l++) {
                        imgblock[k][l] = imgblock[k][l + 1];
                    }
                }
            }

            bram_read_count++;

            XF_TNAME(TYPE, NPPC)
            packed_read1 = 0, packed_read2 = 0, packed_store = 0;

            if (j < ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) { // for each element being processed that is
                                                              // not at borders
                packed_read1 = linebuffer[line0][bram_read_count];
                packed_read2 = linebuffer[line1][bram_read_count];

                for (int q = 0; q < NPPC; ++q) {
                    imgblock[0][FSIZE - 1 + q] = packed_read1.range((step + step * q) - 1, step * q);
                    imgblock[1][FSIZE - 1 + q] = packed_read2.range((step + step * q) - 1, step * q);
                    //                    imgblock[2][FSIZE-1+q] = packed_read3.range((step + step * q) - 1, step * q);
                    // imgblock[4][FSIZE-1+q] = tmp.range((step + step * q) - 1, step * q);
                    packed_store.range((step + step * q) - 1, step * q) =
                        imgblock[2][FSIZE - 2 + q]; // Write back to linebuffer
                }
                linebuffer[lineStore][j] = packed_store;

            } else { // For processing elements at the end of the line.
                for (int r = 0; r < NPPC; ++r) {
                    if (j == ((_src.cols >> XF_BITSHIFT(NPPC)) - 1)) {
                        linebuffer[lineStore][j].range((step + step * r) - 1, step * r) = imgblock[2][FSIZE - 2 + r];
                    }

                    imgblock[0][FSIZE - 1 + r] = 0;
                    imgblock[1][FSIZE - 1 + r] = 0;
                    imgblock[2][FSIZE - 1 + r] = 0;
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
        for (unsigned short col = 0, count = 0; col < _src1.cols; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
            // clang-format on
            XF_TNAME(INTYPE, NPPC) inVal1 = _src1.read(rd_index);
            XF_TNAME(INTYPE, NPPC) inVal2 = _src2.read(rd_index++);
            unsigned short tmp1 = 0;
            /*            if(col == 1923){
                            a = 1;
                        }*/

            /*if (wgt[fr][fc] == 7) { // wgt is -1
                                                partial_sum[fr] -= patch[fr][fc];
                                        } else if (wgt[fr][fc] == 6) { // wgt is 0
                                                partial_sum[fr] += 0;
                                        } else {
                                                partial_sum[fr] += patch[fr][fc] >> (__ABS((char)wgt[fr][fc]));
                                        }*/
            ap_int<17> tmp2 = 0;
            if (BFORMAT == XF_BAYER_GR) {
                if ((((row & 0x0001) == 0) && ((col & 0x0001) == 0)) ||
                    (((row & 0x0001) == 1) && ((col & 0x0001) == 1))) {        // G Pixel
                    tmp1 = inVal2 >> wgts[0];                                  // G has medium level of reduced weight
                } else if ((((row & 0x0001) == 0) && ((col & 0x0001) == 1))) { // R Pixel
                    tmp1 = inVal2 >> wgts[1];                                  // R has lowest level of reduced weight
                } else if (((((row - 1) % 4) == 0) && ((col % 4) == 0)) ||
                           ((((row + 1) % 4) == 0) && (((col - 2) % 4) == 0))) { // B Pixel
                    tmp1 = inVal2 >> wgts[2];                                    // B has low level of reduced weight
                } else if ((((((row - 1) % 4)) == 0) && (((col - 2) % 4) == 0)) ||
                           (((((row + 1) % 4)) == 0) && (((col) % 4) == 0))) { // Calculated B Pixel
                    tmp1 = inVal2 >> wgts[3];                                  // B has highest level of reduced weight
                }
                if ((wgts[0] == 6) || (wgts[1] == 6) || (wgts[2] == 6) || (wgts[3] == 6)) {
                    tmp1 = 0;
                }
            }
            if (BFORMAT == XF_BAYER_BG) {
                if ((((row & 0x0001) == 0) && ((col & 0x0001) == 1)) ||
                    (((row & 0x0001) == 1) && ((col & 0x0001) == 0))) {        // G Pixel
                    tmp1 = inVal2 >> wgts[0];                                  // G has medium level of reduced weight
                } else if ((((row & 0x0001) == 1) && ((col & 0x0001) == 1))) { // R Pixel
                    tmp1 = inVal2 >> wgts[1];                                  // R has lowest level of reduced weight
                } else if (((((row) % 4) == 0) && (((col) % 4) == 0)) ||
                           ((((row - 2) % 4) == 0) && (((col - 2) % 4) == 0))) { // B Pixel
                    tmp1 = inVal2 >> wgts[2];                                    // B has low level of reduced weight
                } else if ((((((row) % 4)) == 0) && (((col - 2) % 4) == 0)) ||
                           (((((row - 2) % 4)) == 0) && (((col) % 4) == 0))) { // Calculated B Pixel
                    tmp1 = inVal2 >> wgts[3];                                  // B has highest level of reduced weight
                }
            }
            tmp2 = inVal1 - tmp1;

            if (tmp2 < 0) {
                tmp2 = 0;
            }

            _dst.write(wr_index++, (XF_CTUNAME(INTYPE, NPPC))tmp2);
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
