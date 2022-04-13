#
# Copyright 2022 Xilinx, Inc.
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
	$(ECHO) "  make all TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform> HOST_ARCH=<aarch64>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=aarch64. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make sd_card TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform> HOST_ARCH=<aarch64>"
	$(ECHO) "      Command to prepare sd_card files."
	$(ECHO) "      By default, HOST_ARCH=aarch64. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform> HOST_ARCH=<aarch64>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=aarch64. HOST_ARCH required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make xclbin TARGET=<x86sim/aiesim/> PLATFORM=<FPGA platform> HOST_ARCH=<aarch64>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) "      By default, HOST_ARCH=aarch64. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make host TARGET=<x86sim/aiesim/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=aarch64. HOST_ARCH is required for SoC shells"
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
TARGET ?= aiesim
HOST_ARCH ?= aarch64

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_vck190_base_202210_1
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

# aie template
XILINX_VITIS_AIETOOLS ?=
XILINX_VITIS_AIELIBS ?=
ifneq ($(wildcard $(XILINX_VITIS)/aietools/.*),)
XILINX_VITIS_AIETOOLS := $(XILINX_VITIS)/aietools
XILINX_VITIS_AIELIBS := -ladf_api_xrt
else
XILINX_VITIS_AIETOOLS := $(XILINX_VITIS)/cardano
XILINX_VITIS_AIELIBS:= -lcardano_api_xrt -lxaiengine -lxrt_core
endif
CXXFLAGS += -I$(XILINX_VITIS_AIETOOLS)/include
LDFLAGS += $(XILINX_VITIS_AIELIBS)
# Setting customized_params in aiecompiler
TAG ?= UUT
UUT_KERNEL ?= fir_interpolate_asym
REF_KERNEL ?= fir_interpolate_asym_ref
UUT_GRAPH ?= fir_interpolate_asym_graph
REF_GRAPH ?= fir_interpolate_asym_ref_graph
NITER ?= 16
NITER_UUT ?= $(NITER)
NITER_REF ?= $$(($(NITER_UUT) / 2))
INPUT_WINDOW_VSIZE ?= 256
UUT_INPUT_WINDOW_VSIZE ?= $(INPUT_WINDOW_VSIZE)
REF_INPUT_WINDOW_VSIZE ?= $$(( $(UUT_INPUT_WINDOW_VSIZE) * $(NITER_UUT) / $(NITER_REF)))
DATA_TYPE ?= cint16
COEFF_TYPE ?= int16
COEFF_SIZE ?= 2
FIR_LEN ?= 9
SHIFT ?= 16
ROUND_MODE ?= 0
CASC_LEN ?= 1
DUAL_IP ?= 0
PORT_API ?= 0
USE_COEFF_RELOAD ?= 0
SYMMETRY_FACTOR ?= 1
INTERPOLATE_FACTOR ?= 3
DECIMATE_FACTOR ?= 1
NUM_OUTPUTS ?= 1
USE_CHAIN ?= 0
USE_CUSTOM_CONSTRAINT ?= 0
UUT_SSR ?= 1
DUAL_INPUT_SAMPLES ?= $$(( $(PORT_API) * $(DUAL_IP) ))
DUAL_OUTPUT_SAMPLES ?= $$(( $(PORT_API) * ($(NUM_OUTPUTS)-1) ))
DIFF_TOLERANCE ?= 0
CC_TOLERANCE ?= 0
DATA_SEED ?= 1
DATA_STIM_TYPE ?= 0
COEFF_STIM_TYPE ?= 0
UUT_SIM_FILE ?= ./data/uut_output.txt
REF_SIM_FILE ?= ./data/ref_output.txt
UUT_SIM_FILE2 ?= ./data/uut_output_2.txt
REF_SIM_FILE2 ?= ./data/ref_output_2.txt
LOC_COEFF_FILE ?= ./data/coeff.txt
LOC_INPUT_FILE ?= ./data/input.txt
UUT_FILE_SUFFIX ?= $(UUT_KERNEL)_$(DATA_TYPE)_$(COEFF_TYPE)_$(FIR_LEN)_$(SHIFT)_$(ROUND_MODE)_$(CASC_LEN)_$(DUAL_IP)_$(USE_COEFF_RELOAD)_$(SYMMETRY_FACTOR)_$(INTERPOLATE_FACTOR)_$(DECIMATE_FACTOR)_$(NUM_OUTPUTS)_$(PORT_API)
LOG_FILE ?= ./logs/log_$(UUT_FILE_SUFFIX).txt
STATUS_LOG_FILE ?= ./logs/status_$(UUT_FILE_SUFFIX).txt
STATUS_FILE ?= $(STATUS_LOG_FILE)
MAX_FIR_PER_KERNEL ?= 256
UUT_TARGET_COMPILE_ARGS ?= --xlopt=1 -stacksize=$$(( $(MAX_FIR_PER_KERNEL) * $(INTERPOLATE_FACTOR) * $(COEFF_SIZE) + 1536))  -Xchess=llvm.xargs="-std=c++2a" -Xchess=main:backend.mist2.xargs="+NOdra"  
REF_TARGET_COMPILE_ARGS ?= 
UUT_WORK_DIR ?= ./Work
REF_WORK_DIR ?= ./Work
PREPROC_ARGS ?= 
USING_REF ?= 
USING_UUT ?= -DUSING_UUT=1
TAG_PREPROC_ARGS ?= -DUUT_GRAPH=$($(TAG)_GRAPH) -DOUTPUT_FILE=$($(TAG)_SIM_FILE) -DOUTPUT_FILE2=$($(TAG)_SIM_FILE2) $(PREPROC_ARGS)  -DINPUT_WINDOW_VSIZE=$($(TAG)_INPUT_WINDOW_VSIZE) -DNITER=$(NITER_$(TAG)) $(USING_$(TAG))

############################ setting AIE Compiler ###########################
ifneq ($(filter aiesim hw_emu, $(TARGET)),)
AIETARGET := hw
else
AIETARGET := $(TARGET)
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
AIE_CXXFLAGS += --dataflow -v $($(TAG)_TARGET_COMPILE_ARGS) -workdir=$($(TAG)_WORK_DIR) -Xpreproc="$(TAG_PREPROC_ARGS) -DCOEFF_FILE=$(LOC_COEFF_FILE) -DINPUT_FILE=$(LOC_INPUT_FILE) -DDATA_TYPE=$(DATA_TYPE) -DCOEFF_TYPE=$(COEFF_TYPE) -DFIR_LEN=$(FIR_LEN) -DSHIFT=$(SHIFT) -DROUND_MODE=$(ROUND_MODE) -DCASC_LEN=$(CASC_LEN) -DDUAL_IP=$(DUAL_IP) -DPORT_API=$(PORT_API) -DUSE_COEFF_RELOAD=$(USE_COEFF_RELOAD) -DSTIM_TYPE=$(COEFF_STIM_TYPE) -DINTERPOLATE_FACTOR=$(INTERPOLATE_FACTOR) -DNUM_OUTPUTS=$(NUM_OUTPUTS) -DUSE_CHAIN=$(USE_CHAIN) -DUSE_CUSTOM_CONSTRAINT=$(USE_CUSTOM_CONSTRAINT) "
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

ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

#Inclue Required Host Source Files
HOST_SRCS += 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/hw

EXE_NAME := 
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(EXE_FILE_DEPS)

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/vck190/'))
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw
VPP_LDFLAGS +=   --config $(CUR_DIR)/system.cfg

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw

endif

######################### binary container global settings ##########################

############################## Setting Rules for AIE (Building Kernels) ##################
$(AIE_CONTAINER): $(XFLIB_DIR)/L2/tests/aie/fir_interpolate_asym/test.cpp $(XFLIB_DIR)/L1/src/aie/fir_interpolate_asym.cpp 
	$(ECHO) "Compiling: libadf.a"
	mkdir -p $(dir $@)
	$(AIECXX) $(AIE_CXXFLAGS) $^
	mv ./libadf.a $(AIE_CONTAINER)

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
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p --package.out_dir  $(SD_CARD) --package.rootfs $(SYSROOT)/../../rootfs.ext4 --package.kernel_image $(K_IMAGE) --package.boot_mode sd  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX) --package.sd_file $(BINARY_CONTAINERS_PKG)
	@echo "### ***** sd_card generation done! ***** ###"
vck190_dfx_hw := true
endif
endif
ifeq ($(vck190_dfx_hw), false)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(SYSROOT)/../../rootfs.ext4 --package.kernel_image $(K_IMAGE) --package.boot_mode sd  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
endif

.PHONY: sd_card
sd_card: $(SD_CARD)
endif
############################## Setting Essential Checks and Building Rules ##############################
ifneq (,$(filter x86sim aiesim, $(TARGET)))
RUN_DEPS += $(AIE_CONTAINER)
else
RUN_DEPS += $(EXE_FILE) $(BINARY_CONTAINERS) $(EMCONFIG)
RUN_DEPS += $(SD_CARD)
endif

.PHONY: mkflag all run
mkflag:
	mkdir -p $(BUILD_DIR)
	rm -rf $(BUILD_DIR)/makefile_args.txt
	@for var in $(MAKEFLAGS); do echo $$var >> $(BUILD_DIR)/makefile_args.txt; done
all: check_device pre_build check_vpp check_platform mkflag $(RUN_DEPS)
run: all
#x86sim
ifeq ($(TARGET), x86sim)
	$(X86SIMULATOR) --pkg-dir=./Work --stop-on-deadlock
	mkdir -p logs
	perl $(XFLIB_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file ./x86simulator_output/data/uut_output.txt --type $(DATA_TYPE) --ssr $(UUT_SSR) --zip --dual $(DUAL_OUTPUT_SAMPLES)
	grep -ve '[XT]' ./x86simulator_output/data/uut_output.txt > ./logs/uut_output.txt
	grep -ve '[XT]' ./x86simulator_output/data/ref_output.txt > ./logs/ref_output.txt
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)
endif
#aiesim
ifeq ($(TARGET), aiesim)
	$(AIESIMULATOR) --pkg-dir=./Work --profile 
	mkdir -p logs
	perl $(XFLIB_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file ./aiesimulator_output/data/uut_output.txt --type $(DATA_TYPE) --ssr $(UUT_SSR) --zip --dual $(DUAL_OUTPUT_SAMPLES)
	grep -ve '[XT]' ./aiesimulator_output/data/uut_output.txt > ./logs/uut_output.txt
	grep -ve '[XT]' ./x86simulator_output/data/ref_output.txt > ./logs/ref_output.txt
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) DATA_TYPE $(DATA_TYPE) COEFF_TYPE $(COEFF_TYPE) FIR_LEN $(FIR_LEN) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE) CASC_LEN $(CASC_LEN) INTERPOLATE_FACTOR $(INTERPOLATE_FACTOR) DECIMATE_FACTOR $(DECIMATE_FACTOR) SYMMETRY_FACTOR $(SYMMETRY_FACTOR) DUAL_IP $(DUAL_IP) USE_COEFF_RELOAD $(USE_COEFF_RELOAD) NUM_OUTPUTS $(NUM_OUTPUTS) UUT_SSR $(UUT_SSR)
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output filter $(NITER_UUT)
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_fir_theoretical_min.tcl $(DATA_TYPE) $(COEFF_TYPE) $(FIR_LEN) $(INPUT_WINDOW_VSIZE) $(CASC_LEN) $(INTERPOLATE_FACTOR) $(DECIMATE_FACTOR) $(SYMMETRY_FACTOR) $(UUT_SSR) $(STATUS_FILE) $(UUT_KERNEL)
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
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(HOST_ARGS)
	
endif
else ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(HOST_ARGS)
	
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Setting Targets ##############################
.PHONY: pre_build
pre_build:
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER_UUT) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) $(PORT_API) 1
	perl $(XFLIB_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(LOC_INPUT_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --split --dual $(DUAL_INPUT_SAMPLES)
	make x86sim TARGET=x86sim TAG=REF
	make cleanall

.PHONY: clean cleanall emconfig
emconfig: $(EMCONFIG)

.PHONY: host
ifeq ($(HOST_ARCH), x86)
host:  pre_build check_xrt $(EXE_FILE)
else
host:  pre_build check_sysroot $(EXE_FILE)
endif

.PHONY: xclbin
ifeq ($(HOST_ARCH), x86)
xclbin: pre_build check_vpp check_xrt $(BINARY_CONTAINERS) $(AIE_CONTAINER)
else
xclbin: pre_build check_vpp check_sysroot $(BINARY_CONTAINERS) $(AIE_CONTAINER)
endif

.PHONY: x86sim
x86sim: $(AIE_CONTAINER)
	$(X86SIMULATOR) --pkg-dir=./Work

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