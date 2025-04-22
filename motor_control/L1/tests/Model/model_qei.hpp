/*
Copyright (C) 2022-2022, Xilinx, Inc.
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#ifndef _MODEL_QEITB_HPP_
#define _MODEL_QEITB_HPP_
#include "ap_int.h"
#include <hls_stream.h>
#include "model_motor.hpp"
#include "model_foc.hpp"
//#include "qei.hpp"

void qei(hls::stream<ap_uint<1> >& A,
         hls::stream<ap_uint<1> >& B,
         hls::stream<ap_uint<1> >& I,
         hls::stream<ap_uint<16> >& output_speed,
         hls::stream<ap_uint<16> >& output_angle,
         hls::stream<ap_uint<1> >& output_dir,
         hls::stream<ap_uint<2> >& err);

class QEI_TB : public Model_base {
    // InversQEI uses the internal parameters w and theta to generate test data for qei API
    // input from motor model
    double w;
    double theta;
    double* input_w_pull;
    double* input_theta_pull;
    // middle signal for qei API
    ap_uint<1> A;
    ap_uint<1> B;
    ap_uint<1> I;
    hls::stream<ap_uint<1> > strm_A;
    hls::stream<ap_uint<1> > strm_B;
    hls::stream<ap_uint<1> > strm_I;
    hls::stream<ap_uint<16> > strm_output_speed;
    hls::stream<ap_uint<16> > strm_output_angle;
    hls::stream<ap_uint<1> > strm_output_dir;
    hls::stream<ap_uint<2> > strm_err;
    ap_uint<16> speed_out;
    ap_uint<16> angle_out;
    ap_uint<1> dir_out;
    ap_uint<2> err_out;

    QEI_TB() {
        input_w_pull = NULL;
        input_theta_pull = NULL;
    }
    void pullInput() {
        assert(input_w_pull != NULL);
        w = *input_w_pull;
        assert(input_theta_pull != NULL);
        theta = *input_theta_pull;
    }
    void updatingABI(

        ) {}
    void updating(double dt) {
        updatingABI();
        qei(strm_A, strm_B, strm_I, strm_output_speed, strm_output_angle, strm_output_dir, strm_err);
    }
    void pushOutput() {
        speed_out = strm_output_speed.read();
        angle_out = strm_output_angle.read();
        dir_out = strm_output_dir.read();
        err_out = strm_err.read();
    }
};

#endif // _MODEL_QEITB_HPP_