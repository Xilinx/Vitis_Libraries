# By Default report is set to none, so report will be generated
# 'estimate' for estimate report generation
# 'system' for system report generation
REPORT:=none

# Default C++ Compiler Flags and xocc compiler flags
CXXFLAGS:=-Wall -O0 -g
CLFLAGS:= -s

ifneq ($(REPORT),none)
CLFLAGS += --report $(REPORT)
endif 

LDCLFLAGS:=$(CLFLAGS)

ifdef XILINX_VITIS
XILINX_VITIS=${XILINX_VITIS}
endif

ifndef XILINX_VITIS
$(error XILINX_VITIS or XILINX_VITIS is not set. Please source the Vitis settings64.{csh,sh} script before attempting to run examples)
endif

VIVADO=$(XILINX_VITIS)/Vivado/bin/vivado


# Use the Xilinx OpenCL compiler
CLC:=$(XILINX_VITIS)/bin/xocc
LDCLC:=$(CLC)
EMCONFIGUTIL := $(XILINX_VITIS)/bin/emconfigutil

# By default build for X86, this could also be set to POWER to build for power
ARCH:=X86

ifeq ($(ARCH),POWER)
DEVICES:= xilinx:adm-pcie-7v3:1ddr-ppc64le:2.1
CXX:=$(XILINX_VITIS)/gnu/ppc64le/4.9.3/lnx64/bin/powerpc64le-linux-gnu-g++
else
DEVICES:= xilinx_vcu1525_dynamic_5_1
CXX:=g++
endif

#if COMMON_REPO is not defined use the default value support existing Designs
COMMON_REPO ?= ../../

# By default build for hardware can be set to
#   hw_emu for hardware emulation
#   sw_emu for software emulation
#   or a collection of all or none of these
TARGETS:=hw

# By default only have one device in the system
NUM_DEVICES:=1

# sanitize_xsa - create a filesystem friendly name from xsa name
#   $(1) - name of xsa
COLON=:
PERIOD=.
UNDERSCORE=_
sanitize_xsa = $(strip $(subst $(PERIOD),$(UNDERSCORE),$(subst $(COLON),$(UNDERSCORE),$(1))))

device2xsa = $(if $(filter $(suffix $(1)),.xpfm),$(shell $(COMMON_REPO)/utility/parsexpmf.py $(1) xsa 2>/dev/null),$(1))
device2sanxsa = $(call sanitize_xsa,$(call device2xsa,$(1)))
device2dep = $(if $(filter $(suffix $(1)),.xpfm),$(dir $(1))/$(shell $(COMMON_REPO)/utility/parsexpmf.py $(1) hw 2>/dev/null) $(1),)


