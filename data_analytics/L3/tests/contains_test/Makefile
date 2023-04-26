#
# Copyright 2022 Xilinx, Inc.
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

# ####################################### Help Section #####################################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO) "  make run"
	$(ECHO) "      Command to run application."
	$(ECHO) ""

# ######################### Include environment variables in utils.mk ####################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L3/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

BUILD_DIR := $(CUR_DIR)/build

# ###################### Setting up default value of HOST_ARCH ####################### 
TARGET ?= sw_emu
HOST_ARCH ?= x86

ARROW_PATH := $(CUR_DIR)/ext/arrow
PYTHON_PATH := $(XILINX_VIVADO)/tps/lnx64/python-3.8.3
NUMPY_PATH := $(PYTHON_PATH)/lib/python3.8/site-packages/numpy/core/include
PY_CMD := $(PYTHON_PATH)/bin/python3.8

# ######################### Setting up Host Variables #########################
#Include Required Host Source Files
SO_SRCS += $(XF_PROJ_ROOT)/L3/src/sw/geospatial/strtree_contains.cpp
SO_SRCS += $(BUILD_DIR)/contains_wrap.cpp

GCC_INTOOL := 8.3.0
BINUTILS_INTOOL := 2.37
include ./utils.mk

CXXFLAGS += -I$(CUR_DIR)/host
CXXFLAGS += -I$(XF_PROJ_ROOT)/L3/include/sw/xf_data_analytics/geospatial

PYFLAGS += -I$(XF_PROJ_ROOT)/L3/include/sw/xf_data_analytics/geospatial

CXXFLAGS += -I$(ARROW_PATH)/cpp/src
CXXFLAGS += -I$(PYTHON_PATH)/include/python3.8 -I$(NUMPY_PATH)
CXXFLAGS += -I$(XILINX_HLS)/include -std=c++11 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label

LDFLAGS += -L$(ARROW_PATH)/cpp/release/release -larrow 
LDFLAGS += -fopenmp

LIBRARY_PATH = $(PYTHON_PATH)/lib:$(ARROW_PATH)/cpp/release/release:$(BUILD_DIR)
PYPATH = $(BUILD_DIR):$(XF_PROJ_ROOT)/L3/src/sw/geospatial:$(CUR_DIR)/build:$(PYTHONPATH)

SO_FILE := $(BUILD_DIR)/_containsModule.so
EXE_NAME := $(SO_FILE)
all: $(EXE_NAME)

PY_ARGS := $(CUR_DIR)/data/points.csv $(CUR_DIR)/data/zones.txt $(CUR_DIR)/data/golden.txt
HOST_ARGS := -point $(CUR_DIR)/data/points.csv -polygon $(CUR_DIR)/data/zones.txt

# Dummy Vitis settings to pass Makefile checks
VPP_FLAGS ?=
VPP_LDFLAGS ?=

# ######################### Setting Targets of Makefile ################################
.PHONY: run pre_build all

CXX_PATH := $(patsubst %/,%,$(dir $(shell which $(CXX))))
CMAKE_PATHS := $(sort $(wildcard $(XILINX_VITIS)/tps/lnx64/cmake-3.2*.*))
CMAKE_PATH := $(word $(words $(CMAKE_PATHS)),$(CMAKE_PATHS))

pre_build:
	mkdir -p $(CUR_DIR)/ext
	make -C $(XF_PROJ_ROOT)/ext/swig DEST_DIR=$(CUR_DIR)/ext
	PATH=$(CMAKE_PATH)/bin:${PATH} && make -C $(XF_PROJ_ROOT)/ext/arrow DEST_DIR=$(CUR_DIR)/ext COMPILER_PATH=$(CXX_PATH)

NUMPY_SRC := $(CUR_DIR)/host/numpy.i

$(NUMPY_SRC):
	wget https://raw.githubusercontent.com/numpy/numpy/maintenance/1.19.x/tools/swig/numpy.i -P $(CUR_DIR)/host

$(SO_FILE): pre_build $(NUMPY_SRC) 
	mkdir -p $(BUILD_DIR)
	$(CUR_DIR)/ext/swig/install/bin/swig -c++ -python $(PYFLAGS) -o $(BUILD_DIR)/contains_wrap.cpp $(CUR_DIR)/host/contains.i
	$(CXX) -fPIC -c $(SO_SRCS) $(CXXFLAGS) $(LDFLAGS)
	$(CXX) -shared strtree_contains.o contains_wrap.o -o $(SO_FILE) $(CXXFLAGS) $(LDFLAGS)

run: $(SO_FILE)
	PYTHONPATH=$(PYPATH) LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH $(PY_CMD) $(CUR_DIR)/host/demo_start.py $(PY_ARGS)

clean:
	rm -rf $(CUR_DIR)/build strtree_contains.o contains_wrap.o 

cleanall: clean
	rm -rf $(NUMPY_SRC) ext 

