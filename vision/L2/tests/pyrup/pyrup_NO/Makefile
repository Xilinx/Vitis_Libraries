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

# -----------------------------------------------------------------------------
#                          project common settings

MK_PATH       := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR       := $(patsubst %/,%,$(dir $(MK_PATH)))
CASE_ROOT     ?= $(CUR_DIR)


#MK_COMMON_DIR := mk

# Below should point to library repo
XF_LIB_DIR    ?= $(abspath $(CASE_ROOT)/../../../..)
MK_COMMON_DIR := $(XF_LIB_DIR)/ext/makefile_templates

.SECONDEXPANSION:

# -----------------------------------------------------------------------------
#                            vitis common setup

# MK_INC_BEGIN vitis_help.mk

.PHONY: help

help::
	@echo ""
	@echo "Makefile Usage:"
	@echo ""
	@echo "  make host xclbin TARGET=<sw_emu|hw_emu|hw> DEVICE=<FPGA platform>"
	@echo "      Command to generate the design for specified target and device."
	@echo ""
	@echo "      TARGET defaults to sw_emu."
	@echo ""
	@echo "      DEVICE is case-insensitive and support awk regex."
	@echo "      For example, \`make xclbin TARGET=hw DEVICE='u200.*qdma'\`"
	@echo "      It can also be an absolute path to platform file."
	@echo ""
	@echo "  make run TARGET=<sw_emu|hw_emu|hw> DEVICE=<FPGA platform>"
	@echo "      Command to run application in emulation."
	@echo ""
	@echo "  make clean "
	@echo "      Command to remove the generated non-hardware files."
	@echo ""
	@echo "  make cleanall"
	@echo "      Command to remove all the generated files."
	@echo ""

# MK_INC_END vitis_help.mk

# MK_INC_BEGIN vivado.mk

TOOL_VERSION ?= 2019.2

ifeq (,$(XILINX_VIVADO))
XILINX_VIVADO = /opt/xilinx/Vivado/$(TOOL_VERSION)
endif
export XILINX_VIVADO

.PHONY: check_vivado
check_vivado:
ifeq (,$(wildcard $(XILINX_VIVADO)/bin/vivado))
	@echo "Cannot locate Vivado installation. Please set XILINX_VIVADO variable." && false
endif

export PATH := $(XILINX_VIVADO)/bin:$(PATH)

# MK_INC_END vivado.mk

# MK_INC_BEGIN vitis.mk

TOOL_VERSION ?= 2019.2

ifeq (,$(XILINX_VITIS))
XILINX_VITIS = /opt/xilinx/Vitis/$(TOOL_VERSION)
endif
export XILINX_VITIS
.PHONY: check_vpp
check_vpp:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/v++))
	@echo "Cannot locate Vitis installation. Please set XILINX_VITIS variable." && false
endif

ifeq (,$(XILINX_XRT))
XILINX_XRT = /opt/xilinx/xrt
endif
export XILINX_XRT
.PHONY: check_xrt
check_xrt:
ifeq (,$(wildcard $(XILINX_XRT)/lib/libxilinxopencl.so))
	@echo "Cannot locate XRT installation. Please set XILINX_XRT variable." && false
endif

export PATH := $(XILINX_VITIS)/bin:$(XILINX_XRT)/bin:$(PATH)

ifeq (,$(LD_LIBRARY_PATH))
LD_LIBRARY_PATH := $(XILINX_XRT)/lib
else
LD_LIBRARY_PATH := $(XILINX_XRT)/lib:$(LD_LIBRARY_PATH)
endif
ifneq (,$(wildcard $(XILINX_VITIS)/bin/ldlibpath.sh))
export LD_LIBRARY_PATH := $(shell $(XILINX_VITIS)/bin/ldlibpath.sh $(XILINX_VITIS)/lib/lnx64.o):$(LD_LIBRARY_PATH)
endif# MK_INC_END vitis.mk

# MK_INC_BEGIN vitis_set_platform.mk

ifneq (,$(wildcard $(DEVICE)))
# Use DEVICE as a file path
XPLATFORM := $(DEVICE)
else
# Use DEVICE as a file name pattern
DEVICE_L := $(shell echo $(DEVICE) | tr A-Z a-z)
# Match the name
ifneq (,$(PLATFORM_REPO_PATHS))
XPLATFORMS := $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/*/*.xpfm))
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard $(XILINX_VITIS)/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard /opt/xilinx/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
endif

define MSG_PLATFORM
No platform matched pattern '$(DEVICE)'.
Available platforms are: $(XPLATFORMS)
To add more platform directories, set the PLATFORM_REPO_PATHS variable.
endef
export MSG_PLATFORM

define MSG_DEVICE
More than one platform matched: $(XPLATFORM)
Please set DEVICE variable more accurately to select only one platform file. For example: DEVICE='u200.*xdma'
endef
export MSG_DEVICE

.PHONY: check_platform
check_platform:
ifeq (,$(XPLATFORM))
	@echo "$${MSG_PLATFORM}" && false
endif
ifneq (,$(word 2,$(XPLATFORM)))
	@echo "$${MSG_DEVICE}" && false
endif

XDEVICE := $(basename $(notdir $(firstword $(XPLATFORM))))# MK_INC_END vitis_set_platform.mk

# Initial definition
KERNELS     :=

# -----------------------------------------------------------------------------
# BEGIN_XF_MK_USER_SECTION
# -----------------------------------------------------------------------------
# TODO:          data creation and other user targets

# a (typically hidden) file as stamp
DATA_STAMP := $(CUR_DIR)/.stamp
$(DATA_STAMP):
	touch $@

.PHONY: data
data: $(DATA_STAMP)

# -----------------------------------------------------------------------------
#                          kernel setup

KSRC_DIR 		:= $(XF_LIB_DIR)/L2/examples/pyrup
XFREQUENCY 		:= 300
VIVADO_FREQUENCY        =$(shell echo $$(( $(XFREQUENCY) * 1000000 )))

XCLBIN_NAME 	:= krnl_pyr_up
KER_NAME    	:= pyr_up_accel
KERNELS += $(KER_NAME):xf_pyr_up_accel.cpp

VPP_CFLAGS  	+=  -I. -I$(XF_LIB_DIR)/L1/include
VPP_CFLAGS  	+= -D__SDSVHLS__ -DHLS_NO_XIL_FPO_LIB


$(KER_NAME)_VPP_CFLAGS := --xp vivado_prop:run.impl_1.strategy=Performance_Explore 

ifeq ($(BOARD), Zynq)
VPP_CFLAGS	+= --clock.defaultFreqHz ${VIVADO_FREQUENCY}
endif


# -----------------------------------------------------------------------------
#                           host setup

SRC_DIR 		:= $(XF_LIB_DIR)/L2/examples/pyrup

EXE_NAME  		:= pyr_up
HOST_ARGS 		= $(XF_LIB_DIR)/data/4k.jpg #$(XCLBIN_FILE)
SRCS      		:= xf_pyr_up_tb xf_pyr_up_accel

# Macro definitions
CXXFLAGS 		+= -D XDEVICE=$(XDEVICE) -DVIVADO_HLS_SIM -D__SDSVHLS__ -DHLS_NO_XIL_FPO_LIB
# Search paths:
CXXFLAGS 		+=  -I. -I$(XF_LIB_DIR)/L1/include -I$(XF_LIB_DIR)/ext/xcl2
# Options
CXXFLAGS 		+= -g

ifeq ($(BOARD), Zynq)

    CXXFLAGS 	+= --sysroot=${SYSROOT} -D__ZYNQ

endif


# EXTRA_OBJS is cannot be compiled from SRC_DIR, user should provide the rule
EXTRA_OBJS 		+= xcl2

EXT_DIR   		= $(XF_LIB_DIR)/ext
xcl2_SRCS 		= $(EXT_DIR)/xcl2/xcl2.cpp
xcl2_HDRS 		= $(EXT_DIR)/xcl2/xcl2.hpp
xcl2_CXXFLAGS 	= -I $(EXT_DIR)/xcl2


# OpenCV related:
ifeq ($(BOARD), Zynq)
    opencv_LDFLAGS	:= -L${SYSROOT}/usr/lib -Wl,-rpath-link=${SYSROOT}/usr/lib/ -L${SYSROOT}/opt/xilinx/xrt/lib -lopencv_imgcodecs
else
    opencv_LDFLAGS	:= -L$(XILINX_VIVADO)/lnx64/tools/opencv/opencv_gcc/
RUN_ENV +=	export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(XILINX_VIVADO)/lnx64/tools/opencv/opencv_gcc;
endif
opencv_LDFLAGS  += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_calib3d -lopencv_features2d -lopencv_flann# -lopencv_imgcodecs

LDFLAGS 		:= $(opencv_LDFLAGS)

# -----------------------------------------------------------------------------
# END_XF_MK_USER_SECTION
# -----------------------------------------------------------------------------

.PHONY: all
all: host xclbin

# MK_INC_BEGIN vitis_kernel_rules.mk

VPP_DIR_BASE ?= _x
XO_DIR_BASE ?= xo
XCLBIN_DIR_BASE ?= xclbin

XCLBIN_DIR_SUFFIX ?= _$(XDEVICE)_$(TARGET)

VPP_DIR = $(CUR_DIR)/$(VPP_DIR_BASE)$(XCLBIN_DIR_SUFFIX)
XO_DIR = $(CUR_DIR)/$(XO_DIR_BASE)$(XCLBIN_DIR_SUFFIX)
XCLBIN_DIR = $(CUR_DIR)/$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)

XFREQUENCY ?= 300

VPP = v++
VPP_CFLAGS += -I$(KSRC_DIR)
VPP_CFLAGS += --target $(TARGET) --platform $(XPLATFORM) --temp_dir $(VPP_DIR) --save-temps --debug
VPP_CFLAGS += --kernel_frequency $(XFREQUENCY) --report_level 2
VPP_LFLAGS += --optimize 2 --jobs 8 \
  --xp "vivado_param:project.writeIntermediateCheckpoints=1"

KERNEL_NAMES := $(foreach k,$(KERNELS),$(word 1, $(subst :, ,$(k))))
XO_FILES := $(foreach k,$(KERNEL_NAMES),$(XO_DIR)/$(k).xo)
XCLBIN_FILE ?= $(XCLBIN_DIR)/$(XCLBIN_NAME).xclbin

define kernel_src_dep
kernelname := $(word 1, $(subst :, ,$(1)))
kernelfile := $(if $(findstring :, $(1)),$(word 2, $(subst :, ,$(1))),$$(kernelname).cpp)
$$(kernelname)_SRCS := $(KSRC_DIR)/$$(kernelfile)
$$(kernelname)_SRCS += $$($$(kernelname)_EXTRA_SRCS)
endef

$(foreach k,$(KERNELS),$(eval $(call kernel_src_dep,$(k))))

define kernel_hdr_dep
kernelname := $(word 1, $(subst :, ,$(1)))
kernelfile := $(if $(findstring :, $(1)),$(basename $(word 2, $(subst :, ,$(1)))),$$(kernelname))
$$(kernelname)_HDRS := $$(wildcard $(KSRC_DIR)/$$(kernelfile).h $(KSRC_DIR)/$$(kernelfile).hpp)
$$(kernelname)_HDRS += $$($(1)_EXTRA_HDRS)
endef

$(foreach k,$(KERNELS),$(eval $(call kernel_hdr_dep,$(k))))

$(XO_DIR)/%.xo: VPP_CFLAGS += $($(*)_VPP_CFLAGS)
$(XO_DIR)/%.xo: $$($$(*)_SRCS) $$($$(*)_HDRS) | check_vpp
	@echo -e "----\nCompiling kernel $*..."
	mkdir -p $(XO_DIR)
	$(VPP) -o $@ --kernel $* --compile $(filter %.cpp,$^) \
		$(VPP_CFLAGS)

$(XCLBIN_FILE): $(XO_FILES) | check_vpp
	@echo -e "----\nCompiling xclbin..."
	mkdir -p $(XCLBIN_DIR)
	$(VPP) -o $@ --link $^ \
		$(VPP_CFLAGS) $(VPP_LFLAGS) \
		$(foreach k,$(KERNEL_NAMES),$($(k)_VPP_CFLAGS)) \
		$(foreach k,$(KERNEL_NAMES),$($(k)_VPP_LFLAGS))

.PHONY: xo xclbin

xo: $(XO_FILES) | check_vpp check_platform

xclbin: $(XCLBIN_FILE) | check_vpp check_platform

# MK_INC_END vitis_kernel_rules.mk

export XCL_BINDIR= $(XCLBIN_DIR)

ifeq ($(BOARD), Zynq)
# MK_INC_BEGIN vitis_zynq_host_rules.mk

OBJ_DIR_BASE ?= obj
BIN_DIR_BASE ?= bin

BIN_DIR_SUFFIX ?= _$(XDEVICE)

OBJ_DIR = $(CUR_DIR)/$(OBJ_DIR_BASE)$(BIN_DIR_SUFFIX)
BIN_DIR = $(CUR_DIR)/$(BIN_DIR_BASE)$(BIN_DIR_SUFFIX)

B_NAME = $(shell dirname $(XPLATFORM))

ifeq ($(ARCH), aarch64)
	CXX := aarch64-linux-gnu-g++
	DEV_FAM = Ultrascale
else ifeq ($(ARCH), aarch32)
	CXX := arm-linux-gnueabihf-g++
	DEV_FAM = 7Series
endif

SDCARD := sd_card

CC := gcc

CXXFLAGS += -std=c++14 -fPIC \
	-I$(SRC_DIR) -I$(XILINX_XRT)/include \
	-Wall -Wno-unknown-pragmas -Wno-unused-label -pthread 

CXXFLAGS += -idirafter $(XILINX_VIVADO)/include

CFLAGS +=
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -lxilinxopencl

LDFLAGS +=

OBJ_FILES = $(foreach s,$(SRCS),$(OBJ_DIR)/$(basename $(s)).o)

define host_hdr_dep
$(1)_HDRS := $$(wildcard $(SRC_DIR)/$(1).h $(SRC_DIR)/$(1).hpp)
$(1)_HDRS += $$($(1)_EXTRA_HDRS)
endef

$(foreach s,$(SRCS),$(eval $(call host_hdr_dep,$(basename $(s)))))

$(OBJ_DIR)/%.o: CXXFLAGS += $($(*)_CXXFLAGS)

$(OBJ_FILES): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $$($$(*)_HDRS) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling object $*..."
	mkdir -p $(@D)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

EXTRA_OBJ_FILES = $(foreach f,$(EXTRA_OBJS),$(OBJ_DIR)/$(f).o)

$(EXTRA_OBJ_FILES): $(OBJ_DIR)/%.o: $$($$(*)_SRCS) $$($$(*)_HDRS) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling extra object $@..."
	mkdir -p $(@D)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

EXE_EXT ?= exe
EXE_FILE ?= $(BIN_DIR)/$(EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)

$(EXE_FILE): $(OBJ_FILES) $(EXTRA_OBJ_FILES) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling host $(notdir $@)..."
	mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

.PHONY: host
host: $(EXE_FILE) | check_vpp check_xrt check_platform
# MK_INC_END vitis_zynq_host_rules.mk
# MK_INC_BEGIN vitis_zynq_test_rules.mk

# -----------------------------------------------------------------------------
#                                clean up

clean:
ifneq (,$(OBJ_DIR_BASE))
	rm -rf $(CUR_DIR)/$(OBJ_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*
endif

cleanx:
ifneq (,$(VPP_DIR_BASE))
	rm -rf $(CUR_DIR)/$(VPP_DIR_BASE)*
endif
ifneq (,$(XO_DIR_BASE))
	rm -rf $(CUR_DIR)/$(XO_DIR_BASE)*
endif
ifneq (,$(XCLBIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(XCLBIN_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*/emconfig.json
endif

cleanall: clean cleanx
	rm -rf *.log plist $(DATA_STAMP)
	rm -rf *.ini sd_card* emulation _vimage .Xil
# -----------------------------------------------------------------------------
#                                simulation run

$(BIN_DIR)/emconfig.json :
	emconfigutil --platform $(XPLATFORM) --od $(BIN_DIR)

    ifeq ($(TARGET), $(filter $(TARGET),sw_emu hw_emu))
        EMU_CONFIG = $(BIN_DIR)/emconfig.json
    endif

emulate : host xclbin $(EMU_CONFIG) sd_card
ifeq ($(TARGET), $(filter $(TARGET),sw_emu hw_emu))
       RUN_ENV += mkdir -p sd_card/data/emulation;
       RUN_ENV += cp -rf $(XILINX_VITIS)/data/emulation/unified sd_card/data/emulation;
       RUN_ENV += mkfatimg sd_card sd_card.img 500000;
       EMU_CMD = launch_emulator -no-reboot -runtime ocl -t $(TARGET) -sd-card-image sd_card.img -device-family $(DEV_FAM);
else ifeq ($(TARGET), hw)
       RUN_ENV += 
       EMU_CMD += @echo "Please copy the contents of sd_card and input images to an SD Card and run on the board"
endif

sd_card : host xclbin
	@echo "Generating sd_card folder...."
	mkdir -p $(SDCARD)/$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)
	cp -rf $(XCLBIN_DIR)/*.xclbin $(SDCARD)/$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)
	cp -rf $(B_NAME)/sw/$(XDEVICE)/boot/generic.readme $(B_NAME)/sw/$(XDEVICE)/xrt/image/* $(EXE_FILE) $(HOST_ARGS) $(SDCARD)
ifeq ($(TARGET), $(filter $(TARGET),sw_emu hw_emu))
	@echo 'cd /mnt/' >> $(SDCARD)/init.sh
	@echo 'export XILINX_VITIS=$$PWD' >> $(SDCARD)/init.sh
	@echo 'export XCL_BINDIR=$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)' >> $(SDCARD)/init.sh
	@echo 'export XCL_EMULATION_MODE=$(TARGET)' >> $(SDCARD)/init.sh
	@echo './$(EXE_NAME).$(EXE_EXT) $(notdir $(HOST_ARGS))' >> $(SDCARD)/init.sh
	@echo 'reboot' >> $(SDCARD)/init.sh
	@echo "sd_card folder generation done!"
else ifeq ($(TARGET), hw)
	@echo 'cp -rf $(XCLBIN_DIR)/BOOT.BIN $(SDCARD)/'
	@echo 'export XCL_BINDIR=$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)' >> $(SDCARD)/init.sh
	@echo './$(EXE_NAME).$(EXE_EXT) $(notdir $(HOST_ARGS))' >> $(SDCARD)/init.sh
	@echo "sd_card folder generation done!"
endif



.PHONY: run run_sw_emu run_hw_emu run_hw check

run_sw_emu:
	make TARGET=sw_emu run

run_hw_emu:
	make TARGET=hw_emu run

run_hw:
	make TARGET=hw run

run: host xclbin $(EMU_CONFIG) emulate
	$(RUN_ENV)
	$(EMU_CMD)

check: run
# MK_INC_END vitis_zynq_test_rules.mk
else
# MK_INC_BEGIN vitis_host_rules.mk

OBJ_DIR_BASE ?= obj
BIN_DIR_BASE ?= bin

BIN_DIR_SUFFIX ?= _$(XDEVICE)

OBJ_DIR = $(CUR_DIR)/$(OBJ_DIR_BASE)$(BIN_DIR_SUFFIX)
BIN_DIR = $(CUR_DIR)/$(BIN_DIR_BASE)$(BIN_DIR_SUFFIX)

CXX := xcpp
CC := gcc

CXXFLAGS += -std=c++14 -fPIC \
	-I$(SRC_DIR) -I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include \
	-Wall -Wno-unknown-pragmas -Wno-unused-label -pthread
CFLAGS +=
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -lxilinxopencl
LDFLAGS += -L$(XILINX_VIVADO)/lnx64/tools/fpo_v7_0 -Wl,--as-needed -lgmp -lmpfr \
	   -lIp_floating_point_v7_0_bitacc_cmodel

OBJ_FILES = $(foreach s,$(SRCS),$(OBJ_DIR)/$(basename $(s)).o)

define host_hdr_dep
$(1)_HDRS := $$(wildcard $(SRC_DIR)/$(1).h $(SRC_DIR)/$(1).hpp)
$(1)_HDRS += $$($(1)_EXTRA_HDRS)
endef

$(foreach s,$(SRCS),$(eval $(call host_hdr_dep,$(basename $(s)))))

$(OBJ_DIR)/%.o: CXXFLAGS += $($(*)_CXXFLAGS)

$(OBJ_FILES): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $$($$(*)_HDRS) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling object $*..."
	mkdir -p $(@D)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

EXTRA_OBJ_FILES = $(foreach f,$(EXTRA_OBJS),$(OBJ_DIR)/$(f).o)

$(EXTRA_OBJ_FILES): $(OBJ_DIR)/%.o: $$($$(*)_SRCS) $$($$(*)_HDRS) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling extra object $@..."
	mkdir -p $(@D)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

EXE_EXT ?= exe
EXE_FILE ?= $(BIN_DIR)/$(EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)

$(EXE_FILE): $(OBJ_FILES) $(EXTRA_OBJ_FILES) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling host $(notdir $@)..."
	mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

.PHONY: host
host: $(EXE_FILE) | check_vpp check_xrt check_platform

# MK_INC_END vitis_host_rules.mk
# MK_INC_BEGIN vitis_test_rules.mk

# -----------------------------------------------------------------------------
#                                clean up

clean:
ifneq (,$(OBJ_DIR_BASE))
	rm -rf $(CUR_DIR)/$(OBJ_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*
endif

cleanx:
ifneq (,$(VPP_DIR_BASE))
	rm -rf $(CUR_DIR)/$(VPP_DIR_BASE)*
endif
ifneq (,$(XO_DIR_BASE))
	rm -rf $(CUR_DIR)/$(XO_DIR_BASE)*
endif
ifneq (,$(XCLBIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(XCLBIN_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*/emconfig.json
endif

cleanall: clean cleanx
	rm -rf *.log plist $(DATA_STAMP)

# -----------------------------------------------------------------------------
#                                simulation run

$(BIN_DIR)/emconfig.json :
	emconfigutil --platform $(XPLATFORM) --od $(BIN_DIR)

ifeq ($(TARGET),sw_emu)
RUN_ENV += export XCL_EMULATION_MODE=sw_emu;
EMU_CONFIG = $(BIN_DIR)/emconfig.json
else ifeq ($(TARGET),hw_emu)
RUN_ENV += export XCL_EMULATION_MODE=hw_emu;
EMU_CONFIG = $(BIN_DIR)/emconfig.json
else ifeq ($(TARGET),hw)
RUN_ENV += echo "TARGET=hw";
EMU_CONFIG =
endif

.PHONY: run run_sw_emu run_hw_emu run_hw check

run_sw_emu:
	make TARGET=sw_emu run

run_hw_emu:
	make TARGET=hw_emu run

run_hw:
	make TARGET=hw run

run: host xclbin $(EMU_CONFIG) $(DATA_STAMP)
	$(RUN_ENV) \
	$(EXE_FILE) $(HOST_ARGS)

check: run

# MK_INC_END vitis_test_rules.mk
endif



.PHONY: build
build: xclbin host

