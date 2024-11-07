#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
# Makefile helper used for MRFFT compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

DATA_TYPE      ?= cint16
TWIDDLE_TYPE   ?= cint16
POINT_SIZE     ?= 48
FFT_NIFFT      ?= 1
SHIFT          ?= 5
ROUND_MODE     ?= 0
SAT_MODE       ?= 1
WINDOW_VSIZE   ?= 48
CASC_LEN       ?= 1
DYN_PT_SIZE    ?= 0
API_IO         ?= 0
PARALLEL_POWER ?= 0
DIFF_MODE      ?= ABS
DIFF_TOLERANCE ?= 4
CC_TOLERANCE   ?= 0.05  # TODO a -1 is present in the ref output (dynamic dft result) so there is 1 difference in each test

#The data generation does not need to be dynamic. Creating MAX point size of data each iteration is fine - just use a subset. The header is dynamic.
DATA_DYN_PT_SIZE = 0

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
PT_SIZE_PWR    = 4

ifeq ($(AIE_VARIANT), 1)
	ifeq ($(API_IO),0)
		INPUTS_PER_TILE := 1
	else
		INPUTS_PER_TILE := 2
	endif
else
	INPUTS_PER_TILE := 1
endif

NUM_INPUTS := $(INPUTS_PER_TILE)
#ifeq ($(PARALLEL_POWER), 0)
#	NUM_INPUTS=$$((1*$(INPUTS_PER_TILE)))
#else ifeq ($(PARALLEL_POWER), 1)
#	NUM_INPUTS=$$((2*$(INPUTS_PER_TILE)))
#else ifeq ($(PARALLEL_POWER), 2)
#	NUM_INPUTS=$$((4*$(INPUTS_PER_TILE)))
#else ifeq ($(PARALLEL_POWER), 3)
#	NUM_INPUTS=$$((8*$(INPUTS_PER_TILE)))
#else ifeq ($(PARALLEL_POWER), 4)
#	NUM_INPUTS=$$((16*$(INPUTS_PER_TILE)))
#else ifeq ($(PARALLEL_POWER), 5)
#	NUM_INPUTS=$$((32*$(INPUTS_PER_TILE)))
#else ifeq ($(PARALLEL_POWER), 6)
#	NUM_INPUTS=$$((64*$(INPUTS_PER_TILE)))
#endif

NUM_OUTPUTS = $(NUM_INPUTS)

PARAM_MAP = DATA_TYPE $(DATA_TYPE) TWIDDLE_TYPE $(TWIDDLE_TYPE) POINT_SIZE $(POINT_SIZE) FFT_NIFFT $(FFT_NIFFT) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE) WINDOW_VSIZE $(WINDOW_VSIZE) CASC_LEN $(CASC_LEN) API_IO $(API_IO) DYN_PT_SIZE $(DYN_PT_SIZE) AIE_VARIANT $(AIE_VARIANT)

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): create_config validate_config create_input sim_ref prep_x86_out
#$(HELPER): create_config create_input prep_x86_out
	make cleanall
	
create_config:
	echo creating configuration;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

validate_config:
	echo validating configuration;\
	vitis --classic -exec ipmetadata_config_checker $(HELPER_ROOT_DIR)/L2/meta/mixed_radix_fft.json ./config.json -newflow

create_input:
	@echo starting create_input
	@echo NUM_INPUTS $(NUM_INPUTS)
	@echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DATA_DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE) $(API_IO) 1  ${PARALLEL_POWER} 0 0 0 0 64;
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DATA_DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE) $(API_IO) 1  ${PARALLEL_POWER} 0 0 0 0 64;
	@echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_mrfft_header.tcl $(INPUT_HEADER_FILE) $(WINDOW_VSIZE) $(NITER) $(DYN_PT_SIZE) $(POINT_SIZE) $(DATA_TYPE) $(DATA_SEED) $(STIM_TYPE);
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_mrfft_header.tcl $(INPUT_HEADER_FILE) $(WINDOW_VSIZE) $(NITER) $(DYN_PT_SIZE) $(POINT_SIZE) $(DATA_TYPE) $(DATA_SEED) $(STIM_TYPE);
	@echo perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE) --ssr $(NUM_INPUTS) --split --dual 0 -k $(DYN_PT_SIZE) -w ${WINDOW_VSIZE} --plioWidth 64;
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE) --ssr $(NUM_INPUTS) --split --dual 0 -k $(DYN_PT_SIZE) -w ${WINDOW_VSIZE} --plioWidth 64; 
	echo Input ready

sim_ref:
	@echo starting sim_ref
	make UUT_KERNEL=mixed_radix_fft_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@echo starting prep_x86_out
	@if [ -s $(HELPER_CUR_DIR)/x86simulator_output/data ]; then \
		x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data` ;\
		echo "X86 files= " $$x86_out_files ;\
		for n in $$x86_out_files; do \
			grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n ;\
		done ;\
	fi

prep_aie_out:
		@echo starting prep_aie_out
		@if [ -s $(HELPER_CUR_DIR)/aiesimulator_output/data ]; then \
				aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
				echo "AIE files= " $$aie_out_files;\
				for n in $$aie_out_files; do \
					grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
				done \
		fi

get_diff:
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_OUTPUTS) --zip --dual 0 -k $(DYN_PT_SIZE) -w ${WINDOW_VSIZE} --plioWidth 64 ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_OUTPUTS) --zip --dual 0 -k $(DYN_PT_SIZE) -w ${WINDOW_VSIZE} --plioWidth 64 ;\
	if [ "$(DYN_PT_SIZE)" = "1" ]; then \
		tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/mixed_radix_fft/output_post_proc.tcl $(HELPER_CUR_DIR)/data/uut_output.txt $(WINDOW_VSIZE) $(NITER) $(POINT_SIZE) $(HELPER_CUR_DIR)/header_pointsizes.txt $(DATA_TYPE) 64 ;\
	fi ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) $(DIFF_MODE)


get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output mixed_radix_ $(NITER)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

cleanup:
	make cleanall
