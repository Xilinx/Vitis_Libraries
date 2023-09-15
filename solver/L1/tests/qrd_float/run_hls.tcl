# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
# SPDX-License-Identifier: X11
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Except as contained in this notice, the name of Advanced Micro Devices
# shall not be used in advertising or otherwise to promote the sale,
# use or other dealings in this Software without prior written authorization
# from Advanced Micro Devices, Inc.


set CSIM 1
set CSYNTH 1
set COSIM 1
set VIVADO_SYN 1
set VIVADO_IMPL 1
set CUR_DIR [pwd]
set XF_PROJ_ROOT $CUR_DIR/../../..
set XPART xcvc1902-vsva2197-2MP-e-S

set PROJ "qrd_test256_float.prj"
set SOLN "sol1"
set TESTPATH "qrd_float"
set QRD_A_ROWS "256"
set QRD_A_COLS "64"

if {![info exists CLKP]} {
  set CLKP 500MHz
}

open_project -reset $PROJ

add_files "${XF_PROJ_ROOT}/L1/include/hw/qrdfloat.hpp" -cflags "-D_DATA_PATH=${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/data/ -I./ -I${XF_PROJ_ROOT}/L1/tests/ -I${XF_PROJ_ROOT}/L1/include/ -I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/L2/include -I${XF_PROJ_ROOT}/../utils/L1/include/"
add_files "${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/src/qrd.cpp" -cflags "-DQRF_A_ROWS=${QRD_A_ROWS} -DQRF_A_COLS=${QRD_A_COLS} -D_DATA_PATH=${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/data/ -I./ -I${XF_PROJ_ROOT}/L1/tests/qrf/host/ -I${XF_PROJ_ROOT}/L1/tests/qrf/kernel/ -I${XF_PROJ_ROOT}/L1/tests/qrf/ -I${XF_PROJ_ROOT}/L1/tests/ -I${XF_PROJ_ROOT}/L1/include/ -I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/../utils/L1/include/"
add_files -tb "${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/test_qrd.cpp" -cflags "-O0 -g -DQRF_A_ROWS=${QRD_A_ROWS} -DQRF_A_COLS=${QRD_A_COLS} -D_DATA_PATH=${XF_PROJ_ROOT}/L1/tests/${TESTPATH}/data/ -I./ -I${XF_PROJ_ROOT}/L1/tests/qrf/host/ -I${XF_PROJ_ROOT}/L1/tests/qrf/kernel/ -I${XF_PROJ_ROOT}/L1/tests/qrf/ -I${XF_PROJ_ROOT}/L1/tests/ -I${XF_PROJ_ROOT}/L1/include/ -I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/../utils/L1/include/"
#-DDEBUG_QRD

#IP to estimate performance
set_top qrd_top

#IP + load buffer + output buffer to estimate resource
#set_top qrd_ip_top

open_solution -reset $SOLN

set_part $XPART
create_clock -period $CLKP
set_clock_uncertainty 0.1

if {$CSIM == 1} {
  csim_design -argv "-data ${XF_PROJ_ROOT}/L1/tests/qrd_float/data"
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design -argv "-data ${XF_PROJ_ROOT}/L1/tests/qrd_float/data"
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit
