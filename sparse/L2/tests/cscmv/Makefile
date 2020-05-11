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

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))

.PHONY: help

help::
	@echo "Makefile Usage:"
	@echo "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA PLATFORM>"
	@echo "      Command to generate the design for specified Target and Shell."
	@echo ""
	@echo "		 TARGET defaults to sw_emu."
	@echo ""
	@echo "      DEVICE is case-insensitive and support awk regex."
	@echo "      For example, \`make xclbin TARGET=hw DEVICE='u280.*xdma'\`"
	@echo "      It can also be an absolute path to platform file."
	@echo ""
	@echo "	 Use 'host' or 'xclbin' as make target to build only wanted binary."
	@echo ""
	@echo "	 make run DEBUG=1 TARGET=<sw_emu|hw_emu>"
	@echo "	 	 Command to run application in emulation"
	@echo ""
	@echo "  make clean "
	@echo "      Command to remove the generated non-hardware files."
	@echo ""
	@echo "  make cleanall"
	@echo "      Command to remove all the generated files."
	@echo ""


# -----------------------------------------------------------------------------
#                          common definition for building hw 
include ./common.mk
# -----------------------------------------------------------------------------

# Target check
TARGET ?= sw_emu
ifeq (,$(filter $(TARGET),sw_emu hw_emu hw))
$(error TARGET is not sw_emu, hw_emu or hw)
endif

TOOL_VERSION ?= 2020.1
DEBUG = 0

ifeq (,$(XILINX_VIVADO))
XILINX_VIVADO = /opt/xilinx/Vivado/$(TOOL_VERSION)
endif
export XILINX_VIVADO

.PHONY: check_vivado
check_vivado:
ifeq (,$(wildcard $(XILINX_VIVADO)/bin/Vivado))
	@echo "Cannot locate Vivado installation. Please set XILINX_VIVADO variable." && false
endif

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

#$(info PATH before = $(PATH))
PATH := $(XILINX_XRT)/bin:$(XILINX_VITIS)/bin:$(XILINX_VIVADO)/bin:$(XILINX_VIVADO)/tps/lnx64/gcc-6.2.0/bin:$(PATH)
#$(info PATH after = $(PATH))

ifeq (,$(LD_LIBRARY_PATH))
LD_LIBRARY_PATH := $(XILINX_XRT)/lib:$(XILINX_VIVADO)/tps/lnx64/gcc-6.2.0/lib64
else
LD_LIBRARY_PATH := $(XILINX_XRT)/lib:$(XILINX_VIVADO)/tps/lnx64/gcc-6.2.0/lib64:$(LD_LIBRARY_PATH)
endif
ifneq (,$(wildcard $(XILINX_VITIS)/bin/ldlibpath.sh))
export LD_LIBRARY_PATH := $(shell $(XILINX_VITIS)/bin/ldlibpath.sh $(XILINX_VITIS)/lib/lnx64.o):$(LD_LIBRARY_PATH)
endif

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

XDEVICE := $(basename $(notdir $(firstword $(XPLATFORM))))

# -----------------------------------------------------------------------------
# BEGIN_XF_MK_USER_SECTION
# -----------------------------------------------------------------------------

DATA_STAMP := $(CUR_DIR)/db_data/dat/.stamp
$(DATA_STAMP):
	make -C $(CUR_DIR)/db_data

.PHONY: data
data: $(DATA_STAMP)

.PHONY: debug
debug:
	@echo "KERNELS are $(KERNELS)"
	@echo "> KERNEL_NAMES are $(KERNEL_NAMES)"
	@echo "> XO_DIR is $(XO_DIR)"
	@echo "> loadColPtrValKernel_SRCS is $(loadColPtrValKernel_SRCS)"
	@echo "> loadColPtrValKernel_HDRS is $(loadColPtrValKernel_HDRS)"
	@echo "> loadColPtrValKernel_VPP_CFLAGS is $(loadColPtrValKernel_VPP_CFLAGS)"
	@echo "> xBarColKernel_SRCS is $(xBarColKernel_SRCS)"
	@echo "> xBarColKernel_HDRS is $(xBarColKernel_HDRS)"
	@echo "> xBarColKernel_VPP_CFLAGS is $(xBarColKernel_VPP_CFLAGS)"
	@echo "> cscRowPktKernel_SRCS is $(cscRowPktKernel_SRCS)"
	@echo "> cscRowPktKernel_HDRS is $(cscRowPktKernel_HDRS)"
	@echo "> cscRowPktKernel_VPP_CFLAGS is $(cscRowPktKernel_VPP_CFLAGS)"
	@echo "> storeDatPktKernel_SRCS is $(storeDatPktKernel_SRCS)"
	@echo "> storeDatPktKernel_HDRS is $(storeDatPktKernel_HDRS)"
	@echo "> storeDatPktKernel_VPP_CFLAGS is $(storeDatPktKernel_VPP_CFLAGS)"

XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L2/tests/*}')
XFLIB_DIR := $(abspath $(XF_PROJ_ROOT))

# -----------------------------------------------------------------------------

KSRC_DIR = $(XFLIB_DIR)/L2/src/hw

XCLBIN_NAME := cscmv
KERNELS := loadColPtrValKernel:loadColPtrValKernel.cpp xBarColKernel:xBarColKernel.cpp cscRowPktKernel:cscRowPktKernel.cpp storeDatPktKernel:storeDatPktKernel.cpp      

EXTRA_HDRS = $(XFLIB_DIR)/L2/include/hw/cscKernel.hpp \
		     $(XFLIB_DIR)/L2/include/hw/cscMatMoverL2.hpp
# FIXME L1 dependency still missing

VPP_CFLAGS += -I$(XFLIB_DIR)/L1/include/hw \
		       -I$(XFLIB_DIR)/L2/include/hw \
		       -I$(XFLIB_DIR)/../blas/L1/include/hw

ifneq (,$(shell echo $(XPLATFORM) | awk '/u280/'))
# U280
VPP_LFLAGS += --config conn_u280.ini
else ifneq (,$(XPLATFORM))
$(warning Unsupported platform $(XPLATFORM))
endif
VPP_LFLAGS += --config opts.ini

# -----------------------------------------------------------------------------

EXE_NAME = cscmv
#
#HOST_ARGS = -xclbin $(XCLBIN_FILE) -in $(CUR_DIR)/db_data/dat
HOST_ARGS = $(XCLBIN_FILE) 32 16 64 
#
SRC_DIR = $(XFLIB_DIR)/L2/src/sw
#
CXXFLAGS += -I$(XFLIB_DIR)/L2/include/sw -I$(XFLIB_DIR)/L1/include/sw
#
SRCS = xcl2.cpp cscmv.cpp 
#test_q1_EXTRA_HDRS += $(SRC_DIR)/q1.hpp $(SRC_DIR)/cfg.hpp
#test_q1_CXXFLAGS += -I$(EXT_DIR)/xcl2
#
#EXTRA_OBJS += xcl2
#
#EXT_DIR = $(XFLIB_DIR)/ext
#xcl2_SRCS = $(EXT_DIR)/xcl2/xcl2.cpp
#xcl2_HDRS = $(EXT_DIR)/xcl2/xcl2.hpp
#xcl2_CXXFLAGS = -I $(EXT_DIR)/xcl2

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

ifeq (0,$(DEBUG))
	VPP_CFLAGS += -O3
else
	VPP_CFLAGS += -g -O0
endif

VPP_CFLAGS += -I$(KSRC_DIR)
VPP_CFLAGS += $(COMMON_DEFS)
VPP_CFLAGS += --target $(TARGET) --platform $(XPLATFORM) --temp_dir $(VPP_DIR) --save-temps
VPP_CFLAGS += --kernel_frequency $(XFREQUENCY) --report_level 2

MAKE_GEN_INI_FILE ?= $(CUR_DIR)/make_gen_$(XDEVICE).ini
.PHONY: write_ini
ifneq (,$(MAKE_GEN_INI))
write_ini: export MAKE_GEN_INI := $(MAKE_GEN_INI)
write_ini:
	@echo "----Generating $(notdir $(MAKE_GEN_INI_FILE)) ..."
	@echo "$${MAKE_GEN_INI}" > $(MAKE_GEN_INI_FILE)
VPP_CFLAGS += --config $(MAKE_GEN_INI_FILE)
endif
VPP_CFLAGS += --config compile_opts.ini

KERNEL_NAMES := $(foreach k,$(KERNELS),$(word 1, $(subst :, ,$(k))))
XO_FILES := $(foreach k,$(KERNEL_NAMES),$(XO_DIR)/$(k).xo)
XCLBIN_FILE ?= $(XCLBIN_DIR)/$(XCLBIN_NAME).xclbin

define kernel_src_dep
$(word 1, $(subst :, ,$(1)))_SRCS := $(KSRC_DIR)/$(word 2, $(subst :, ,$(1)))
$(word 1, $(subst :, ,$(1)))_SRCS += $($(word 1, $(subst :, ,$(1)))_EXTRA_SRCS)
endef

$(foreach k,$(KERNELS),$(eval $(call kernel_src_dep,$(k))))


define kernel_hdr_dep
kernelname := $(word 1, $(subst :, ,$(1)))
kernelfile := $(if $(findstring :, $(1)),$(basename $(word 2, $(subst :, ,$(1)))),$$(kernelname))
$(kernelname)_HDRS := $(wildcard $(KSRC_DIR)/$(kernelfile).h $(KSRC_DIR)/$(kernelfile).hpp)
$(kernelname)_HDRS += $($(1)_EXTRA_HDRS)
endef

$(foreach k,$(KERNELS),$(eval $(call kernel_hdr_dep,$(k))))

#$(XO_DIR)/%.xo: VPP_CFLAGS += $($(*)_VPP_CFLAGS)
#$(XO_DIR)/%.xo: $$($$(*)_SRCS) $$($$(*)_HDRS) | check_vpp
$(XO_DIR)/%.xo: $(%_SRCS) | check_vpp 
	@echo -e "----\nCompiling kernel $*...\n"
	mkdir -p $(XO_DIR)
	$(VPP) -o $@ --kernel $* --compile $(filter %.cpp,$($*_SRCS)) \
		$(VPP_CFLAGS)

$(XCLBIN_FILE): $(XO_FILES) | check_vpp
	@echo -e "----\nCompiling xclbin $(XCLBIN_DIR)/$(XCLBIN_NAME)..."
	mkdir -p $(XCLBIN_DIR)
	$(VPP) -o $@ --link $^ \
		$(VPP_CFLAGS) $(VPP_LFLAGS) \
		$(foreach k,$(KERNEL_NAMES),$($(k)_VPP_CFLAGS)) \
		$(foreach k,$(KERNEL_NAMES),$($(k)_VPP_LFLAGS))

.PHONY: xo xclbin

xo: write_ini check_vpp check_platform $(XO_FILES)

xclbin: write_ini check_vpp check_platform $(XCLBIN_FILE)

# MK_INC_END vitis_kernel_rules.mk

# MK_INC_BEGIN vitis_host_rules.mk

OBJ_DIR_BASE ?= obj
BIN_DIR_BASE ?= bin

BIN_DIR_SUFFIX ?= _$(XDEVICE)

OBJ_DIR = $(CUR_DIR)/$(OBJ_DIR_BASE)$(BIN_DIR_SUFFIX)
BIN_DIR = $(CUR_DIR)/$(BIN_DIR_BASE)$(BIN_DIR_SUFFIX)

#CXX := xcpp
CXX := g++
CC := gcc

ifeq (0,$(DEBUG))
	CXXFLAGS += -O3
else
	CXXFLAGS += -g -O0
endif
CXXFLAGS += -std=c++14 -fPIC \
	-I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include \
	-Wall -Wno-unknown-pragmas -Wno-unused-label -pthread
CXXFLAGS += $(COMMON_DEFS)
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -lxilinxopencl
LDFLAGS += -L$(XILINX_VIVADO)/lnx64/tools/fpo_v7_0 -Wl,--as-needed -lgmp -lmpfr \
	   -lIp_floating_point_v7_0_bitacc_cmodel \
	   -lstdc++ \
	   -lrt \
	   -Wl,--rpath=$(XILINX_XRT)/lib

OBJ_FILES = $(foreach s,$(SRCS),$(OBJ_DIR)/$(basename $(s)).o)


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling object $*..."
	mkdir -p $(OBJ_DIR)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

EXE_EXT ?= exe
EXE_FILE ?= $(BIN_DIR)/$(EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)

$(EXE_FILE): $(OBJ_FILES) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling host $(notdir $@)..."
	mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

.PHONY: host
host: check_vpp check_xrt check_platform $(EXE_FILE)

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
ifneq (,$(MAKE_GEN_INI_FILE))
	rm -rf $(MAKE_GEN_INI_FILE)
endif

cleanall: clean cleanx
	rm -rf *.log plist $(DATA_STAMP)
	rm -rf .Xil

# -----------------------------------------------------------------------------
#                                simulation run

$(BIN_DIR)/emconfig.json :
	emconfigutil -f $(XPLATFORM) --od $(BIN_DIR)

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

.PHONY: run check

run: host xclbin $(EMU_CONFIG)
	$(RUN_ENV) \
	$(EXE_FILE) $(HOST_ARGS)

check: run

# MK_INC_END vitis_test_rules.mk

.PHONY: build
build: xclbin host
