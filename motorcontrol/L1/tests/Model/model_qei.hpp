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