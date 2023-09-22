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

############################## Help Section ##############################
.PHONY: help

help::
	@echo ""
	@echo "Makefile Usage:"
	@echo ""
	@echo "  make run TARGET=<cosim/csim/csynth/vivado_syn/vivado_impl> PLATFORM=<FPGA platform> PLATFORM_REPO_PATHS=<path to platform directories>"
	@echo "      Command to run the selected tasks for specified device."
	@echo ""
	@echo "      Valid tasks are cosim, csynth, csim, vivado_syn, vivadp_impl"
	@echo ""
	@echo "      PLATFORM is case-insensitive and support awk regex."
	@echo "      For example, \`make run PLATFORM='u200.*xdma' TARGET=cosim\`"
	@echo "      It can also be an absolute path to platform file."
	@echo ""
	@echo "      PLATFORM_REPO_PATHS variable is used to specify the paths in which the platform files will be"
	@echo "      searched for."
	@echo ""
	@echo "  make run TARGET=cosim/csim/csynth/vivado_syn/vivado_impl XPART=<FPGA part name>"
	@echo "      Alternatively, the FPGA part can be speficied via XPART."
	@echo "      For example, \`make run XPART='xcu200-fsgd2104-2-e' TARGET=cosim\`"
	@echo "      When XPART is set, PLATFORM will be ignored."
	@echo ""
	@echo "  make clean "
	@echo "      Command to remove the generated files."
	@echo ""

############################## Setting up Project Variables ##############################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
export CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
export XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L1/*}')

# setting default value
PLATFORM ?= xilinx_u200_gen3x16_xdma_2_202110_1

export PATH := $(XILINX_VIVADO)/bin:$(PATH)
WORK_DIR ?= hls
TARGET ?= csim
CONFIG_FILE ?= $(CUR_DIR)/hls_config.cfg
CONFIG_TMPL ?= $(CUR_DIR)/hls_config.tmpl

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

############################## Checking value ##############################
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
# MK_INC_BEGIN vitis_set_platform.mk

ifneq (,$(wildcard $(PLATFORM)))
# Use PLATFORM as a file path
XPLATFORM := $(PLATFORM)
else
# Use PLATFORM as a file name pattern
DEVICE_L := $(shell echo $(PLATFORM) | tr A-Z a-z)
# 1. search paths specified by variable
ifneq (,$(PLATFORM_REPO_PATHS))
# 1.1 as exact name
XPLATFORM := $(strip $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/$(DEVICE_L)/$(DEVICE_L).xpfm)))
# 1.2 as a pattern
ifeq (,$(XPLATFORM))
XPLATFORMS := $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/*/*.xpfm))
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif # 1.2
endif # 1
# 2. search Vitis installation
ifeq (,$(XPLATFORM))
# 2.1 as exact name vitis < 2022.2
XPLATFORM := $(strip $(wildcard $(XILINX_VITIS)/platforms/$(DEVICE_L)/$(DEVICE_L).xpfm))
ifeq (,$(XPLATFORM))
# 2.2 as exact name vitis >= 2022.2
XPLATFORM := $(strip $(wildcard $(XILINX_VITIS)/base_platforms/$(DEVICE_L)/$(DEVICE_L).xpfm))
# 2.3 as a pattern vitis < 2022.2
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard $(XILINX_VITIS)/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
# 2.4 as a pattern vitis > 2022.2
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard $(XILINX_VITIS)/base_platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif # 2.4
endif # 2.3
endif # 2.2
endif # 2
# 3. search default locations
ifeq (,$(XPLATFORM))
# 3.1 as exact name
XPLATFORM := $(strip $(wildcard /opt/xilinx/platforms/$(DEVICE_L)/$(DEVICE_L).xpfm))
# 3.2 as a pattern
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard /opt/xilinx/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif # 3.2
endif # 3
endif
XPLATFORM := $(firstword $(XPLATFORM))

XDEVICE := $(basename $(notdir $(XPLATFORM)))

ifeq (1, $(words $(XPLATFORM)))
# Query the part name of device
ifneq (,$(wildcard $(XILINX_VITIS)/bin/platforminfo))
override XPART := $(shell $(XILINX_VITIS)/bin/platforminfo --json="hardwarePlatform.devices[0].fpgaPart" --platform $(XPLATFORM) | sed 's/^[^:]*://g' | sed 's/[^a-zA-Z0-9]/-/g' | sed 's/-\+/-/g')
endif
else
PART_ERROR := "To add more platform directories, set the PLATFORM_REPO_PATHS variable or point PLATFORM variable to the full path of platform .xpfm file."
endif

check_part: check_vpp
ifeq (,$(XPART))
	@echo "$(PART_ERROR)"
	@echo "XPART is not set and cannot be inferred. Please run \`make help\` for usage info." && false
endif
else # XPART
check_part:
	@echo "Using part $(XPART)"
endif # XPART

define CONFIG_GEN_PY
import os, string
with open('$(CONFIG_TMPL)', 'r') as fr:
	t = fr.read()
with open('$(CONFIG_FILE)', 'w') as f:
	f.write(string.Template(t).substitute(**dict(os.environ)))
endef
export CONFIG_GEN_PY

VITIS_PYTHON3 = LD_LIBRARY_PATH=$(XILINX_VITIS)/tps/lnx64/python-3.8.3/lib $(XILINX_VITIS)/tps/lnx64/python-3.8.3/bin/python3

$(CONFIG_FILE): $(CONFIG_TMPL)
	@echo "$${CONFIG_GEN_PY}" | (${VITIS_PYTHON3})
	
all: check_vivado check_part $(CONFIG_FILE)
ifneq ($(TARGET_REL), csim)
	v++ -c --mode hls --config $(CONFIG_FILE) --work_dir $(WORK_DIR) --part $(XPART)
endif

run: all
ifneq ($(TARGET_REL), csynth)
	@echo $(TARGET_REL)
	vitis-run --mode hls --config $(CONFIG_FILE) --$(TARGET_REL) --work_dir $(WORK_DIR)  --part $(XPART)
endif

clean:
	rm -rf $(CONFIG_FILE) *_hls.log $(WORK_DIR)