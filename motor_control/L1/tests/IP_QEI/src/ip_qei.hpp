/*
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
