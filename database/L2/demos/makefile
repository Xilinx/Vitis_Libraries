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
CUR_DIR ?= $(CUR_DIR)

.SECONDEXPANSION:

# -----------------------------------------------------------------------------
#                            common setup

.PHONY: help

help::
	@echo ""
	@echo "Makefile Usage:"
	@echo ""
	@echo "  make run TARGET=<sw_emu|hw_emu|hw>"
	@echo "      Command to build xclbin files to be used in demos"
	@echo ""
	@echo "  make run TARGET=<sw_emu|hw_emu|hw> TB=<Q1|Q2|...> MDOE=<FPGA|CPU>"
	@echo "      Command to run a specific demo."
	@echo ""
	@echo "  make clean "
	@echo "      Command to remove the generated non-hardware files."
	@echo ""

# Target check
TARGET ?= sw_emu
ifeq ($(filter $(TARGET),sw_emu hw_emu hw),)
$(error TARGET is not sw_emu, hw_emu or hw)
endif

.PHONY: all
all: host

DEVICE ?= xilinx_u280_xdma_201920_3
ifneq ($(findstring u280, $(DEVICE)), u280)
$(error [ERROR]: This project is not supported for $(DEVICE).)
endif


HOST_ARCH := x86
include $(CUR_DIR)/utils.mk

XDEVICE := $(basename $(notdir $(firstword $(XPLATFORM))))

# -----------------------------------------------------------------------------
# data creation and other user targets

DATA_STAMP := $(CUR_DIR)/db_data/dat$(SF)/.stamp
$(DATA_STAMP):
	make -C $(CUR_DIR)/db_data

.PHONY: data
data: $(DATA_STAMP)

# -----------------------------------------------------------------------------
# kernel setup

# override the default and use a user provide path
XCLBIN_FILE_H = $(CUR_DIR)/build_join_partition/xclbin_$(XDEVICE)_$(TARGET)/gqe_join.xclbin
XCLBIN_FILE_A = $(CUR_DIR)/build_aggr_partition/xclbin_$(XDEVICE)_$(TARGET)/gqe_aggr.xclbin

$(XCLBIN_FILE_H):
	make -C $(CUR_DIR)/build_join_partition xclbin
$(XCLBIN_FILE_A):
	make -C $(CUR_DIR)/build_aggr_partition xclbin

xclbin: $(XCLBIN_FILE_A) $(XCLBIN_FILE_H)

# -----------------------------------------------------------------------------
# host setup

XFLIB_DIR = $(abspath $(CUR_DIR)/../..)
SRC_BASE_DIR = $(XFLIB_DIR)/L2/demos/host

MODE ?= FPGA
SF ?= 1

ifeq ($(MODE),CPU)
  TB_DIR = cpu
else ifeq ($(MODE),FPGA)
ifeq ($(SF),1)
  TB_DIR = sf1_fpga
else ifeq ($(SF),30)
  TB_DIR = sf30_fpga
else
  $(error Scale factor other than 1 and 30 is not supported in host code)
endif # SF
else
  $(error Please set MODE as either 'fpga' or 'cpu')
endif # MODE

TB ?= Q2
HOST_ARGS = -xclbin $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -b 0

ifeq ($(TB),Q1)
  EXE_NAME = test_q1_$(MODE)_$(SF)
  SRCS = test_q1.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q01/$(TB_DIR)
  HOST_ARGS = -xclbin $(XCLBIN_FILE_A) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -p 22 -b 0
else ifeq ($(TB),Q2)
  EXE_NAME = test_q2_$(MODE)_$(SF)
  SRCS = test_q2.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q02/$(TB_DIR)
else ifeq ($(TB),Q3)
  EXE_NAME = test_q3_$(MODE)_$(SF)
  SRCS = test_q3.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q03/$(TB_DIR)
  HOST_ARGS = -xclbin_a $(XCLBIN_FILE_A) -xclbin_h $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF)
else ifeq ($(TB),Q4)
  EXE_NAME = test_q4_$(MODE)_$(SF)
  SRCS = test_q4.cpp
  HOST_ARGS = -xclbin $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -p 22 -b 0
  SRC_DIR = $(SRC_BASE_DIR)/q04/$(TB_DIR)
else ifeq ($(TB),Q5)
  EXE_NAME = test_q5_$(MODE)_$(SF)
  SRCS = test_q5.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q05/$(TB_DIR)
else ifeq ($(TB),Q6)
  EXE_NAME = test_q6_$(MODE)_$(SF)
  SRCS = test_q6.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q06/$(TB_DIR)
else ifeq ($(TB),Q7)
  EXE_NAME = test_q7_$(MODE)_$(SF)
  SRCS = test_q7.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q07/$(TB_DIR)
  HOST_ARGS = -xclbin $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -b 0 -p 2
else ifeq ($(TB),Q8)
  EXE_NAME = test_q8_$(MODE)_$(SF)
  SRCS = test_q8.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q08/$(TB_DIR)
else ifeq ($(TB),Q9)
  EXE_NAME = test_q9_$(MODE)_$(SF)
  SRCS = test_q9.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q09/$(TB_DIR)
  HOST_ARGS = -xclbin $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -b 0 -p 32
else ifeq ($(TB),Q10)
  EXE_NAME = test_q10_$(MODE)_$(SF)
  SRCS = test_q10.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q10/$(TB_DIR)
  HOST_ARGS = -xclbin_a $(XCLBIN_FILE_A) -xclbin_h $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF)
else ifeq ($(TB),Q11)
  EXE_NAME = test_q11_$(MODE)_$(SF)
  SRCS = test_q11.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q11/$(TB_DIR)
  HOST_ARGS = -xclbin_a $(XCLBIN_FILE_A) -xclbin_h $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF)
else ifeq ($(TB),Q12)
  EXE_NAME = test_q12_$(MODE)_$(SF)
  SRCS = test_q12.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q12/$(TB_DIR)
  HOST_ARGS = -xclbin $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -b 0 -p 23
else ifeq ($(TB),Q13)
  EXE_NAME = test_q13_$(MODE)_$(SF)
  SRCS = test_q13.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q13/$(TB_DIR)
  HOST_ARGS = -xclbin_a $(XCLBIN_FILE_A) -xclbin_h $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -pa 18 -ph 32
else ifeq ($(TB),Q14)
  EXE_NAME = test_q14_$(MODE)_$(SF)
  SRCS = test_q14.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q14/$(TB_DIR)
else ifeq ($(TB),Q15)
  EXE_NAME = test_q15_$(MODE)_$(SF)
  SRCS = test_q15.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q15/$(TB_DIR)
  HOST_ARGS = -xclbin $(XCLBIN_FILE_A) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -p 22 -b 0
else ifeq ($(TB),Q16)
  EXE_NAME = test_q16_$(MODE)_$(SF)
  SRCS = test_q16.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q16/$(TB_DIR)
else ifeq ($(TB),Q17)
  EXE_NAME = test_q17_$(MODE)_$(SF)
  SRCS = test_q17.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q17/$(TB_DIR)
else ifeq ($(TB),Q18)
  EXE_NAME = test_q18_$(MODE)_$(SF)
  SRCS = test_q18.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q18/$(TB_DIR)
  HOST_ARGS = -xclbin_a $(XCLBIN_FILE_A) -xclbin_h $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -p 20
else ifeq ($(TB),Q19)
  EXE_NAME = test_q19_$(MODE)_$(SF)
  SRCS = test_q19.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q19/$(TB_DIR)
else ifeq ($(TB),Q20)
  EXE_NAME = test_q20_$(MODE)_$(SF)
  SRCS = test_q20.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q20/$(TB_DIR)
  HOST_ARGS = -xclbin_a $(XCLBIN_FILE_A) -xclbin_h $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -p 22
else ifeq ($(TB),Q21)
  EXE_NAME = test_q21_$(MODE)_$(SF)
  SRCS = test_q21.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q21/$(TB_DIR)
  HOST_ARGS = -xclbin $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -p 64
else ifeq ($(TB),Q22)
  EXE_NAME = test_q22_$(MODE)_$(SF)
  SRCS = test_q22.cpp
  SRC_DIR = $(SRC_BASE_DIR)/q22/$(TB_DIR)
  HOST_ARGS = -xclbin $(XCLBIN_FILE_H) -in $(CUR_DIR)/db_data/dat$(SF)  -c $(SF) -p 16
endif

test_q1_EXTRA_HDRS += $(SRC_DIR)/q1.hpp

test_q2_EXTRA_HDRS += $(SRC_DIR)/q2.hpp

test_q3_EXTRA_HDRS += $(SRC_DIR)/q3.hpp

test_q4_EXTRA_HDRS += $(SRC_DIR)/q4.hpp

test_q7_EXTRA_HDRS += $(SRC_DIR)/q5.hpp

test_q7_EXTRA_HDRS += $(SRC_DIR)/q6.hpp

test_q7_EXTRA_HDRS += $(SRC_DIR)/q7.hpp

test_q8_EXTRA_HDRS += $(SRC_DIR)/q8.hpp

test_q9_EXTRA_HDRS += $(SRC_DIR)/q9.hpp

test_q10_EXTRA_HDRS += $(SRC_DIR)/q10.hpp

test_q11_EXTRA_HDRS += $(SRC_DIR)/q11.hpp

test_q12_EXTRA_HDRS += $(SRC_DIR)/q12.hpp

test_q13_EXTRA_HDRS += $(SRC_DIR)/q13.hpp

test_q14_EXTRA_HDRS += $(SRC_DIR)/q14.hpp

test_q15_EXTRA_HDRS += $(SRC_DIR)/q15.hpp

test_q16_EXTRA_HDRS += $(SRC_DIR)/q16.hpp

test_q17_EXTRA_HDRS += $(SRC_DIR)/q17.hpp

test_q18_EXTRA_HDRS += $(SRC_DIR)/q18.hpp

test_q19_EXTRA_HDRS += $(SRC_DIR)/q19.hpp

test_q20_EXTRA_HDRS += $(SRC_DIR)/q20.hpp

test_q21_EXTRA_HDRS += $(SRC_DIR)/q21.hpp

test_q22_EXTRA_HDRS += $(SRC_DIR)/q22.hpp


CXXFLAGS += -D XDEVICE=$(XDEVICE) -I$(XFLIB_DIR)/L1/include/hw -I$(XFLIB_DIR)/L3/include/sw -I$(SRC_BASE_DIR)  -g

# EXTRA_OBJS is cannot be compiled from SRC_DIR, user should provide the rule
EXTRA_OBJS += xcl2

EXT_DIR = $(XFLIB_DIR)/ext
xcl2_SRCS = $(EXT_DIR)/xcl2/xcl2.cpp
xcl2_HDRS = $(EXT_DIR)/xcl2/xcl2.hpp
xcl2_CXXFLAGS = -I$(EXT_DIR)/xcl2
CXXFLAGS += $(xcl2_CXXFLAGS)

.PHONY: debugvar
debugvar:
	@echo $(EXE_NAME)
	@echo $(EXE_FILE)

BIN_DIR_SUFFIX := _$(MODE)_$(SF)_$(XDEVICE)

# -----------------------------------------------------------------------------

# MK_INC_BEGIN vitis_host_rules.mk

OBJ_DIR_BASE ?= obj
BIN_DIR_BASE ?= bin

BIN_DIR_SUFFIX ?= _$(XDEVICE)

OBJ_DIR = $(CUR_DIR)/$(OBJ_DIR_BASE)$(BIN_DIR_SUFFIX)
BIN_DIR = $(CUR_DIR)/$(BIN_DIR_BASE)$(BIN_DIR_SUFFIX)

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
host: check_vpp check_xrt check_platform $(EXE_FILE)

# MK_INC_END vitis_host_rules.mk

# -----------------------------------------------------------------------------
#                                clean up

clean:
ifneq ($(OBJ_DIR_BASE),)
	rm -rf $(OBJ_DIR_BASE)*
endif
ifneq ($(BIN_DIR_BASE),)
	rm -rf $(BIN_DIR_BASE)*
endif

cleanall: clean
	rm -rf  $(DATA_STAMP)

# -----------------------------------------------------------------------------
#                                simulation run

$(BIN_DIR)/emconfig.json :
	emconfigutil --platform $(XPLATFORM) --od $(BIN_DIR) --nd 2

ifeq ($(TARGET),sw_emu)
EMU_MODE = export XCL_EMULATION_MODE=sw_emu
EMU_CONFIG = $(BIN_DIR)/emconfig.json
else ifeq ($(TARGET),hw_emu)
EMU_MODE = export XCL_EMULATION_MODE=hw_emu
EMU_CONFIG = $(BIN_DIR)/emconfig.json
else ifeq ($(TARGET),hw)
EMU_MODE = echo "TARGET=hw"
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
	$(EMU_MODE); \
	$(EXE_FILE) $(HOST_ARGS)

check: run

