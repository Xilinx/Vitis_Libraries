#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
if {![info exists ::env(XF_PROJ_ROOT)]} {
  echo "Please set XF_PROJ_ROOT firstly"
}

set XF_PROJ_ROOT $::env(XF_PROJ_ROOT)

open_project -reset prj
add_files "../kernel/re_engine_kernel.cpp" -cflags "-DHLS_TEST -I../kernel -I${XF_PROJ_ROOT}/L1/include/hw/xf_search -I${XF_PROJ_ROOT}/L2/include/xf_search "

add_files -tb "../host/test.cpp " -cflags "-DHLS_TEST -I${XF_PROJ_ROOT}/L2/include/xf_search -I${XF_PROJ_ROOT}/L1/include/hw/xf_search -I../kernel -I${XF_PROJ_ROOT}/ext/oniguruma/lib/include -I${XF_PROJ_ROOT}/L1/include/sw/xf_search -L${XF_PROJ_ROOT}/ext/oniguruma/lib/lib -lonig -L${XF_PROJ_ROOT}/L2/tests/reEngine/re_compile -lxfcompile"

set_top reEngineKernel
open_solution -reset sol


#set_part xcu250-figd2104-2L-e
set_part xcu200-fsgd2104-2-e
create_clock -period 3.33 -name default

csim_design -ldflags "-Wl,-rpath,${XF_PROJ_ROOT}/ext/oniguruma/lib/lib -L${XF_PROJ_ROOT}/ext/oniguruma/lib/lib -lonig -Wl,-rpath,${XF_PROJ_ROOT}/L2/tests/reEngine/re_compile -L${XF_PROJ_ROOT}/L2/tests/reEngine/re_compile -lxfcompile " -argv "-dir ${XF_PROJ_ROOT}/L2/tests/reEngine/log_data/ -ln 1000"
#csynth_design
#cosim_design -ldflags "-Wl,-rpath,${XF_PROJ_ROOT}/ext/oniguruma/lib/lib -L${XF_PROJ_ROOT}/ext/oniguruma/lib/lib -lonig -Wl,-rpath,${XF_PROJ_ROOT}/L2/tests/reEngine/re_compile -L${XF_PROJ_ROOT}/L2/tests/reEngine/re_compile -lxfcompile " -trace_level all
##export_design -flow impl -rtl verilog -format ip_catalog
exit
