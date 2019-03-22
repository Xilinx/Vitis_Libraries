

############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 2019 Xilinx Inc. All rights reserved.
############################################################
set HLS_REGR_HOME $::env(HLS_REGR_HOME)
open_project prj

set_top top_trmv

#
add_files ../main.cpp -cflags " -I /proj/autoesl/fengx/work/xSolver-dev/include "
add_files -tb ../main.cpp -cflags " -I /proj/autoesl/fengx/work/xSolver-dev/include "

open_solution "solution"

set_part {xcvu9p-fsgd2104-2l-e}
create_clock -period 4.0 -name default

csim_design
csynth_design
cosim_design -argv $argv -rtl verilog

#export_design -evaluate verilog
#export_design -format ip_catalog
