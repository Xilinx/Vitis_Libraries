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

.SECONDEXPANSION:

.PHONY: help
help:
	@echo "Please refer to README.md in parent directory for makefile usage."

# Target check
TARGET ?= sw_emu
ifeq ($(filter $(TARGET),sw_emu hw_emu hw),)
$(error TARGET is not sw_emu, hw_emu or hw)
endif

# -----------------------------------------------------------------------------
#                            common tool setup

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
endif

# MK_INC_END vitis.mk

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

XDEVICE := $(basename $(notdir $(firstword $(XPLATFORM))))

# MK_INC_END vitis_set_platform.mk

# -----------------------------------------------------------------------------

.PHONY: debug
debug:
	@echo "KERNELS are $(KERNELS)"
	@echo "> KERNEL_NAMES are $(KERNEL_NAMES)"
	@echo "> gqeJoin_SRCS is $(gqeJoin_SRCS)"
	@echo "> gqeJoin_HDRS is $(gqeJoin_HDRS)"
	@echo "> gqeJoin_VPP_CFLAGS is $(gqeJoin_VPP_CFLAGS)"
	@echo "> gqePart_SRCS is $(gqePart_SRCS)"
	@echo "> gqePart_HDRS is $(gqePart_HDRS)"
	@echo "> gqePart_VPP_CFLAGS is $(gqePart_VPP_CFLAGS)"

XFLIB_DIR = $(abspath $(CUR_DIR)/../../..)
KSRC_DIR = $(XFLIB_DIR)/L2/src

XCLBIN_NAME := gqe_join
KERNELS := gqeJoin:gqe_join.cpp gqePart:gqe_part.cpp

gqeJoin_EXTRA_HDRS = $(XFLIB_DIR)/L2/include/gqe_join.hpp
# FIXME still many missing
gqeJoin_VPP_CFLAGS += -I$(XFLIB_DIR)/L1/include/hw \
		       -I$(XFLIB_DIR)/L2/include \
		       -I$(XFLIB_DIR)/../utils/L1/include

gqePart_EXTRA_HDRS += $(XFLIB_DIR)/L2/include/gqe_part.hpp
# FIXME still many missing
gqePart_VPP_CFLAGS += -I$(XFLIB_DIR)/L1/include/hw \
		       -I$(XFLIB_DIR)/L2/include \
		       -I$(XFLIB_DIR)/../utils/L1/include

ifneq (,$(shell echo $(XPLATFORM) | awk '/u280/'))
# U280
VPP_LFLAGS += --config conn_u280.ini
CXXFLAGS += -DUSE_HBM
else ifneq (,$(XPLATFORM))
$(warning Unsupported platform $(XPLATFORM))
endif

# -----------------------------------------------------------------------------

VPP_LFLAGS += --config opts.ini 

XFREQUENCY := 200

# -----------------------------------------------------------------------------

.PHONY: all
all: xclbin

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

MAKE_GEN_INI_FILE ?= $(CUR_DIR)/make_gen_$(XDEVICE).ini
.PHONY: write_ini
ifneq (,$(MAKE_GEN_INI))
write_ini: export MAKE_GEN_INI := $(MAKE_GEN_INI)
write_ini:
	@echo "----Generating $(notdir $(MAKE_GEN_INI_FILE)) ..."
	@echo "$${MAKE_GEN_INI}" > $(MAKE_GEN_INI_FILE)
VPP_CFLAGS += --config $(MAKE_GEN_INI_FILE)
endif

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

xo: write_ini check_vpp check_platform $(XO_FILES)

xclbin: write_ini check_vpp check_platform $(XCLBIN_FILE)

# MK_INC_END vitis_kernel_rules.mk

