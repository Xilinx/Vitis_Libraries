#+-------------------------------------------------------------------------------
# The following parameters are assigned with default values. These parameters can
# be overridden through the make command line
#+-------------------------------------------------------------------------------

REPORT := no
PROFILE := no
DEBUG := no

#'estimate' for estimate report generation
#'system' for system report generation
ifneq ($(REPORT), no)
CLFLAGS += --report estimate
CLFLAGS += --report system
endif

#Generates profile summary report
ifeq ($(PROFILE), yes)
CLFLAGS += --profile_kernel data:all:all:all
endif

#Generates debug summary report
ifeq ($(DEBUG), yes)
CLFLAGS += --dk protocol:all:all:all
endif

#Checks for XILINX_SDX
#ifndef XILINX_SDX
#$(error XILINX_SDX variable is not set, please set correctly and rerun)
#endif

#   sanitize_xsa - create a filesystem friendly name from xsa name
#   $(1) - name of xsa
COLON=:
PERIOD=.
UNDERSCORE=_
sanitize_xsa = $(strip $(subst $(PERIOD),$(UNDERSCORE),$(subst $(COLON),$(UNDERSCORE),$(1))))

device2xsa = $(if $(filter $(suffix $(1)),.xpfm),$(shell $(COMMON_REPO)/utility/parsexpmf.py $(1) xsa 2>/dev/null),$(1))
device2sanxsa = $(call sanitize_xsa,$(call device2xsa,$(1)))
device2dep = $(if $(filter $(suffix $(1)),.xpfm),$(dir $(1))/$(shell $(COMMON_REPO)/utility/parsexpmf.py $(1) hw 2>/dev/null) $(1),)

# Cleaning stuff
RM = rm -f
RMDIR = rm -rf

ECHO:= @echo

