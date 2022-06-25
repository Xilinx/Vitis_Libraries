/*
 * Copyright 2019 Xilinx, Inc.
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

/**
 * @file bicubicinterpolator.hpp
 *
 * @brief This file contains top function of test case.
 */

#ifndef _BICUBIC_INTERPOLATOR_HPP_
#define _BICUBIC_INTERPOLATOR_HPP_

#include "ap_int.h"
#include "hls_math.h"
#include "hls_stream.h"
#include "resize_mem.hpp"
#include <iostream>
#include <math.h>

namespace xf {
namespace codec {
namespace details {

/**
 * @class Bicubic interpolation is an extension of cubic interpolation for
 * interpolating data points on a two-dimensional regular grid.
 * The interpolated surface is smoother than corresponding surfaces obtained by
 * bilinear interpolation or nearest-neighbor interpolation.
 * It can be used to resize images.
 *
 * @param _W  _W-bit representing the first parameter for ap_fixed type
 * @param _I _I-bit representing the numbers above the decimal point and _W-_I-bits representing the value below the
 * decimal point.
 * @param _WBIT the input width of pixels
 * @param _NPPC 1 representing 1-pixel/clock for one interpolation or 8 representing 8-pixel/clock for one interpolation
 */

template <int _W, int _I, int _WBIT, int _NPPC>
class BicubicInterpolator {
   public:
    // constructor
    BicubicInterpolator() {}

    typedef ap_fixed<_W, _I> fixed_t;
    typedef ap_uint<_WBIT * _NPPC> pixel_t;
    typedef ap_uint<72> pixel_8x_t;

    fixed_t FMul(fixed_t in1, fixed_t in2) {
        //#pragma HLS inline off
        fixed_t r = 0;
#pragma HLS resource variable = r core = FMul_fulldsp
        r = in1 * in2;
        return r;
    }

    fixed_t cubicInterpolate(fixed_t window[4], fixed_t x) {
#pragma HLS inline
        fixed_t a = 0.5;
        fixed_t b = 2.0;
        fixed_t c = 3.0;
        fixed_t d = 4.0;
        fixed_t e = 5.0;

        fixed_t tmpC = FMul(c, (window[1] - window[2]));
        fixed_t tmpB = FMul(b, window[0]);
        fixed_t tmpE = FMul(e, window[1]);
        fixed_t tmpD = FMul(d, window[2]);
        fixed_t tmpA = FMul(a, x);
        fixed_t tmp1 = FMul(x, (tmpC + window[3] - window[0]));
        fixed_t tmp2 = FMul(x, (tmpB - tmpE + tmpD - window[3] + tmp1));
        return window[1] + FMul(tmpA, (window[2] - window[0] + tmp2));
    }

    fixed_t bicubicInterpolate(Window<4, 4, fixed_t>& window, fixed_t y, fixed_t x) {
#pragma HLS inline
        fixed_t arr[4];
        arr[0] = cubicInterpolate(window.val[0], x);
        arr[1] = cubicInterpolate(window.val[1], x);
        arr[2] = cubicInterpolate(window.val[2], x);
        arr[3] = cubicInterpolate(window.val[3], x);

        return cubicInterpolate(arr, y);
    }

    // using the formulas
    fixed_t cubicInterpolate_one(Window<1, 4, fixed_t>& window, fixed_t x) {
#pragma HLS inline off
        fixed_t a = 0.5;
        fixed_t b = 2.0;
        fixed_t c = 3.0;
        fixed_t d = 4.0;
        fixed_t e = 5.0;

        fixed_t tmpC = FMul(c, (window.val[0][1] - window.val[0][2]));
        fixed_t tmpB = FMul(b, window.val[0][0]);
        fixed_t tmpE = FMul(e, window.val[0][1]);
        fixed_t tmpD = FMul(d, window.val[0][2]);
        fixed_t tmpA = FMul(a, x);
        fixed_t tmp1 = FMul(x, (tmpC + window.val[0][3] - window.val[0][0]));
        fixed_t tmp2 = FMul(x, (tmpB - tmpE + tmpD - window.val[0][3] + tmp1));
        return window.val[0][1] + FMul(tmpA, (window.val[0][2] - window.val[0][0] + tmp2));
    }

    void resizeDown_1x(
        hls::stream<pixel_t>& src_strm, int srcW, int srcH, int dstW, int dstH, hls::stream<pixel_t>& dst_srm) {
        typedef short N16;
        const int NTAPS = 4;

        LineBuffer<4, 8024, pixel_t> linebuf; // the common image width is 8k
        Window<4, 4, fixed_t> window;         // the window is 4*4
        pixel_t pix_in, temp_in[NTAPS], temp_out[NTAPS];

        N16 row, col;
        N16 rows, cols;
        N16 rows_rw = -1;
        N16 cols_rw = -1;
        N16 rel_row, rel_col;

        bool col_rd_en;
        bool col_wr_en;
        bool row_rd_en;
        bool row_wr_en; // row or col read or write flag

        fixed_t row_ratio = ((fixed_t)dstH) / (fixed_t)srcH;
        fixed_t col_ratio = ((fixed_t)dstW) / (fixed_t)srcW;
        fixed_t row_ratio_recip = ((fixed_t)1) / row_ratio;
        fixed_t col_ratio_recip = ((fixed_t)1) / col_ratio;

        ap_ufixed<_W - _I, 0> du, dv;

        int row_rate = (ap_fixed<4, 2, AP_RND>(0.5) + row_ratio * 65536);
        int col_rate = (ap_fixed<4, 2, AP_RND>(0.5) + col_ratio * 65536);

        rows = (srcH > dstH) ? srcH : dstH;
        cols = (srcW > dstW) ? srcW : dstW;
        assert(rows <= srcH || rows <= dstH);
        assert(cols <= srcW || cols <= dstW);

    INTERPOLATE_ROW:
        for (row = 0; row < (rows + 3); row++) {
#pragma HLS LOOP_TRIPCOUNT min = 514 max = 8194
        COL_LOOP:
            for (col = 0; col < (cols + 3); col++) {
#pragma HLS LOOP_TRIPCOUNT min = 514 max = 8194
#pragma HLS PIPELINE
#pragma HLS DEPENDENCE array inter false
                // row write?
                if (col == 0) {
                    if (row_rate <= 65536) { // Down scaling, writes are less frequent than reads
                        row_rd_en = true;
                        N16 drow = row * row_ratio;
                        fixed_t y = (fixed_t)drow * row_ratio_recip;
                        du.range(_W - _I - 1, 0) = y.range(_W - _I - 1, 0);
                        if (rows_rw != drow) {
                            row_wr_en = true;
                            rows_rw = drow;
                        } else
                            row_wr_en = false;
                    }
                }

                // col write?
                if (col_rate <= 65536) { // Down scaling, writes are less frequent than reads
                    col_rd_en = true;
                    N16 dcol = col * col_ratio;
                    fixed_t x = (fixed_t)dcol * col_ratio_recip;
                    dv.range(_W - _I - 1, 0) = x.range(_W - _I - 1, 0);
                    if (col == 0 || (col > 0 && cols_rw != dcol)) {
                        col_wr_en = true;
                        cols_rw = dcol;
                    } else
                        col_wr_en = false;
                }

                for (int i = 0; i < NTAPS; i++) {
                    temp_out[i] = linebuf.val[i][col];
                }

                if (col_rd_en) {
                    window.shift_left();
                    if (row_rd_en) {
                        if (col < cols && row < rows) {
                            pix_in = src_strm.read();
                            // std::cout << pix_in << std::endl;
                            temp_in[NTAPS - 1] = pix_in;
                        } else {
                            temp_in[NTAPS - 1] = temp_out[NTAPS - 1];
                        }
                    }
                }

                for (int i = NTAPS - 1; i > 0; i--) {
                    temp_in[i - 1] = temp_out[i];
                }

                for (int i = 0; i < NTAPS; i++) {
                    if (col == 0 && row == 0) {
                        window.insert(temp_in[NTAPS - 1], i, 0);
                        window.insert(temp_in[NTAPS - 1], i, 1);
                        window.insert(temp_in[NTAPS - 1], i, 2);
                        window.insert(temp_in[NTAPS - 1], i, 3);
                    } else if (col == 0 && row > 0) {
                        window.insert(temp_in[i], i, 0);
                        window.insert(temp_in[i], i, 1);
                        window.insert(temp_in[i], i, 2);
                        window.insert(temp_in[i], i, 3);
                    } else if (col < cols) {
                        (row > 0) ? window.insert(temp_in[i], i, NTAPS - 1)
                                  : window.insert(temp_in[NTAPS - 1], i, NTAPS - 1);
                    }
                }

                for (int i = 0; i < NTAPS; i++) {
                    linebuf.val[i][col] = (row > 0) ? temp_in[i] : temp_in[NTAPS - 1];
                }

#ifndef __SYNTHESIS__
// for (int m = 0; m < 4; m++) {
//   for (int n = 0; n < 4; n++) {
//     std::cout << window.getval(m, n) << " ";
//   }
//   std::cout << std::endl;
// }
// std::cout << std::endl;
#endif
                if (row >= 2 && col >= 2 && row_wr_en && col_wr_en && row < (rows + 2) && col < (cols + 2)) {
                    fixed_t dstPixel = bicubicInterpolate(window, du, dv);
                    dst_srm.write((pixel_t)dstPixel);
                }
            } // col
        }     // row
    }

    void resizeDown_8x(
        hls::stream<pixel_t>& src_strm, int srcW, int srcH, int dstW, int dstH, hls::stream<pixel_8x_t>& dst_srm) {
        typedef short N16;
        const int NTAPS = 4;

        LineBuffer<4, 1024, pixel_t> linebuf;
        Window<4, 4, fixed_t> window;
        pixel_t pix_in;
        pixel_8x_t pix_out;

        ap_uint<_WBIT> temp_in[NTAPS][_NPPC], temp_out[NTAPS][_NPPC], temp_pix[_NPPC];
#pragma HLS array_partition variable = temp_in dim = 1 complete
#pragma HLS array_partition variable = temp_in dim = 2 complete
#pragma HLS array_partition variable = temp_out dim = 1 complete
#pragma HLS array_partition variable = temp_out dim = 2 complete
#pragma HLS array_partition variable = temp_pix dim = 1 complete

        N16 row, col;
        N16 rows, cols, new_row, new_col;
        N16 rows_rw = -1;
        N16 cols_rw[_NPPC] = {-1};
        N16 rel_row, rel_col;

        bool col_rd_en[_NPPC]; // 8x/clock
        bool col_wr_en[_NPPC]; // 8x/clock
        bool row_rd_en;
        bool row_wr_en; // row or col read or write flag

//#pragma HLS array_partition variable = col_rd_en dim = 0 complete
#pragma HLS array_partition variable = col_wr_en dim = 0 complete

        fixed_t row_ratio = ((fixed_t)dstH) / (fixed_t)srcH;
        fixed_t col_ratio = ((fixed_t)dstW) / (fixed_t)srcW;
        fixed_t row_ratio_recip = ((fixed_t)1) / row_ratio;
        fixed_t col_ratio_recip = ((fixed_t)1) / col_ratio;
        ap_ufixed<_W - _I, 0> du, dv[_NPPC];
        ap_uint<4> index_pix = 0;

        int row_rate = (ap_fixed<4, 2, AP_RND>(0.5) + row_ratio * 65536);
        int col_rate = (ap_fixed<4, 2, AP_RND>(0.5) + col_ratio * 65536);

        rows = (srcH > dstH) ? srcH : dstH;
        cols = (srcW > dstW) ? srcW : dstW;
        new_row = rows + 3;
        new_col = (cols >> 3) + 1;
        N16 end_row = rows + 2;
        N16 end_col = cols + 2;

        assert(rows <= srcH || rows <= dstH);
        assert(cols <= srcW || cols <= dstW);

    ROW_LOOP:
        for (row = 0; row < new_row; row++) {
#pragma HLS LOOP_TRIPCOUNT min = 66 max = 1026
        COL_LOOP:
            for (col = 0; col < new_col; col++) {
#pragma HLS LOOP_TRIPCOUNT min = 65 max = 1025
#pragma HLS PIPELINE II = 1
#pragma HLS DEPENDENCE array inter false
                // row write?
                if (col == 0) {
                    if (row_rate <= 65536) { // Down scaling, writes are less frequent than reads
                        row_rd_en = true;
                        N16 drow = row * row_ratio;
                        fixed_t y = (fixed_t)drow * row_ratio_recip;
                        du.range(_W - _I - 1, 0) = y.range(_W - _I - 1, 0);
                        if (rows_rw != drow) {
                            row_wr_en = true;
                            rows_rw = drow;
                        } else
                            row_wr_en = false;
                    }
                }

                // col write?
                rel_col = col << 3;
            NPPC_LOOP:
                for (N16 nc = 0; nc < _NPPC; nc++, rel_col++) {
#pragma HLS unroll
                    if (col_rate <= 65536) { // Down scaling, writes are less frequent than reads
                        col_rd_en[nc] = true;
                        N16 dcol = rel_col * col_ratio;
                        fixed_t x = (fixed_t)dcol * col_ratio_recip;
                        dv[nc].range(_W - _I - 1, 0) = x.range(_W - _I - 1, 0);

                        if (rel_col == 0 || (rel_col > 0 && cols_rw[nc] != dcol)) {
                            col_wr_en[nc] = true;
                            cols_rw[(ap_uint<3>)(nc + 1)] = dcol;
                        } else {
                            col_wr_en[nc] = false;
                            cols_rw[(ap_uint<3>)(nc + 1)] = dcol;
                        }
                    }
                } // nc

            // get a element in linebuf
            READ_FROM_LINEBUF_TO_TEMPOUT:
                for (int i = 0; i < NTAPS; i++) {
                    pixel_t tmp = linebuf.val[i][col];
                    temp_out[i][0] = tmp.range(7, 0);
                    temp_out[i][1] = tmp.range(15, 8);
                    temp_out[i][2] = tmp.range(23, 16);
                    temp_out[i][3] = tmp.range(31, 24);
                    temp_out[i][4] = tmp.range(39, 32);
                    temp_out[i][5] = tmp.range(47, 40);
                    temp_out[i][6] = tmp.range(55, 48);
                    temp_out[i][7] = tmp.range(63, 56);
                } // i

                // read a element from stream
                if (col_rd_en[0]) {
                    // window.shift_left();
                    if (row_rd_en) {
                        if (col < (new_col - 1) && row < rows) {
                            ap_uint<_WBIT> tmp;
                            pix_in = src_strm.read();
                            for (int nb = 0; nb < _NPPC; nb++) {
#pragma HLS unroll
                                tmp.range(_WBIT - 1, 0) = pix_in.range(nb * _WBIT + _WBIT - 1, nb * _WBIT);
                                temp_in[NTAPS - 1][nb] = tmp;
                            } // nb
                        } else {
                            for (int nb = 0; nb < _NPPC; nb++) {
#pragma HLS unroll
                                temp_in[NTAPS - 1][nb] = temp_out[NTAPS - 1][nb];
                            } // nb
                        }
                    }
                }

            // put all elements in temp_in into temp_out
            LOAD_FROM_TEMPOUT_TO_TEMPIN:
                for (int i = NTAPS - 1; i > 0; i--) {
                    for (int nc = 0; nc < _NPPC; nc++) {
                        temp_in[i - 1][nc] = temp_out[i][nc];
                    }
                }

                rel_col = col << 3;
            LOAD_TO_WIN_NPPC:
                for (int nc = 0; nc < _NPPC; nc++, rel_col++) {
#pragma HLS unroll
                    if (col_rd_en[nc]) window.shift_left();
                LOAD_TO_WIN_ONE:
                    for (int i = 0; i < NTAPS; i++) {
                        if (rel_col == 0 && row == 0) {
                            window.insert(temp_in[NTAPS - 1][0], i, 0);
                            window.insert(temp_in[NTAPS - 1][0], i, 1);
                            window.insert(temp_in[NTAPS - 1][0], i, 2);
                            window.insert(temp_in[NTAPS - 1][0], i, 3);
                        } else if (rel_col == 0 && row > 0) {
                            window.insert(temp_in[i][0], i, 0);
                            window.insert(temp_in[i][0], i, 1);
                            window.insert(temp_in[i][0], i, 2);
                            window.insert(temp_in[i][0], i, 3);
                        } else if (rel_col < cols) {
                            (row > 0) ? window.insert(temp_in[i][nc], i, NTAPS - 1)
                                      : window.insert(temp_in[NTAPS - 1][nc], i, NTAPS - 1);
                        }
                    } // i
#ifndef __SYNTHESIS__
                    for (int m = 0; m < 4; m++) {
                        for (int n = 0; n < 4; n++) {
                            std::cout << window.getval(m, n) << " ";
                        }
                        std::cout << std::endl;
                    }
                    std::cout << std::endl;
#endif
                    if (row >= 2 && rel_col >= 2 && row_wr_en && col_wr_en[nc] && row < end_row && rel_col < end_col) {
                        fixed_t dstPixel = bicubicInterpolate(window, du, dv[nc]);
                        temp_pix[index_pix++] = (ap_uint<_WBIT>)dstPixel;
                    }
                } // nb

                if (index_pix > 0) {
                    pix_out.range(7, 0) = temp_pix[0].range(7, 0);
                    pix_out.range(15, 8) = temp_pix[1].range(7, 0);
                    pix_out.range(23, 16) = temp_pix[2].range(7, 0);
                    pix_out.range(31, 24) = temp_pix[3].range(7, 0);
                    pix_out.range(39, 32) = temp_pix[4].range(7, 0);
                    pix_out.range(47, 40) = temp_pix[5].range(7, 0);
                    pix_out.range(55, 48) = temp_pix[6].range(7, 0);
                    pix_out.range(63, 56) = temp_pix[7].range(7, 0);
                    pix_out.range(71, 64) = index_pix;
                    dst_srm.write(pix_out);
                    index_pix = 0;
                }

            WRITE_BACK_LINEBUF:
                for (int i = 0; i < NTAPS; i++) {
                    pixel_t tmp;
                    if (row > 0) {
                        tmp.range(7, 0) = temp_in[i][0].range(7, 0);
                        tmp.range(15, 8) = temp_in[i][1].range(7, 0);
                        tmp.range(23, 16) = temp_in[i][2].range(7, 0);
                        tmp.range(31, 24) = temp_in[i][3].range(7, 0);
                        tmp.range(39, 32) = temp_in[i][4].range(7, 0);
                        tmp.range(47, 40) = temp_in[i][5].range(7, 0);
                        tmp.range(55, 48) = temp_in[i][6].range(7, 0);
                        tmp.range(63, 56) = temp_in[i][7].range(7, 0);
                    } else {
                        tmp.range(7, 0) = temp_in[NTAPS - 1][0].range(7, 0);
                        tmp.range(15, 8) = temp_in[NTAPS - 1][1].range(7, 0);
                        tmp.range(23, 16) = temp_in[NTAPS - 1][2].range(7, 0);
                        tmp.range(31, 24) = temp_in[NTAPS - 1][3].range(7, 0);
                        tmp.range(39, 32) = temp_in[NTAPS - 1][4].range(7, 0);
                        tmp.range(47, 40) = temp_in[NTAPS - 1][5].range(7, 0);
                        tmp.range(55, 48) = temp_in[NTAPS - 1][6].range(7, 0);
                        tmp.range(63, 56) = temp_in[NTAPS - 1][7].range(7, 0);
                    }
                    linebuf.val[i][col] = tmp;
                } // i
            }     // new_col
        }         // row
    }

    void resizeDown_opt_8x(int srcW,
                           int srcH,
                           int dstW,
                           int dstH,
                           hls::stream<pixel_t>& src_strm,
                           hls::stream<pixel_8x_t>& dst_srm,
                           hls::stream<bool>& e_dst) {
        typedef short N16;
        const int NTAPS = 4;

        LineBuffer<4, 1024, pixel_t> linebuf;

        Window<1, 4, fixed_t> x_win;
        Window<1, 4, fixed_t> y_win;
        Window<1, 8, fixed_t> y_buff;
        pixel_t pix_in;
        pixel_8x_t pix_out;

        ap_uint<_WBIT> temp_in[NTAPS][_NPPC], temp_out[NTAPS][_NPPC], temp_pix[_NPPC];
#pragma HLS array_partition variable = temp_in dim = 1 complete
#pragma HLS array_partition variable = temp_in dim = 2 complete
#pragma HLS array_partition variable = temp_out dim = 1 complete
#pragma HLS array_partition variable = temp_out dim = 2 complete
#pragma HLS array_partition variable = temp_pix dim = 1 complete

        N16 row, col;
        N16 rows, cols, new_row, new_col;
        N16 rows_rw = -1;
        N16 cols_rw[_NPPC] = {-1};
        N16 rel_row, rel_col;
        N16 end_row, end_col;

        bool col_rd_en[_NPPC]; // 8x/clock
        bool col_wr_en[_NPPC]; // 8x/clock
        bool row_rd_en;
        bool row_wr_en; // row or col read or write flag

//#pragma HLS array_partition variable = col_rd_en dim = 0 complete
#pragma HLS array_partition variable = col_wr_en dim = 0 complete

        fixed_t row_ratio = ((fixed_t)dstH) / (fixed_t)srcH;
        fixed_t col_ratio = ((fixed_t)dstW) / (fixed_t)srcW;
        fixed_t row_ratio_recip = ((fixed_t)1) / row_ratio;
        fixed_t col_ratio_recip = ((fixed_t)1) / col_ratio;

        ap_ufixed<_W - _I, 0> du, dv[_NPPC];

        ap_uint<4> index_pix = 0;
        ap_uint<3> num_pix = (fixed_t)_NPPC * col_ratio;

        int row_rate = (ap_fixed<4, 2, AP_RND>(0.5) + row_ratio * 65536);
        int col_rate = (ap_fixed<4, 2, AP_RND>(0.5) + col_ratio * 65536);

        rows = (srcH > dstH) ? srcH : dstH;
        cols = (srcW > dstW) ? srcW : dstW;
        new_row = rows + 3;
        new_col = (cols >> 3) + 1;
        end_row = rows + 2;
        end_col = cols + 2;
        assert(rows <= srcH || rows <= dstH);
        assert(cols <= srcW || cols <= dstW);

    INTERPOLATE_ROW:
        for (row = 0; row < new_row; row++) {
#pragma HLS LOOP_TRIPCOUNT min = 66 max = 1026
        COL_LOOP:
            for (col = 0; col < new_col; col++) {
#pragma HLS LOOP_TRIPCOUNT min = 65 max = 1025
#pragma HLS PIPELINE II = 1
#pragma HLS DEPENDENCE array inter false
                // row write?
                if (col == 0) {
                    if (row_rate <= 65536) { // Down scaling, writes are less frequent than reads
                        row_rd_en = true;
                        N16 drow = row * row_ratio;
                        fixed_t y = (fixed_t)drow * row_ratio_recip;
                        du.range(_W - _I - 1, 0) = y.range(_W - _I - 1, 0);
                        if (rows_rw != drow) {
                            row_wr_en = true;
                            rows_rw = drow;
                        } else
                            row_wr_en = false;
                    }
                }

                // col write?
                rel_col = col << 3;
            NPPC_LOOP:
                for (N16 nc = 0; nc < _NPPC; nc++, rel_col++) {
#pragma HLS unroll
                    if (col_rate <= 65536) { // Down scaling, writes are less frequent than reads
                        col_rd_en[nc] = true;
                        N16 dcol = rel_col * col_ratio;
                        fixed_t x = (fixed_t)dcol * col_ratio_recip;
                        dv[nc].range(_W - _I - 1, 0) = x.range(_W - _I - 1, 0);

                        if (rel_col == 0 || (rel_col > 0 && cols_rw[nc] != dcol)) {
                            col_wr_en[nc] = true;
                            cols_rw[(ap_uint<3>)(nc + 1)] = dcol;
                        } else {
                            col_wr_en[nc] = false;
                            cols_rw[(ap_uint<3>)(nc + 1)] = dcol;
                        }
                    }
                } // _NPPC

            // get a element in linebuf
            READ_FROM_LINEBUF_TO_TEMPOUT:
                for (int i = 0; i < NTAPS; i++) {
                    pixel_t tmp = linebuf.val[i][col];
                    temp_out[i][0] = tmp.range(7, 0);
                    temp_out[i][1] = tmp.range(15, 8);
                    temp_out[i][2] = tmp.range(23, 16);
                    temp_out[i][3] = tmp.range(31, 24);
                    temp_out[i][4] = tmp.range(39, 32);
                    temp_out[i][5] = tmp.range(47, 40);
                    temp_out[i][6] = tmp.range(55, 48);
                    temp_out[i][7] = tmp.range(63, 56);
                } // i

                // read a element from stream
                if (col_rd_en[0]) {
                    // window.shift_left();
                    if (row_rd_en) {
                        if (col < (new_col - 1) && row < rows) {
                            ap_uint<_WBIT> tmp;
                            pix_in = src_strm.read();
                            for (int nb = 0; nb < _NPPC; nb++) {
#pragma HLS unroll
                                tmp.range(_WBIT - 1, 0) = pix_in.range(nb * _WBIT + _WBIT - 1, nb * _WBIT);
                                temp_in[NTAPS - 1][nb] = tmp;
                            } // nb
                        } else {
                            for (int nb = 0; nb < _NPPC; nb++) {
#pragma HLS unroll
                                temp_in[NTAPS - 1][nb] = temp_out[NTAPS - 1][nb];
                            } // nb
                        }
                    }
                }

                // put all elements in temp_in into temp_out
                rel_col = col << 3;
            LOAD_FROM_TEMPOUT_TO_TEMPIN:
                for (int nc = 0; nc < _NPPC; nc++, rel_col++) {
                    for (int i = NTAPS - 1; i > 0; i--) {
                        fixed_t fix;
                        fix = temp_out[i][nc];
                        temp_in[i - 1][nc] = fix;
                    }

                    if (row == 0 && rel_col < cols) {
                        y_win.val[0][0] = temp_in[NTAPS - 1][nc];
                        y_win.val[0][1] = temp_in[NTAPS - 1][nc];
                        y_win.val[0][2] = temp_in[NTAPS - 1][nc];
                        y_win.val[0][3] = temp_in[NTAPS - 1][nc];
                    } else if (rel_col < cols) {
                        y_win.val[0][0] = temp_in[0][nc];
                        y_win.val[0][1] = temp_in[1][nc];
                        y_win.val[0][2] = temp_in[2][nc];
                        y_win.val[0][3] = temp_in[3][nc];
                    }
                    y_buff.val[0][nc] = cubicInterpolate_one(y_win, du);
                }

                rel_col = col << 3;
            LOAD_TO_WIN_NPPC:
                for (int nc = 0; nc < _NPPC; nc++, rel_col++) {
                    if (col_rd_en[nc]) x_win.shift_left();

                    if (rel_col == 0) {
                        x_win.insert(y_buff.val[0][nc], 0, 0);
                        x_win.insert(y_buff.val[0][nc], 0, 1);
                        x_win.insert(y_buff.val[0][nc], 0, 2);
                        x_win.insert(y_buff.val[0][nc], 0, 3);
                    } else if (rel_col < cols)
                        x_win.insert(y_buff.val[0][nc], 0, NTAPS - 1);

                    if (row >= 2 && rel_col >= 2 && row_wr_en && col_wr_en[nc] && row < end_row && rel_col < end_col) {
                        fixed_t dstPixel = cubicInterpolate_one(x_win, dv[nc]);
                        temp_pix[index_pix++] = (ap_uint<_WBIT>)dstPixel;
                    }
                } // nb

                if (index_pix > 0) {
                    pix_out.range(7, 0) = temp_pix[0].range(7, 0);
                    pix_out.range(15, 8) = temp_pix[1].range(7, 0);
                    pix_out.range(23, 16) = temp_pix[2].range(7, 0);
                    pix_out.range(31, 24) = temp_pix[3].range(7, 0);
                    pix_out.range(39, 32) = temp_pix[4].range(7, 0);
                    pix_out.range(47, 40) = temp_pix[5].range(7, 0);
                    pix_out.range(55, 48) = temp_pix[6].range(7, 0);
                    pix_out.range(63, 56) = temp_pix[7].range(7, 0);
                    pix_out.range(71, 64) = index_pix;
                    dst_srm.write(pix_out);
                    e_dst.write(false);
                    index_pix = 0;
                }

            WRITE_BACK_LINEBUF:
                for (int i = 0; i < NTAPS; i++) {
                    pixel_t tmp;
                    if (row > 0) {
                        tmp.range(7, 0) = temp_in[i][0].range(7, 0);
                        tmp.range(15, 8) = temp_in[i][1].range(7, 0);
                        tmp.range(23, 16) = temp_in[i][2].range(7, 0);
                        tmp.range(31, 24) = temp_in[i][3].range(7, 0);
                        tmp.range(39, 32) = temp_in[i][4].range(7, 0);
                        tmp.range(47, 40) = temp_in[i][5].range(7, 0);
                        tmp.range(55, 48) = temp_in[i][6].range(7, 0);
                        tmp.range(63, 56) = temp_in[i][7].range(7, 0);
                    } else {
                        tmp.range(7, 0) = temp_in[NTAPS - 1][0].range(7, 0);
                        tmp.range(15, 8) = temp_in[NTAPS - 1][1].range(7, 0);
                        tmp.range(23, 16) = temp_in[NTAPS - 1][2].range(7, 0);
                        tmp.range(31, 24) = temp_in[NTAPS - 1][3].range(7, 0);
                        tmp.range(39, 32) = temp_in[NTAPS - 1][4].range(7, 0);
                        tmp.range(47, 40) = temp_in[NTAPS - 1][5].range(7, 0);
                        tmp.range(55, 48) = temp_in[NTAPS - 1][6].range(7, 0);
                        tmp.range(63, 56) = temp_in[NTAPS - 1][7].range(7, 0);
                    }
                    linebuf.val[i][col] = tmp;
                } // i
            }     // new_col
        }         // row
        e_dst.write(true);
    }
};

#if NPPC == 1
/**
 * @brief The function is loading the pixels of image into stream
 *
 * @param width representing the number of input image each row
 * @param height representing the number of input image each column
 * @param axi_src the hbm port for input
 * @param src_strm the input stream of bicubic interpolator
 */
void loadToStrm(ap_uint<32> width,
                ap_uint<32> height,
                ap_uint<WDATA>* axi_src,
                hls::stream<ap_uint<WDATA> >& src_strm) {
#pragma HLS INLINE off
LOAD_STRM:
    for (ap_uint<32> i = 0; i < (width * height); i++) {
#pragma HLS PIPELINE II = 1
        src_strm.write(axi_src[i]);
    }
}

/**
 * @brief The function is putting the result of interpolation into memory hbm
 *
 * @param width representing the number of output image each row
 * @param height representing the number of output image each column
 * @param dst_strm the output stream of bicubic interpolator
 * @param axi_dst the hbm port for output
 */
void loadToImage(ap_uint<32> width,
                 ap_uint<32> height,
                 hls::stream<ap_uint<WDATA> >& dst_strm,
                 ap_uint<WDATA>* axi_dst) {
#pragma HLS INLINE off
LOAD_IMAGE:
    for (ap_uint<32> i = 0; i < (width * height); i++) {
#pragma HLS PIPELINE II = 1
        axi_dst[i] = dst_strm.read();
    }
}

#else

/**
 * @brief The function is loading the pixels of image into stream
 *
 * @param width representing the number of input image each row
 * @param height representing the number of input image each column
 * @param axi_src the hbm port for input
 * @param src_strm the input stream of bicubic interpolator
 */
void loadToStrm(ap_uint<32> width,
                ap_uint<32> height,
                ap_uint<WDATA>* axi_src,
                hls::stream<ap_uint<WDATA> >& src_strm) {
#pragma HLS INLINE off
LOAD_STRM:
    for (ap_uint<32> i = 0; i<(width * height)>> 3; i++) {
#pragma HLS PIPELINE II = 1
        src_strm.write(axi_src[i]);
    }
}

/**
 * @brief The function is picking out valid value of interpolation from 72-bits, the (0, 63) saving the valid value and
 * the (64, 71) representing the number of valid value of interpolation
 *
 * @param dst_strm the output of bicubic interpolator
 * @param e_dst    the flag of output
 * @param pixel_strm the compact 64-bits or representing 8 pixels
 */
void pickOutStrm(hls::stream<ap_uint<72> >& dst_strm,
                 hls::stream<bool>& e_dst,
                 hls::stream<ap_uint<WDATA> >& pixel_strm) {
#pragma HLS INLINE off
    bool stop(false);
    ap_uint<72> pixel_72;
    ap_uint<64> pixel_out;
    int num_cur = 0;

PICK_UP_PIXELS:
    while (!stop) {
#pragma HLS PIPELINE II = 1
        e_dst.read(stop);
        if (!stop) {
            dst_strm.read(pixel_72);
            int num_pixs = pixel_72.range(71, 64);
            num_cur += num_pixs;
            if (num_cur < 8) {
                pixel_out.range(num_cur * 8 - 1, (num_cur - num_pixs) * 8) = pixel_72.range(num_pixs * 8 - 1, 0);
            } else {
                int tmp = num_cur - 8;
                if (tmp != 0) {
                    pixel_out.range(63, (num_cur - num_pixs) * 8) = pixel_72.range(num_pixs * 8 - 1, 0);
                    pixel_strm.write(pixel_out);
                    pixel_out.range(tmp * 8 - 1, 0) = pixel_72.range(num_pixs * 8 - 1, (num_pixs - tmp) * 8);
                } else {
                    pixel_out.range(63, (num_cur - num_pixs) * 8) = pixel_72.range(num_pixs * 8 - 1, 0);
                    pixel_strm.write(pixel_out);
                }
                num_cur = tmp;
            }
        }
    }
    if (num_cur > 0) pixel_strm.write(pixel_out);
}

/**
 * @brief The function is putting the result of interpolation into memory hbm
 *
 * @param width representing the number of output image each row
 * @param height representing the number of output image each column
 * @param dst_strm the output stream of bicubic interpolator
 * @param axi_dst the hbm port for output
 */
void loadToImage(ap_uint<32> width,
                 ap_uint<32> height,
                 hls::stream<ap_uint<WDATA> >& pixel_strm,
                 ap_uint<WDATA>* axi_dst) {
#pragma HLS INLINE off
    ap_uint<64> pixel_64;

LOAD_IMAGE:
    for (ap_uint<32> i = 0; i < DivCeil(width * height, 8); i++) {
#pragma HLS PIPELINE II = 1
        pixel_strm.read(pixel_64);
        // for (int i = 0; i < 8; i++) std::cout << (int)pixel_64.range((i + 1) * 8 - 1, i * 8) << std::endl;
        axi_dst[i] = pixel_64;
    }
}
#endif

} // namespace details

/**
 * @brief Resize scales the image from bigger to smaller based bicubic interpolation algorithm and it takes advantage of
 * uram storage features to implement 8-pixels/clock.
 *
 * @param configs the stored parameters representing src_width, src_height, dst_width, dst_height.
 * @param axi_src the hbm memory for input
 * @param axi_dst the hbm memory for output
 *
 */
inline void resizeTop(ap_uint<32>* configs, ap_uint<WDATA>* axi_src, ap_uint<WDATA>* axi_dst) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    // define the bicubic interpolator
    hls::stream<ap_uint<WDATA> > src_strm("src_strm");
#pragma HLS RESOURCE variable = src_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = src_strm depth = 512

#if NPPC == 1
    hls::stream<ap_uint<WDATA> > dst_strm("dst_strm");
#pragma HLS RESOURCE variable = dst_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = dst_strm depth = 512

    ap_uint<32> src_width = configs[0];
    ap_uint<32> src_height = configs[1];
    ap_uint<32> dst_width = configs[2];
    ap_uint<32> dst_height = configs[3];

    details::BicubicInterpolator<W, I, WBIT, NPPC> interpolator;
    details::loadToStrm(src_width, src_height, axi_src, src_strm);
    interpolator.resizeDown_1x(src_strm, src_width, src_height, dst_width, dst_height, dst_strm);
    details::loadToImage(dst_width, dst_height, dst_strm, axi_dst);
#else
    hls::stream<ap_uint<72> > dst_strm("dst_strm");
#pragma HLS RESOURCE variable = dst_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = dst_strm depth = 512

    hls::stream<bool> e_dst("e_dst");
#pragma HLS RESOURCE variable = e_dst core = FIFO_LUTRAM
#pragma HLS STREAM variable = e_dst depth = 512

    hls::stream<ap_uint<WDATA> > pixel_strm("pixel_strm");
#pragma HLS RESOURCE variable = pixel_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = pixel_strm depth = 512

    ap_uint<32> src_width = configs[0];
    ap_uint<32> src_height = configs[1];
    ap_uint<32> dst_width = configs[2];
    ap_uint<32> dst_height = configs[3];

    details::BicubicInterpolator<W, I, WBIT, NPPC> interpolator;
    details::loadToStrm(src_width, src_height, axi_src, src_strm);
    interpolator.resizeDown_opt_8x(src_width, src_height, dst_width, dst_height, src_strm, dst_strm, e_dst);
    details::pickOutStrm(dst_strm, e_dst, pixel_strm);
    details::loadToImage(dst_width, dst_height, pixel_strm, axi_dst);
#endif
}

} // namespace codec
} // namespace xf
#endif // _BICUBIC_INTERPOLATOR_HPP_
