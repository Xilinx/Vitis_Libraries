#
# Copyright 2019-2020 Xilinx, Inc.
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
# makefile-generator v1.0.3
#

# ####################################### Help Section #####################################
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
	$(ECHO) "  make TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86>"
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
	$(ECHO) "  make host HOST_ARCH=<aarch32/aarch64/x86>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  NOTE: For SoC shells, ENV variable SYSROOT needs to be set."
	$(ECHO) ""

# ##################### Setting up default value of TARGET ##########################
TARGET ?= sw_emu

# ################### Setting up default value of DEVICE ##############################
DEVICE ?= xilinx_u200_xdma_201830_2

# ###################### Setting up default value of HOST_ARCH ####################### 
HOST_ARCH ?= x86

# #################### Checking if DEVICE in blacklist #############################
ifeq ($(findstring u280, $(DEVICE)), u280)
$(error [ERROR]: This project is not supported for $(DEVICE).)
endif
ifeq ($(findstring u250, $(DEVICE)), u250)
$(error [ERROR]: This project is not supported for $(DEVICE).)
endif
ifeq ($(findstring zcu102, $(DEVICE)), zcu102)
$(error [ERROR]: This project is not supported for $(DEVICE).)
endif
ifeq ($(findstring zcu104, $(DEVICE)), zcu104)
$(error [ERROR]: This project is not supported for $(DEVICE).)
endif

# #################### Checking if DEVICE in whitelist ############################
ifneq ($(findstring u200, $(DEVICE)), u200)
ifneq ($(findstring u50, $(DEVICE)), u50)
$(warning [WARNING]: This project has not been tested for $(DEVICE). It may or may not work.)
endif
endif

# ######################## Setting up Project Variables #################################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L2/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

# ######################### Include environment variables in utils.mk ####################
include ./utils.mk
XDEVICE := $(call device2xsa, $(DEVICE))
TEMP_DIR := _x_temp.$(TARGET).$(XDEVICE)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(XDEVICE)
BUILD_DIR := build_dir.$(TARGET).$(XDEVICE)
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(XDEVICE)
EMCONFIG_DIR := $(BUILD_DIR)
XCLBIN_DIR := $(CUR_DIR)/$(BUILD_DIR)
export XCL_BINDIR = $(XCLBIN_DIR)

# ######################### Setting up Host Variables #########################
#Include Required Host Source Files
HOST_SRCS += $(XFLIB_DIR)/L2/examples/warptransform/xf_warp_transform_tb.cpp
HOST_SRCS += $(XFLIB_DIR)/ext/xcl2/xcl2.cpp
CXXFLAGS += -I$(XFLIB_DIR)/L2/tests/warptransform/warptransform_PERSPECTIVE_NN_NO_RGBA_URAM
CXXFLAGS += -I$(XFLIB_DIR)/L2/examples/warptransform
CXXFLAGS += -I$(XFLIB_DIR)/ext/xcl2/
CXXFLAGS += -I$(XFLIB_DIR)/L1/include

LDFLAGS += -lopencv_videoio -lopencv_imgcodecs -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_flann -lopencv_video -lopencv_calib3d -lopencv_highgui


ifeq ($(TARGET),sw_emu)
CXXFLAGS += -D SW_EMU_TEST
endif

ifeq ($(TARGET),hw_emu)
CXXFLAGS += -D HW_EMU_TEST
endif

# ######################### Host compiler global settings ############################
CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include -std=c++11 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label
LDFLAGS += -L$(XILINX_XRT)/lib -lOpenCL -lpthread -lrt -Wno-unused-label -Wno-narrowing -DVERBOSE
CXXFLAGS += -fmessage-length=0 -O3
CXXFLAGS += -I$(CUR_DIR)/src/ 

ifeq ($(HOST_ARCH), x86)
LDFLAGS += -L$(XILINX_VIVADO)/lnx64/tools/fpo_v7_0 -Wl,--as-needed -lgmp -lmpfr -lIp_floating_point_v7_0_bitacc_cmodel
endif
ifneq (,$(shell echo $(XPLATFORM) | awk '/u200/'))
CXXFLAGS += -I$(OPENCV_INCLUDE)
LDFLAGS += -L$(OPENCV_LIB)
else ifneq (,$(shell echo $(XPLATFORM) | awk '/zcu102/'))
CXXFLAGS += -I$(SYSROOT)/usr/include
CXXFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += -L$(SYSROOT)/usr/lib
LDFLAGS += -L${SYSROOT}/opt/xilinx/xrt/lib
else ifneq (,$(shell echo $(XPLATFORM) | awk '/u50/'))
CXXFLAGS += -I$(OPENCV_INCLUDE)
LDFLAGS += -L$(OPENCV_LIB)
else ifneq (,$(shell echo $(XPLATFORM) | awk '/zcu104/'))
CXXFLAGS += -I$(SYSROOT)/usr/include
CXXFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += -L$(SYSROOT)/usr/lib
LDFLAGS += -L${SYSROOT}/opt/xilinx/xrt/lib
endif

# ################### Setting package and image directory #######################

EXE_NAME := warptransform
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
ifeq ($(TARGET),$(filter $(TARGET),sw_emu))
HOST_ARGS := $(XFLIB_DIR)/data/128x128.png 
LIBRARY_PATH =$(OPENCV_LIB):$(LD_LIBRARY_PATH):$(XILINX_XRT)/lib
endif
ifeq ($(TARGET),$(filter $(TARGET),hw_emu))
HOST_ARGS := $(XFLIB_DIR)/data/128x128.png  
LIBRARY_PATH =$(OPENCV_LIB):$(LD_LIBRARY_PATH):$(XILINX_XRT)/lib
endif
ifeq ($(TARGET),$(filter $(TARGET),hw))
HOST_ARGS := $(XFLIB_DIR)/data/4k.jpg 
LIBRARY_PATH =$(OPENCV_LIB):$(LD_LIBRARY_PATH):$(XILINX_XRT)/lib
endif

# ##################### Kernel compiler global settings ##########################
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps --optimize 2
VPP_FLAGS += --jobs 8
VPP_FLAGS += -I$(XFLIB_DIR)/L1/include/

VPP_FLAGS += -I$(XFLIB_DIR)/L2/tests/warptransform/warptransform_PERSPECTIVE_NN_NO_RGBA_URAM
VPP_FLAGS += -I$(XFLIB_DIR)/L2/examples/warptransform


VPP_FLAGS += 
VPP_FLAGS += 

# ############################ Declaring Binary Containers ##########################

BINARY_CONTAINERS += $(BUILD_DIR)/krnl_warptransform.xclbin
BINARY_CONTAINER_krnl_warptransform_OBJS += $(TEMP_DIR)/warptransform_accel.xo

# ######################### Setting Targets of Makefile ################################
DATA_FILE += $(XFLIB_DIR)/data/4k.jpg
DATA_FILE += $(XFLIB_DIR)/data/128x128.png

.PHONY: all clean cleanall docs emconfig
all: check_vpp check_platform  $(EXE_FILE) $(BINARY_CONTAINERS) emconfig

.PHONY: check_env
check_env:
ifeq (,$(OPENCV_INCLUDE))
	@echo "Cannot find OpenCV include path. Please set OPENCV_INCLUDE variable" && false
endif
ifeq (,$(OPENCV_LIB))
	@echo "Cannot find Opencv lib path. Please set OPENCV_LIB variable" && false
endif

.PHONY: host
host: check_env check_xrt check_sysroot $(EXE_FILE)

.PHONY: xclbin
xclbin: check_env check_vpp check_sysroot $(BINARY_CONTAINERS)

.PHONY: build
build: xclbin

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/warptransform_accel.xo: $(XFLIB_DIR)/L2/examples/warptransform/xf_warp_transform_accel.cpp
	$(ECHO) "Compiling Kernel: warptransform_accel"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(warptransform_accel_VPP_FLAGS) $(VPP_FLAGS) -k warptransform_accel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o'$@' '$<'


$(BUILD_DIR)/krnl_warptransform.xclbin: $(BINARY_CONTAINER_krnl_warptransform_OBJS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(BUILD_DIR) --report_dir $(BUILD_REPORT_DIR)/krnl_warptransform $(VPP_LDFLAGS) $(VPP_LDFLAGS_krnl_warptransform) -o '$@' $(+)


# ################# Setting Rules for Host (Building Host Executable) ################
$(EXE_FILE): $(HOST_SRCS) | check_xrt
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(XPLATFORM) --od $(EMCONFIG_DIR)


# ###############Setting Essential Checks And Running Rules For Vitis Flow #############
run: all
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	LD_LIBRARY_PATH=$(LIBRARY_PATH) XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS)
else
	LD_LIBRARY_PATH=$(LIBRARY_PATH) $(EXE_FILE) $(HOST_ARGS)
endif

# ################################# Cleaning Rules ##################################
cleanh:
	-$(RMDIR) $(EXE_FILE) vitis_* TempConfig system_estimate.xtxt *.rpt .run/
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj* bin* *.csv *.jpg *.jpeg *.png

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl* start_simulation.sh *.xclbin
	-$(RMDIR) _x_temp.*/_x.* _x_temp.*/.Xil _x_temp.*/profile_summary.* xo_* _x*
	-$(RMDIR) _x_temp.*/dltmp* _x_temp.*/kernel_info.dat _x_temp.*/*.log 
	-$(RMDIR) _x_temp.* 

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  build_dir.* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary $(CUR_DIR)/*.raw package_* run_script.sh .ipcache *.str
	-$(RMDIR) $(XFLIB_DIR)/common/data/*.xe2xd* $(XFLIB_DIR)/common/data/*.orig*

	-$(RMDIR) $(AIE_CONTAINERS) $(CUR_DIR)/Work $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut aiesimulator_output .AIE_SIM_CMD_LINE_OPTIONS

clean: cleanh
