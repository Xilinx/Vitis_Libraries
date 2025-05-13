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

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
DIFF_MODE ?= PERCENT
DIFF_TOLERANCE ?= 0.01
ifeq ($(AIE_VARIANT), 2)
DIFF_MODE          = ABS
endif
ifeq ($(TT_TWIDDLE), cint32)
DIFF_MODE          = ABS
DIFF_TOLERANCE     = 4
endif
DATA_SEED ?= 0

INPUT_WINDOW_VSIZE = $$(($(WINDOW_VSIZE_D1)*$(WINDOW_VSIZE_D2)))

# dummy value, should not be used effectively
PT_SIZE_PWR = 4
DYN_PT_SIZE = 0
PARALLEL_POWER = 0

ifeq ($(AIE_VARIANT), 1)
	ifeq ($(API_IO),0)
		INPUTS_PER_TILE := 1
	else
		INPUTS_PER_TILE := 2
	endif
else
	INPUTS_PER_TILE := 1
endif

ifeq ($(PARALLEL_POWER), 0)
	NUM_INPUTS=$$((1*$(INPUTS_PER_TILE)))
else ifeq ($(PARALLEL_POWER), 1)
	NUM_INPUTS=$$((2*$(INPUTS_PER_TILE)))
else ifeq ($(PARALLEL_POWER), 2)
	NUM_INPUTS=$$((4*$(INPUTS_PER_TILE)))
else ifeq ($(PARALLEL_POWER), 3)
	NUM_INPUTS=$$((8*$(INPUTS_PER_TILE)))
else ifeq ($(PARALLEL_POWER), 4)
	NUM_INPUTS=$$((16*$(INPUTS_PER_TILE)))
else ifeq ($(PARALLEL_POWER), 5)
	NUM_INPUTS=$$((32*$(INPUTS_PER_TILE)))
else ifeq ($(PARALLEL_POWER), 6)
	NUM_INPUTS=$$((64*$(INPUTS_PER_TILE)))
endif

ifeq ($(DYN_PT_SIZE), 1)
	DYN_PT_HEADER_MODE := 1
else
	DYN_PT_HEADER_MODE := 0
endif

NUM_OUTPUTS = $(NUM_INPUTS)

PARAM_MAP = DATA_TYPE_D1 $(DATA_TYPE_D1) DATA_TYPE_D2 $(DATA_TYPE_D2) TWIDDLE_TYPE $(TWIDDLE_TYPE) POINT_SIZE_D1 $(POINT_SIZE_D1)  POINT_SIZE_D2 $(POINT_SIZE_D2) FFT_NIFFT $(FFT_NIFFT) SHIFT $(SHIFT) CASC_LEN $(CASC_LEN) WINDOW_VSIZE_D1 $(WINDOW_VSIZE_D1) WINDOW_VSIZE_D2 $(WINDOW_VSIZE_D2) API_IO $(API_IO) USE_WIDGETS $(USE_WIDGETS) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE) TWIDDLE_MODE $(TWIDDLE_MODE) DATA_OUT_TYPE $(DATA_OUT_TYPE) AIE_VARIANT $(AIE_VARIANT)

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER):s create_input
	make cleanall

create_input:
	@echo starting create_input
	@echo NUM_INPUTS $(NUM_INPUTS)
	@echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE_D1) $(API_IO) 1  ${PARALLEL_POWER} 0 0 0 0 64;
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE_D1) $(API_IO) 1  ${PARALLEL_POWER} 0 0 0 0 64;
	@echo perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE_D1) --ssr $(NUM_INPUTS) --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} --plioWidth 64;
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE_D1) --ssr $(NUM_INPUTS) --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} --plioWidth 64;
	echo Input ready

sim_ref:
	@echo starting sim_ref
	make UUT_KERNEL=fft_ifft_dit_1ch_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

get_diff:
	@echo starting get_diff
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE_D2) --ssr $(NUM_OUTPUTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} --plioWidth 64 ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE_D2) --ssr $(NUM_OUTPUTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} --plioWidth 64
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) $(DIFF_MODE);\
    tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) $(DIFF_MODE)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output fftMain $(NITER)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
