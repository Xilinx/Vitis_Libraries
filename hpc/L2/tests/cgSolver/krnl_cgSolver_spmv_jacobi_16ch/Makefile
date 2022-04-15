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
# vitis makefile-generator v2.0.6

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<hw/> PLATFORM=<FPGA platform> HOST_ARCH=<x86>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw/> PLATFORM=<FPGA platform> HOST_ARCH=<x86>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make xclbin TARGET=<hw/> PLATFORM=<FPGA platform> HOST_ARCH=<x86>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make host TARGET=<hw/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  NOTE: For embedded devices, e.g. zcu102/zcu104/vck190, env variable SYSROOT and EDGE_COMMON_SW need to be set first, and HOST_ARCH is either aarch32 or aarch64. For example,"
	$(ECHO) "       export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "       export EDGE_COMMON_SW=< path-to-rootfs-and-Image-files >"
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

############################## Setting up Project Variables ##############################

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L2/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

# setting devault value
TARGET ?= hw
HOST_ARCH ?= x86

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_u280_xdma_201920_3
endif

# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  u280
PLATFORM_BLOCKLIST +=  other

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
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -L$(XILINX_HLS)/lnx64/tools/fpo_v7_0  -Wl,--as-needed -lOpenCL -lxrt_coreutil -lgmp -lmpfr -lIp_floating_point_v7_0_bitacc_cmodel 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
else ifeq ($(HOST_ARCH), aarch64)
CXXFLAGS += -I$(CUR_DIR)/src/ -fmessage-length=0 --sysroot=$(SYSROOT)  -I$(SYSROOT)/usr/include/xrt -I$(XILINX_HLS)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(SYSROOT)/usr/lib -L$(XILINX_VITIS_AIETOOLS)/lib/aarch64.o -Wl,--as-needed -lxilinxopencl -lxrt_coreutil 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
endif
CXXFLAGS += $(EXTRA_CXXFLAGS)
VPP_FLAGS += $(EXTRA_VPP_FLAGS)

########################## Setting up Host Variables ##########################
ifeq ($(TARGET),sw_emu)
CXXFLAGS += -D SW_EMU_TEST
endif
ifeq ($(TARGET),hw_emu)
CXXFLAGS += -D HW_EMU_TEST
endif

ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

#Inclue Required Host Source Files
HOST_SRCS += $(XFLIB_DIR)/L2/src/sw/cgSolver/test_cgSolver_spmv_jacobi.cpp $(XFLIB_DIR)/../blas/L2/src/xcl2/xcl2.cpp 
CXXFLAGS +=  -D HW_CLK=4e-9 -D CG_numTasks=1 -D CG_dataType=double -D CG_numChannels=16 -D CG_parEntries=4 -D CG_instrBytes=64 -D SPARSE_dataType=double -D SPARSE_indexType=uint16_t -D SPARSE_parEntries=4 -D SPARSE_hbmChannels=16 -D SPARSE_hbmMemBits=256
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/ -I $(XFLIB_DIR)/L2/include/sw/cgSolver -I $(XFLIB_DIR)/L2/include/common/cgSolver -I $(XFLIB_DIR)/L2/include/sw/ -I $(XFLIB_DIR)/../blas/L1/tests/sw/include/ -I $(XFLIB_DIR)/../blas/L2/include/xcl2 -I $(XFLIB_DIR)/../sparse/L2/include/sw/fp64 -I $(XFLIB_DIR)/..
CXXFLAGS += -g -O0

EXE_NAME := host.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(EXE_FILE_DEPS)

HOST_ARGS := $(BUILD_DIR)/cgSolver.xclbin 32 1e-12 $(CUR_DIR)/sig_dat/  $(CUR_DIR)/vec_dat ted_B
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/u280/'))
VPP_FLAGS +=   
VPP_FLAGS +=  -D CG_numTasks=1 -D CG_dataType=double -D CG_instrBytes=64 -D CG_tkStrWidth=8 -D CG_parEntries=4 -D CG_numChannels=16 -D CG_vecParEntries=4 -D SPARSE_dataType=double -D SPARSE_dataBits=64 -D SPARSE_parEntries=4 -D SPARSE_indexType=uint16_t -D SPARSE_indexBits=16 -D SPARSE_maxRows=4096 -D SPARSE_maxCols=4096 -D SPARSE_accLatency=8 -D SPARSE_hbmMemBits=256 -D SPARSE_hbmChannels=16
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/ -I $(XFLIB_DIR)/L1/include/hw/ -I $(XFLIB_DIR)/L1/include/hw/cgSolver -I $(XFLIB_DIR)/L2/include/hw/cgSolver -I $(XFLIB_DIR)/L2/include/hw/cgSolver/spmv -I $(XFLIB_DIR)/L2/include/common/cgSolver -I $(XFLIB_DIR)/L2/include/hw -I $(XFLIB_DIR)/../sparse/L1/include/hw -I $(XFLIB_DIR)/../sparse/L2/include/hw/fp64 -I $(XFLIB_DIR)/.. -I $(XFLIB_DIR)/../blas/L2/include/memKernel/hw/xf_blas -I $(XFLIB_DIR)/../blas/L1/include/hw -I $(XFLIB_DIR)/L2/tests/cgSolver/krnl_cgSolver_spmv_jacobi_16ch/kernel

else 
VPP_FLAGS +=  -D CG_numTasks=1 -D CG_dataType=double -D CG_instrBytes=64 -D CG_tkStrWidth=8 -D CG_parEntries=4 -D CG_numChannels=16 -D CG_vecParEntries=4 -D SPARSE_dataType=double -D SPARSE_dataBits=64 -D SPARSE_parEntries=4 -D SPARSE_indexType=uint16_t -D SPARSE_indexBits=16 -D SPARSE_maxRows=4096 -D SPARSE_maxCols=4096 -D SPARSE_accLatency=8 -D SPARSE_hbmMemBits=256 -D SPARSE_hbmChannels=16
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/ -I $(XFLIB_DIR)/L1/include/hw/ -I $(XFLIB_DIR)/L1/include/hw/cgSolver -I $(XFLIB_DIR)/L2/include/hw/cgSolver -I $(XFLIB_DIR)/L2/include/hw/cgSolver/spmv -I $(XFLIB_DIR)/L2/include/common/cgSolver -I $(XFLIB_DIR)/L2/include/hw -I $(XFLIB_DIR)/../sparse/L1/include/hw -I $(XFLIB_DIR)/../sparse/L2/include/hw/fp64 -I $(XFLIB_DIR)/.. -I $(XFLIB_DIR)/../blas/L2/include/memKernel/hw/xf_blas -I $(XFLIB_DIR)/../blas/L1/include/hw -I $(XFLIB_DIR)/L2/tests/cgSolver/krnl_cgSolver_spmv_jacobi_16ch/kernel

endif

######################### binary container global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/u280/'))
VPP_FLAGS_krnl_timer += --hls.clock 333000000:krnl_timer
VPP_FLAGS_krnl_duplicate += --hls.clock 333000000:krnl_duplicate
VPP_FLAGS_krnl_dupTk4 += --hls.clock 333000000:krnl_dupTk4
VPP_FLAGS_krnl_loadArbParam += --hls.clock 333000000:krnl_loadArbParam
VPP_FLAGS_krnl_loadAval += --hls.clock 333000000:krnl_loadAval
VPP_FLAGS_krnl_loadPkApar += --hls.clock 333000000:krnl_loadPkApar
VPP_FLAGS_krnl_storeApk += --hls.clock 333000000:krnl_storeApk
VPP_FLAGS_krnl_update_xk += --hls.clock 333000000:krnl_update_xk
VPP_FLAGS_krnl_update_rk_jacobi += --hls.clock 333000000:krnl_update_rk_jacobi
VPP_FLAGS_krnl_update_pk += --hls.clock 333000000:krnl_update_pk
VPP_FLAGS_krnl_control += --hls.clock 333000000:krnl_control
VPP_FLAGS_fwdParParamKernel += --hls.clock 333000000:fwdParParamKernel
VPP_FLAGS_moveXkernel += --hls.clock 333000000:moveXkernel
VPP_FLAGS_selMultXkernel += --hls.clock 333000000:selMultXkernel
VPP_FLAGS_rowAccKernel += --hls.clock 333000000:rowAccKernel
VPP_FLAGS_assembleYkernel += --hls.clock 333000000:assembleYkernel
ifneq ($(HOST_ARCH), x86)
VPP_LDFLAGS_cgSolver += --clock.defaultFreqHz 333000000
else
VPP_LDFLAGS_cgSolver += --kernel_frequency 300
endif
VPP_LDFLAGS_cgSolver_temp := --config $(CUR_DIR)/conn_u280.cfg
VPP_LDFLAGS_cgSolver += $(VPP_LDFLAGS_cgSolver_temp)

else 
VPP_FLAGS_krnl_timer += --hls.clock 333000000:krnl_timer
VPP_FLAGS_krnl_duplicate += --hls.clock 333000000:krnl_duplicate
VPP_FLAGS_krnl_dupTk4 += --hls.clock 333000000:krnl_dupTk4
VPP_FLAGS_krnl_loadArbParam += --hls.clock 333000000:krnl_loadArbParam
VPP_FLAGS_krnl_loadAval += --hls.clock 333000000:krnl_loadAval
VPP_FLAGS_krnl_loadPkApar += --hls.clock 333000000:krnl_loadPkApar
VPP_FLAGS_krnl_storeApk += --hls.clock 333000000:krnl_storeApk
VPP_FLAGS_krnl_update_xk += --hls.clock 333000000:krnl_update_xk
VPP_FLAGS_krnl_update_rk_jacobi += --hls.clock 333000000:krnl_update_rk_jacobi
VPP_FLAGS_krnl_update_pk += --hls.clock 333000000:krnl_update_pk
VPP_FLAGS_krnl_control += --hls.clock 333000000:krnl_control
VPP_FLAGS_fwdParParamKernel += --hls.clock 333000000:fwdParParamKernel
VPP_FLAGS_moveXkernel += --hls.clock 333000000:moveXkernel
VPP_FLAGS_selMultXkernel += --hls.clock 333000000:selMultXkernel
VPP_FLAGS_rowAccKernel += --hls.clock 333000000:rowAccKernel
VPP_FLAGS_assembleYkernel += --hls.clock 333000000:assembleYkernel
VPP_LDFLAGS_cgSolver_temp := --config $(CUR_DIR)/opts.cfg
VPP_LDFLAGS_cgSolver += $(VPP_LDFLAGS_cgSolver_temp)

endif
ifeq ($(HOST_ARCH), x86)
BINARY_CONTAINERS += $(BUILD_DIR)/cgSolver.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/cgSolver_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/cgSolver.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/krnl_timer.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/krnl_timer.cpp 
	$(ECHO) "Compiling Kernel: krnl_timer"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_timer) $(VPP_FLAGS) -k krnl_timer -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_timer.xo
$(TEMP_DIR)/krnl_duplicate.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/krnl_duplicate.cpp 
	$(ECHO) "Compiling Kernel: krnl_duplicate"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_duplicate) $(VPP_FLAGS) -k krnl_duplicate -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_duplicate.xo
$(TEMP_DIR)/krnl_dupTk4.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/spmv/krnl_dupTk4.cpp 
	$(ECHO) "Compiling Kernel: krnl_dupTk4"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_dupTk4) $(VPP_FLAGS) -k krnl_dupTk4 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_dupTk4.xo
$(TEMP_DIR)/krnl_loadArbParam.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/spmv/krnl_loadArbParam.cpp 
	$(ECHO) "Compiling Kernel: krnl_loadArbParam"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_loadArbParam) $(VPP_FLAGS) -k krnl_loadArbParam -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_loadArbParam.xo
$(TEMP_DIR)/krnl_loadAval.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/spmv/krnl_loadAval.cpp 
	$(ECHO) "Compiling Kernel: krnl_loadAval"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_loadAval) $(VPP_FLAGS) -k krnl_loadAval -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_loadAval.xo
$(TEMP_DIR)/krnl_loadPkApar.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/spmv/krnl_loadPkApar.cpp 
	$(ECHO) "Compiling Kernel: krnl_loadPkApar"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_loadPkApar) $(VPP_FLAGS) -k krnl_loadPkApar -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_loadPkApar.xo
$(TEMP_DIR)/krnl_storeApk.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/spmv/krnl_storeApk.cpp 
	$(ECHO) "Compiling Kernel: krnl_storeApk"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_storeApk) $(VPP_FLAGS) -k krnl_storeApk -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_storeApk.xo
$(TEMP_DIR)/krnl_update_xk.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/krnl_update_xk.cpp 
	$(ECHO) "Compiling Kernel: krnl_update_xk"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_update_xk) $(VPP_FLAGS) -k krnl_update_xk -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_update_xk.xo
$(TEMP_DIR)/krnl_update_rk_jacobi.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/krnl_update_rk_jacobi.cpp 
	$(ECHO) "Compiling Kernel: krnl_update_rk_jacobi"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_update_rk_jacobi) $(VPP_FLAGS) -k krnl_update_rk_jacobi -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_update_rk_jacobi.xo
$(TEMP_DIR)/krnl_update_pk.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/krnl_update_pk.cpp 
	$(ECHO) "Compiling Kernel: krnl_update_pk"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_update_pk) $(VPP_FLAGS) -k krnl_update_pk -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_update_pk.xo
$(TEMP_DIR)/krnl_control.xo: $(XFLIB_DIR)/L2/src/hw/cgSolver/krnl_control.cpp 
	$(ECHO) "Compiling Kernel: krnl_control"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_krnl_control) $(VPP_FLAGS) -k krnl_control -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/krnl_control.xo
$(TEMP_DIR)/fwdParParamKernel.xo: $(XFLIB_DIR)/../sparse/L2/src/hw/fp64/fwdParParamKernel.cpp 
	$(ECHO) "Compiling Kernel: fwdParParamKernel"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_fwdParParamKernel) $(VPP_FLAGS) -k fwdParParamKernel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/fwdParParamKernel.xo
$(TEMP_DIR)/moveXkernel.xo: $(XFLIB_DIR)/../sparse/L2/src/hw/fp64/moveXkernel.cpp 
	$(ECHO) "Compiling Kernel: moveXkernel"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_moveXkernel) $(VPP_FLAGS) -k moveXkernel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/moveXkernel.xo
$(TEMP_DIR)/selMultXkernel.xo: $(XFLIB_DIR)/../sparse/L2/src/hw/fp64/selMultXkernel.cpp 
	$(ECHO) "Compiling Kernel: selMultXkernel"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_selMultXkernel) $(VPP_FLAGS) -k selMultXkernel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/selMultXkernel.xo
$(TEMP_DIR)/rowAccKernel.xo: $(XFLIB_DIR)/../sparse/L2/src/hw/fp64/rowAccKernel.cpp 
	$(ECHO) "Compiling Kernel: rowAccKernel"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_rowAccKernel) $(VPP_FLAGS) -k rowAccKernel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/rowAccKernel.xo
$(TEMP_DIR)/assembleYkernel.xo: $(XFLIB_DIR)/../sparse/L2/src/hw/fp64/assembleYkernel.cpp 
	$(ECHO) "Compiling Kernel: assembleYkernel"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_assembleYkernel) $(VPP_FLAGS) -k assembleYkernel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_cgSolver_OBJS += $(TEMP_DIR)/assembleYkernel.xo
BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_cgSolver_OBJS)
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/cgSolver $(VPP_LDFLAGS)  $(VPP_LDFLAGS_cgSolver) $(AIE_LDFLAGS)   -o $@ $^

############################## Setting Rules for Host (Building Host Executable) ##############################
ifeq ($(HOST_ARCH), x86)
$(EXE_FILE): $(EXE_FILE_DEPS) |  check_xrt
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

else
$(EXE_FILE): $(EXE_FILE_DEPS) |  check_sysroot
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

endif

$(EMCONFIG):
	emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)
############################## Preparing sdcard folder ##############################
ifneq ($(HOST_ARCH), x86)
ifneq (,$(findstring zc706, $(PLATFORM_NAME)))
K_IMAGE := $(SYSROOT)/../../uImage
else
K_IMAGE := $(SYSROOT)/../../Image
endif
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
	@echo '        echo "ERROR: Embedded host run failed, RC=$$return_code"' >> $(RUN_SCRIPT)
	@echo 'else' >> $(RUN_SCRIPT)
	@echo '        echo "INFO: TEST PASSED, RC=0"' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo 'echo "INFO: Embedded host run completed."' >> $(RUN_SCRIPT)
	@echo 'exit $$return_code' >> $(RUN_SCRIPT)
DATA_FILE := 
DATA_DIR := 
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
vck190_dfx_hw := false
$(SD_CARD): $(EXE_FILE) $(BINARY_CONTAINERS) $(RUN_SCRIPT) $(EMCONFIG)
	@echo "Generating sd_card folder...."
	mkdir -p $(SD_CARD)
	chmod a+rx $(BUILD_DIR)/run_script.sh
ifneq (,$(findstring vck190_base_dfx, $(PLATFORM_NAME)))
ifeq ($(TARGET),hw)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p $(PACKAGE_FILES) -o $(BINARY_CONTAINERS_PKG)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p --package.out_dir  $(SD_CARD) --package.rootfs $(SYSROOT)/../../rootfs.ext4 --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX) --package.sd_file $(BINARY_CONTAINERS_PKG)
	@echo "### ***** sd_card generation done! ***** ###"
vck190_dfx_hw := true
endif
endif
ifeq ($(vck190_dfx_hw), false)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(SYSROOT)/../../rootfs.ext4 --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
endif

.PHONY: sd_card
sd_card: $(SD_CARD)
endif
############################## Setting Essential Checks and Building Rules ##############################
RUN_DEPS += $(EXE_FILE) $(BINARY_CONTAINERS) $(EMCONFIG)
RUN_DEPS += $(SD_CARD)

.PHONY: mkflag all run
mkflag:
	mkdir -p $(BUILD_DIR)
	rm -rf $(BUILD_DIR)/makefile_args.txt
	@for var in $(MAKEFLAGS); do echo $$var >> $(BUILD_DIR)/makefile_args.txt; done
all: check_device  check_vpp check_platform mkflag $(RUN_DEPS)
run: all
#hw_emu
ifneq (,$(filter hw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
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
ifneq ($(JENKINS_INTERNAL_BUILD), 1)
	$(ECHO) "This makefile does not directly support converting .xclbin to .awsxclbin, please refer https://github.com/aws/aws-fpga/blob/master/Vitis/README.md for next operations"
else
	$(ECHO) "Running inside Xilinx regression without converting to .awsxclbin"
	$(EXE_FILE) $(HOST_ARGS)
	
endif
else ifeq ($(HOST_ARCH), x86)
	$(EXE_FILE) $(HOST_ARGS)
	
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Setting Targets ##############################

.PHONY: clean cleanall emconfig
emconfig: $(EMCONFIG)

.PHONY: host
ifeq ($(HOST_ARCH), x86)
host:  check_xrt $(EXE_FILE)
else
host:  check_sysroot $(EXE_FILE)
endif

.PHONY: xclbin
ifeq ($(HOST_ARCH), x86)
xclbin:  check_vpp check_xrt $(BINARY_CONTAINERS) 
else
xclbin:  check_vpp check_sysroot $(BINARY_CONTAINERS) 
endif

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE) vitis_* TempConfig system_estimate.xtxt *.rpt .run/
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.*/_x.* _x_temp.*/.Xil _x_temp.*/profile_summary.* xo_* _x*
	-$(RMDIR) _x_temp.*/dltmp* _x_temp.*/kernel_info.dat _x_temp.*/*.log
	-$(RMDIR) _x_temp.* 

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR)  $(CUR_DIR)/Work $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut

clean: cleanh
