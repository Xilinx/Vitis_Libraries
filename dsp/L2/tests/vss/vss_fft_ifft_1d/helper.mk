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
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

UUT_FILE_SUFFIX = $(UUT_KERNEL)_$(DATA_TYPE)_$(TWIDDLE_TYPE)_$(POINT_SIZE)_$(FFT_NIFFT)_$(SHIFT)_$(CASC_LEN)_$(DYN_PT_SIZE)_$(WINDOW_VSIZE)_$(API_IO)_$(USE_WIDGETS)_$(ROUND_MODE)_$(SAT_MODE)
LOG_FILE = ./logs/log_$(UUT_FILE_SUFFIX).txt
STATUS_LOG_FILE = ./logs/status_$(UUT_FILE_SUFFIX).txt
REF_INPUT_FILE = ./data/input_ref.txt
REF_SIM_FILE = ./data/ref_output.txt
STATUS_FILE = $(STATUS_LOG_FILE)
DIFF_MODE = PERCENT
ifeq ($(AIE_VARIANT), 2)
DIFF_MODE          = ABS
endif

INPUT_WINDOW_VSIZE = $(POINT_SIZE)
REF_INPUT_FILE = ./data/input_ref.txt

ifeq ($(POINT_SIZE), 16)
	PT_SIZE_PWR       := 4
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 32)
	PT_SIZE_PWR       := 5
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 64)
	PT_SIZE_PWR       := 6
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 128)
	PT_SIZE_PWR       := 7
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 256)
	PT_SIZE_PWR       := 8
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 512)
	PT_SIZE_PWR       := 9
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 1024)
	PT_SIZE_PWR       := 10
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 2048)
	PT_SIZE_PWR       := 11
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 4096)
	PT_SIZE_PWR       := 12
	TAG_PAR_PWR       := 0
	REF_SSR           := 1
else ifeq ($(POINT_SIZE), 8192)
	PT_SIZE_PWR       := 13
	TAG_PAR_PWR       := 1
	REF_SSR           := 2
else ifeq ($(POINT_SIZE), 16384)
	PT_SIZE_PWR       := 14
	TAG_PAR_PWR       := 2
	REF_SSR           := 4
else ifeq ($(POINT_SIZE), 32768)
	PT_SIZE_PWR       := 15
	TAG_PAR_PWR       := 3
	REF_SSR           := 8
else ifeq ($(POINT_SIZE), 65536)
	PT_SIZE_PWR       := 16
	TAG_PAR_PWR       := 4
	REF_SSR           := 16
endif

ifeq ($(AIE_VARIANT), 1)
	ifeq ($(API_IO),0)
		INPUTS_PER_TILE := 1
	else
		INPUTS_PER_TILE := 2
	endif
else
	INPUTS_PER_TILE := 1
endif

DYN_PT_SIZE = 0
DYN_PT_HEADER_MODE = 0

PARAM_MAP = DATA_TYPE $(DATA_TYPE) TWIDDLE_TYPE $(TWIDDLE_TYPE) POINT_SIZE $(POINT_SIZE) FFT_NIFFT $(FFT_NIFFT) SHIFT $(SHIFT) CASC_LEN $(CASC_LEN) DYN_PT_SIZE $(DYN_PT_SIZE) API_IO $(API_IO) USE_WIDGETS $(USE_WIDGETS) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE) AIE_VARIANT $(AIE_VARIANT)

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER):  create_input sim_ref prep_x86_out
	make cleanall


create_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(FRONT_INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE) $(API_IO) 1  $(TAG_PAR_PWR)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(BACK_INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE) $(API_IO) 1  $(TAG_PAR_PWR);
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(REF_INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE) $(API_IO) 1  $(TAG_PAR_PWR);
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(FRONT_INPUT_FILE) --type $(DATA_TYPE) --ssr ${SSR} --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE};\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(BACK_INPUT_FILE) --type $(DATA_TYPE) --ssr ${SSR} --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE};
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_INPUT_FILE) --type $(DATA_TYPE) --ssr $(REF_SSR) --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE}
	echo Input ready

sim_ref:
	make UUT_KERNEL=fft_ifft_dit_1ch_ref REF_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF
	echo "done with sim_ref"

prep_x86_out:
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr $(REF_SSR) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} ;\

prep_aie_out:
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

check_op_ref: prep_x86_out prep_aie_out
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr $(TAG_PAR_PWR) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(FRONT_OUTPUT_FILE) --type $(DATA_TYPE) --ssr $(SSR) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(BACK_OUTPUT_FILE) --type $(DATA_TYPE) --ssr $(SSR) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt 4 $(CC_TOLERANCE) ABS

get_latency:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output fftMain $(NITER)

get_theoretical_min:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_fft_theoretical_min.tcl $(DATA_TYPE) $(TWIDDLE_TYPE) $(INPUT_WINDOW_VSIZE) $(POINT_SIZE) $(CASC_LEN) $(STATUS_FILE) $(UUT_KERNEL) $(TAG_PAR_PWR) $(API_IO)

get_status: check_op_ref
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
