#
# Copyright 2021 Xilinx, Inc.
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

###############################################################################
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################
# Values for diff_tolerance are obtained experimentally. They are non-zero for streaming cases because
# number of lanes differs between reference model and uut. They are non-zero for ssr cases since uut and reference
# model split the lookup angle into differet sub-angles. Sincos lookup values of these sub-angles suffer from information
# loss due to rounding. Further, there is a rounding error from their multiplication.
HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
DIFF_TOLERANCE ?= 4
CC_TOLERANCE ?= 0
INPUT_FILE ?= 
UUT_SSR_INPUT_WINDOW_VSIZE ?=
SPLIT_ZIP_FILE ?=

ifneq ($(UUT_SSR), 1)
	ifeq ($(shell expr $(SFDR) \> 60), 1)
		ifeq ($(DATA_TYPE), cint32)
			DIFF_TOLERANCE := 25750
		endif
		ifeq ($(DATA_TYPE), cint16)
			DIFF_TOLERANCE := 4
		endif
		ifeq ($(DATA_TYPE), cfloat)
			DIFF_TOLERANCE := 0.8
		endif	
	else
		ifeq ($(DATA_TYPE), cfloat)
			DIFF_TOLERANCE := 450
		endif
		ifeq ($(DATA_TYPE), cint32)
			DIFF_TOLERANCE := 26351012
		endif
		ifeq ($(DATA_TYPE), cint16)
			DIFF_TOLERANCE := 520
		endif	
	endif
endif

diff:
	@echo executing diff.tcl with new diff_tolerances
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) abs
	@echo diff script done

gen_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) 0 1

ssr_split:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --split --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE)

ssr_zip:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --zip --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE)

get_latency:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output dds $(NITER)

get_theoretical_min:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_dds_theoretical_min.tcl $(DATA_TYPE) $(MIXER_MODE) $(UUT_SSR_INPUT_WINDOW_VSIZE) $(UUT_SSR) $(STATUS_FILE) $(UUT_KERNEL)