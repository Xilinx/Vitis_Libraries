# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
# vitis makefile-generator v2.0.9

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<aiesim/x86sim/sw_emu/hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<aiesim/x86sim/sw_emu/hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  NOTE: For embedded platform the following setup steps are required:"
	$(ECHO) "      a.If the platform and common-image are downloaded from Xilinx Download Center(Suggested):"
	$(ECHO) "        Run the sdk.sh script from the common-image directory to install sysroot using the command : ./sdk.sh -y -d ./ -p "
	$(ECHO) "        Unzip the rootfs file : gunzip ./rootfs.ext4.gz"
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "      b.User could also define SYSROOT, K_IMAGE and ROOTFS by themselves: "
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "        export K_IMAGE=< path-to-Image-files >"
	$(ECHO) "        export ROOTFS=< path-to-rootfs >"
	$(ECHO) ""
	$(ECHO) "  make clean"
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

# setting default value
TARGET ?= aiesim
HOST_ARCH ?= aarch64

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_vck190_base_202310_1
endif

# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  vck190
PLATFORM_BLOCKLIST += 

include ./utils.mk
TEMP_DIR := _x_temp.$(TARGET).$(PLATFORM_NAME)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(PLATFORM_NAME)
BUILD_DIR := build_dir.$(TARGET).$(PLATFORM_NAME)
ifneq ($(RESULT_DIR),)
BUILD_DIR = $(RESULT_DIR)
endif
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(PLATFORM_NAME)
EMCONFIG := $(BUILD_DIR)/emconfig.json
XCLBIN_DIR := $(CUR_DIR)/$(BUILD_DIR)
export XCL_BINDIR = $(XCLBIN_DIR)

EXE_FILE_DEPS :=
BINARY_CONTAINERS_DEPS :=
RUN_DEPS :=

# get global setting
ifeq ($(HOST_ARCH), x86)
CXXFLAGS += -fmessage-length=0 -I$(CUR_DIR)/src/ -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -L$(XILINX_HLS)/lnx64/tools/fpo_v7_1 -Wl,--as-needed -lOpenCL -lxrt_coreutil -lgmp -lmpfr -lIp_floating_point_v7_1_bitacc_cmodel 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
else ifeq ($(HOST_ARCH), aarch64)
CXXFLAGS += -I$(CUR_DIR)/src/ -fmessage-length=0 --sysroot=$(SYSROOT)  -I$(SYSROOT)/usr/include/xrt -I$(XILINX_HLS)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(SYSROOT)/usr/lib -Wl,--as-needed -lxilinxopencl -lxrt_coreutil 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
endif
CXXFLAGS += $(EXTRA_CXXFLAGS)
VPP_FLAGS += $(EXTRA_VPP_FLAGS)

# aie template
XILINX_VITIS_AIETOOLS := $(XILINX_VITIS)/aietools
ifeq ($(HOST_ARCH), x86)
AIE_CXXFLAGS_INC +=  -I $(XILINX_VITIS)/aietools/include 
AIE_LDFLAGS_LD += -L $(XILINX_VITIS)/aietools/lib/lnx64.o -ladf_api_xrt 
else
AIE_CXXFLAGS_INC +=  -I $(XILINX_VITIS)/aietools/include 
AIE_LDFLAGS_LD += -L $(XILINX_VITIS)/aietools/lib/aarch64.o  -ladf_api_xrt 
endif
CXXFLAGS += $(AIE_CXXFLAGS_INC)
LDFLAGS += $(AIE_LDFLAGS_LD)

AIE_WORK_DIR ?= Work
AIE_PKG_DIR ?= Work

# Setting customized_params in aiecompiler

############################ setting AIE Compiler ###########################
ifneq ($(filter aiesim hw_emu hw, $(TARGET)),)
AIETARGET := hw
else
AIETARGET := x86sim
endif

AIE_CXXFLAGS += --target=$(AIETARGET) --platform=$(XPLATFORM) --config aie.cfg  
AIE_CXXFLAGS += $(AIE_CXXFLAGS_INC)
AIE_CONTAINER = $(TEMP_DIR)/libadf.a
AIE_WORK_DIR = $(CUR_DIR)/Work
AIE_PKG_DIR = $(AIE_WORK_DIR)

AIE_CXXFLAGS += -I $(XFLIB_DIR)/L1/include/ -I $(XFLIB_DIR)/L2/include/

$(AIE_WORK_DIR)/ps/c_rts/aie_control_xrt.cpp: $(AIE_CONTAINER)

EXE_FILE_DEPS += $(AIE_CONTAINER)
BINARY_CONTAINERS_DEPS += $(AIE_CONTAINER)
AIESIMFLAGS += $(EXTRA_AIESIMFLAGS)

########################## Setting up Host Variables ##########################

#Inclue Required Host Source Files
ifeq ($(ps_on_x86), on)
HOST_SRCS += $(XFLIB_DIR)/L3/tests/synthetic_aperture/PS_host/host.cpp $(CUR_DIR)/Work/ps/c_rts/aie_control_xrt.cpp 
CXXFLAGS +=  -D __PS_ENABLE_AIE__
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include -I $(XFLIB_DIR)/L3/include -I $(XFLIB_DIR)/L3/tests/synthetic_aperture/aie_graph
CXXFLAGS += -include $(XFLIB_DIR)/L1/include/kernels.hpp
else
HOST_SRCS += $(XFLIB_DIR)/L3/tests/synthetic_aperture/PS_host/host.cpp $(CUR_DIR)/Work/ps/c_rts/aie_control_xrt.cpp 
CXXFLAGS +=  -D __PS_ENABLE_AIE__
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include -I $(XFLIB_DIR)/L3/include -I $(XFLIB_DIR)/L3/tests/synthetic_aperture/aie_graph
CXXFLAGS += -include $(XFLIB_DIR)/L1/include/kernels.hpp
endif

# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
#CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := host.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(INSTANCE_FILES)  $(EXE_FILE_DEPS)

HOST_ARGS := -xclbin synthetic_aperture.xclbin -data data/
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif
ifeq ($(TARGET),$(filter $(TARGET),sw_emu))
HOST_ARGS := -xclbin $(BUILD_DIR)/synthetic_aperture.xclbin -data data/
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif
endif

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/vck190/'))
VPP_FLAGS +=   
VPP_LDFLAGS +=   --config $(CUR_DIR)/system.cfg
VPP_PACKAGE +=   --package.defer_aie_run

else 

endif

######################### binary container global settings ##########################
VPP_FLAGS_mm2s1 +=  
VPP_FLAGS_mm2s1 += --hls.clock 100000000:mm2s1
VPP_FLAGS_mm2s2 +=  
VPP_FLAGS_mm2s2 += --hls.clock 100000000:mm2s2
VPP_FLAGS_mm2s3 +=  
VPP_FLAGS_mm2s3 += --hls.clock 100000000:mm2s3
VPP_FLAGS_mm2s4 +=  
VPP_FLAGS_mm2s4 += --hls.clock 100000000:mm2s4
VPP_FLAGS_mm2s5 +=  
VPP_FLAGS_mm2s5 += --hls.clock 100000000:mm2s5
VPP_FLAGS_mm2s6 +=  
VPP_FLAGS_mm2s6 += --hls.clock 100000000:mm2s6
VPP_FLAGS_mm2s7 +=  
VPP_FLAGS_mm2s7 += --hls.clock 100000000:mm2s7
VPP_FLAGS_mm2s8 +=  
VPP_FLAGS_mm2s8 += --hls.clock 100000000:mm2s8
VPP_FLAGS_mm2s9 +=  
VPP_FLAGS_mm2s9 += --hls.clock 100000000:mm2s9
VPP_FLAGS_mm2s10 +=  
VPP_FLAGS_mm2s10 += --hls.clock 100000000:mm2s10
VPP_FLAGS_mm2s11 +=  
VPP_FLAGS_mm2s11 += --hls.clock 100000000:mm2s11
VPP_FLAGS_mm2s12 +=  
VPP_FLAGS_mm2s12 += --hls.clock 100000000:mm2s12
VPP_FLAGS_mm2s13 +=  
VPP_FLAGS_mm2s13 += --hls.clock 100000000:mm2s13
VPP_FLAGS_mm2s14 +=  
VPP_FLAGS_mm2s14 += --hls.clock 100000000:mm2s14
VPP_FLAGS_mm2s15 +=  
VPP_FLAGS_mm2s15 += --hls.clock 100000000:mm2s15
VPP_FLAGS_mm2s16 +=  
VPP_FLAGS_mm2s16 += --hls.clock 100000000:mm2s16
VPP_FLAGS_mm2s17 +=  
VPP_FLAGS_mm2s17 += --hls.clock 100000000:mm2s17
VPP_FLAGS_mm2s18 +=  
VPP_FLAGS_mm2s18 += --hls.clock 100000000:mm2s18
VPP_FLAGS_mm2s19 +=  
VPP_FLAGS_mm2s19 += --hls.clock 100000000:mm2s19
VPP_FLAGS_mm2s20 +=  
VPP_FLAGS_mm2s20 += --hls.clock 100000000:mm2s20
VPP_FLAGS_mm2s21 +=  
VPP_FLAGS_mm2s21 += --hls.clock 100000000:mm2s21
VPP_FLAGS_mm2s22 +=  
VPP_FLAGS_mm2s22 += --hls.clock 100000000:mm2s22
VPP_FLAGS_mm2s23 +=  
VPP_FLAGS_mm2s23 += --hls.clock 100000000:mm2s23
VPP_FLAGS_mm2s24 +=  
VPP_FLAGS_mm2s24 += --hls.clock 100000000:mm2s24
VPP_FLAGS_mm2s25 +=  
VPP_FLAGS_mm2s25 += --hls.clock 100000000:mm2s25
VPP_FLAGS_mm2s26 +=  
VPP_FLAGS_mm2s26 += --hls.clock 100000000:mm2s26
VPP_FLAGS_mm2s27 +=  
VPP_FLAGS_mm2s27 += --hls.clock 100000000:mm2s27
VPP_FLAGS_mm2s28 +=  
VPP_FLAGS_mm2s28 += --hls.clock 100000000:mm2s28
VPP_FLAGS_s2mm1 +=  
VPP_FLAGS_s2mm1 += --hls.clock 100000000:s2mm1
VPP_FLAGS_s2mm2 +=  
VPP_FLAGS_s2mm2 += --hls.clock 100000000:s2mm2
VPP_FLAGS_s2mm3 +=  
VPP_FLAGS_s2mm3 += --hls.clock 100000000:s2mm3
VPP_FLAGS_s2mm4 +=  
VPP_FLAGS_s2mm4 += --hls.clock 100000000:s2mm4
VPP_FLAGS_s2mm5 +=  
VPP_FLAGS_s2mm5 += --hls.clock 100000000:s2mm5
VPP_FLAGS_s2mm6 +=  
VPP_FLAGS_s2mm6 += --hls.clock 100000000:s2mm6
ifneq ($(HOST_ARCH_temp), x86)
VPP_LDFLAGS_synthetic_aperture += --clock.defaultFreqHz 100000000
else
VPP_LDFLAGS_synthetic_aperture += --kernel_frequency 100
endif

ifneq ($(SD_CARD_NEEDED), on)
BINARY_CONTAINERS += $(BUILD_DIR)/synthetic_aperture.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/synthetic_aperture_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/synthetic_aperture.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/mm2s1.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s1.cpp 
	$(ECHO) "Compiling Kernel: mm2s1"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s1) $(VPP_FLAGS) -k mm2s1 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s1.xo
$(TEMP_DIR)/mm2s2.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s2.cpp 
	$(ECHO) "Compiling Kernel: mm2s2"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s2) $(VPP_FLAGS) -k mm2s2 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s2.xo
$(TEMP_DIR)/mm2s3.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s3.cpp 
	$(ECHO) "Compiling Kernel: mm2s3"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s3) $(VPP_FLAGS) -k mm2s3 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s3.xo
$(TEMP_DIR)/mm2s4.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s4.cpp 
	$(ECHO) "Compiling Kernel: mm2s4"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s4) $(VPP_FLAGS) -k mm2s4 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s4.xo
$(TEMP_DIR)/mm2s5.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s5.cpp 
	$(ECHO) "Compiling Kernel: mm2s5"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s5) $(VPP_FLAGS) -k mm2s5 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s5.xo
$(TEMP_DIR)/mm2s6.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s6.cpp 
	$(ECHO) "Compiling Kernel: mm2s6"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s6) $(VPP_FLAGS) -k mm2s6 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s6.xo
$(TEMP_DIR)/mm2s7.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s7.cpp 
	$(ECHO) "Compiling Kernel: mm2s7"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s7) $(VPP_FLAGS) -k mm2s7 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s7.xo
$(TEMP_DIR)/mm2s8.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s8.cpp 
	$(ECHO) "Compiling Kernel: mm2s8"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s8) $(VPP_FLAGS) -k mm2s8 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s8.xo
$(TEMP_DIR)/mm2s9.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s9.cpp 
	$(ECHO) "Compiling Kernel: mm2s9"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s9) $(VPP_FLAGS) -k mm2s9 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s9.xo
$(TEMP_DIR)/mm2s10.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s10.cpp 
	$(ECHO) "Compiling Kernel: mm2s10"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s10) $(VPP_FLAGS) -k mm2s10 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s10.xo
$(TEMP_DIR)/mm2s11.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s11.cpp 
	$(ECHO) "Compiling Kernel: mm2s11"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s11) $(VPP_FLAGS) -k mm2s11 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s11.xo
$(TEMP_DIR)/mm2s12.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s12.cpp 
	$(ECHO) "Compiling Kernel: mm2s12"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s12) $(VPP_FLAGS) -k mm2s12 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s12.xo
$(TEMP_DIR)/mm2s13.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s13.cpp 
	$(ECHO) "Compiling Kernel: mm2s13"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s13) $(VPP_FLAGS) -k mm2s13 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s13.xo
$(TEMP_DIR)/mm2s14.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s14.cpp 
	$(ECHO) "Compiling Kernel: mm2s14"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s14) $(VPP_FLAGS) -k mm2s14 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s14.xo
$(TEMP_DIR)/mm2s15.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s15.cpp 
	$(ECHO) "Compiling Kernel: mm2s15"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s15) $(VPP_FLAGS) -k mm2s15 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s15.xo
$(TEMP_DIR)/mm2s16.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s16.cpp 
	$(ECHO) "Compiling Kernel: mm2s16"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s16) $(VPP_FLAGS) -k mm2s16 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s16.xo
$(TEMP_DIR)/mm2s17.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s17.cpp 
	$(ECHO) "Compiling Kernel: mm2s17"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s17) $(VPP_FLAGS) -k mm2s17 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s17.xo
$(TEMP_DIR)/mm2s18.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s18.cpp 
	$(ECHO) "Compiling Kernel: mm2s18"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s18) $(VPP_FLAGS) -k mm2s18 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s18.xo
$(TEMP_DIR)/mm2s19.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s19.cpp 
	$(ECHO) "Compiling Kernel: mm2s19"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s19) $(VPP_FLAGS) -k mm2s19 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s19.xo
$(TEMP_DIR)/mm2s20.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s20.cpp 
	$(ECHO) "Compiling Kernel: mm2s20"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s20) $(VPP_FLAGS) -k mm2s20 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s20.xo
$(TEMP_DIR)/mm2s21.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s21.cpp 
	$(ECHO) "Compiling Kernel: mm2s21"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s21) $(VPP_FLAGS) -k mm2s21 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s21.xo
$(TEMP_DIR)/mm2s22.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s22.cpp 
	$(ECHO) "Compiling Kernel: mm2s22"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s22) $(VPP_FLAGS) -k mm2s22 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s22.xo
$(TEMP_DIR)/mm2s23.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s23.cpp 
	$(ECHO) "Compiling Kernel: mm2s23"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s23) $(VPP_FLAGS) -k mm2s23 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s23.xo
$(TEMP_DIR)/mm2s24.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s24.cpp 
	$(ECHO) "Compiling Kernel: mm2s24"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s24) $(VPP_FLAGS) -k mm2s24 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s24.xo
$(TEMP_DIR)/mm2s25.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s25.cpp 
	$(ECHO) "Compiling Kernel: mm2s25"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s25) $(VPP_FLAGS) -k mm2s25 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s25.xo
$(TEMP_DIR)/mm2s26.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s26.cpp 
	$(ECHO) "Compiling Kernel: mm2s26"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s26) $(VPP_FLAGS) -k mm2s26 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s26.xo
$(TEMP_DIR)/mm2s27.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s27.cpp 
	$(ECHO) "Compiling Kernel: mm2s27"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s27) $(VPP_FLAGS) -k mm2s27 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s27.xo
$(TEMP_DIR)/mm2s28.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/mm2s28.cpp 
	$(ECHO) "Compiling Kernel: mm2s28"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_mm2s28) $(VPP_FLAGS) -k mm2s28 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/mm2s28.xo
$(TEMP_DIR)/s2mm1.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/s2mm1.cpp 
	$(ECHO) "Compiling Kernel: s2mm1"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_s2mm1) $(VPP_FLAGS) -k s2mm1 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/s2mm1.xo
$(TEMP_DIR)/s2mm2.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/s2mm2.cpp 
	$(ECHO) "Compiling Kernel: s2mm2"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_s2mm2) $(VPP_FLAGS) -k s2mm2 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/s2mm2.xo
$(TEMP_DIR)/s2mm3.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/s2mm3.cpp 
	$(ECHO) "Compiling Kernel: s2mm3"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_s2mm3) $(VPP_FLAGS) -k s2mm3 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/s2mm3.xo
$(TEMP_DIR)/s2mm4.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/s2mm4.cpp 
	$(ECHO) "Compiling Kernel: s2mm4"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_s2mm4) $(VPP_FLAGS) -k s2mm4 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/s2mm4.xo
$(TEMP_DIR)/s2mm5.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/s2mm5.cpp 
	$(ECHO) "Compiling Kernel: s2mm5"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_s2mm5) $(VPP_FLAGS) -k s2mm5 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/s2mm5.xo
$(TEMP_DIR)/s2mm6.xo: $(XFLIB_DIR)/L3/tests/synthetic_aperture/PL_kernels/s2mm6.cpp 
	$(ECHO) "Compiling Kernel: s2mm6"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_s2mm6) $(VPP_FLAGS) -k s2mm6 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_synthetic_aperture_OBJS += $(TEMP_DIR)/s2mm6.xo
BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_synthetic_aperture_OBJS)
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/synthetic_aperture $(VPP_LDFLAGS)  $(VPP_LDFLAGS_synthetic_aperture) $(AIE_LDFLAGS)   -o $@ $^

############################## Setting Rules for AIE (Building Kernels) ##################
$(AIE_CONTAINER):  $(XFLIB_DIR)/L3/tests/synthetic_aperture/aie_graph/graph.cpp 
	$(ECHO) "Compiling: libadf.a"
	mkdir -p $(dir $@)
	$(VPP) -c --mode aie $(AIE_CXXFLAGS) -o $@ $(filter %.s %.c %.cc %.cpp %cxx, $^)

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXE_FILE): $(EXE_FILE_DEPS)
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $(filter %.s %.c %.cc %.cpp %cxx, $^) $(CXXFLAGS) $(LDFLAGS)

$(EMCONFIG):
	emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)

############################## Preparing sdcard folder ##############################
ifeq ($(SD_CARD_NEEDED), on)
RUN_SCRIPT := $(BUILD_DIR)/run_script.sh
$(RUN_SCRIPT):
	rm -rf $(RUN_SCRIPT)
	@echo 'export LD_LIBRARY_PATH=/mnt:/tmp:$(LIBRARY_PATH)' >> $(RUN_SCRIPT)
ifneq ($(filter sw_emu hw_emu, $(TARGET)),)
	@echo 'export XCL_EMULATION_MODE=$(TARGET)' >> $(RUN_SCRIPT)
endif
	@echo 'export XILINX_VITIS=/mnt' >> $(RUN_SCRIPT)
	@echo 'export XILINX_XRT=/usr' >> $(RUN_SCRIPT)
	@echo 'if [ -f platform_desc.txt  ]; then' >> $(RUN_SCRIPT)
	@echo '        cp platform_desc.txt /etc/xocl.txt' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo './$(EXE_NAME) $(PKG_HOST_ARGS)' >> $(RUN_SCRIPT)
	@echo 'return_code=$$?' >> $(RUN_SCRIPT)
	@echo 'if [ $$return_code -ne 0 ]; then' >> $(RUN_SCRIPT)
	@echo '        echo "ERROR: TEST FAILED, RC=$$return_code"' >> $(RUN_SCRIPT)
	@echo 'else' >> $(RUN_SCRIPT)
	@echo '        echo "INFO: TEST PASSED, RC=0"' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo 'echo "INFO: Embedded host run completed."' >> $(RUN_SCRIPT)
	@echo 'exit $$return_code' >> $(RUN_SCRIPT)
DATA_FILE := $(custom_data_file)
DATA_DIR := data $(custom_data_dir)
SD_FILES += $(RUN_SCRIPT)
SD_FILES += $(EXE_FILE)
SD_FILES += $(EMCONFIG)
SD_FILES += xrt.ini
SD_FILES += $(DATA_FILE)# where define DATAFILE in json
SD_FILES_WITH_PREFIX = $(foreach sd_file,$(SD_FILES), $(if $(filter $(sd_file),$(wildcard $(sd_file))), --package.sd_file $(sd_file)))
SD_DIRS_WITH_PREFIX = $(foreach sd_dir,$(DATA_DIR),--package.sd_dir $(sd_dir))
PACKAGE_FILES := $(BINARY_CONTAINERS)
PACKAGE_FILES += $(AIE_CONTAINER)
SD_CARD := $(CUR_DIR)/package_$(TARGET)
$(SD_CARD): host xclbin $(RUN_SCRIPT) $(EMCONFIG) check_kimage check_rootfs
	@echo "Generating sd_card folder...."
	mkdir -p $(SD_CARD)
	chmod a+rx $(BUILD_DIR)/run_script.sh
# 1. DFX HW Flow
ifeq ($(dfx_hw), on)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p $(PACKAGE_FILES) $(VPP_PACKAGE) -o $(BINARY_CONTAINERS_PKG)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE) --package.boot_mode sd  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX) --package.sd_file $(BINARY_CONTAINERS_PKG)
	@echo "### ***** sd_card generation done! ***** ###"
endif
# 2. PS_ON_X86 - From 2022.2, Target:sw_emu
ifeq ($(ps_on_x86), on)
	@echo "### ***** running PS X86 SE_EMU ***** ###"
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir $(SD_CARD) --package.emu_ps x86
endif
# 3. AIE_ON_X86 Flow
ifeq ($(pcie_aie), on)
	@echo "### ***** running AIE ON_X86 ***** ###"
	${VPP} -p $(VPP_PACKAGE) -t ${TARGET} -f ${XPLATFORM} ${AIE_CONTAINER} ${BINARY_CONTAINERS} -o $(BINARY_CONTAINERS_PKG) --package.boot_mode ospi
	@echo "### ***** sd_card generation done! ***** ###"
endif
# 4. General Embeded flow
ifeq ($(dfx_hw), off)
ifeq ($(ps_on_x86), off)
ifeq ($(pcie_aie), off)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE) --package.boot_mode sd  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
endif
endif
endif

.PHONY: sd_card
sd_card: $(SD_CARD)
endif
############################## Setting Essential Checks and Building Rules ##############################
ifneq (,$(filter x86sim aiesim, $(TARGET)))
RUN_DEPS += $(AIE_CONTAINER)
else
RUN_DEPS += host xclbin $(EMCONFIG)
RUN_DEPS += $(SD_CARD)
endif

.PHONY: mkflag all run
mkflag:
	mkdir -p $(BUILD_DIR)
	rm -rf $(BUILD_DIR)/makefile_args.txt
	@for var in $(MAKEFLAGS); do echo $$var >> $(BUILD_DIR)/makefile_args.txt; done

all: check_device check_vpp check_platform mkflag $(RUN_DEPS)

run: all
#x86sim
ifeq ($(TARGET), x86sim)
	$(X86SIMULATOR) --pkg-dir=$(AIE_PKG_DIR) 
	
endif
#aiesim
ifeq ($(TARGET), aiesim)
	$(AIESIMULATOR) --pkg-dir=$(AIE_PKG_DIR) --profile  $(AIESIMFLAGS)
	
endif
#hw_emu
ifneq (,$(filter hw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
ifeq ($(pcie_aie), on)
	cp $(AIE_WORK_DIR)/reports/dma_lock_report.json ./
	cp $(AIE_WORK_DIR)/ps/c_rts/aie_control_config.json ./
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS) $(BINARY_CONTAINERS_PKG)
	
endif
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS)
	
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_output.log || exit 1
	
endif
endif
#sw_emu
ifneq (,$(filter sw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS) 
	
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_output.log || exit 1
	
endif
endif
#hw
ifeq ($(TARGET), hw)
ifneq (,$(findstring aws-vu9p-f1, $(PLATFORM_NAME)))
ifeq (,$(wildcard $(BUILD_DIR)/synthetic_aperture.awsxclbin))
	$(ECHO) "This makefile does not directly support converting .xclbin to .awsxclbin, please refer https://github.com/aws/aws-fpga/blob/master/Vitis/README.md for next operations"
else
	$(ECHO) "Running HW using generated .awsxclbin"
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(subst .xclbin,.awsxclbin,$(HOST_ARGS))
	
endif
else ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(HOST_ARGS)
	
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Setting Targets ##############################

.PHONY: clean cleanall emconfig valid_params
emconfig: $(EMCONFIG)

.PHONY: host
ifeq ($(HOST_ARCH), x86)
host: check_xrt   $(EXE_FILE)
else
host: check_sysroot   $(EXE_FILE)
endif

.PHONY: xclbin
ifeq ($(HOST_ARCH), x86)
xclbin: check_vpp check_xrt  $(BINARY_CONTAINERS)
else
xclbin: check_vpp check_sysroot  $(BINARY_CONTAINERS)
endif

#################### Parameterized File Generation ##########################

gen_instances: $(GENERATED_FILES)

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE)  vitis_* TempConfig system_estimate.xtxt *.rpt .run/  $(INST_TB_FILES)
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png *.db

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.* _x* $(INST_FILES)

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR) sd_card* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR) $(AIE_CONTAINER) aiesimulator_output .AIE_SIM_CMD_LINE_OPTIONS x86simulator_output $(AIE_WORK_DIR) $(AIE_PKG_DIR) $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut
	-$(RMDIR)  *.html $(GENERATED_FILES)

clean: cleanh