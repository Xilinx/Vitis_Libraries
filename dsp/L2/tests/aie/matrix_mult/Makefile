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
# vitis makefile-generator v2.0.8

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  make xclbin TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) ""
	$(ECHO) "  make host TARGET=<x86sim/aiesim/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) ""
	$(ECHO) "  NOTE: For embedded devices, e.g. zcu102/zcu104/vck190, HOST_ARCH is either aarch32 or aarch64."
	$(ECHO) "      a.IF Download the platform, and common-image from Xilinx Download Center(Suggested):"
	$(ECHO) "        Run the sdk.sh script from the common-image directory to install sysroot using the command : ./sdk.sh -y -d ./ -p "
	$(ECHO) "        Unzip the rootfs file : gunzip ./rootfs.ext4.gz"
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "      b. User could also define SYSROOT, K_IMAGE and ROOTFS by themselves: "
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "        export K_IMAGE=< path-to-Image-files >"
	$(ECHO) "        export ROOTFS=< path-to-rootfs >"
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
TARGET ?= aiesim
HOST_ARCH ?= aarch64

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_vck190_base_202220_1
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
XILINX_VITIS_AIETOOLS ?=
XILINX_VITIS_AIELIBS ?=
ifneq ($(wildcard $(XILINX_VITIS)/aietools/.*),)
XILINX_VITIS_AIETOOLS := $(XILINX_VITIS)/aietools
ifeq ($(HOST_ARCH), x86)
XILINX_VITIS_AIELIBS := -L$(XILINX_VITIS_AIETOOLS)/lib/lnx64.o -ladf_api_xrt
else
XILINX_VITIS_AIELIBS := -L$(XILINX_VITIS_AIETOOLS)/lib/aarch64.o -ladf_api_xrt
endif
else
XILINX_VITIS_AIETOOLS := $(XILINX_VITIS)/cardano
XILINX_VITIS_AIELIBS:= -lcardano_api_xrt -lxaiengine -lxrt_core
endif
CXXFLAGS += -I$(XILINX_VITIS_AIETOOLS)/include
LDFLAGS += $(XILINX_VITIS_AIELIBS)
# Setting customized_params in aiecompiler
TAG ?= UUT
UUT_KERNEL ?= matrix_mult
REF_KERNEL ?= matrix_mult_ref
UUT_GRAPH ?= matrix_mult_graph
REF_GRAPH ?= matrix_mult_ref_graph
NITER ?= 16
NITER_UUT ?= $(NITER)
NITER_REF ?= $(NITER)
INPUT_WINDOW_VSIZE ?= 256
UUT_INPUT_WINDOW_VSIZE ?= $(INPUT_WINDOW_VSIZE)
REF_INPUT_WINDOW_VSIZE ?= $$(( $(UUT_INPUT_WINDOW_VSIZE) * $(NITER_UUT) / $(NITER_REF)))
DATA_TYPE ?= cint16
SHIFT ?= 20
ROUND_MODE ?= 0
CASC_LEN ?= 1
P_CASC_LEN ?= 1
DIFF_TOLERANCE ?= 0
CC_TOLERANCE ?= 0
DATA_SEED ?= 1
DATA_STIM_TYPE ?= 0
STIM_TYPE_A ?= 0
STIM_TYPE_B ?= 0
STACKSIZE_VAL ?= 2048
T_DATA_A ?= cint16
T_DATA_B ?= $(T_DATA_A)
P_SHIFT ?= $(SHIFT)
P_ROUND_MODE ?= $(ROUND_MODE)
P_DIM_A ?= 16
P_DIM_AB ?= $(P_DIM_A)
P_DIM_B ?= $(P_DIM_A)
ROW_MAJOR ?= 0
COL_MAJOR ?= 1
P_INPUT_WINDOW_VSIZE_A ?= $$(( $(P_DIM_A) * $(P_DIM_AB)))
P_INPUT_WINDOW_VSIZE_B ?= $$(( $(P_DIM_B) * $(P_DIM_AB)))
P_ADD_TILING_A ?= 1
P_ADD_TILING_B ?= 1
P_ADD_DETILING_OUT ?= 1
NUM_MACS_PER_KERNEL ?= $(shell echo $$(( ( $(P_DIM_A) * $(P_DIM_AB) * $(P_DIM_B) ) / $(P_CASC_LEN) )))
P_DIM_A_LEADING ?= $(ROW_MAJOR)
P_DIM_B_LEADING ?= $(COL_MAJOR)
P_DIM_OUT_LEADING ?= $(ROW_MAJOR)
UUT_SIM_FILE ?= ./data/uut_output.txt
REF_SIM_FILE ?= ./data/ref_output.txt
UUT_SIM_FILE2 ?= ./data/uut_output_2.txt
REF_SIM_FILE2 ?= ./data/ref_output_2.txt
LOC_INPUT_FILE_A ?= ./data/inputA.txt
LOC_INPUT_FILE_B ?= ./data/inputB.txt
UUT_FILE_SUFFIX ?= $(UUT_KERNEL)_$(T_DATA_A)_$(T_DATA_B)_$(STIM_TYPE_A)_$(STIM_TYPE_B)_$(P_SHIFT)_$(P_ROUND_MODE)_$(P_DIM_A)_$(P_DIM_AB)_$(P_DIM_B)_$(P_INPUT_WINDOW_VSIZE_A)_$(P_INPUT_WINDOW_VSIZE_B)_$(P_ADD_TILING_A)_$(P_ADD_TILING_B)_$(P_ADD_DETILING_OUT)_$(P_DIM_A_LEADING)_$(P_DIM_B_LEADING)_$(P_DIM_OUT_LEADING)
LOG_FILE ?= ./logs/log_$(UUT_FILE_SUFFIX).txt
STATUS_LOG_FILE ?= ./logs/status_$(UUT_FILE_SUFFIX).txt
STATUS_FILE ?= $(STATUS_LOG_FILE)
UUT_TARGET_COMPILE_ARGS ?= -Xchess=llvm.xargs="-std=c++2a" -Xchess=main:backend.mist2.xargs="+NOdra"  -Xchess=main:backend.mist2.pnll="off" 
REF_TARGET_COMPILE_ARGS ?= 
UUT_WORK_DIR ?= ./Work
REF_WORK_DIR ?= ./Work
PREPROC_ARGS ?=  -DINPUT_FILE_A=$(LOC_INPUT_FILE_A) -DINPUT_FILE_B=$(LOC_INPUT_FILE_B) -DT_DATA_A=$(T_DATA_A) -DT_DATA_B=$(T_DATA_B) -DP_DIM_A=$(P_DIM_A) -DP_DIM_AB=$(P_DIM_AB) -DP_DIM_B=$(P_DIM_B) -DP_SHIFT=$(P_SHIFT) -DP_ROUND_MODE=$(P_ROUND_MODE) -DP_DIM_A_LEADING=$(P_DIM_A_LEADING) -DP_DIM_B_LEADING=$(P_DIM_B_LEADING) -DP_DIM_OUT_LEADING=$(P_DIM_OUT_LEADING) -DP_ADD_TILING_A=$(P_ADD_TILING_A) -DP_ADD_TILING_B=$(P_ADD_TILING_B) -DP_ADD_DETILING_OUT=$(P_ADD_DETILING_OUT) -DP_INPUT_WINDOW_VSIZE_A=$(P_INPUT_WINDOW_VSIZE_A) -DP_INPUT_WINDOW_VSIZE_B=$(P_INPUT_WINDOW_VSIZE_B) -DP_CASC_LEN=$(P_CASC_LEN) -DSTIM_TYPE_A=$(STIM_TYPE_A) -DSTIM_TYPE_B=$(STIM_TYPE_B) -DNITER=$(NITER)
REF_PREPROC_ARGS ?= -DUUT_GRAPH=$(REF_GRAPH) -DOUTPUT_FILE=$(REF_SIM_FILE) $(PREPROC_ARGS)
UUT_PREPROC_ARGS ?= -DUUT_GRAPH=$(UUT_GRAPH) -DOUTPUT_FILE=$(UUT_SIM_FILE) $(PREPROC_ARGS) -DUSING_UUT=1
PARAM_MAP ?= T_DATA_A $(T_DATA_A) T_DATA_B $(T_DATA_B) P_DIM_A $(P_DIM_A) P_DIM_AB $(P_DIM_AB) P_DIM_B $(P_DIM_B) P_SHIFT $(P_SHIFT) P_ROUND_MODE $(P_ROUND_MODE) P_DIM_A_LEADING $(P_DIM_A_LEADING) P_DIM_B_LEADING $(P_DIM_B_LEADING) P_DIM_OUT_LEADING $(P_DIM_OUT_LEADING) P_ADD_TILING_A $(P_ADD_TILING_A) P_ADD_TILING_B $(P_ADD_TILING_B) P_ADD_DETILING_OUT $(P_ADD_DETILING_OUT) P_INPUT_WINDOW_VSIZE_A $(P_INPUT_WINDOW_VSIZE_A) P_INPUT_WINDOW_VSIZE_B $(P_INPUT_WINDOW_VSIZE_B) P_CASC_LEN $(P_CASC_LEN)

############################ setting AIE Compiler ###########################
ifneq ($(filter aiesim hw_emu hw, $(TARGET)),)
AIETARGET := hw
else
AIETARGET := x86sim
endif
AIE_CXXFLAGS += --target=$(AIETARGET) --platform=$(XPLATFORM) --pl-freq=1000 
AIE_CXXFLAGS += -I $(XILINX_VITIS_AIETOOLS)/include
AIE_CONTAINER = $(TEMP_DIR)/libadf.a
AIE_CXXFLAGS += -I $(XFLIB_DIR)/L1/include/aie
AIE_CXXFLAGS += -I $(XFLIB_DIR)/L1/src/aie
AIE_CXXFLAGS += -I $(XFLIB_DIR)/L1/tests/aie/inc
AIE_CXXFLAGS += -I $(XFLIB_DIR)/L1/tests/aie/src
AIE_CXXFLAGS += -I $(XFLIB_DIR)/L2/include/aie
AIE_CXXFLAGS += -I $(XFLIB_DIR)/L2/tests/aie/common/inc
AIE_CXXFLAGS += -I $(CUR_DIR)
AIE_CXXFLAGS += --dataflow -v $($(TAG)_TARGET_COMPILE_ARGS) -workdir=$($(TAG)_WORK_DIR) -Xpreproc="$($(TAG)_PREPROC_ARGS)"
$(CUR_DIR)/Work/ps/c_rts/aie_control_xrt.cpp: $(AIE_CONTAINER)

EXE_FILE_DEPS += $(AIE_CONTAINER)
BINARY_CONTAINERS_DEPS += $(AIE_CONTAINER)
########################## Setting up Host Variables ##########################
ifeq ($(TARGET),sw_emu)
CXXFLAGS += -D SW_EMU_TEST
endif
ifeq ($(TARGET),hw_emu)
CXXFLAGS += -D HW_EMU_TEST
endif

#Inclue Required Host Source Files
ifeq ($(ps_on_x86), on)
HOST_SRCS += 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/hw
else
HOST_SRCS += 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/hw
endif

# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := 
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(EXE_FILE_DEPS)

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/vck190/'))
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw
VPP_PACKAGE +=   --package.defer_aie_run

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw

endif

######################### binary container global settings ##########################

############################## Setting Rules for AIE (Building Kernels) ##################
$(AIE_CONTAINER): $(CUR_DIR)//test.cpp $(XFLIB_DIR)/L1/src/aie/matrix_mult_untiler.cpp $(XFLIB_DIR)/L1/src/aie/matrix_mult_tiler.cpp $(XFLIB_DIR)/L1/src/aie/matrix_mult.cpp  | pre_build
	$(ECHO) "Compiling: libadf.a"
	mkdir -p $(dir $@)
	$(AIECXX) $(AIE_CXXFLAGS) $^
	mv ./libadf.a $(AIE_CONTAINER)

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXE_FILE): $(EXE_FILE_DEPS)
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

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
#3. General Embeded flow
ifeq ($(dfx_hw), off)
ifeq ($(ps_on_x86), off)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE) --package.boot_mode sd  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
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
	$(X86SIMULATOR) --pkg-dir=./Work 
	$(if $(filter-out REF,$(TAG)), mkdir -p logs)
	$(if $(filter-out REF,$(TAG)), grep -ve '[XT]' ./x86simulator_output/data/uut_output.txt > ./logs/uut_output.txt)
	$(if $(filter-out REF,$(TAG)), grep -ve '[XT]' ./x86simulator_output/data/ref_output.txt > ./logs/ref_output.txt)
	$(if $(filter-out REF,$(TAG)), perl $(XFLIB_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --untile --inFile ./logs/uut_output.txt --tileInPlace --inRow $(P_DIM_A) --inCol $(P_DIM_B) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --colMajor $(P_DIM_OUT_LEADING) --isTiled $(P_ADD_DETILING_OUT) )
	$(if $(filter-out REF,$(TAG)), tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE))
	$(if $(filter-out REF,$(TAG)), tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff_exit.tcl ./)
endif
#aiesim
ifeq ($(TARGET), aiesim)
	$(AIESIMULATOR) --pkg-dir=./Work --profile 
	mkdir -p logs
	grep -ve '[XT]' ./aiesimulator_output/data/uut_output.txt > ./logs/uut_output.txt
	grep -ve '[XT]' ./x86simulator_output/data/ref_output.txt > ./logs/ref_output.txt
	perl $(XFLIB_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --untile --inFile ./logs/uut_output.txt --tileInPlace --inRow $(P_DIM_A) --inCol $(P_DIM_B) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --colMajor $(P_DIM_OUT_LEADING) --isTiled $(P_ADD_DETILING_OUT) 
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) STIM_TYPE_A $(STIM_TYPE_A) STIM_TYPE_B $(STIM_TYPE_B) NITER $(NITER)
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(NUM_MACS_PER_KERNEL) $(P_CASC_LEN) $(STATUS_FILE) ./aiesimulator_output matMult_impl1
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_mtx_theoretical_min.tcl $(T_DATA_A) $(T_DATA_B) $(NUM_MACS_PER_KERNEL) $(P_CASC_LEN) $(STATUS_FILE) $(UUT_KERNEL)
	$(XFLIB_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(XFLIB_DIR)/L2/tests/aie/common/scripts
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff_exit.tcl ./
endif
#hw_emu
ifneq (,$(filter hw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS)
	
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT))  2>&1 | tee $(SD_CARD)/qemu_stdout.log
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_stdout.log || exit 1
	
endif
endif
#sw_emu
ifneq (,$(filter sw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS) 
	
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT))  2>&1 | tee $(SD_CARD)/qemu_stdout.log
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_stdout.log || exit 1
	
endif
endif
#hw
ifeq ($(TARGET), hw)
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(HOST_ARGS)
	
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Setting Targets ##############################
.PHONY: pre_build
pre_build:
	$(if $(filter-out REF,$(TAG)), tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP))
	$(if $(filter-out REF,$(TAG)), vitis -exec ipmetadata_config_checker $(XFLIB_DIR)/L2/meta/$(UUT_KERNEL).json ./config.json -newflow)
	$(if $(filter-out REF,$(TAG)), make -f $(CUR_DIR)/golden.mk GOLDEN_CUR_DIR=$(CUR_DIR) GOLDEN_ROOT_DIR=$(XFLIB_DIR) LOC_INPUT_FILE_A=$(LOC_INPUT_FILE_A) LOC_INPUT_FILE_B=$(LOC_INPUT_FILE_B) P_INPUT_WINDOW_VSIZE_A=$(P_INPUT_WINDOW_VSIZE_A) STIM_TYPE_A=$(STIM_TYPE_A) INPUT_FILE_B=$(INPUT_FILE_B) P_INPUT_WINDOW_VSIZE_B=$(P_INPUT_WINDOW_VSIZE_B) STIM_TYPE_B=$(STIM_TYPE_B) NITER_UUT=$(NITER_UUT) P_DIM_A=$(P_DIM_A) P_DIM_AB=$(P_DIM_AB) T_DATA_A=$(T_DATA_A) T_DATA_B=$(T_DATA_B) P_CASC_LEN=$(P_CASC_LEN) P_DIM_A_LEADING=$(P_DIM_A_LEADING) P_ADD_TILING_A=$(P_ADD_TILING_A) P_DIM_B=$(P_DIM_B) P_DIM_B_LEADING=$(P_DIM_B_LEADING) P_ADD_TILING_B=$(P_ADD_TILING_B) )

.PHONY: clean cleanall emconfig
emconfig: $(EMCONFIG)

.PHONY: host
ifeq ($(HOST_ARCH), x86)
host: check_xrt  pre_build $(EXE_FILE)
else
host: check_sysroot  pre_build $(EXE_FILE)
endif

.PHONY: xclbin
ifeq ($(HOST_ARCH), x86)
xclbin: check_vpp check_xrt pre_build $(BINARY_CONTAINERS)
else
xclbin: check_vpp check_sysroot pre_build $(BINARY_CONTAINERS)
endif

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE) vitis_* TempConfig system_estimate.xtxt *.rpt .run/
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.*

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR) sd_card* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR) $(AIE_CONTAINER) aiesimulator_output .AIE_SIM_CMD_LINE_OPTIONS $(CUR_DIR)/Work $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut
	-$(RMDIR) 

clean: cleanh