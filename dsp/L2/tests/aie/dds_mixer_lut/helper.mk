#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
DIFF_MODE ?= ABS
CC_TOLERANCE ?= 0
# INPUT_FILE ?=
UUT_SSR_INPUT_WINDOW_VSIZE ?=
SPLIT_ZIP_FILE ?=
PARAM_MAP = DATA_TYPE $(DATA_TYPE) \
			MIXER_MODE $(MIXER_MODE) \
			USE_PHASE_RELOAD $(USE_PHASE_RELOAD) \
			P_API $(P_API) \
			UUT_SSR $(UUT_SSR) \
			INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE) \
			ROUND_MODE $(ROUND_MODE) \
			SAT_MODE $(SAT_MODE) \
			NITER $(NITER) \
			INITIAL_DDS_OFFSET $(INITIAL_DDS_OFFSET) \
			DDS_PHASE_INC $(DDS_PHASE_INC) \
			DATA_SEED $(DATA_SEED) \
			DATA_STIM_TYPE $(DATA_STIM_TYPE) \
			SFDR $(SFDR) \
			DIFF_MODE $(DIFF_MODE) \
			DIFF_TOLERANCE $(DIFF_TOLERANCE) \
			AIE_VARIANT $(AIE_VARIANT)\
            PHASE_RELOAD_API $(PHASE_RELOAD_API)\
            USE_PHASE_INC_RELOAD $(USE_PHASE_INC_RELOAD)

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
ifneq ($(UUT_SSR), 1)
	ifeq ($(shell expr $(SFDR) \> 60), 1)
		ifeq ($(DATA_TYPE), cint32)
			DIFF_TOLERANCE := 25750
		endif
		ifeq ($(DATA_TYPE), cint16)
			DIFF_TOLERANCE := 5
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
	@echo helper.mk stage:  diff
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) $(DIFF_MODE)
	@echo diff script done

gen_input:
	@echo helper.mk stage:  gen_input
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) 0 1 0 0 0 0 0 64

gen_phase_offset:
	@echo helper.mk stage:  gen_phase_offset
#8 because the uint32 single value has to go into a mimimum window size of 32 bytes or 8 uint32s
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) 8 $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 int32 0 1 0 0 0 0 0 64

ssr_split:
	@echo helper.mk stage:  ssr_split
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --split --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE) --plioWidth 64

ssr_zip:
	@echo helper.mk stage:  ssr_zip
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --zip --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE) --plioWidth 64

get_status:
	@echo helper.mk stage:  get_status
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) USE_OUTPUTS_IF_NO_INPUTS

get_stats:
	@echo helper.mk stage:	get_stats
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output dds $(NITER)



harvest_mem:
	@echo helper.mk stage:	harvest_mem
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
