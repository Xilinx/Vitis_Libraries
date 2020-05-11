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
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L2/tests/bfs/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

TARGET ?= sw_emu
HOST_ARCH := x86
SYSROOT := ${SYSROOT}
DEVICE ?= xilinx_u200_xdma_201830_2


ifeq ($(findstring zc, $(DEVICE)), zc)
$(error [ERROR]: This project is not supported for $(DEVICE).)
endif

ifneq ($(findstring u200, $(DEVICE)), u200)
ifneq ($(findstring u250, $(DEVICE)), u250)
$(warning [WARNING]: This project has not been tested for $(DEVICE). It may or may not work.)
endif
endif

include ./utils.mk

XDEVICE := $(call device2xsa, $(DEVICE))
TEMP_DIR := _x_temp.$(TARGET).$(XDEVICE)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(XDEVICE)
BUILD_DIR := build_dir.$(TARGET).$(XDEVICE)
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(XDEVICE)
EMCONFIG_DIR := $(BUILD_DIR)

# Setting tools
VPP := v++
SDCARD := sd_card
EMU_DIR := $(SDCARD)/data/emulation

############################## Setting up Host Variables ##############################
#Include Required Host Source Files
HOST_SRCS += $(CUR_DIR)/host/main.cpp
HOST_SRCS += $(XFLIB_DIR)/ext/xcl2/xcl2.cpp

CXXFLAGS += -I$(XFLIB_DIR)/L2/include
CXXFLAGS += -I$(XFLIB_DIR)/L2/tests/bfs/host
CXXFLAGS += -I$(XFLIB_DIR)/L2/tests/bfs/kernel
CXXFLAGS += -I$(XFLIB_DIR)/../database/L1/include/hw
CXXFLAGS += -I$(XFLIB_DIR)/../quantitative_finance/L1/include
CXXFLAGS += -I$(XFLIB_DIR)/../utils/L1/include
CXXFLAGS += -I$(XFLIB_DIR)/ext/xcl2



# Host compiler global settings
CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label
LDFLAGS += -L$(XILINX_XRT)/lib -lOpenCL -lpthread -lrt -Wno-unused-label -Wno-narrowing -DVERBOSE
CXXFLAGS += -fmessage-length=0 -O3 
CXXFLAGS +=-I$(CUR_DIR)/src/ 


EXE_NAME := host.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
SOC_HOST_ARGS :=  -xclbin $(BUILD_DIR)/bfs_kernel.xclbin -o ./L2/tests/bfs/data/test_offset.csr -c ./L2/tests/bfs/data/test_column.csr -g ./L2/tests/bfs/data/test_golden.mtx -i ./data/test_golden.mtx -i 0

HOST_ARGS :=  -xclbin $(BUILD_DIR)/bfs_kernel.xclbin -o $(XFLIB_DIR)/L2/tests/bfs/data/test_offset.csr -c $(XFLIB_DIR)/L2/tests/bfs/data/test_column.csr -g $(XFLIB_DIR)/L2/tests/bfs/data/test_golden.mtx -i 0

ifneq ($(HOST_ARCH), x86)
	LDFLAGS += --sysroot=$(SYSROOT)
endif

############################## Setting up Kernel Variables ##############################
# Kernel compiler global settings
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps
LDCLFLAGS += --optimize 2 --jobs 8

ifneq (,$(shell echo $(XPLATFORM) | awk '/u200/'))
VPP_FLAGS += --config $(CUR_DIR)/conn_u200_u250.ini
else ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
VPP_FLAGS += --config $(CUR_DIR)/conn_u200_u250.ini
endif

VPP_FLAGS += -I$(XFLIB_DIR)/L2/include
VPP_FLAGS += -I$(XFLIB_DIR)/L2/tests/bfs/kernel
VPP_FLAGS += -I$(XFLIB_DIR)/../database/L1/include/hw
VPP_FLAGS += -I$(XFLIB_DIR)/../quantitative_finance/L1/include
VPP_FLAGS += -I$(XFLIB_DIR)/../utils/L1/include

bfs_kernel_VPP_FLAGS +=  -D KERNEL_NAME=bfs_kernel

############################## Declaring Binary Containers ##############################
BINARY_CONTAINERS += $(BUILD_DIR)/bfs_kernel.xclbin
BINARY_CONTAINER_bfs_kernel_OBJS += $(TEMP_DIR)/bfs_kernel.xo

############################## Setting Targets ##############################
CP = cp -rf
DATA = ./data

.PHONY: all clean cleanall docs emconfig
all: check_vpp check_platform | $(EXE_FILE) $(BINARY_CONTAINERS) emconfig sd_card


.PHONY: host
host: $(EXE_FILE) | check_xrt

.PHONY: xclbin
xclbin: check_vpp | $(BINARY_CONTAINERS)

.PHONY: build
build: xclbin

############################## Setting Rules for Binary Containers (Building Kernels) ##############################
$(TEMP_DIR)/bfs_kernel.xo: $(CUR_DIR)/kernel/bfs_kernel.cpp
	$(ECHO) "Compiling Kernel: bfs_kernel"
	mkdir -p $(TEMP_DIR)
	$(VPP) $(bfs_kernel_VPP_FLAGS) $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -c -k bfs_kernel -I'$(<D)' -o'$@' '$<'

$(BUILD_DIR)/bfs_kernel.xclbin: $(BINARY_CONTAINER_bfs_kernel_OBJS)
	mkdir -p $(BUILD_DIR)
	$(VPP) $(VPP_FLAGS) --temp_dir $(BUILD_DIR) --report_dir $(BUILD_REPORT_DIR)/bfs_kernel -l $(LDCLFLAGS) $(LDCLFLAGS_bfs_kernel) -o'$@' $(+)

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXE_FILE): $(HOST_SRCS) | check_xrt
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(XPLATFORM) --od $(EMCONFIG_DIR)

############################## Setting Essential Checks and Running Rules ##############################
run: all
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
ifeq ($(HOST_ARCH), x86)
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS)
else
	mkdir -p $(EMU_DIR)
	$(CP) $(XILINX_VITIS)/data/emulation/unified $(EMU_DIR)
	mkfatimg $(SDCARD) $(SDCARD).img 500000
	launch_emulator -no-reboot -runtime ocl -t $(TARGET) -sd-card-image $(SDCARD).img -device-family $(DEV_FAM)
endif
else
ifeq ($(HOST_ARCH), x86)
	$(EXE_FILE) $(HOST_ARGS)
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Preparing sdcard folder ##############################
sd_card: $(EXE_FILE) $(BINARY_CONTAINERS) emconfig
ifneq ($(HOST_ARCH), x86)
	mkdir -p $(SDCARD)/$(BUILD_DIR)
	mkdir -p $(SDCARD)/data
	$(CP) $(B_NAME)/sw/$(XDEVICE)/boot/generic.readme $(B_NAME)/sw/$(XDEVICE)/xrt/image/* xrt.ini $(EXE_FILE) $(SDCARD)
	$(CP) $(BUILD_DIR)/*.xclbin $(SDCARD)/$(BUILD_DIR)/
	$(CP) $(XFLIB_DIR)/L2/tests/bfs/data/test_offset.csr $(SDCARD)/
	$(CP) $(XFLIB_DIR)/L2/tests/bfs/data/test_column.csr $(SDCARD)/
	$(CP) $(XFLIB_DIR)/L2/tests/bfs/data/test_golden.mtx $(SDCARD)/
	$(CP) data $(SDCARD)
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(ECHO) 'cd /mnt/' >> $(SDCARD)/init.sh
	$(ECHO) 'export XILINX_VITIS=$$PWD' >> $(SDCARD)/init.sh
	$(ECHO) 'export XCL_EMULATION_MODE=$(TARGET)' >> $(SDCARD)/init.sh
	$(ECHO) '$(EXE_FILE) $(SOC_HOST_ARGS)' >> $(SDCARD)/init.sh
	$(ECHO) 'reboot' >> $(SDCARD)/init.sh
else
	[ -f $(SDCARD)/BOOT.BIN ] && echo "INFO: BOOT.BIN already exists" || $(CP) $(BUILD_DIR)/sd_card/BOOT.BIN $(SDCARD)/
	$(ECHO) '$(EXE_FILE) $(SOC_HOST_ARGS)' >> $(SDCARD)/init.sh
endif
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
	-$(RMDIR) $(BUILD_DIR) sd_card* build_dir.* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary $(CUR_DIR)/*.raw
	-$(RMDIR) $(XFLIB_DIR)/common/data/*.xe2xd* $(XFLIB_DIR)/common/data/*.orig*


clean: cleanh
