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

# ------------------------------------------------------------
#						Help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Device."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

# ------------------------------------------------------------
#						Build Environment Setup

include ./utils.mk
include ./config.mk

TOOL_VERSION ?= 2019.2

#check environment setup
ifndef XILINX_VITIS
  XILINX_VITIS = /opt/xilinx/Vitis/$(TOOL_VERSION)
  export XILINX_VITIS
endif
ifndef XILINX_VIVADO
  XILINX_VIVADO = /opt/xilinx/Vivado/$(TOOL_VERSION)
  export XILINX_VIVADO
endif
ifndef XILINX_XRT
  XILINX_XRT = /opt/xilinx/xrt
  export XILINX_XRT
endif

.PHONY: check_vivado
check_vivado:
ifeq (,$(wildcard $(XILINX_VIVADO)/bin/vivado))
	@echo "Cannot locate Vivado installation. Please set XILINX_VIVADO variable." && false
endif

.PHONY: check_vpp
check_vpp:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/v++))
	@echo "Cannot locate Vitis installation. Please set XILINX_VITIS variable." && false
endif

.PHONY: check_xrt
check_xrt:
ifeq (,$(wildcard $(XILINX_XRT)/lib/libxilinxopencl.so))
	@echo "Cannot locate XRT installation. Please set XILINX_XRT variable." && false
endif

export PATH := $(XILINX_VITIS)/bin:$(XILINX_XRT)/bin:$(PATH)

ifeq (,$(LD_LIBRARY_PATH))
  LD_LIBRARY_PATH := $(XILINX_XRT)/lib
else
  LD_LIBRARY_PATH := $(XILINX_XRT)/lib:$(LD_LIBRARY_PATH)
endif
ifneq (,$(wildcard $(XILINX_VITIS)/bin/ldlibpath.sh))
  export LD_LIBRARY_PATH := $(shell $(XILINX_VITIS)/bin/ldlibpath.sh $(XILINX_VITIS)/lib/lnx64.o):$(LD_LIBRARY_PATH)
endif

# sw_emu, hw_emu, hw
TARGET ?= sw_emu
ifeq ($(filter $(TARGET),sw_emu hw_emu hw),)
  $(error TARGET is not sw_emu, hw_emu or hw)
endif

# Target device
DEVICE ?= xilinx_u200_xdma_201830_2

ifneq (,$(wildcard $(DEVICE)))
  # Use DEVICE as a file path
  XPLATFORM := $(DEVICE)
else
  # Use DEVICE as a file name pattern
  DEVICE_L := $(shell echo $(DEVICE) | tr A-Z a-z)
  # Match the name
ifneq (,$(PLATFORM_REPO_PATHS))
  XPLATFORMS := $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/*/*.xpfm))
  XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
ifeq (,$(XPLATFORM))
  XPLATFORMS := $(wildcard $(XILINX_VITIS)/platforms/*/*.xpfm)
  XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
ifeq (,$(XPLATFORM))
  XPLATFORMS := $(wildcard /opt/xilinx/platforms/*/*.xpfm)
  XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
endif

define MSG_PLATFORM
No platform matched pattern '$(DEVICE)'.
Available platforms are: $(XPLATFORMS)
To add more platform directories, set the PLATFORM_REPO_PATHS variable.
endef
export MSG_PLATFORM

define MSG_DEVICE
More than one platform matched: $(XPLATFORM)
Please set DEVICE variable more accurately to select only one platform file. For example: DEVICE='u200.*xdma'
endef
export MSG_DEVICE

.PHONY: check_platform
check_platform:
ifeq (,$(XPLATFORM))
	@echo "$${MSG_PLATFORM}" && false
endif
ifneq (,$(word 2,$(XPLATFORM)))
	@echo "$${MSG_DEVICE}" && false
endif

XDEVICE := $(basename $(notdir $(firstword $(XPLATFORM))))

# ------------------------------------------------------------
#						Directory Setup

XF_PROJ_ROOT ?= $(CUR_DIR)/../../..
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))

XFLIB_DIR := $(shell readlink -f $(XF_PROJ_ROOT))

BUILD_DIR := $(CUR_DIR)/build
TEMP_DIR := $(CUR_DIR)/_x_temp.$(TARGET).$(XDEVICE)
SRC_DIR := $(XFLIB_DIR)/L2/demos/lz4/
TB_DIR := $(XFLIB_DIR)/L2/tests/src/

# ------------------------------------------------------------
#                      kernel setup

KSRC_DIR = $(XFLIB_DIR)/L2/src/

VPP = $(XILINX_VITIS)/bin/v++

# HLS src files
HLS_SRC_DIR = $(XFLIB_DIR)/L1/include/hw

# Compilation flags
VPP_FLAGS = -I$(HLS_SRC_DIR) \
			 -I$(KSRC_DIR) \
			 -I$(XFLIB_DIR)/L2/include/
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps
VPP_FLAGS += -DPARALLEL_BLOCK=$(PARALLEL_BLOCK)


VPP_DIRS = --temp_dir $(TEMP_DIR)/_x.$(TARGET) \
			--report_dir $(CUR_DIR)/reports/_x.$(TARGET)

# Linking flags
VPP_LINK_FLAGS = --optimize 2 --jobs 8 \

VPP_LINK_DIRS = --temp_dir $(TEMP_DIR)/_build.$(TARGET)\
				 --report_dir $(CUR_DIR)/reports/_build.$(TARGET)/

XCLBIN_FILE = $(BUILD_DIR)/xclbin_$(XDEVICE)_$(TARGET)/compress_decompress.xclbin

XO_FILES = $(TEMP_DIR)/xf_compress.xo \
		   $(TEMP_DIR)/xf_decompress.xo

COMPRESS_KERNEL_SRCS = $(KSRC_DIR)/lz4_compress_mm.cpp
DECOMPRESS_KERNEL_SRCS = $(KSRC_DIR)/lz4_decompress_mm.cpp

COMPRESS_KERNEL_NAME = xilLz4Compress
DECOMPRESS_KERNEL_NAME = xilLz4Decompress

KERNELS = $(COMPRESS_KERNEL_NAME)
KERNELS += $(DECOMPRESS_KERNEL_NAME)

# ------------------------------------------------------------
#                      kernel rules

# Building kernel
$(TEMP_DIR)/xf_compress.xo: $(COMPRESS_KERNEL_SRCS) $(HLS_SRC_DIR)
	@echo -e "----\nCompiling compression kernel $*..."
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) $(VPP_DIRS) -c -k $(COMPRESS_KERNEL_NAME) -I'$(<D)' -o'$@' '$<'


$(TEMP_DIR)/xf_decompress.xo: $(DECOMPRESS_KERNEL_SRCS) $(HLS_SRC_DIR)
	@echo -e "----\nCompiling decompression kernel $*..."
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) $(VPP_DIRS) -c -k $(DECOMPRESS_KERNEL_NAME) -I'$(<D)' -o'$@' '$<'


# xclbin Binary creation
$(XCLBIN_FILE): $(TEMP_DIR)/xf_compress.xo $(TEMP_DIR)/xf_decompress.xo
	mkdir -p $(BUILD_DIR)
	$(VPP) $(VPP_FLAGS) $(VPP_LINK_FLAGS) $(VPP_LINK_DIRS) -l \
	-o'$@' $(+)

# ------------------------------------------------------------
#                       host setup

CXX := xcpp
HOST_EXE := xil_lz4
#EXE_EXT = exe

CXXFLAGS +=-I$(CUR_DIR)/src/
CXXFLAGS +=-I$(TB_DIR)/
CXXFLAGS +=-I$(XILINX_XRT)/include/

CXXFLAGS +=-I$(XFLIB_DIR)/common/libs/xcl2/
CXXFLAGS +=-I$(XFLIB_DIR)/common/libs/cmdparser/
CXXFLAGS +=-I$(XFLIB_DIR)/common/libs/logger/
CXXFLAGS +=-I$(XFLIB_DIR)/common/thirdParty/xxhash/

#Host and Common sources
SRCS += $(SRC_DIR)/src/host.cpp
SRCS += $(TB_DIR)/lz4.cpp
SRCS += $(XFLIB_DIR)/common/libs/xcl2/xcl2.cpp
SRCS += $(XFLIB_DIR)/common/libs/cmdparser/cmdlineparser.cpp
SRCS += $(XFLIB_DIR)/common/libs/logger/logger.cpp
SRCS += $(XFLIB_DIR)/common/thirdParty/xxhash/xxhash.c

CXXFLAGS += -fmessage-length=0
		-DXDEVICE=$(XDEVICE) \
	    -Wall -Wno-unknown-pragmas -Wno-unused-label -pthread

LDFLAGS += -L$(XILINX_XRT)/lib/ -lOpenCL -pthread
LDFLAGS += -lrt -Wno-unused-label -Wno-narrowing -std=c++0x -DVERBOSE

EXE_FILE = $(BUILD_DIR)/$(HOST_EXE)


# ------------------------------------------------------------
#                       host rules

$(EXE_FILE): $(SRCS) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling host $(notdir $@)..."
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)


# ------------------------------------------------------------
#                      build rules

.PHONY: all help host xclbin cleanh cleank cleanall clean

all: host xclbin

host: $(EXE_FILE) | check_vpp check_xrt check_platform

xclbin: $(XCLBIN_FILE)

cleank:
	rm -f _x_temp*/*.xo
	rm -f $(BUILD_DIR)/*.xclbin
	rm -rf _x_temp*/_x.* _x_temp*/.Xil _x_temp*/profile_summary.* sample_*
	rm -rf _x_temp*/dltmp* _x_temp*/kernel_info.dat _x_temp*/*.log
	
cleanh:
	rm -rf $(EXE_FILE)
	-$(RMDIR) $(EXE_FILE)
	-$(RMDIR) vitis_* TempConfig system_estimate.xtxt *.rpt .run/
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

cleanall: cleanh cleank
	rm -rf $(BUILD_DIR)
	-$(RMDIR) _x_temp* $(CUR_DIR)/reports
	-$(RMDIR) $(XFLIB_DIR)/common/data/*.xe2xd* $(XFLIB_DIR)/common/data/*.orig* 

clean: cleanh


# ------------------------------------------------------------
#                      simulation run

$(BUILD_DIR)/emconfig.json :
		emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)

HOST_ARGS = -sx $(XCLBIN_FILE) -v $(XFLIB_DIR)/common/data/sample.txt 


ifeq ($(TARGET),sw_emu)
  RUN_ENV = export XCL_EMULATION_MODE=sw_emu
  EMU_CONFIG = $(BUILD_DIR)/emconfig.json
else ifeq ($(TARGET),hw_emu)
  RUN_ENV = export XCL_EMULATION_MODE=hw_emu
  EMU_CONFIG = $(BUILD_DIR)/emconfig.json
else ifeq ($(TARGET),hw)
  RUN_ENV = echo "TARGET=hw"
  EMU_CONFIG =
endif


run: host xclbin $(EMU_CONFIG)
	$(RUN_ENV); \
	$(EXE_FILE) $(HOST_ARGS)

check: run

.PHONY: build
build: xclbin host
