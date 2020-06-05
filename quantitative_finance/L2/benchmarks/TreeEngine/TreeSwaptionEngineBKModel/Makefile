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
############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO) "  make sd_card TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86>"
	$(ECHO) "      Command to prepare sd_card files."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make build TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make host DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  NOTE: For SoC shells, ENV variable SYSROOT needs to be set."
	$(ECHO) ""

############################## Setting up Project Variables ##############################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L2/benchmarks/TreeEngine/TreeSwaptionEngineBKModel/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

TARGET ?= sw_emu
HOST_ARCH := x86
SYSROOT := ${SYSROOT}
DEVICE ?= xilinx_u200_xdma_201830_2

include ./utils.mk

XDEVICE := $(call device2xsa, $(DEVICE))
TEMP_DIR := _x_temp.$(TARGET).$(XDEVICE)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(XDEVICE)
BUILD_DIR := build_dir.$(TARGET).$(XDEVICE)
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(XDEVICE)
EMCONFIG_DIR := $(BUILD_DIR)

# Setting tools
VPP := v++

############################## Setting up Host Variables ##############################
#Include Required Host Source Files
HOST_SRCS += $(XFLIB_DIR)/L2/benchmarks/TreeEngine/TreeSwaptionEngineBKModel/host/main.cpp
HOST_SRCS += $(XFLIB_DIR)/ext/xcl2/xcl2.cpp


CXXFLAGS += -DPRAGMA
CXXFLAGS += -I$(XFLIB_DIR)/L1/include
CXXFLAGS += -I$(XFLIB_DIR)/L2/include
CXXFLAGS += -I$(XFLIB_DIR)/L2/benchmarks/TreeEngine/TreeSwaptionEngineBKModel/host
CXXFLAGS += -I$(XFLIB_DIR)/L2/benchmarks/TreeEngine/TreeSwaptionEngineBKModel/kernel
CXXFLAGS += -I$(XFLIB_DIR)/ext/xcl2



CXXFLAGS += -I$(XFLIB_DIR)/ext/xcl2
CXXFLAGS += -I$(XFLIB_DIR)/ext/quantlib
CXXFLAGS += -I$(XFLIB_DIR)/ext/rng
CXXFLAGS += -I$(XFLIB_DIR)/L3/include
CXXFLAGS += -I$(XFLIB_DIR)/L3/include/models

ifeq ($(TARGET),sw_emu)
CXXFLAGS += -D SW_EMU_TEST
endif

# Host compiler global settings
CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -L$(XILINX_XRT)/lib -lOpenCL -lpthread -lrt -Wno-unused-label -Wno-narrowing -DVERBOSE 
LDFLAGS += -L$(XILINX_VIVADO)/lnx64/tools/fpo_v7_0 -Wl,--as-needed -lgmp -lmpfr -lIp_floating_point_v7_0_bitacc_cmodel
CXXFLAGS += -fmessage-length=0 -O3 
CXXFLAGS +=-I$(CUR_DIR)/src/ 


EXE_NAME := host.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
HOST_ARGS :=  -xclbin $(BUILD_DIR)/scanTreeKernel.xclbin

ifneq ($(HOST_ARCH), x86)
	LDFLAGS += --sysroot=$(SYSROOT)
endif

############################## Setting up Kernel Variables ##############################
# Kernel compiler global settings
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps
LDCLFLAGS += --optimize 2 --jobs 8
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
CXXFLAGS += -D USE_DDR -D KN=4
VPP_FLAGS += --config $(CUR_DIR)/conn_u250.ini
else ifneq (,$(XPLATFORM))
$(warning WARNING: Unsupported platform $(XPLATFORM))
endif

ifneq (,$(shell echo $(XPLATFORM) | awk '/u200/'))
CXXFLAGS += -D USE_DDR -D KN=2
VPP_FLAGS += --config $(CUR_DIR)/conn_u200.ini
else ifneq (,$(XPLATFORM))
$(warning WARNING: Unsupported platform $(XPLATFORM))
endif

ifeq ($(TARGET),sw_emu)
VPP_CFLAGS += -D VIVADO_HLS_SIM
endif
ifeq ($(TARGET),hw_emu)
VPP_CFLAGS += -D VIVADO_HLS_SIM
endif
VPP_FLAGS +=-I$(XFLIB_DIR)/L1/include
VPP_FLAGS +=-I$(XFLIB_DIR)/L2/include

VPP_FLAGS += -I$(XFLIB_DIR)/L1/include
VPP_FLAGS += -I$(XFLIB_DIR)/L2/include
VPP_FLAGS += -I$(XFLIB_DIR)/L2/benchmarks/TreeEngine/TreeSwaptionEngineBKModel/kernel

scanTreeKernel1_VPP_FLAGS +=  -D DPRAGMA -D KERNEL_NAME=scanTreeKernel1
scanTreeKernel2_VPP_FLAGS +=  -D DPRAGMA -D KERNEL_NAME=scanTreeKernel2
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
scanTreeKernel3_VPP_FLAGS +=  -D DPRAGMA -D KERNEL_NAME=scanTreeKernel3
scanTreeKernel4_VPP_FLAGS +=  -D DPRAGMA -D KERNEL_NAME=scanTreeKernel4
endif
VPP_FLAGS += -DHW_EMU_DEBUG 

# Kernel linker flags
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
LDCLFLAGS_scanTreeKernel +=  --nk scanTreeKernel1:1:scanTreeKernel1 --nk scanTreeKernel2:1:scanTreeKernel2 --nk scanTreeKernel3:1:scanTreeKernel3 --nk scanTreeKernel4:1:scanTreeKernel4
else
LDCLFLAGS_scanTreeKernel +=  --nk scanTreeKernel1:1:scanTreeKernel1 --nk scanTreeKernel2:1:scanTreeKernel2
endif

############################## Declaring Binary Containers ##############################
BINARY_CONTAINERS += $(BUILD_DIR)/scanTreeKernel.xclbin
BINARY_CONTAINER_scanTreeKernel_OBJS += $(TEMP_DIR)/scanTreeKernel1.xo
BINARY_CONTAINER_scanTreeKernel_OBJS += $(TEMP_DIR)/scanTreeKernel2.xo
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
BINARY_CONTAINER_scanTreeKernel_OBJS += $(TEMP_DIR)/scanTreeKernel3.xo
BINARY_CONTAINER_scanTreeKernel_OBJS += $(TEMP_DIR)/scanTreeKernel4.xo
endif

############################## Setting Targets ##############################
CP = cp -rf

.PHONY: all clean cleanall docs emconfig
all: check_vpp check_platform | $(EXE_FILE) $(BINARY_CONTAINERS) emconfig 
.PHONY: host
host: $(EXE_FILE) | check_xrt

.PHONY: xclbin
xclbin: check_vpp | $(BINARY_CONTAINERS)

.PHONY: build
build: xclbin

############################## Setting Rules for Binary Containers (Building Kernels) ##############################
$(TEMP_DIR)/scanTreeKernel1.xo: kernel/scan_tree_kernel1.cpp kernel/scan_tree_kernel.cpp
	$(ECHO) "Compiling Kernel: scanTreeKernel1"
	mkdir -p $(TEMP_DIR)
	$(VPP) $(scanTreeKernel1_VPP_FLAGS) $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -c -k scanTreeKernel1 -I'$(<D)' -o $@ $^
$(TEMP_DIR)/scanTreeKernel2.xo: $(XFLIB_DIR)/L2/benchmarks/TreeEngine/TreeSwaptionEngineBKModel/kernel/scan_tree_kernel2.cpp
	$(ECHO) "Compiling Kernel: scanTreeKernel2"
	mkdir -p $(TEMP_DIR)
	$(VPP) $(scanTreeKernel2_VPP_FLAGS) $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -c -k scanTreeKernel2 -I'$(<D)' -o $@ $^
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
$(TEMP_DIR)/scanTreeKernel3.xo: kernel/scan_tree_kernel3.cpp kernel/scan_tree_kernel.cpp
	$(ECHO) "Compiling Kernel: scanTreeKernel3"
	mkdir -p $(TEMP_DIR)
	$(VPP) $(scanTreeKernel3_VPP_FLAGS) $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -c -k scanTreeKernel3 -I'$(<D)' -o $@ $^
$(TEMP_DIR)/scanTreeKernel4.xo: kernel/scan_tree_kernel4.cpp kernel/scan_tree_kernel.cpp
	$(ECHO) "Compiling Kernel: scanTreeKernel4"
	mkdir -p $(TEMP_DIR)
	$(VPP) $(scanTreeKernel4_VPP_FLAGS) $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -c -k scanTreeKernel4 -I'$(<D)' -o $@ $^
endif

$(BUILD_DIR)/scanTreeKernel.xclbin: $(BINARY_CONTAINER_scanTreeKernel_OBJS)
	mkdir -p $(BUILD_DIR)
	$(VPP) $(VPP_FLAGS) --temp_dir $(BUILD_DIR) --report_dir $(BUILD_REPORT_DIR)/scanTreeKernel -l $(LDCLFLAGS) $(LDCLFLAGS_scanTreeKernel) -o'$@' $(+)

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXE_FILE): $(HOST_SRCS) | check_xrt
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(XPLATFORM) --od $(EMCONFIG_DIR)

############################## Setting Essential Checks and Running Rules ##############################
run: all
ifneq ($(findstring u250, $(DEVICE)), u250)
ifneq ($(findstring u200, $(DEVICE)), u200)
$(error This example is not supported for $(DEVICE))
endif
endif

ifeq ($(findstring zc, $(DEVICE)), zc)
$(error This example is not supported for $(DEVICE))
endif

ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS)
else
	$(EXE_FILE) $(HOST_ARGS)
endif

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE) vitis_* TempConfig system_estimate.xtxt *.rpt .run/
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl* start_simulation.sh *.xclbin
	-$(RMDIR) _x_temp.*/_x.* _x_temp.*/.Xil _x_temp.*/profile_summary.* 
	-$(RMDIR) _x_temp.*/dltmp* _x_temp.*/kernel_info.dat _x_temp.*/*.log 
	-$(RMDIR) _x_temp.* 

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  build_dir.* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary $(CUR_DIR)/*.raw
	-$(RMDIR) $(XFLIB_DIR)/common/data/*.xe2xd* $(XFLIB_DIR)/common/data/*.orig*


clean: cleanh
