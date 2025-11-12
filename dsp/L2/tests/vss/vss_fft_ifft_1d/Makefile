# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
# vitis makefile-generator v2.0.10

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw_emu/hw> PLATFORM=<FPGA platform>"
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
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L2/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)
SCRIPTS_DIR := ${CUR_DIR}/scripts_mk
VITIS_LIB_ROOT = $(dir $(XF_PROJ_ROOT))
export XF_PROJ_ROOT
export VITIS_LIB_ROOT

# setting default value
TARGET ?= hw_emu
PARAMS ?= test_0_tool_canary_aie
PARAMS_FILE ?= multi_params.json 

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := vck190
endif

#setting SYSROOT
ifneq (,$(findstring vek385, $(PLATFORM)))
ifeq (,$(wildcard $(YOCTO_ARTIFACTS)))
$(error YOCTO_ARTIFACTS ENV variable is not set, please set ENV variable correctly and rerun)
endif
export YOCTO_ARTIFACTS
export SYSROOT = ${YOCTO_ARTIFACTS}/amd-cortexa78-mali-common_meta-edf-app-sdk/sdk/sysroots/cortexa72-cortexa53-amd-linux
VPP_FLAGS_CFG += --part xc2ve3858-ssva2112-2MP-e-S
else
VPP_FLAGS_CFG += -t hw --platform $(XPLATFORM)
endif

# add warnning for sw_emu
ifeq ($(TARGET),sw_emu)
$(error Error: The sw_emu target is no longer supported starting from 2025.1.)
endif
# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST += vck190 vek280 vek385
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
CXXFLAGS += -fmessage-length=0 -I$(CUR_DIR)/src/ -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include -std=c++17 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -L$(XILINX_HLS)/lnx64/tools/fpo_v7_1 -Wl,--as-needed -lOpenCL -lxrt_coreutil -lgmp -lmpfr -lIp_floating_point_v7_1_bitacc_cmodel 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
else ifeq ($(HOST_ARCH), aarch64)
CXXFLAGS += -I$(CUR_DIR)/src/ -fmessage-length=0 --sysroot=$(SYSROOT)  -I$(SYSROOT)/usr/include/xrt -I$(XILINX_HLS)/include -std=c++17 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
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

TAG ?= UUT
UUT_KERNEL ?= fft_ifft_dit_1ch
REF_KERNEL ?= fft_ifft_dit_1ch_ref
UUT_GRAPH ?= vss_fft_ifft_1d_graph
REF_GRAPH ?= fft_ifft_dit_1ch_ref_graph
STACKSIZE_VAL ?= 16384
UUT_SIM_FILE ?= ./data/uut_output.txt
REF_SIM_FILE ?= ./data/ref_output.txt
FRONT_INPUT_FILE ?= ./data/input_front.txt
BACK_INPUT_FILE ?= ./data/input_back.txt
FRONT_OUTPUT_FILE ?= ./data/output_front.txt
BACK_OUTPUT_FILE ?= ./data/output_back.txt
INPUT_FILE ?= ./data/input.txt
REF_INPUT_FILE ?= ./data/input_ref.txt
UUT_TARGET_COMPILE_ARGS ?= --aie.Xchess=llvm.xargs="-std=c++2a" --aie.Xchess=main:backend.mist2.xargs="+NOdra" --aie.Xchess=main:backend.mist2.pnll="off" --aie.stacksize=$(STACKSIZE_VAL)
REF_TARGET_COMPILE_ARGS ?= 
REF_PREPROC_ARGS ?= --aie.Xpreproc=-DUUT_GRAPH=$(REF_GRAPH) --aie.Xpreproc=-DOUTPUT_FILE=$(REF_SIM_FILE) --aie.Xpreproc=-DINPUT_FILE=$(REF_INPUT_FILE) $(PREPROC_ARGS) 
UUT_PREPROC_ARGS ?= --aie.Xpreproc=-DUUT_GRAPH=$(UUT_GRAPH) --aie.Xpreproc=-DOUTPUT_FILE=$(UUT_SIM_FILE) --aie.Xpreproc=-DFRONT_INPUT_FILE=$(FRONT_INPUT_FILE) --aie.Xpreproc=-DBACK_INPUT_FILE=$(BACK_INPUT_FILE) --aie.Xpreproc=-DFRONT_OUTPUT_FILE=$(FRONT_OUTPUT_FILE) --aie.Xpreproc=-DBACK_OUTPUT_FILE=$(BACK_OUTPUT_FILE) $(PREPROC_ARGS) --aie.Xpreproc=-DUSING_UUT=1

############################ setting AIE Compiler ###########################
ifneq ($(filter aiesim hw_emu hw, $(TARGET)),)
AIETARGET := hw
else
AIETARGET := x86sim
endif

AIE_CXXFLAGS += -I $(XFLIB_DIR)/L1/include/aie -I $(XFLIB_DIR)/L1/src/aie -I $(XFLIB_DIR)/L1/tests/aie/inc -I $(XFLIB_DIR)/L1/tests/aie/src -I $(XFLIB_DIR)/L2/include/aie -I $(XFLIB_DIR)/L2/tests/aie/common/inc -I $(CUR_DIR)
AIE_CXXFLAGS += --aie.verbose  $($(TAG)_TARGET_COMPILE_ARGS)  $($(TAG)_PREPROC_ARGS)

$(AIE_WORK_DIR)/ps/c_rts/aie_control_xrt.cpp: $(AIE_CONTAINER)

EXE_FILE_DEPS += $(AIE_CONTAINER)
BINARY_CONTAINERS_DEPS += $(AIE_CONTAINER)  
AIESIMFLAGS += $(EXTRA_AIESIMFLAGS)

########################## Setting up Host Variables ##########################

#Inclue Required Host Source Files
HOST_SRCS += host.cpp 
CXXFLAGS +=  -D __PS_ENABLE_AIE__ -D USING_UUT=1
CXXFLAGS +=  -I $(SYSROOT)/usr/include/xrt/ -I $(XFLIB_DIR)/L2/include/aie -I $(XFLIB_DIR)/L2/tests/aie/common/inc -I $(XFLIB_DIR)/L1/include/aie -I $(XFLIB_DIR)/L1/include/vss/common -I $(XFLIB_DIR)/L1/include/vss/vss_fft_ifft_1d -I $(XFLIB_DIR)/L1/src/aie -I $(XFLIB_DIR)/L1/tests/aie/inc -I $(XFLIB_DIR)/L1/tests/aie/src -I PROJECT -I $(XFLIB_DIR)/L1/include/hw
CXXFLAGS += --sysroot=$(SYSROOT) -DPOINT_SIZE=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "POINT_SIZE")  -DNITER=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "NITER")   -DSSR=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "SSR") -DTT_DATA=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "DATA_TYPE")
LDFLAGS +=  -L $(SYSROOT)/usr/lib/
LDFLAGS += --sysroot=$(SYSROOT)

# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := host.elf
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(INSTANCE_FILES)  $(EXE_FILE_DEPS)

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/vck190/'))
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw
VPP_LDFLAGS +=   --config $(CUR_DIR)/system.cfg
VPP_PACKAGE +=   --package.defer_aie_run

else ifneq (,$(shell echo $(XPLATFORM) | awk '/vek280/'))
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw
VPP_LDFLAGS +=   --config $(CUR_DIR)/system.cfg
VPP_PACKAGE +=   --package.defer_aie_run

else ifneq (,$(shell echo $(XPLATFORM) | awk '/vek385/'))
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw
VPP_LDFLAGS +=   --config $(CUR_DIR)/system.cfg
VPP_PACKAGE +=   --package.defer_aie_run

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw

endif

######################### binary container global settings ##########################
VPP_FLAGS_mm2s_wrapper += -DNSTREAM=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "SSR") -DPOINT_SIZE=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "POINT_SIZE") -DNITER=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "NITER") -DDATAWIDTH=$(DATAWIDTH)
VPP_FLAGS_s2mm_wrapper += -DNSTREAM=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "SSR") -DPOINT_SIZE=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "POINT_SIZE") -DNITER=$(shell $(VITIS_PYTHON3) paramset.py $(PARAMS_FILE) $(PARAMS) "NITER") -DDATAWIDTH=$(DATAWIDTH)

ifneq ($(PACKAGE_NEEDED), on)
BINARY_CONTAINERS += $(BUILD_DIR)/kernel.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/kernel_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/kernel.xclbin
BINARY_NAME := kernel
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################

ifeq ($(wildcard hls_config_mm2s_wrapper.tmpl),)
-include gen_param.mk
endif
$(TEMP_DIR)/mm2s_wrapper/mm2s_wrapper.xo: $(XFLIB_DIR)/L1/tests/hw/mm2s/mm2s.cpp 
	$(ECHO) "Compiling Kernel: mm2s_wrapper"
	mkdir -p $(TEMP_DIR)
ifeq ($(wildcard hls_config_mm2s_wrapper.tmpl),)
	$(VPP) -c $(VPP_FLAGS_mm2s_wrapper) $(VPP_FLAGS) -k mm2s_wrapper -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
else
	set hls_config_mm2s_wrapper.tmpl hls_config_mm2s_wrapper.cfg && \
	    printf '%s\n' "$$CONFIG_GEN_PY" > gen_config.py && \
	    ${PYTHON3} gen_config.py \
		hls_config_mm2s_wrapper.tmpl \
		hls_config_mm2s_wrapper.cfg && \
	    rm -f gen_config.py
	$(VPP) -c --mode hls --config hls_config_mm2s_wrapper.cfg --work_dir $(TEMP_DIR)/mm2s_wrapper $(VPP_FLAGS_CFG)
endif
BINARY_CONTAINER_kernel_OBJS += $(TEMP_DIR)/mm2s_wrapper/mm2s_wrapper.xo
ifeq ($(wildcard hls_config_s2mm_wrapper.tmpl),)
-include gen_param.mk
endif
$(TEMP_DIR)/s2mm_wrapper/s2mm_wrapper.xo: $(XFLIB_DIR)/L1/tests/hw/s2mm/s2mm.cpp 
	$(ECHO) "Compiling Kernel: s2mm_wrapper"
	mkdir -p $(TEMP_DIR)
ifeq ($(wildcard hls_config_s2mm_wrapper.tmpl),)
	$(VPP) -c $(VPP_FLAGS_s2mm_wrapper) $(VPP_FLAGS) -k s2mm_wrapper -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
else
	set hls_config_s2mm_wrapper.tmpl hls_config_s2mm_wrapper.cfg && \
	    printf '%s\n' "$$CONFIG_GEN_PY" > gen_config.py && \
	    ${PYTHON3} gen_config.py \
		hls_config_s2mm_wrapper.tmpl \
		hls_config_s2mm_wrapper.cfg && \
	    rm -f gen_config.py
	$(VPP) -c --mode hls --config hls_config_s2mm_wrapper.cfg --work_dir $(TEMP_DIR)/s2mm_wrapper $(VPP_FLAGS_CFG)
endif
BINARY_CONTAINER_kernel_OBJS += $(TEMP_DIR)/s2mm_wrapper/s2mm_wrapper.xo

BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_kernel_OBJS) vss_fft_ifft_1d/vss_fft_ifft_1d.vss
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/kernel $(VPP_LDFLAGS)  $(VPP_LDFLAGS_kernel) $(AIE_LDFLAGS)   -o $@ $^

############################## Setting Rules for AIE (Building Kernels) ##################

include ./aie_libadf.mk
include ./helper.mk

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXE_FILE): $(EXE_FILE_DEPS)
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $(filter %.s %.c %.cc %.cpp %cxx, $^) $(CXXFLAGS) $(LDFLAGS)

$(EMCONFIG):
	emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)

############################## Package final xclbin or sd_card folder (for embedded) ##############################
# AIE_ON_X86 Flow
ifeq ($(pcie_versal), on)
	@echo "### ***** running AIE ON_X86 ***** ###"
	${VPP} -p $(VPP_PACKAGE) -t ${TARGET} -f ${XPLATFORM} ${AIE_CONTAINER} ${BINARY_CONTAINERS} -o $(BINARY_CONTAINERS_PKG) --package.boot_mode ospi
	@echo "### ***** xclbin generation done! ***** ###"
endif

ifeq ($(SD_CARD_NEEDED), on)
RUN_SCRIPT := $(BUILD_DIR)/run_script.sh
$(RUN_SCRIPT):
	rm -rf $(RUN_SCRIPT)
	@echo 'export LD_LIBRARY_PATH=/mnt:/tmp${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}} ' >> $(RUN_SCRIPT)
ifneq ($(filter hw_emu, $(TARGET)),)
	@echo 'export XCL_EMULATION_MODE=$(TARGET)' >> $(RUN_SCRIPT)
endif
	@echo 'export XILINX_VITIS=/mnt' >> $(RUN_SCRIPT)
	@echo 'export XILINX_XRT=/usr' >> $(RUN_SCRIPT)
	@echo 'if [ -f platform_desc.txt  ]; then' >> $(RUN_SCRIPT)
	@echo '        cp platform_desc.txt /etc/xocl.txt' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
ifneq (,$(findstring AIE2PS, $(AIE_TYPE)))
	@echo 'echo "INFO: Load the pdi and dtbo using fpgautil"' >> $(RUN_SCRIPT)
	@echo 'fpgautil -b $(BINARY_NAME).pdi -o ${BINARY_NAME}.dtbo' >> $(RUN_SCRIPT)
endif
	@echo './$(EXE_NAME) $(PKG_HOST_ARGS)' >> $(RUN_SCRIPT)
	@echo 'return_code=$$?' >> $(RUN_SCRIPT)
	@echo 'if [ $$return_code -ne 0 ]; then' >> $(RUN_SCRIPT)
	@echo '        echo "ERROR: TEST FAILED, RC=$$return_code "' >> $(RUN_SCRIPT)
	@echo 'else' >> $(RUN_SCRIPT)
	@echo '        echo "INFO: TEST PASSED, RC=0 "' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo 'echo "INFO: Embedded host run completed."' >> $(RUN_SCRIPT)
	@echo 'exit $$return_code' >> $(RUN_SCRIPT)
DATA_FILE := data/ref_output.txt data/input_front.txt $(custom_data_file)
DATA_DIR := $(custom_data_dir)
DATA_FILE := $(strip $(DATA_FILE))
DATA_DIR := $(strip $(DATA_DIR))
SD_FILES += $(RUN_SCRIPT)
SD_FILES += $(EXE_FILE)
SD_FILES += $(EMCONFIG)
SD_FILES += xrt.ini
SD_FILES += $(DATA_FILE)# where define DATAFILE in json
SD_FILES_WITH_PREFIX = $(foreach sd_file,$(SD_FILES), $(if $(filter $(sd_file),$(wildcard $(sd_file))), --package.sd_file $(sd_file)))
SD_DIRS_WITH_PREFIX = $(foreach sd_dir,$(DATA_DIR),--package.sd_dir $(sd_dir))
PACKAGE_FILES := $(BINARY_CONTAINERS)
PACKAGE_FILES += $(AIE_CONTAINER)
PACKAGE_DIR := $(CUR_DIR)/package_$(TARGET)
PACKAGE_DIR_HW := $(CUR_DIR)/package.$(TARGET)
$(PACKAGE_DIR): host xclbin $(RUN_SCRIPT) $(EMCONFIG) #check_kimage check_rootfs
	@echo "Generating sd_card folder...."
	mkdir -p $(PACKAGE_DIR)
	chmod a+rx $(BUILD_DIR)/run_script.sh
# 1. DFX HW Flow
ifeq ($(dfx_hw), on)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p $(PACKAGE_FILES) $(VPP_PACKAGE) -o $(BINARY_CONTAINERS_PKG)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p --package.out_dir  $(PACKAGE_DIR) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE) --package.boot_mode sd  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX) --package.sd_file $(BINARY_CONTAINERS_PKG)
	@echo "### ***** sd_card generation done! ***** ###"
endif
# 2. General Embeded flow
ifeq ($(dfx_hw), off)
ifeq ($(pcie_versal), off)
ifneq (,$(findstring AIE2PS, $(AIE_TYPE)))
	$(VPP) -s -t $(TARGET) -p -f $(PACKAGE_FILES) -o $(BINARY_CONTAINERS_PKG) $(VPP_PACKAGE) --package.out_dir $(PACKAGE_DIR)
ifeq (${TARGET},hw)
	@mkdir -p $(PACKAGE_DIR)/sd_card
	@cp -rf $(PACKAGE_DIR)/${BINARY_NAME}.dtbo $(PACKAGE_DIR)/*.bif $(BINARY_CONTAINERS_PKG) $(EXE_FILE) $(RUN_SCRIPT) $(PACKAGE_DIR)/sd_card
	mv $(PACKAGE_DIR)/${BINARY_NAME}.pdi $(PACKAGE_DIR)/sd_card/${BINARY_NAME}.pdi
	@for dir in $(DATA_DIR); do \
	    cp -r $$dir/* $(PACKAGE_DIR)/sd_card/ ; \
	done
	@for f in $(DATA_FILE); do \
	    [ -e "$$f" ] && cp -r $$f $(PACKAGE_DIR)/sd_card/ ; \
	done
	@ln -sfn $(PACKAGE_DIR) $(PACKAGE_DIR_HW)
endif	
else
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(PACKAGE_DIR) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE) --package.boot_mode sd  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
endif
	@echo "### ***** sd_card generation done! ***** ###"
endif
endif

.PHONY: sd_card
sd_card: $(PACKAGE_DIR)
endif

############################## Setting Essential Checks and Building Rules ##############################
ifneq (,$(filter x86sim aiesim, $(TARGET)))
RUN_DEPS += $(AIE_CONTAINER)
else
RUN_DEPS += host xclbin $(EMCONFIG)
ifeq ($(SD_CARD_NEEDED), on)
ifneq (,$(findstring AIE2PS, $(AIE_TYPE)))
RUN_DEPS += $(PACKAGE_DIR)
ifeq ($(TARGET),hw_emu)
RUN_DEPS += qemu_combined copy_wic
endif
else
RUN_DEPS += $(PACKAGE_DIR)
endif
endif
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
	mkdir -p logs
	$(if $(filter-out REF,$(TAG)), $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk get_status HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) UUT_SIM_FILE=$(UUT_SIM_FILE) REF_SIM_FILE=$(REF_SIM_FILE) UUT_KERNEL=$(UUT_KERNEL)")
	$(if $(filter-out REF,$(TAG)), tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff_exit.tcl ./)
endif
#aiesim
ifeq ($(TARGET), aiesim)
	$(AIESIMULATOR) --pkg-dir=$(AIE_PKG_DIR) --profile  $(AIESIMFLAGS)
	mkdir -p logs
	$(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk get_status HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) UUT_SIM_FILE=$(UUT_SIM_FILE) REF_SIM_FILE=$(REF_SIM_FILE) UUT_KERNEL=$(UUT_KERNEL)"
	$(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk get_latency HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) UUT_KERNEL=$(UUT_KERNEL)"
	$(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk get_stats HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) UUT_KERNEL=$(UUT_KERNEL)"
	$(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk get_theoretical_min HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) UUT_KERNEL=$(UUT_KERNEL) "
	$(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk harvest_mem HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) UUT_KERNEL=$(UUT_KERNEL) "
	tclsh $(XFLIB_DIR)/L2/tests/aie/common/scripts/diff_exit.tcl ./
endif
#hw_emu
ifneq (,$(filter hw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
ifeq ($(pcie_versal), on)
	cp $(AIE_WORK_DIR)/reports/dma_lock_report.json ./
	cp $(AIE_WORK_DIR)/ps/c_rts/aie_control_config.json ./
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(BINARY_CONTAINERS_PKG) $(HOST_ARGS) 
	
else
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(BINARY_CONTAINERS_PKG) $(HOST_ARGS)
	
endif
else
ifneq (,$(findstring AIE2PS, $(AIE_TYPE)))
	cd ${PACKAGE_DIR} && ./launch_hw_emu.sh -qemu-config ${QEMU_COMBINED}/combined.qemuboot.conf -login "amd-edf" -password "amd-edf" -run-app " mount /dev/sda2 /media; cd /media; ls; bash $(notdir $(RUN_SCRIPT)) " 2>&1 | tee qemu_output.log
else
	@echo $(RUN_DEPS)
	$(PACKAGE_DIR)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
endif
	grep "TEST PASSED, RC=0" $(PACKAGE_DIR)/qemu_output.log || exit 1
	
endif
endif
#hw
ifeq ($(TARGET), hw)
ifneq (,$(findstring aws-vu9p-f1, $(PLATFORM_NAME)))
ifeq (,$(wildcard $(BUILD_DIR)/kernel.awsxclbin))
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

############################## qemu combined and config ##############################
ifneq (,$(findstring AIE2PS, $(AIE_TYPE)))
QEMU_COMBINED := ${PACKAGE_DIR}/base_camp_images
QEMU_PREBUILT := amd-cortexa78-mali-common_vek385_qemu_prebuilt

qemu_combined:
	cd ${PACKAGE_DIR}; \
	rm -rf ${QEMU_COMBINED}; \
	mkdir -p ${QEMU_COMBINED}; \
	cp -prf ${YOCTO_ARTIFACTS}/${QEMU_PREBUILT}/* ${QEMU_COMBINED}/.; 
endif

############################## copy files to quem ##############################
ifneq (,$(findstring AIE2PS, $(AIE_TYPE)))
PDI_DIR := ${PACKAGE_DIR}/${BINARY_NAME}.pdi
DTBO_DIR := ${PACKAGE_DIR}/${BINARY_NAME}.dtbo
WIC_PARTITION := edf-linux-disk-image-amd-cortexa78-mali-common.rootfs.wic.ufs:2
copy_wic:
	bash ${SCRIPTS_DIR}/run_copy_wic.sh ${BINARY_CONTAINERS_PKG} ${EXE_FILE} ${DTBO_DIR} ${PDI_DIR} ${RUN_SCRIPT} ${QEMU_COMBINED} ${WIC_PARTITION} "${DATA_DIR}" ${DATA_FILE}
endif

############################## Setting Targets ##############################
.PHONY: pre_build
pre_build:
	$(if $(filter-out REF,$(TAG)), $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/vss/scripts/gen_vss_configs.py $(XFLIB_DIR) $(CUR_DIR) $(PARAMS) vss_fft_ifft_1d_params.cfg fft_aie)
	$(if $(filter-out REF,$(TAG)), $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) UUT_KERNEL=$(UUT_KERNEL) TAG=$(TAG) FRONT_INPUT_FILE=$(FRONT_INPUT_FILE) BACK_INPUT_FILE=$(BACK_INPUT_FILE) REF_INPUT_FILE=$(REF_INPUT_FILE) REF_SIM_FILE=$(REF_SIM_FILE)")
	$(if $(filter hw_emu,$(TARGET)), make -f $(XFLIB_DIR)/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d.mk vss HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) AIETARGET=hw)
	$(if $(filter hw,$(TARGET)), make -f $(XFLIB_DIR)/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d.mk vss HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR) AIETARGET=hw)
	$(if $(filter hw_emu,$(TARGET)), $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk gen_conn_cfg HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR)")
	$(if $(filter hw,$(TARGET)), $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/paramenv.py --parameter_file $(PARAMS_FILE) --instance_name $(PARAMS) --command "make -f $(CUR_DIR)/helper.mk gen_conn_cfg HELPER_CUR_DIR=$(CUR_DIR) HELPER_ROOT_DIR=$(XFLIB_DIR)")

.PHONY: clean cleanall emconfig valid_params
emconfig: $(EMCONFIG)

.PHONY: host
ifeq ($(HOST_ARCH), x86)
host: check_xrt  pre_build $(EXE_FILE)
else
host: check_sysroot  pre_build $(EXE_FILE)
endif

.PHONY: xclbin
ifeq ($(HOST_ARCH), x86)
xclbin: check_vpp check_xrt  pre_build $(BINARY_CONTAINERS)
else
xclbin: check_vpp check_sysroot  pre_build $(BINARY_CONTAINERS)
endif

#################### Parameterized File Generation ##########################

GENERATED_FILES += uut_config.h
uut_config.h: $(XFLIB_DIR)/L2/tests/aie/common/scripts/tb_gen.py 
	$(VITIS_PYTHON3) $(XFLIB_DIR)/scripts/instance_generator.py -o uut_config.h --param-set $(PARAMS) --param-file $(PARAMS_FILE) --file "$(XFLIB_DIR)/L2/tests/aie/common/scripts/tb_gen.py" --func "generate_testbench"

.PHONY: valid_params gen_instances

valid_params:

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