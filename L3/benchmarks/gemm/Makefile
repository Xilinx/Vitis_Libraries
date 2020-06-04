 
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

# -----------------------------------------------------------------------------
#                          project common settings

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))

.SECONDEXPANSION:

# -----------------------------------------------------------------------------
#                            common tool setup


.PHONY: help

help::
	@echo ""
	@echo "Makefile Usage:"
	@echo ""
	@echo "  make host "
	@echo "      Command to generate host."
	@echo ""
	@echo "  make clean "
	@echo "      Command to remove the generated non-hardware files."
	@echo ""
	@echo "  make cleanall"
	@echo "      Command to remove all the generated files."
	@echo ""

ifeq (,$(XILINX_XRT))
XILINX_XRT = /opt/xilinx/xrt
endif
export XILINX_XRT
.PHONY: check_xrt
check_xrt:
ifeq (,$(wildcard $(XILINX_XRT)/lib/libxilinxopencl.so))
	@echo "Cannot locate XRT installation. Please set XILINX_XRT variable." && false
endif

export PATH := $(XILINX_XRT)/bin:$(PATH)

ifeq (,$(LD_LIBRARY_PATH))
LD_LIBRARY_PATH := $(XILINX_XRT)/lib
else
LD_LIBRARY_PATH := $(XILINX_XRT)/lib:$(LD_LIBRARY_PATH)
endif

# -----------------------------------------------------------------------------
# BEGIN_XF_MK_USER_SECTION
# -----------------------------------------------------------------------------

XF_PROJ_ROOT ?= $(CUR_DIR)/../../..
XFLIB_DIR := $(abspath $(XF_PROJ_ROOT))

XCLBIN_FILE :=
KERNELS :=

# -----------------------------------------------------------------------------

SRC_DIR = $(CUR_DIR)

EXE_NAME = gemm_bench.exe
HOST_ARGS =

SRCS = gemm_bench.cpp

CXXFLAGS += -g -I$(XILINX_XRT)/include -I $(XFLIB_DIR)/L3/include/sw -I $(XFLIB_DIR)/L3/benchmarks/


XFBLAS_dataType ?= short
XFBLAS_numKernels ?= 1

ifneq (${XFBLAS_numKernels}, 1)
  $(info [INFO] Asynchronous Concurrent Execution Is Enabled.)
  CXXFLAGS += -DXFBLAS_LAUNCH_ASYNC
endif


# -----------------------------------------------------------------------------
# END_XF_MK_USER_SECTION
# -----------------------------------------------------------------------------

.PHONY: all
all: host 

OBJ_DIR_BASE ?= obj
BIN_DIR_BASE ?= bin

OBJ_DIR = $(CUR_DIR)/$(OBJ_DIR_BASE)$(BIN_DIR_SUFFIX)
BIN_DIR = $(CUR_DIR)/$(BIN_DIR_BASE)$(BIN_DIR_SUFFIX)

RUN_ENV =
OBJ_FILES = 
EXTRA_OBJS = 

CXX := xcpp
CC := gcc

TARGET = hw

CXXFLAGS += -O0 -std=c++11 -fPIC -Wextra -Wall -Wno-ignored-attributes -Wno-unused-parameter -Wno-unused-variable
CFLAGS +=
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -lxilinxopencl
LDFLAGS += -L$(XILINX_XRT)/lib -lz -lstdc++ -lrt -pthread -lxrt_core -ldl -luuid

BUILD_DIR = out_host
EXE_EXT ?= exe
EXE_FILE ?= $(BUILD_DIR)/$(EXE_NAME)

$(EXE_FILE): $(SRCS) | check_xrt 
	@echo -e "----\nCompiling host $(notdir $@)..."
	mkdir -p $(BUILD_DIR)
	$(CC) -D XFBLAS_dataType=$(XFBLAS_dataType) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

.PHONY: host
host: check_xrt $(EXE_FILE)

xclbin: download_xclbin

build: download_xclbin

download_xclbin:
ifeq (,$(wildcard ./xf_blas_xclbins))
	wget -nc "https://www.xilinx.com/bin/public/openDownload?filename=vitis_BLAS_library_r1.0_xclbin.tar"
	tar -zxvf openDownload?filename=vitis_BLAS_library_r1.0_xclbin.tar
endif


# -----------------------------------------------------------------------------
#                                clean up

clean:
ifneq (,$(OBJ_DIR_BASE))
	rm -rf $(CUR_DIR)/$(OBJ_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*
endif

cleanall: clean 
	rm -rf *.log plist $(DATA_STAMP) $(BUILD_DIR)

.PHONY: run 

run: host
	$(EXE_FILE) xf_blas_xclbins/u200_xdma_201830_2/gemm_short_2kernel/gemx.xclbin xf_blas_xclbins/u200_xdma_201830_2/gemm_short_2kernel/config_info.dat 256 256 256 ./data/$(XFBLAS_dataType)/ ${XFBLAS_numKernels}


check: run
