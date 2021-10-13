# Copyright 2019-2021 Xilinx, Inc.
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
# vitis makefile-generator v2.0.3

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform> HOST_ARCH=<x86 x86  >"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform> HOST_ARCH=<x86 x86  >"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=. HOST_ARCH required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make xclbin TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform> HOST_ARCH=<x86 x86  >"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) "      By default, HOST_ARCH=. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make host HOST_ARCH=<hw/hw_emu/sw_emu/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=. HOST_ARCH is required for SoC shells"
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
TARGET ?= sw_emu
HOST_ARCH ?= 

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_u250_xdma_201830_2
endif

# ###dddd################# Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  xilinx_u250_xdma_201830_2 u200 u50
PLATFORM_BLOCKLIST +=  vck

include ./utils.mk

TEMP_DIR := _x_temp.$(TARGET).$(XPLATFORM)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(XPLATFORM)
BUILD_DIR := build_dir.$(TARGET).$(XPLATFORM)
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(XPLATFORM)
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

#Inclue Required Host Source Files
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
HOST_SRCS += $(XFLIB_DIR)/L2/src/memKernel/sw/main.cpp $(XFLIB_DIR)/L2/src/xcl2/xcl2.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/sw -I $(XFLIB_DIR)/L2/include/xcl2 -I $(XFLIB_DIR)/L1/include/hw
CXXFLAGS += -O3

else ifneq (,$(shell echo $(XPLATFORM) | awk '/u200/'))
HOST_SRCS += $(XFLIB_DIR)/L2/src/memKernel/sw/main.cpp $(XFLIB_DIR)/L2/src/xcl2/xcl2.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/sw -I $(XFLIB_DIR)/L2/include/xcl2 -I $(XFLIB_DIR)/L1/include/hw
CXXFLAGS += -O3

else ifneq (,$(shell echo $(XPLATFORM) | awk '/u50/'))
HOST_SRCS += $(XFLIB_DIR)/L2/src/memKernel/sw/main.cpp $(XFLIB_DIR)/L2/src/xcl2/xcl2.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/sw -I $(XFLIB_DIR)/L2/include/xcl2 -I $(XFLIB_DIR)/L1/include/hw
CXXFLAGS += -O3

else 
HOST_SRCS += $(XFLIB_DIR)/L2/src/memKernel/sw/main.cpp $(XFLIB_DIR)/L2/src/xcl2/xcl2.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/sw -I $(XFLIB_DIR)/L2/include/xcl2 -I $(XFLIB_DIR)/L1/include/hw
CXXFLAGS += -O3

endif
EXE_NAME := host.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(EXE_FILE_DEPS)

HOST_ARGS := $(BUILD_DIR)/blas.xclbin $(BUILD_DIR)/data
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
VPP_FLAGS +=   --config $(CUR_DIR)/conn_u250.cfg
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw -I $(XFLIB_DIR)/L1/include/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw/xf_blas/gemm -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw

else ifneq (,$(shell echo $(XPLATFORM) | awk '/u200/'))
VPP_FLAGS +=   --config $(CUR_DIR)/conn_u200.cfg
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw -I $(XFLIB_DIR)/L1/include/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw/xf_blas/gemm -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw

else ifneq (,$(shell echo $(XPLATFORM) | awk '/u50/'))
VPP_FLAGS +=   --config $(CUR_DIR)/conn_u50.cfg
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw -I $(XFLIB_DIR)/L1/include/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw/xf_blas/gemm -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/L1/include/hw -I $(XFLIB_DIR)/L1/include/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw/xf_blas/gemm -I $(XFLIB_DIR)/L1/include/hw/xf_blas/helpers/utils -I $(XFLIB_DIR)/L2/include/memKernel -I $(XFLIB_DIR)/L2/include/memKernel/hw/xf_blas -I $(XFLIB_DIR)/L1/include/hw

endif

VPP_FLAGS_blasKernel += --hls.clock 300000000:blasKernel

ifneq ($(HOST_ARCH), x86)
VPP_LDFLAGS_blas += --clock.defaultFreqHz 300000000
else
VPP_LDFLAGS_blas += --kernel_frequency 300
endif
VPP_LDFLAGS_blas_temp := --config opts.cfg
VPP_LDFLAGS_blas += $(VPP_LDFLAGS_blas_temp)

########################## binary container global settings ##########################
BINARY_CONTAINER_blas_OBJS += $(TEMP_DIR)/blasKernel.xo
BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_blas_OBJS)
ifeq ($(HOST_ARCH), x86)
BINARY_CONTAINERS += $(BUILD_DIR)/blas.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/blas_pkg.xclbin
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/blas.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/blasKernel.xo: $(XFLIB_DIR)/L2/src/memKernel/hw/kernel.cpp 
	$(ECHO) "Compiling Kernel: blasKernel"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_blasKernel) $(VPP_FLAGS) -k blasKernel -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o'$@' '$<'
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(BUILD_DIR) --report_dir $(BUILD_REPORT_DIR)/blas $(VPP_LDFLAGS)  $(VPP_LDFLAGS_blas) $(AIE_LDFLAGS)   -o $@ $^

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
ifneq (,$(findstring zc706, $(XPLATFORM)))
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
$(SD_CARD): $(EXE_FILE) $(BINARY_CONTAINERS) $(RUN_SCRIPT) $(EMCONFIG)
	@echo "Generating sd_card folder...."
	mkdir -p $(SD_CARD)
	chmod a+rx $(BUILD_DIR)/run_script.sh
	$(VPP) -t $(TARGET) --platform $(PLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(SYSROOT)/../../rootfs.ext4 --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"

.PHONY: sd_card
sd_card: $(SD_CARD)

endif
############################## Setting Essential Checks and Building Rules ##############################
ifneq (,$(filter x86sim aiesim, $(TARGET)))
RUN_DEPS += $(AIE_CONTAINER)
else
RUN_DEPS += $(AIE_CONTAINER) $(EXE_FILE) $(BINARY_CONTAINERS) $(EMCONFIG)
RUN_DEPS += $(SD_CARD)
endif

run: check_device pre_build $(RUN_DEPS)
#x86sim
ifeq ($(TARGET), x86sim)
	$(X86SIMULATOR) --pkg-dir=./Work 
	
endif
#aiesim
ifeq ($(TARGET), aiesim)
	$(AIESIMULATOR) --pkg-dir=./Work --profile 
	
endif
#hw_emu and sw_emu
ifneq (,$(filter sw_emu hw_emu, $(TARGET)))
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
ifeq ($(HOST_ARCH), x86)
	$(EXE_FILE) $(HOST_ARGS)
	
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Setting Targets ##############################

-include params.mk

.PHONY: pre_build
pre_build:
	make data_gen

.PHONY: all clean cleanall emconfig
emconfig: $(EMCONFIG)
ifeq ($(HOST_ARCH), x86)
all: pre_build check_vpp check_platform check_xrt $(EXE_FILE) $(BINARY_CONTAINERS) emconfig
else
all: pre_build check_vpp check_platform check_sysroot $(EXE_FILE) $(BINARY_CONTAINERS) emconfig sd_card
endif

.PHONY: host
ifeq ($(HOST_ARCH), x86)
host:  pre_build check_xrt $(EXE_FILE)
else
host:  pre_build check_sysroot $(EXE_FILE)
endif

.PHONY: xclbin
ifeq ($(HOST_ARCH), x86)
xclbin: pre_build check_vpp check_xrt $(BINARY_CONTAINERS) 
else
xclbin: pre_build check_vpp check_sysroot $(BINARY_CONTAINERS) 
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
	-$(RMDIR) _x_temp.*/_x.* _x_temp.*/.Xil _x_temp.*/profile_summary.* xo_* _x*
	-$(RMDIR) _x_temp.*/dltmp* _x_temp.*/kernel_info.dat _x_temp.*/*.log
	-$(RMDIR) _x_temp.* 

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  build_dir.* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR) $(XFLIB_DIR)/common/data/*.xe2xd* $(XFLIB_DIR)/common/data/*.orig*
	-$(RMDIR)  $(CUR_DIR)/Work $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut

clean: cleanh