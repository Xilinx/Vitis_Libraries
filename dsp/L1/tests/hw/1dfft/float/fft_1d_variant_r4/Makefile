# Copyright (C) 2026 Advanced Micro Devices, Inc.
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

# FFT 1D HLS Radix-4 test (vitis_fft_variant library).
# make run TARGET=csim | csynth | cosim | vivado_syn | vivado_impl
# Needs: source Vitis env; PLATFORM or XPART for csynth/cosim/impl.

############################## Help Section ##############################
.PHONY: help
help::
	@echo ""
	@echo "Makefile Usage:"
	@echo "  make run TARGET=<cosim/csim/csynth/vivado_syn/vivado_impl> PLATFORM=<FPGA platform> PLATFORM_REPO_PATHS=<path to platform directories>"
	@echo "      Valid tasks: cosim, csynth, csim, vivado_syn, vivado_impl"
	@echo "  make run TARGET=... XPART=<FPGA part name>"
	@echo "  make clean"
	@echo ""

############################## Setting up Project Variables ##############################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
export CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
export XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L1/*}')

CLOCK ?= 2.5
export HLS_CLOCK := $(CLOCK)
NUM_FFT ?= 4096
export NUM_FFT
PLATFORM ?= vck190
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
	@echo "Cannot locate Vivado. Set XILINX_VIVADO." && false
endif

.PHONY: check_vpp
check_vpp:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/v++))
	@echo "Cannot locate Vitis v++. Set XILINX_VITIS." && false
endif

.PHONY: check_part
ifeq (,$(XPART))
# Platform to XPART resolution (simplified)
ifneq (,$(wildcard $(PLATFORM)))
XPLATFORM := $(PLATFORM)
else
DEVICE_L := $(shell echo $(PLATFORM) | tr A-Z a-z)
ifneq (,$(PLATFORM_REPO_PATHS))
XPLATFORM := $(strip $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/$(DEVICE_L)/$(DEVICE_L).xpfm)))
ifeq (,$(XPLATFORM))
XPLATFORMS := $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/*/*.xpfm))
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
endif
ifeq (,$(XPLATFORM))
XPLATFORM := $(strip $(wildcard $(XILINX_VITIS)/base_platforms/$(DEVICE_L)/$(DEVICE_L).xpfm))
endif
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard $(XILINX_VITIS)/base_platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(DEVICE_L)/')))
endif
endif
XPLATFORM := $(firstword $(XPLATFORM))
ifneq (,$(XPLATFORM))
# Use known part for default platforms (platforminfo can pollute XPART on some envs)
ifeq (vck190,$(DEVICE_L))
override XPART := xcvc1902-vsva2197-2MP-e-S
else
ifneq (,$(wildcard $(XILINX_VITIS)/bin/platforminfo))
override XPART := $(shell $(XILINX_VITIS)/bin/platforminfo --json="hardwarePlatform.devices[0].fpgaPart" --platform $(XPLATFORM) 2>/dev/null | sed 's/^[^:]*://g' | sed 's/[^a-zA-Z0-9]/-/g' | sed 's/-\+/-/g')
endif
endif
endif
PART_ERROR := Set PLATFORM (e.g. vck190) or XPART (e.g. xcvc1902-vsva2197-2MP-e-S).

check_part: check_vpp
ifeq (,$(XPART))
	@echo "$(PART_ERROR)"
	@echo "Run: make run TARGET=csim  (no part needed), or set PLATFORM/XPART for csynth/cosim/impl." && false
endif
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

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" -exec test -f '{}/bin/python3' \; -print 2>/dev/null | head -n 1)
VITIS_PYTHON3 = $(if $(TAPYTHON),LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3,python3)

# Always regenerate hls_config.cfg from the template: Make cannot know when
# NUM_FFT or CLOCK changed on the command line, so a stale cfg would keep
# -DNUM_FFT=4096 even for NUM_FFT=16384 runs.
$(CONFIG_FILE): $(CONFIG_TMPL) FORCE
	@echo "$${CONFIG_GEN_PY}" | ($(VITIS_PYTHON3))

FORCE:

# csim: config only (no Vivado required for config). csynth/cosim/impl: check_vivado + check_part + v++ -c
ifeq ($(TARGET_REL), csim)
all: $(CONFIG_FILE)
	@echo "Config ready for csim (NUM_FFT=$(NUM_FFT))."
else
all: check_vivado $(CONFIG_FILE)
all: check_part
	v++ -c --mode hls --config $(CONFIG_FILE) --work_dir $(WORK_DIR) --part $(XPART)
endif

.PHONY: run clean
run: all
ifneq ($(TARGET_REL), csynth)
	@echo "Running: $(TARGET_REL)"
	vitis-run --mode hls --config $(CONFIG_FILE) --$(TARGET_REL) --work_dir $(WORK_DIR) --part $(XPART)
endif

clean:
	rm -rf $(CONFIG_FILE) *_hls.log $(WORK_DIR)
