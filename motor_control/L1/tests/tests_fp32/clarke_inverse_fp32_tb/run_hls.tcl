#
# Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
# SPDX-License-Identifier: X11
#

source settings.tcl

set PROJ "clarke_inverse_fp32_hls.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 10
}

open_project -reset $PROJ

add_files "${XF_PROJ_ROOT}/L1/tests/tests_fp32/clarke_inverse_fp32_tb/src/clarke_inverse_fp32_top.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/models_fp -I${XF_PROJ_ROOT}/L1/tests/tests_fp32/clarke_inverse_fp32_tb/src"
add_files -tb "${XF_PROJ_ROOT}/L1/tests/tests_fp32/clarke_inverse_fp32_tb/tb/test_clarke_inverse_fp32.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/models_fp -I${XF_PROJ_ROOT}/L1/tests/tests_fp32/clarke_inverse_fp32_tb/src -I${XF_PROJ_ROOT}/L1/tests/tests_fp32/precision_common"
set_top Clarke_Inverse_2p_fp32_top

open_solution -reset $SOLN


set_part $XPART
create_clock -period $CLKP
set_clock_uncertainty 1.25

if {$CSIM == 1} {
  csim_design
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit
