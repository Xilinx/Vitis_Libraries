#
# Copyright 2019 Xilinx, Inc.
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

C_COMPUTE_UNITS := 2
H_COMPUTE_UNITS := 2
D_COMPUTE_UNITS := 9 
PARALLEL_BLOCK  := 8
MULTIPLE_BYTES  := 8

CXXFLAGS += -DPARALLEL_BLOCK=$(PARALLEL_BLOCK) -DC_COMPUTE_UNIT=$(C_COMPUTE_UNITS) -DH_COMPUTE_UNIT=$(H_COMPUTE_UNITS) -DD_COMPUTE_UNIT=$(D_COMPUTE_UNITS) -DOVERLAP_HOST_DEVICE
