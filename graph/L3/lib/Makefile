#
# Copyright 2020 Xilinx, Inc.
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
############################### Setting up Project Variables ##############################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L3/lib/*}')
XFLIB_DIR = $(XF_PROJ_ROOT)

CUR_DIR ?= $(XF_PROJ_ROOT)/L3/lib

HOST_ARCH := x86
RMDIR = rm -rf

ifndef XILINX_XRT
  XILINX_XRT = /opt/xilinx/xrt
  export XILINX_XRT
endif

#Checks for g++
GCC_INTOOL := 8.3.0
CXX := g++
CXX_REQ := $(shell echo $(GCC_INTOOL) | cut -f 1 -d ".")
ifeq ($(HOST_ARCH), x86)
ifneq ($(shell expr $(shell echo "__GNUG__" | g++ -E -x c++ - | tail -1) \>= $(CXX_REQ)), 1)
ifndef XILINX_VIVADO
$(error [ERROR]: g++ version too old. Please use $(CXX_REQ) or above)
else
CXX := $(XILINX_VIVADO)/tps/lnx64/gcc-$(GCC_INTOOL)/bin/g++
ifeq ($(LD_LIBRARY_PATH),)
export LD_LIBRARY_PATH := $(XILINX_VIVADO)/tps/lnx64/gcc-$(GCC_INTOOL)/lib64
else
export LD_LIBRARY_PATH := $(XILINX_VIVADO)/tps/lnx64/gcc-$(GCC_INTOOL)/lib64:$(LD_LIBRARY_PATH)
endif
$(warning [WARNING]: g++ version too old. Using g++ provided by the tool: $(CXX))
endif
endif
else ifeq ($(HOST_ARCH), aarch64)
CXX := $(XILINX_VITIS)/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-g++
else ifeq ($(HOST_ARCH), aarch32)
CXX := $(XILINX_VITIS)/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-g++
endif

############################## Setting up Host Variables ##############################
CXXFLAGS += -I$(XFLIB_DIR)/L2/include
CXXFLAGS += -I$(XFLIB_DIR)/ext/xcl2
CXXFLAGS += -I$(XFLIB_DIR)/L3/include
CXXFLAGS += -I$(XFLIB_DIR)/L3/include/graphPartition/grappolo/include
CXXFLAGS += -I$(XFLIB_DIR)/../utils/L1/include

CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include -I$(XILINX_XRM)/include -std=c++17 -Wall -Wno-unknown-pragmas -Wno-unused-label
LDFLAGS += -L$(XILINX_XRT)/lib -lOpenCL -pthread -lrt -Wno-unused-label -Wno-narrowing -DVERBOSE -L$(XILINX_XRM)/lib -lxrm -fopenmp
CXXFLAGS += -fmessage-length=0 -DNDEBUG  #-D_GLIBCXX_USE_CXX11_ABI=0
CXXFLAGS +=-I$(CUR_DIR)/src/ 
#CXXFLAGS += -g -O0
#CXXFLAGS += -OFAST -O3

ifneq ($(HOST_ARCH), x86)
	LDFLAGS += --sysroot=$(SYSROOT)
endif

CXXFLAGS += -D USE_HBM

XRM_CXXFLAGS += -I$(XILINX_XRM)/include
XRM_LDFLAGS +=

############################## Setting Rules for so (Building Dynamic Library) ##############################

#libxrm = $(XFLIB_DIR)/L3/lib/libxrm.so
libxrm = $(CUR_DIR)/libxrm.so.1
libxrm : $(libxrm)

# ##################### Setting up default value of TARGET ##########################
TARGET ?= hw_emu

ifeq ($(TARGET),hw_emu)
CXXFLAGS += -D HW_EMU_TEST
LIBXRM = $(libxrm)
check:
	@echo "EMULATION_MODE: $(TARGET)" > $(CUR_DIR)/libgraphL3.json
endif

ifeq ($(TARGET),hw)
check:
	@echo "TEST_MODE: $(TARGET)" > $(CUR_DIR)/libgraphL3.json
endif

libGRAPHL3 = $(CUR_DIR)/libgraphL3.so
libgraphL3 : $(libGRAPHL3) check

#for graph partition API
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/louvainPartition/ParLV.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/louvainPartition/partitionLouvain.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/louvainPartition/xilinxlouvain.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/RngStream.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/parseInputFiles.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/parallelLouvainMethod.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/utilityClusteringFunctions.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/parseInputParameters.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/coloringDistanceOne.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/vertexFollowing.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/utilityFunctions.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/driverForGraphClustering.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/buildNextPhase.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/louvainMultiPhaseRun.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/graphPartition/grappolo/src/parallelLouvainWithColoring.cpp

#for graph API
CPP_PATH += $(XFLIB_DIR)/L3/src/common.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/louvainRun.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_louvainmodularity.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_twohop.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_sp.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_trianglecount.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_labelpropagation.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_bfs.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_wcc.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_scc.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_convertcsrcsc.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_similaritysparse.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_similaritydense.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/op_pagerank.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/xf_graph_L3_handle.cpp
CPP_PATH += $(XFLIB_DIR)/L3/src/xf_graph_L3.cpp
CPP_PATH += $(XFLIB_DIR)/ext/xcl2/xcl2.cpp

XRMEMU_PATH += $(XFLIB_DIR)/L3/src/xrm_emu.cpp

$(libGRAPHL3): ${CPP_PATH} $(LIBXRM)
	echo "target: $(TARGET)"
	echo "libGRAPHL3: $(libGRAPHL3)"
	mkdir -p $(CUR_DIR)
	$(CXX) -fPIC $(CXXFLAGS) -rdynamic -shared -o $@ $^ $(LDFLAGS) -w -Wl,--export-dynamic
$(libxrm): ${XRMEMU_PATH}
	mkdir -p $(CUR_DIR)
	$(CXX) -fPIC $(XRM_CXXFLAGS) -rdynamic -shared -o $@ $^ $(XRM_LDFLAGS) -w -Wl,--export-dynamic
	ln -s $(libxrm) $(CUR_DIR)/libxrm.so

############################## Cleaning Rules ##############################
clean:
	-$(RMDIR) ${CUR_DIR}/libgraphL3.so ${CUR_DIR}/libgraphL3.json
