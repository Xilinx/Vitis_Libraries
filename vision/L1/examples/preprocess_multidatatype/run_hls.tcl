# (C) Copyright 2020 - 2021 Xilinx, Inc.
# SPDX-License-Identifier: Apache-2.0

source settings.tcl 

set VITIS_LIBS ../../../

set PROJ "preprocess_multidatatype.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 3.3
}

if {![info exists XPART]} {
  set XPART xcvc1902-vsva2197-2MP-e-S
}

open_project -reset $PROJ

add_files "${VITIS_LIBS}/L1/examples/preprocess_multidatatype/xf_preprocess_accel.cpp" -cflags "-I${VITIS_LIBS}/L1/include -I ${VITIS_LIBS}/L1/examples/preprocess_multidatatype/config/ -I ./ -D__SDSVHLS__ -std=c++14" -csimflags "-I${VITIS_LIBS}/L1/include -I ${VITIS_LIBS}/L1/examples/preprocess_multidatatype/config/ -I ./ -D__SDSVHLS__ -std=c++14"
set_top preprocess_accel

open_solution -reset $SOLN

set_part $XPART
create_clock -period $CLKP

csynth_design
export_design -rtl verilog -format ip_catalog

exit