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
	$(ECHO) "  make all TARGET=<hw/aiesim/x86sim/hw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw/aiesim/x86sim/hw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  make xclbin TARGET=<hw/aiesim/x86sim/hw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) ""
	$(ECHO) "  make host TARGET=<hw/aiesim/x86sim/hw_emu/>"
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
	$(ECHO) "  make cleanall TARGET=<hw/aiesim/x86sim/hw_emu/>"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

############################## Setting up Project Variables ##############################

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L2/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

# setting devault value
TARGET ?= hw_emu
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

############################ setting AIE Compiler ###########################
ifneq ($(filter aiesim hw_emu hw, $(TARGET)),)
AIETARGET := hw
else
AIETARGET := x86sim
endif
AIE_CXXFLAGS += --target=$(AIETARGET) --platform=$(XPLATFORM) --pl-freq=300 
AIE_CXXFLAGS += -I $(XILINX_VITIS_AIETOOLS)/include
AIE_CONTAINER = $(TEMP_DIR)/libadf.a
AIE_CXXFLAGS += -I $(XFLIB_DIR)/L1/include/aie
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
HOST_SRCS += $(XFLIB_DIR)/L2/tests/aie/erode_rect_3x3/16bit_aie_gmio/host.cpp $(CUR_DIR)/Work/ps/c_rts/aie_control_xrt.cpp 
CXXFLAGS +=  -D __PS_ENABLE_AIE__
CXXFLAGS +=  -I $(SYSROOT)/usr/include/xrt/ -I $(SYSROOT)/usr/include/ -I $(SYSROOT)/usr/include/opencv4 -I $(XFLIB_DIR)/L1/include/aie -I $(CUR_DIR) -I $(XFLIB_DIR)/ext/xcl2/ -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/ext/xcl2/ -I $(XFLIB_DIR)/L1/include
CXXFLAGS += --sysroot=$(SYSROOT) -O3 -I$(XILINX_HLS)/include -I$(XILINX_HLS)/include -I$(XILINX_HLS)/include
LDFLAGS +=  -L $(SYSROOT)/usr/lib/ -L $(XFLIB_DIR)/L1/lib/sw/aarch64-linux
LDFLAGS +=  -l smartTilerStitcher -l opencv_videoio -l opencv_imgcodecs -l opencv_core -l opencv_imgproc -l opencv_features2d -l opencv_flann -l opencv_video -l opencv_calib3d -l opencv_highgui -l opencv_videoio -l opencv_imgcodecs -l opencv_core -l opencv_imgproc -l opencv_features2d -l opencv_flann -l opencv_video -l opencv_calib3d -l opencv_highgui
LDFLAGS += --sysroot=$(SYSROOT)
else
HOST_SRCS += $(XFLIB_DIR)/L2/tests/aie/erode_rect_3x3/16bit_aie_gmio/host.cpp $(CUR_DIR)/Work/ps/c_rts/aie_control_xrt.cpp 
CXXFLAGS +=  -D __PS_ENABLE_AIE__
CXXFLAGS +=  -I $(SYSROOT)/usr/include/xrt/ -I $(SYSROOT)/usr/include/ -I $(SYSROOT)/usr/include/opencv4 -I $(XFLIB_DIR)/L1/include/aie -I $(CUR_DIR) -I $(XFLIB_DIR)/ext/xcl2/ -I $(XFLIB_DIR)/L1/include -I $(XFLIB_DIR)/ext/xcl2/ -I $(XFLIB_DIR)/L1/include
CXXFLAGS += --sysroot=$(SYSROOT) -O3 -I$(XILINX_HLS)/include -I$(XILINX_HLS)/include -I$(XILINX_HLS)/include
LDFLAGS +=  -L $(SYSROOT)/usr/lib/ -L $(XFLIB_DIR)/L1/lib/sw/aarch64-linux
LDFLAGS +=  -l smartTilerStitcher -l opencv_videoio -l opencv_imgcodecs -l opencv_core -l opencv_imgproc -l opencv_features2d -l opencv_flann -l opencv_video -l opencv_calib3d -l opencv_highgui -l opencv_videoio -l opencv_imgcodecs -l opencv_core -l opencv_imgproc -l opencv_features2d -l opencv_flann -l opencv_video -l opencv_calib3d -l opencv_highgui
LDFLAGS += --sysroot=$(SYSROOT)
endif

# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := host.elf
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(EXE_FILE_DEPS)

ifeq ($(TARGET),$(filter $(TARGET),hw_emu))
HOST_ARGS := kernel.xclbin $(XFLIB_DIR)/data/HD.jpg
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif
endif
ifeq ($(TARGET),$(filter $(TARGET),hw))
HOST_ARGS := kernel.xclbin $(XFLIB_DIR)/data/4k.jpg
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif
endif

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/vck190/'))
VPP_FLAGS +=   
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/
VPP_LDFLAGS +=   --config $(CUR_DIR)/system.cfg
VPP_PACKAGE +=   --package.defer_aie_run

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/

endif

######################### binary container global settings ##########################

ifneq ($(SD_CARD_NEEDED), on)
BINARY_CONTAINERS += $(BUILD_DIR)/kernel.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/kernel_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/kernel.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_kernel_OBJS)
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/kernel $(VPP_LDFLAGS)  $(VPP_LDFLAGS_kernel) $(AIE_LDFLAGS)   -o $@ $^

############################## Setting Rules for AIE (Building Kernels) ##################
$(AIE_CONTAINER): $(XFLIB_DIR)/L2/tests/aie/erode_rect_3x3/16bit_aie_gmio/graph.cpp $(CUR_DIR)/xf_erode_rect_3x3.cc  
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
DATA_FILE := $(XFLIB_DIR)/data/HD.jpg $(XFLIB_DIR)/data/4k.jpg $(custom_data_file)
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
	
endif
#aiesim
ifeq ($(TARGET), aiesim)
	$(AIESIMULATOR) --pkg-dir=./Work --profile 
	
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

############################## Setting Targets ##############################

.PHONY: clean cleanall emconfig
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

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE) vitis_* TempConfig system_estimate.xtxt *.rpt .run/
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png *.db

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.* _x*

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR) sd_card* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR) $(AIE_CONTAINER) aiesimulator_output .AIE_SIM_CMD_LINE_OPTIONS x86simulator_output $(CUR_DIR)/Work $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut
	-$(RMDIR) $(CUR_DIR)/ISS_RPC_SERVER_PORT $(CUR_DIR)/pl_sample_counts 

clean: cleanh