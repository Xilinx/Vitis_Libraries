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
# vitis hls makefile-generator v2.0.0

set CSIM 1
set CSYNTH 1
set COSIM 1
set VIVADO_SYN 1
set VIVADO_IMPL 1
set CUR_DIR [pwd]
set XF_PROJ_ROOT $CUR_DIR/../../../..
set XPART xcu50-fsvh2104-2-e

set PROJ "sparseSimilarity.prj"
set SOLN "solution_OCL_REGION_0"

if {![info exists CLKP]} {
  set CLKP 300MHz
}

open_project -reset $PROJ

add_files "kernel/sparseSimilarityKernel.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/kernel"
add_files -tb "host/test_similarity.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/kernel -I${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/host"
set_top sparseSimilarityKernel

open_solution -reset $SOLN



set_part $XPART
create_clock -period $CLKP

if {$CSIM == 1} {
  csim_design -argv "-similarityType 1 -graphType 0 -dataType 0 -sourceID 3 -offset ${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/data/cosine_sparse_offset.csr -indiceWeight ${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/data/cosine_sparse_indice_weight.csr -golden ${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/data/cosine_sparse.mtx"
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design -argv "-similarityType 1 -graphType 0 -dataType 0 -sourceID 3 -offset ${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/data/cosine_sparse_offset.csr -indiceWeight ${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/data/cosine_sparse_indice_weight.csr -golden ${XF_PROJ_ROOT}/L1/tests/hw/sparse_similarity/data/cosine_sparse.mtx"
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit