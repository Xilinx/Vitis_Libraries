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

# This makefile prints generic help of HLS makefiles.

# MK_BEGIN


.PHONY: run setup runhls clean

CSIM ?= 0
CSYNTH ?= 0
COSIM ?= 0
VIVADO_SYN ?= 0
VIVADO_IMPL ?= 0
QOR_CHECK ?= 0


# at least RTL synthesis before check QoR
ifeq (1,$(QOR_CHECK))
ifeq (0,$(VIVADO_IMPL))
override VIVADO_SYN := 1
endif
endif

# need synthesis before cosim or vivado
ifeq (1,$(VIVADO_IMPL))
override CSYNTH := 1
endif

ifeq (1,$(VIVADO_SYN))
override CSYNTH := 1
endif

ifeq (1,$(COSIM))
override CSYNTH := 1
endif

run: setup runhls

setup: | check_part
	@rm -f ./settings.tcl
	@if [ -n "$$CLKP" ]; then echo 'set CLKP $(CLKP)' >> ./settings.tcl ; fi
	@echo 'set XPART $(XPART)' >> ./settings.tcl
	@echo 'set CSIM $(CSIM)' >> ./settings.tcl
	@echo 'set CSYNTH $(CSYNTH)' >> ./settings.tcl
	@echo 'set COSIM $(COSIM)' >> ./settings.tcl
	@echo 'set VIVADO_SYN $(VIVADO_SYN)' >> ./settings.tcl
	@echo 'set VIVADO_IMPL $(VIVADO_IMPL)' >> ./settings.tcl
	@echo 'set QOR_CHECK $(QOR_CHECK)' >> ./settings.tcl
	@echo 'set XF_PROJ_ROOT "$(XF_PROJ_ROOT)"' >> ./settings.tcl
	@echo "Configured: settings.tcl"
	@echo "----"
	@cat ./settings.tcl
	@echo "----"

HLS ?= vivado_hls
runhls: setup | check_vivado
	$(HLS) -f run_hls.tcl;

clean:
	rm -rf *.prj *_hls.log settings.tcl

.PHONY: check
check: run

