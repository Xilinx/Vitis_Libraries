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

#ifndef _IP_QEI_HPP_
#define _IP_QEI_HPP_

#include "ap_int.h"
#include <hls_stream.h>
#include "qei.hpp"
#define TESTNUMBER (10000000)
typedef ap_uint<1> t_bin_qei;
typedef ap_uint<2> t_err_qei;

void hls_qei(hls::stream<t_bin_qei>& qei_A,
             hls::stream<t_bin_qei>& qei_B,
             hls::stream<t_bin_qei>& qei_I,
             hls::stream<ap_uint<32> >& qei_RPM_THETA_m,
             hls::stream<t_bin_qei>& qei_dir,
             hls::stream<t_err_qei>& qei_err,
             volatile int& qei_args_cpr,
             volatile int& qei_args_ctrl,
             volatile int& qei_stts_RPM_THETA_m,
             volatile int& qei_stts_dir,
             volatile int& qei_stts_err);

#endif // _IP_QEI_HPP_
