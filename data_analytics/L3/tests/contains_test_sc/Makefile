# Copyright 2019-2022 Xilinx, Inc.
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
# sc makefile-generator v1.0.0

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform> HOST_ARCH=<x86>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform> HOST_ARCH=<x86>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make host HOST_ARCH=<hw/hw_emu/sw_emu/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

############################## Setting up Project Variables ##############################

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L3/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

# setting devault value
TARGET ?= sw_emu
HOST_ARCH ?= x86
PLATFORM_NAME ?= xilinx_u200_gen3x16_xdma_1_202110_1

ARROW_PATH := $(CUR_DIR)/ext/arrow
PYTHON_PATH := $(XILINX_VIVADO)/tps/lnx64/python-3.8.3
NUMPY_PATH := $(PYTHON_PATH)/lib/python3.8/site-packages/numpy/core/include
PY_CMD := $(PYTHON_PATH)/bin/python3.8

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_u200_gen3x16_xdma_2_202110_1
endif

# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  u200 u200
PLATFORM_BLOCKLIST +=  zc

GCC_INTOOL := 8.3.0
BINUTILS_INTOOL := 2.37
include ./utils.mk

TEMP_DIR := _x_temp.$(TARGET).$(PLATFORM_NAME)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(PLATFORM_NAME)
BUILD_DIR := build_dir.$(TARGET).$(PLATFORM_NAME)
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(PLATFORM_NAME)
EMCONFIG := $(BUILD_DIR)/emconfig.json
XCLBIN_DIR := $(CUR_DIR)/$(BUILD_DIR)
export XCL_BINDIR = $(XCLBIN_DIR)

BINARY_CONTAINERS_DEPS :=
RUN_DEPS :=

# set debug switch
ifneq ($(debug),yes)
#CXXFLAGS += -O3
endif

# get global setting
ifdef XILINX_SC_PFM_CONFIG
CXXFLAGS += -DXILINX_SC_PFM_CONFIG=$(XILINX_SC_PFM_CONFIG)
endif
ifdef XILINX_SC_PFM_EXT
CXXFLAGS += -DXILINX_SC_PFM_EXT=$(XILINX_SC_PFM_EXT)
endif
ifeq ($(HOST_ARCH), x86)
CXXFLAGS += -I $(XILINX_VITIS)/system_compiler/include -I $(XILINX_HLS)/include 
LDFLAGS += -L$(XILINX_XRT)/lib -L$(XILINX_VITIS)/system_compiler/lib/x86 -lvpp_acc -l$(LIB_XRT) -lxrt_coreutil  -Wl,-rpath=$(XILINX_VITIS)/system_compiler/lib/x86:$(XILINX_XRT)/lib:$(GCC_HOME)/lib64  -Wl,--enable-new-dtags -lpthread 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --temp_dir $(TEMP_DIR) --save-temps -g -I $(XILINX_VITIS)/system_compiler/include 
VPP_LDFLAGS += 
else ifeq ($(HOST_ARCH), aarch64)
CXXFLAGS += 
LDFLAGS += 
VPP_FLAGS += 
VPP_LDFLAGS += 
endif
CXXFLAGS += $(EXTRA_CXXFLAGS)
VPP_FLAGS += $(EXTRA_VPP_FLAGS)

ifeq ($(TARGET),sw)
  $(error Error: The sw target is not supported anymore. Please use sw_emu instead)
else ifeq ($(TARGET),sw_emu)
  LIB_XRT  := xrt_swemu
  HOST_PREAMBLE := XCL_EMULATION_MODE=sw_emu
else ifeq ($(TARGET),hw_emu)
  LIB_XRT  := xrt_hwemu
  HOST_PREAMBLE := XCL_EMULATION_MODE=hw_emu
  ifneq (,$(findstring -g,$(EXTRA_VPPFLAGS) $(CXXFLAGS)))
    # for sourcing pre/post xsim scripts
    ifneq ($(XILINX_SC_HW_EMU),0)
      HOST_PREAMBLE += XILINX_SC_HW_EMU=1 XILINX_SC_BUILD_DIR=$(PWD)/$(BUILD_DIR)
    endif
  endif
else ifeq ($(TARGET),hw)
  LIB_XRT  := xrt_core
endif

########################## Setting up Host Variables ##########################

#Inclue Required Host Source Files
SO_SRCS += $(XFLIB_DIR)/L3/src/sw/geospatial/strtree_contains.cpp
SO_SRCS += $(BUILD_DIR)/contains_wrap.cpp  

CXXFLAGS +=  -I $(XFLIB_DIR)/L2/include/hw -I $(CUR_DIR)/host -I $(CUR_DIR)/kernel -I $(XFLIB_DIR)/../utils/L1/include -I $(XFLIB_DIR)/../database/L1/include/hw -I $(XFLIB_DIR)/L1/include/sw -I $(XFLIB_DIR)/L3/include/sw -I $(XFLIB_DIR)/../utils/L1/include
CXXFLAGS += -I$(XFLIB_DIR)/L3/include/sw/xf_data_analytics/geospatial -D_STRTree_Kernel_
CXXFLAGS += -I$(ARROW_PATH)/cpp/src
CXXFLAGS += -I$(PYTHON_PATH)/include/python3.8 -I$(NUMPY_PATH)
PYFLAGS += -I$(XF_PROJ_ROOT)/L3/include/sw/xf_data_analytics/geospatial
CXXFLAGS += -O3 -std=c++11
LDFLAGS += -L$(ARROW_PATH)/cpp/release/release -larrow 
LDFLAGS += -fopenmp

LIBRARY_PATH = $(PYTHON_PATH)/lib:$(ARROW_PATH)/cpp/release/release:$(BUILD_DIR)
PYPATH = $(BUILD_DIR):$(XF_PROJ_ROOT)/L3/src/sw/geospatial:$(CUR_DIR)/build:$(PYTHONPATH)

SO_FILE := $(BUILD_DIR)/_containsModule.so

PY_ARGS := $(CUR_DIR)/data/points.csv $(CUR_DIR)/data/zones.txt $(CUR_DIR)/data/golden.txt
HOST_ARGS := -point $(CUR_DIR)/data/points.csv -polygon $(CUR_DIR)/data/zones.txt

MAKEDEPEND = $(CXX) $< -MM -MP -MF $(basename $@).d -MT $@  $(CXXFLAGS)

HOST_ARGS := -in $(CUR_DIR)/data/points10k.csv -p $(CUR_DIR)/data/golden_points_10k.txt -n $(CUR_DIR)/data/golden_nodes_10k.txt
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif

########################## Kernel compiler global settings ##########################
VPP_FLAGS +=  -I $(XFLIB_DIR)/L2/include/hw -I $(CUR_DIR)/kernel -I $(XFLIB_DIR)/L1/include/hw -I $(XFLIB_DIR)/L2/include/hw -I $(XFLIB_DIR)/../database/L1/include/hw 

######################### binary container global settings ##########################
VPP_FLAGS += --hls.clock 300000000:strtree_acc_STRTree_Kernel
ifneq ($(HOST_ARCH), x86)
VPP_LDFLAGS += --clock.defaultFreqHz 200000000
else
VPP_LDFLAGS += --kernel_frequency 200
endif

ifeq ($(HOST_ARCH), x86)
BINARY_CONTAINERS_TMP := $(BUILD_DIR)/$(TARGET).o
BINARY_CONTAINERS := $(BUILD_DIR)/$(TARGET).xclbin
ifeq ($(TARGET),sw_emu)
  BINARY_CONTAINERS_TMP :=
endif
else
# placeholder for non_x86
endif

.SECONDEXPANSION:
# ################ Setting Rules for Binary Containers (Building Kernels) ################
ACC_SRCS_STRTree_Kernel += $(CUR_DIR)/kernel/strtree_kernel.cpp
ACC_OBJS_STRTree_Kernel  := $(addprefix $(TEMP_DIR)/, $(addsuffix .o,$(basename $(ACC_SRCS_STRTree_Kernel))))
$(ACC_OBJS_STRTree_Kernel): $(TEMP_DIR)/%.o : %.cpp $$(@D)/.f
	@echo "--> Making $@ from: $?"
	$(MAKEDEPEND)
	$(VPP) $(VPP_FLAGS) $(VPP_FLAGS_STRTree_Kernel) -o $@  -c $<
BINARY_CONTAINERS_DEPS  += $(ACC_OBJS_STRTree_Kernel) 
$(BINARY_CONTAINERS_TMP) : $(BINARY_CONTAINERS_DEPS)
	@echo "--> Making $@ from: $?"
	$(VPP) $(VPP_FLAGS) $(VPP_LDFLAGS) -o $(BINARY_CONTAINERS) -l $^
SO_FILE_DEPS += $(BINARY_CONTAINERS_TMP)
SO_FILE_DEPS += $(BINARY_CONTAINERS_DEPS)

############################## Setting Rules for Host (Building Host Executable) ##############################
.PHONY: run pre_build

CXX_PATH := $(patsubst %/,%,$(dir $(CXX)))

pre_build:
	mkdir -p $(CUR_DIR)/ext
	make -C $(XF_PROJ_ROOT)/ext/swig DEST_DIR=$(CUR_DIR)/ext
	PATH=$(XILINX_VITIS)/tps/lnx64/cmake-3.21.4/bin:${PATH} && make -C $(XF_PROJ_ROOT)/ext/arrow DEST_DIR=$(CUR_DIR)/ext COMPILER_PATH=$(CXX_PATH)

NUMPY_SRC := $(CUR_DIR)/host/numpy.i

$(NUMPY_SRC): pre_build
	wget https://raw.githubusercontent.com/numpy/numpy/maintenance/1.19.x/tools/swig/numpy.i -P $(CUR_DIR)/host

$(SO_FILE): $(NUMPY_SRC) $(SO_FILE_DEPS)
	$(MAKEDEPEND)
	mkdir -p $(BUILD_DIR)
	$(CUR_DIR)/ext/swig/install/bin/swig -c++ -python $(PYFLAGS) -o $(BUILD_DIR)/contains_wrap.cpp $(CUR_DIR)/host/contains.i
	$(CXX) -fPIC -c $(SO_SRCS) $(CXXFLAGS) $(LDFLAGS)
	$(CXX) -o $(SO_FILE) -shared contains_wrap.o strtree_contains.o $(SO_FILE_DEPS) $(CXXFLAGS) $(LDFLAGS)

ifeq ($(HOST_ARCH), x86)
$(TEMP_DIR)/%.o : %.cpp $$(@D)/.f
	@echo "--> Making $@ from: $?"
	mkdir -p $(BUILD_DIR)
	$(MAKEDEPEND)
	$(CXX) -o $@ $(CXXFLAGS)  -I . -c $<
else
# place holder for arch64
endif

$(EMCONFIG):
	emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)

%/.f:
	mkdir -p $(dir $@)
	touch $@

.PRECIOUS: %/.f

RUN_DEPS += $(SO_FILE) $(EMCONFIG)

run: check_device  $(RUN_DEPS)
#sw_emu
ifneq (,$(filter sw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH PYTHONPATH=$(PYPATH) \
	$(HOST_PREAMBLE) $(PY_CMD) $(CUR_DIR)/host/demo_start.py $(PY_ARGS)
	
else
# place holder for arch64
endif
endif

#hw_emu
ifneq (,$(filter hw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH PYTHONPATH=$(PYPATH) \
	$(HOST_PREAMBLE) $(PY_CMD) $(CUR_DIR)/host/demo_start.py $(PY_ARGS)
	
else
# place holder for arch64
endif
endif

#hw
ifeq ($(TARGET), hw)
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH PYTHONPATH=$(PYPATH) \
	$(HOST_PREAMBLE) $(PY_CMD) $(CUR_DIR)/host/demo_start.py $(PY_ARGS)
	
else
# place holder for arch64
endif
endif

############################## Setting Targets ##############################

.PHONY: all clean cleanall emconfig
emconfig: $(EMCONFIG)
ifeq ($(HOST_ARCH), x86)
all:  check_vpp check_platform check_xrt $(SO_FILE) $(BINARY_CONTAINERS) emconfig
else
all:  check_vpp check_platform check_sysroot $(SO_FILE) $(BINARY_CONTAINERS) emconfig sd_card
endif

.PHONY: host xclbin
ifeq ($(HOST_ARCH), x86)
host:  check_xrt $(SO_FILE)
else
host:  check_sysroot $(SO_FILE)
endif
xclbin: $(BINARY_CONTAINERS_TMP)

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(SO_FILE) vitis_* TempConfig system_estimate.xtxt *.rpt .run/ *.o
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.*/_x.* _x_temp.*/.Xil _x_temp.*/profile_summary.* xo_* _x*
	-$(RMDIR) _x_temp.*/dltmp* _x_temp.*/kernel_info.dat _x_temp.*/*.log
	-$(RMDIR) _x_temp.* 

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  build_dir.* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR) $(XFLIB_DIR)/common/data/*.xe2xd* $(XFLIB_DIR)/common/data/*.orig*

clean: cleanh
