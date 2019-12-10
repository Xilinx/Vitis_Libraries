 
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
	@echo "  make api "
	@echo "      Command to build shared library file."
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

XF_PROJ_ROOT ?= $(CUR_DIR)/../../../..
XFLIB_DIR := $(abspath $(XF_PROJ_ROOT))

XCLBIN_FILE :=
KERNELS :=

# -----------------------------------------------------------------------------

SRC_DIR = $(CUR_DIR)

EXE_NAME = 
HOST_ARGS =

SRCS = $(XFLIB_DIR)/L3/src/sw/api.cpp

CXXFLAGS += -g -I$(XILINX_XRT)/include -I $(XFLIB_DIR)/L3/include/sw/xf_blas

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
OBJ_FILES = $(CUR_DIR)/$(OBJ_DIR_BASE)/api.o
EXTRA_OBJS = 

CXX := /usr/bin/gcc
CC := /urs/bin/gcc

CXXFLAGS += -O3 -std=c++11 -fPIC -fopenmp -Wextra -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-result
CFLAGS +=
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -lxilinxopencl
LDFLAGS += -L$(XILINX_XRT)/lib -lz -lstdc++ -lrt -pthread -lxrt_core -ldl -luuid

EXE_EXT ?= exe
EXE_FILE =

API_OUT = xfblas.so

.PHONY: host

host: api

api: check_xrt $(API_OUT)

$(OBJ_FILES):  $(SRCS)
	mkdir -p $(OBJ_DIR_BASE)
	$(CXX) -shared -c -o $@ $< $(CXXFLAGS)
	
$(API_OUT): $(OBJ_FILES)
	mkdir -p lib
	$(CXX) -shared -o lib/$@ $(OBJ_FILES) $(LDFLAGS)
	chmod a+rx lib


# -----------------------------------------------------------------------------
#                                clean up

clean:
ifneq (,$(OBJ_DIR_BASE))
	rm -rf $(CUR_DIR)/$(OBJ_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*
endif
	rm -rf lib

cleanall: clean 

.PHONY: run 

run: api 

check: run


