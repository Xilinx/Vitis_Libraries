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
	@echo "	 make run DEBUG=yes TARGET=<sw_emu|hw_emu>"
	@echo "	 	 Command to run application in emulation"
	@echo ""
	@echo "  make clean "
	@echo "      Command to remove the generated non-hardware files."
	@echo ""
	@echo "  make cleanall"
	@echo "      Command to remove all the generated files."
	@echo ""


# -----------------------------------------------------------------------------
#                          common de.cfgtion for building hw 
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))                          
include ./params_u280.mk
else
include ./params_u50.mk
endif
# -----------------------------------------------------------------------------

# Target check
TARGET ?= sw_emu
ifeq (,$(filter $(TARGET),sw_emu hw_emu hw))
$(error TARGET is not sw_emu, hw_emu or hw)
endif
HOST_ARCH := x86
SYSROOT := $(SYSROOT)
DEVICE ?= xilinx_u280_xdma_201920_1
#DEVICE ?= xilinx_u50_gen3x16_xdma_201920_3

ifneq ($(findstring u280, $(DEVICE)), u280)
ifneq ($(findstring u50, $(DEVICE)), u50)
$(error [ERROR]: This project is not supported for $(DEVICE).)
endif
endif

include ./utils.mk

ifeq (sw_emu,$(TARGET))
	SEQ_KERNEL=1
else
	SEQ_KERNEL=0
endif

TOOL_VERSION ?= 2020.1
DEBUG = no 

ifeq (,$(XILINX_VIVADO))
XILINX_VIVADO = /opt/xilinx/vivado/$(TOOL_VERSION)
endif
export XILINX_VIVADO

BOOST_INCLUDE = $(XILINX_VIVADO)/tps/boost_1_64_0
BOOST_LIB = $(XILINX_VIVADO)/lib/lnx64.o
XDEVICE := $(call device2xsa, $(DEVICE))

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

XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L2/tests/*}')
XFLIB_DIR := $(abspath $(XF_PROJ_ROOT))

# -----------------------------------------------------------------------------

KSRC_DIR = $(XFLIB_DIR)/L2/src/hw

XCLBIN_NAME := cscmv
ifneq (sw_emu,$(TARGET))
	KERNELS := loadColKernel:loadColKernel.cpp bufTransColVecKernel:bufTransColVecKernel.cpp bufTransNnzColKernel:bufTransNnzColKernel.cpp xBarColKernel:xBarColKernel.cpp cscRowKernel:cscRowKernel.cpp readWriteHbmKernel:readWriteHbmKernel.cpp      
else
	KERNELS := cscmvSeqKernel:cscmvSeqKernel.cpp
endif

EXTRA_HDRS = $(XFLIB_DIR)/L2/include/hw/cscKernel.hpp \
		     $(XFLIB_DIR)/L2/include/hw/cscMatMoverL2.hpp

# FIXME L1 dependency still missing

VPP_FLAGS += -I$(XFLIB_DIR)/L1/include/hw \
		       -I$(XFLIB_DIR)/L2/include/hw \
		       -I$(XFLIB_DIR)/../blas/L1/include/hw

# U280
ifneq (sw_emu,$(TARGET))
ifneq (,$(shell echo $(XPLATFORM) | awk '/u280/'))
	VPP_LDFLAGS += --config $(CUR_DIR)/conn_u280.cfg
else
	VPP_LDFLAGS += --config $(CUR_DIR)/conn_u50.cfg
endif 
else
ifneq (,$(shell echo $(XPLATFORM) | awk '/u280/'))
	VPP_LDFLAGS += --config $(CUR_DIR)/conn_u280_seq.cfg
else
	VPP_LDFLAGS += --config $(CUR_DIR)/conn_u50_seq.cfg
endif 
endif

ifneq (sw_emu,$(TARGET))
	VPP_LDFLAGS += --config opts.cfg
else 
	VPP_LDFLAGS += --config opts_seq.cfg
endif

# -----------------------------------------------------------------------------

EXE_NAME = cscmv
GEN_PARTITION_EXE_NAME = gen_partition
GEN_BIN_EXE_NAME = gen_bin
#
#HOST_ARGS = -xclbin $(XCLBIN_FILE) -in $(CUR_DIR)/db_data/dat
HOST_ARGS = $(XCLBIN_FILE) $(BIN_DIR)/app.bin 
#
SRC_DIR = $(XFLIB_DIR)/L2/src/sw
#
CXXFLAGS += -I$(XFLIB_DIR)/L2/include/sw -I$(XFLIB_DIR)/L1/include/sw
GEN_PARTITION_CXXFLAGS += -I$(XFLIB_DIR)/L2/include/sw -I$(XFLIB_DIR)/L1/include/sw -I$(BOOST_INCLUDE)
GEN_BIN_CXXFLAGS += -I$(XFLIB_DIR)/L2/include/sw -I$(XFLIB_DIR)/L1/include/sw -I$(BOOST_INCLUDE)
#
ifneq (sw_emu,$(TARGET))
	SRCS = xcl2.cpp cscmv.cpp
else
	SRCS = xcl2.cpp cscmvSeq.cpp
endif
GEN_PARTITION_SRCS = gen_partition.cpp
GEN_BIN_SRCS = gen_bin.cpp

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

XFREQUENCY ?= 250

VPP = v++

ifeq (no,$(DEBUG))
	VPP_FLAGS += -O3
else
	VPP_FLAGS += -g -O0
endif

VPP_FLAGS += -I$(KSRC_DIR)
VPP_FLAGS += $(COMMON_DEFS)
VPP_FLAGS += --target $(TARGET) -f $(XPLATFORM) --temp_dir $(VPP_DIR) --save-temps
VPP_FLAGS += --kernel_frequency $(XFREQUENCY) --report_level 2

MAKE_GEN_INI_FILE ?= $(CUR_DIR)/make_gen_$(XDEVICE).cfg
.PHONY: write.cfg
ifneq (,$(MAKE_GEN_INI))
write.cfg: export MAKE_GEN_INI := $(MAKE_GEN_INI)
write.cfg:
	@echo "----Generating $(notdir $(MAKE_GEN_INI_FILE)) ..."
	@echo "$${MAKE_GEN_INI}" > $(MAKE_GEN_INI_FILE)
VPP_FLAGS += --config $(MAKE_GEN_INI_FILE)
endif
ifneq (sw_emu,$(TARGET))
	VPP_FLAGS += --config compile_opts.cfg
else
	VPP_FLAGS += --config compile_opts_seq.cfg
endif

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

#$(XO_DIR)/%.xo: VPP_FLAGS += $($(*)_VPP_FLAGS)
#$(XO_DIR)/%.xo: $$($$(*)_SRCS) $$($$(*)_HDRS) | check_vpp
$(XO_DIR)/%.xo: $(%_SRCS) | check_vpp 
	@echo -e "----\nCompiling kernel $*...\n"
	mkdir -p $(XO_DIR)
	$(VPP) -o $@ --kernel $* --compile $(filter %.cpp,$($*_SRCS)) \
		$(VPP_FLAGS)

$(XCLBIN_FILE): $(XO_FILES) | check_vpp
	@echo -e "----\nCompiling xclbin $(XCLBIN_DIR)/$(XCLBIN_NAME)... $(XCLBIN_FILE)"
	mkdir -p $(XCLBIN_DIR)
	$(VPP) -o $@ --link $^ \
		$(VPP_FLAGS) $(VPP_LDFLAGS) \
		$(foreach k,$(KERNEL_NAMES),$($(k)_VPP_FLAGS)) \
		$(foreach k,$(KERNEL_NAMES),$($(k)_VPP_LDFLAGS))

.PHONY: xo xclbin

xo: write.cfg check_vpp check_platform $(XO_FILES)

xclbin: write.cfg check_vpp check_platform $(XCLBIN_FILE)

# MK_INC_END vitis_kernel_rules.mk

# MK_INC_BEGIN vitis_host_rules.mk

OBJ_DIR_BASE ?= obj
BIN_DIR_BASE ?= bin

BIN_DIR_SUFFIX ?= _$(XDEVICE)

OBJ_DIR = $(CUR_DIR)/$(OBJ_DIR_BASE)$(BIN_DIR_SUFFIX)
BIN_DIR = $(CUR_DIR)/$(BIN_DIR_BASE)$(BIN_DIR_SUFFIX)

ifeq (no,$(DEBUG))
	CXXFLAGS += -O3
	GEN_PARTITION_CXXFLAGS += -O3
	GEN_BIN_CXXFLAGS += -O3
else
	CXXFLAGS += -g -O0
	GEN_PARTITION_CXXFLAGS += -g -O0
	GEN_BIN_CXXFLAGS += -g -O0
endif
CXXFLAGS += -std=c++11 -fPIC \
	-I$(XILINX_XRT)/include -I$(XILINX_VIVADO)/include \
	-Wall -Wno-unknown-pragmas -Wno-unused-label
CXXFLAGS += $(COMMON_DEFS)
GEN_PARTITION_CXXFLAGS += -std=c++11
GEN_PARTITION_CXXFLAGS += $(GEN_PARTITION_DEFS)

GEN_BIN_CXXFLAGS += -std=c++11
GEN_BIN_CXXFLAGS += $(GEN_BIN_DEFS)

ifneq ($(HOST_ARCH), x86)
	LDCLFLAGS += --sysroot=$(SYSROOT)
endif

LDCLFLAGS += -lpthread -L$(XILINX_XRT)/lib -lOpenCL
LDCLFLAGS += -L$(XILINX_VIVADO)/lnx64/tools/fpo_v7_0 -Wl,--as-needed -lgmp -lmpfr \
	   -lIp_floating_point_v7_0_bitacc_cmodel \
	   -lstdc++ \
	   -lrt \
	   -Wl,--rpath=$(XILINX_XRT)/lib
GEN_PARTITION_LDFLAGS += -lpthread -L$(XILINX_XRT)/lib \
				 -L$(BOOST_LIB) \
				 -lboost_iostreams -lz \
				 -lstdc++ \
				 -lrt \
	   		     -Wl,--rpath=$(XILINX_XRT)/lib

GEN_BIN_LDFLAGS += -lpthread -L$(XILINX_XRT)/lib \
				 -L$(BOOST_LIB) \
				 -lboost_iostreams -lz \
				 -lstdc++ \
				 -lrt \
	   		     -Wl,--rpath=$(XILINX_XRT)/lib

OBJ_FILES = $(foreach s,$(SRCS),$(OBJ_DIR)/$(basename $(s)).o)
GEN_PARTITION_OBJ_FILES = $(foreach s,$(GEN_PARTITION_SRCS),$(OBJ_DIR)/$(basename $(s)).o)
GEN_BIN_OBJ_FILES = $(foreach s,$(GEN_BIN_SRCS),$(OBJ_DIR)/$(basename $(s)).o)


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling object $*..."
	mkdir -p $(OBJ_DIR)
	$(CXX) -o $@ -c $< $(CXXFLAGS)

EXE_EXT ?= exe
EXE_FILE ?= $(BIN_DIR)/$(EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)
GEN_PARTITION_EXE_FILE ?= $(BIN_DIR)/$(GEN_PARTITION_EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)
GEN_BIN_EXE_FILE ?= $(BIN_DIR)/$(GEN_BIN_EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)

$(EXE_FILE): $(OBJ_FILES) | check_vpp check_xrt check_platform
	@echo -e "----\nCompiling host $(notdir $@)..."
	mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDCLFLAGS)

$(GEN_PARTITION_EXE_FILE): $(GEN_PARTITION_OBJ_FILES)
	@echo -e "----\nCompiling gen_partition $(notdir $@)..."
	mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(GEN_PARTITION_CXXFLAGS) $(GEN_PARTITION_LDFLAGS)

$(GEN_BIN_EXE_FILE): $(GEN_BIN_OBJ_FILES)
	@echo -e "----\nCompiling gen_bin $(notdir $@)..."
	mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(GEN_BIN_CXXFLAGS) $(GEN_BIN_LDFLAGS)

.PHONY: gen_partition gen_bin host

gen_partition: $(GEN_PARTITION_EXE_FILE)
gen_bin: $(GEN_BIN_EXE_FILE)
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
RUN_ENV += XCL_EMULATION_MODE=sw_emu
EMU_CONFIG = $(BIN_DIR)/emconfig.json
else ifeq ($(TARGET),hw_emu)
RUN_ENV += XCL_EMULATION_MODE=hw_emu
EMU_CONFIG = $(BIN_DIR)/emconfig.json
else ifeq ($(TARGET),hw)
RUN_ENV += echo "TARGET=hw";
EMU_CONFIG = $(BIN_DIR)/emconfig.json
endif

.PHONY: run check

MTX_FILE = $(XFLIB_DIR)/L2/tests/mtxFiles/bcsstm01.mtx
run: gen_bin host xclbin $(EMU_CONFIG)
	$(GEN_BIN_EXE_FILE) -config-write $(BIN_DIR)/app.bin $(BIN_DIR)/inVec.bin $(BIN_DIR)/outVec.bin $(MTX_FILE)
	cp $(BIN_DIR)/emconfig.json ./
	$(RUN_ENV) $(EXE_FILE) $(HOST_ARGS)

check: run

# MK_INC_END vitis_test_rules.mk

.PHONY: build
build: xclbin host
