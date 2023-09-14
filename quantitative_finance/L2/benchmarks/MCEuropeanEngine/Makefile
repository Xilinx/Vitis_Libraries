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
	$(ECHO) "  make all TARGET=<hw_emu/sw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw_emu/sw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  make xclbin TARGET=<hw_emu/sw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) ""
	$(ECHO) "  make host TARGET=<hw_emu/sw_emu/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) ""
	$(ECHO) "  NOTE: For embedded devices, e.g. zcu102/zcu104/vck190, HOST_ARCH is either aarch32 or aarch64."
	$(ECHO) "      a.IF Download the platform, and common-image from Xilinx Download Center(Suggested):"
	$(ECHO) "        Run the sdk.sh script from the common-image directory to install sysroot using the command : ./sdk.sh -y -d ./ -p "
	$(ECHO) "        Unzip the rootfs file : gunzip ./rootfs.ext4.gz"
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "      b.User could also define SYSROOT, K_IMAGE and ROOTFS by themselves: "
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "        export K_IMAGE=< path-to-Image-files >"
	$(ECHO) "        export ROOTFS=< path-to-rootfs >"
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall TARGET=<hw_emu/sw_emu/>"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

############################## Setting up Project Variables ##############################

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L2/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

# setting devault value
TARGET ?= sw_emu
HOST_ARCH ?= x86

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_u250_gen3x16_xdma_4_1_202210_1
endif

# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  u250
PLATFORM_BLOCKLIST +=  zc

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

########################## Setting up Host Variables ##########################

#Inclue Required Host Source Files
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
HOST_SRCS += host/test.cpp $(XFLIB_DIR)/ext/xcl2/xcl2.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/host -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/kernel -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/ext/quantlib -I $(XFLIB_DIR)/ext/rng   -I $(XFLIB_DIR)/../utils/L1/include -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/ext/quantlib -I $(XFLIB_DIR)/ext/rng   -I $(XFLIB_DIR)/../utils/L1/include
CXXFLAGS += -O3  -I$(XILINX_HLS)/include
LDFLAGS +=  -L$(XILINX_HLS)/lnx64/tools/fpo_v7_1 -lgmp -lmpfr -lIp_floating_point_v7_1_bitacc_cmodel
else 
ifeq ($(ps_on_x86), on)
HOST_SRCS += host/test.cpp $(XFLIB_DIR)/ext/xcl2/xcl2.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/host -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/kernel -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/ext/quantlib -I $(XFLIB_DIR)/ext/rng   -I $(XFLIB_DIR)/../utils/L1/include -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/ext/quantlib -I $(XFLIB_DIR)/ext/rng   -I $(XFLIB_DIR)/../utils/L1/include
CXXFLAGS += -O3  -I$(XILINX_HLS)/include
LDFLAGS +=  -L$(XILINX_HLS)/lnx64/tools/fpo_v7_1 -lgmp -lmpfr -lIp_floating_point_v7_1_bitacc_cmodel
else
HOST_SRCS += host/test.cpp $(XFLIB_DIR)/ext/xcl2/xcl2.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/host -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/kernel -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/ext/quantlib -I $(XFLIB_DIR)/ext/rng   -I $(XFLIB_DIR)/../utils/L1/include -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/ext/quantlib -I $(XFLIB_DIR)/ext/rng   -I $(XFLIB_DIR)/../utils/L1/include
CXXFLAGS += -O3  -I$(XILINX_HLS)/include
LDFLAGS +=  -L$(XILINX_HLS)/lnx64/tools/fpo_v7_1 -lgmp -lmpfr -lIp_floating_point_v7_1_bitacc_cmodel
endif
endif
# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := test.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(INSTANCE_FILES)  $(EXE_FILE_DEPS)

HOST_ARGS :=  -xclbin $(BUILD_DIR)/kernel_mc.xclbin
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif
LIBRARY_PATH := $(XILINX_HLS)/lnx64/tools/fpo_v7_1

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
VPP_FLAGS +=   --config $(CUR_DIR)/conn_u250.cfg
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/kernel -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include -I $(XFLIB_DIR)/L2/benchmarks/MCEuropeanEngine/kernel -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/L2/include

endif

######################### binary container global settings ##########################
VPP_FLAGS_kernel_mc +=  -D DPRAGMA
VPP_FLAGS_kernel_mc += --hls.clock 300000000:kernel_mc
ifneq ($(HOST_ARCH_temp), x86)
VPP_LDFLAGS_kernel_mc += --clock.defaultFreqHz 300000000
else
VPP_LDFLAGS_kernel_mc += --kernel_frequency 300
endif

ifneq ($(SD_CARD_NEEDED), on)
BINARY_CONTAINERS += $(BUILD_DIR)/kernel_mc.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/kernel_mc_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/kernel_mc.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/kernel_mc.xo: kernel/kernel_mc.cpp 
	$(ECHO) "Compiling Kernel: kernel_mc"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_kernel_mc) $(VPP_FLAGS) -k kernel_mc -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_kernel_mc_OBJS += $(TEMP_DIR)/kernel_mc.xo
BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_kernel_mc_OBJS)
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/kernel_mc $(VPP_LDFLAGS)  $(VPP_LDFLAGS_kernel_mc) $(AIE_LDFLAGS)   -o $@ $^

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
DATA_FILE := $(custom_data_file)
DATA_DIR := $(custom_data_dir)
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
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX) --package.sd_file $(BINARY_CONTAINERS_PKG)
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
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
endif
endif
endif

.PHONY: sd_card
sd_card: $(SD_CARD)
endif
############################## Setting Essential Checks and Building Rules ##############################
RUN_DEPS += host xclbin $(EMCONFIG)
RUN_DEPS += $(SD_CARD)

.PHONY: mkflag all run
mkflag:
	mkdir -p $(BUILD_DIR)
	rm -rf $(BUILD_DIR)/makefile_args.txt
	@for var in $(MAKEFLAGS); do echo $$var >> $(BUILD_DIR)/makefile_args.txt; done

all: check_device check_vpp check_platform mkflag $(RUN_DEPS)

run: all
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
ifeq (,$(wildcard $(BUILD_DIR)/kernel_mc.awsxclbin))
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

.PHONY: clean cleanall emconfig gen_instances valid_params
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

gen_instances: $(INSTANCE_FILES)
	@echo "Nothing to generate."

valid_params:
	@echo "Nothing to validate."

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE)  vitis_* TempConfig system_estimate.xtxt *.rpt .run/  $(INST_TB_FILES)
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png *.db

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.* _x* $(INST_FILES)

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR)  $(AIE_WORK_DIR) $(AIE_PKG_DIR) $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut
	-$(RMDIR)  *.html $(INSTANCE_FILES)

clean: cleanh