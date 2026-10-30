# Copyright (C) 2026-2026, Advanced Micro Devices, Inc.
# SPDX-License-Identifier: X11

.PHONY: help

help::
	@echo ""
	@echo "Makefile Usage:"
	@echo "  make run TARGET=<csim/cosim/csynth/vivado_syn/vivado_impl> XPART=<FPGA part name> NUM_MOTORS=<count>"
	@echo "  make clean"
	@echo ""

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
export CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
export XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L1/*}')

export PATH := $(XILINX_VIVADO)/bin:$(PATH)
WORK_DIR ?= hls
TARGET ?= csim
NUM_MOTORS ?= 1
export NUM_MOTORS
ifeq ($(NUM_MOTORS),1)
export TB_FILE := $(CUR_DIR)/src/test_ip_foc_multiple_model.cpp
export MODEL_SIM_CFLAGS := -DIP_FOC_MODEL_SIM -I$(XF_PROJ_ROOT)/L1/tests/IP_FOC/src -I$(XF_PROJ_ROOT)/L1/tests/Model
else
export TB_FILE := $(CUR_DIR)/src/test_ip_foc_multiple.cpp
export MODEL_SIM_CFLAGS :=
endif
CONFIG_FILE ?= $(CUR_DIR)/hls_config.cfg
ifeq ($(TARGET), $(filter $(TARGET), csim cosim))
CONFIG_TMPL ?= $(CUR_DIR)/hls_config_sim.tmpl
else
CONFIG_TMPL ?= $(CUR_DIR)/hls_config.tmpl
endif
ifneq (,$(wildcard $(XILINX_VITIS)/bin/ldlibpath.sh))
export LD_LIBRARY_PATH := $(shell $(XILINX_VITIS)/bin/ldlibpath.sh $(XILINX_VITIS)/lib/lnx64.o):$(LD_LIBRARY_PATH)
endif

ifeq ($(TARGET), vivado_syn)
TARGET_REL = impl
export VIVADO_FLOW := syn
else
export VIVADO_FLOW := impl
ifeq ($(TARGET), vivado_impl)
TARGET_REL = impl
else
TARGET_REL = $(TARGET)
endif
endif

.PHONY: check_vivado
check_vivado:
ifeq (,$(wildcard $(XILINX_VIVADO)/bin/vivado))
	@echo "Cannot locate Vivado installation. Please set XILINX_VIVADO variable." && false
endif

.PHONY: check_vpp
check_vpp:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/v++))
	@echo "Cannot locate Vitis installation. Please set XILINX_VITIS variable." && false
endif

.PHONY: check_part
ifeq (,$(XPART))
check_part: check_vpp
	@echo "XPART is not set. Please pass XPART=<part>." && false
else
check_part:
	@echo "Using part $(XPART)"
endif

define CONFIG_GEN_PY
import os, string
with open('$(CONFIG_TMPL)', 'r') as fr:
	t = fr.read()
with open('$(CONFIG_FILE)', 'w') as f:
	f.write(string.Template(t).substitute(**dict(os.environ)))
endef
export CONFIG_GEN_PY

$(CONFIG_FILE): $(CONFIG_TMPL)
	@echo "$${CONFIG_GEN_PY}" | python3

all: check_vivado check_part $(CONFIG_FILE)
ifneq ($(TARGET_REL), csim)
	v++ -c --mode hls --config $(CONFIG_FILE) --work_dir $(WORK_DIR) --part $(XPART)
endif

run: all
ifneq ($(TARGET_REL), csynth)
	@echo $(TARGET_REL)
	vitis-run --mode hls --config $(CONFIG_FILE) --$(TARGET_REL) --work_dir $(WORK_DIR) --part $(XPART)
endif

clean:
	rm -rf $(CONFIG_FILE) *_hls.log hls hls_* *.cfg
