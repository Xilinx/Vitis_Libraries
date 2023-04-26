#
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
#

source settings.tcl


set PROJ "ip_foc_periodic_ap_fixed.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 10
}

open_project -reset $PROJ

add_files "${CUR_DIR}/src/ip_foc.cpp" -cflags "-O0 -I${XF_PROJ_ROOT}/L1/include/hw -I${CUR_DIR}/src"
add_files -tb "${CUR_DIR}/src/test_ip_foc.cpp" -cflags "-O0 -I${XF_PROJ_ROOT}/L1/include/hw -I${CUR_DIR}/src -I${XF_PROJ_ROOT}/L1/tests/Model -Wno-write-strings"
set_top hls_foc_periodic_ap_fixed

open_solution -reset $SOLN

set_part $XPART
create_clock -period $CLKP
set_clock_uncertainty 1.25

#config_interface -s_axilite_auto_restart_counter 1
#config_interface -s_axilite_sw_reset

if {$CSIM == 1} {
#  csim_design -ldflags -lncurses
}

if {$CSYNTH == 1} {
# Set any optimization directives
#  set_directive_reset foc_core_ap_fixed sin_table
#  set_directive_reset foc_core_ap_fixed cos_table
  csynth_design
}

if {$COSIM == 1} {
#  cosim_design -ldflags -lncurses
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit
