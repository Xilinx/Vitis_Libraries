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

ifndef XILINX_XRT
    $(error XILINX_XRT not defined)
endif

SW_API_DIR = ../../include/sw

CXX = g++
HOST_CXXFLAGS = -O0 -std=c++11 -fPIC -Wextra -Wall -Wno-ignored-attributes -Wno-unused-parameter -Wno-unused-variable
HOST_INCLUDE = -I$(XILINX_XRT)/include -I$(SW_API_DIR)
HOST_LIB = -L$(XILINX_XRT)/lib -lz -lxilinxopencl -lstdc++ -lrt -pthread

XCL2_SRC = $(SW_API_DIR)/xf_blas/xcl2/xcl2.cpp

######################################################

.PHONY: all

all: clean gemm_test.exe

clean: 
	rm -rf *.exe

gemm_test.exe: gemm_test.cpp $(XCL2_SRC)
	$(CXX) $(HOST_CXXFLAGS) $(HOST_INCLUDE) $(HOST_LIB) -o $@ $^