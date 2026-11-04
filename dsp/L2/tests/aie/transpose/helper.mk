#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#################################################################################
# Makefile helper used for transpose compilation, simulation and QoR harvest.
#################################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

HELPER:= $(HELPER_CUR_DIR)/.HELPER
SEED ?= 1

NUM_INPUT_SAMPLES = $(shell echo $$(( $(DIM_ROWS) * $(DIM_COLS) )))

# Memory access width, in samples, for T_DATA on this AIE_VARIANT - mirrors
# fnVecSampleNumMax()/fnVecSampleNumMax() in transpose_utils.hpp / aie_common.py.
# k_max_read_write_bytes: AIE(1)=32, AIE_ML(2)=32, AIE_MLv2(22)=64
#
# NOTE: a shell `case` statement must not be used inside $(shell ...) here - GNU Make matches
# the parentheses of its own $(...) call syntax by naive counting, and a case statement's bare
# `word)` pattern-closing parens (with no preceding `(`) are indistinguishable to that counter
# from the call's own closing paren. That silently truncates the $(shell ...) call at the first
# `)` in the case statement and leaves the rest of the case statement as literal trailing
# Makefile text appended to the variable's value - not a shell syntax error make itself catches.
# `if`/`elif`/`fi` contains no bare `)` and is safe.
MEM_WIDTH_BYTES = $(shell if [ $(AIE_VARIANT) -eq 22 ]; then echo 64; else echo 32; fi)
TYPE_SIZE_BYTES = $(shell if [ "$(T_DATA)" = "int16" ]; then echo 2; \
	elif [ "$(T_DATA)" = "cint16" ]; then echo 4; \
	elif [ "$(T_DATA)" = "int32" ]; then echo 4; \
	elif [ "$(T_DATA)" = "float" ]; then echo 4; \
	elif [ "$(T_DATA)" = "cint32" ]; then echo 8; \
	elif [ "$(T_DATA)" = "cfloat" ]; then echo 8; \
	fi)
MEM_WIDTH = $(shell echo $$(( $(MEM_WIDTH_BYTES) / $(TYPE_SIZE_BYTES) )))

# gen_input.tcl (shared across every IP) computes samplesLines=floor(samplesPerFrame/
# samplesPerLine) and silently DISCARDS the remainder if the requested window_vsize isn't
# already a multiple of its own PLIO line-packing granularity - e.g. for int16 at a 64-bit PLIO
# width that granularity is 4 samples/line, and DIM_ROWS*DIM_COLS=17*23=391 is not a multiple of
# 4, so asking for window_vsize=391 silently yields only 388 real samples. Since transpose
# deliberately allows arbitrary DIM_ROWS/DIM_COLS, request the already-padded (and therefore
# always aligned) size instead, and tell transpose_partition_pad.pl the resulting true
# per-iteration stride via --rawFrameSize (see its usage text for the full explanation).
PAD_ROWS = $(shell echo $$(( ($(DIM_ROWS) + $(MEM_WIDTH) - 1) / $(MEM_WIDTH) * $(MEM_WIDTH) )))
PAD_COLS = $(shell echo $$(( ($(DIM_COLS) + $(MEM_WIDTH) - 1) / $(MEM_WIDTH) * $(MEM_WIDTH) )))
RAW_FRAME_SIZE = $(shell echo $$(( $(PAD_ROWS) * $(PAD_COLS) )))
WINDOW_VSIZE = $(RAW_FRAME_SIZE)

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3
PLIO_WIDTH = 64

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
STATUS_JSON = ./logs/status_$(UUT_KERNEL)_$(PARAMS).json
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) T_DATA $(T_DATA) DIM_ROWS $(DIM_ROWS) DIM_COLS $(DIM_COLS) UUT_SSR $(UUT_SSR) CASC_LEN $(CASC_LEN) SPLIT_MODE $(SPLIT_MODE) CONJUGATE $(CONJUGATE)

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_input:
	@echo helper.mk stage: create_input
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(T_DATA) 0 1 0 0 0 0 0 $(PLIO_WIDTH)
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/transpose_partition_pad.pl --file $(LOC_INPUT_FILE) --rows $(DIM_ROWS) --cols $(DIM_COLS) --ssr $(UUT_SSR) --cascLen $(CASC_LEN) --splitMode $(SPLIT_MODE) --type $(T_DATA) --memWidth $(MEM_WIDTH) --rawFrameSize $(RAW_FRAME_SIZE) --niter $(NITER) --plioWidth $(PLIO_WIDTH) --split
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/transpose_partition_pad.pl --file $(LOC_INPUT_FILE) --outFile ./data/input_ref.txt --rows $(DIM_ROWS) --cols $(DIM_COLS) --type $(T_DATA) --memWidth $(MEM_WIDTH) --rawFrameSize $(RAW_FRAME_SIZE) --niter $(NITER) --plioWidth $(PLIO_WIDTH) --pad-single
	echo Input ready

sim_ref:
	@echo helper.mk stage: sim_ref
	make UUT_KERNEL=transpose_kernel_ref UUT_SIM_FILE=./data/ref_output.txt LOC_INPUT_FILE=./data/input_ref.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@echo helper.mk stage: prep_x86_out
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@echo helper.mk stage: prep_aie_out
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

get_diff:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/transpose_partition_pad.pl --file $(UUT_SIM_FILE) --outFile ./data/uut_output_combined.txt --rows $(DIM_ROWS) --cols $(DIM_COLS) --ssr $(UUT_SSR) --cascLen $(CASC_LEN) --splitMode $(SPLIT_MODE) --type $(T_DATA) --memWidth $(MEM_WIDTH) --niter $(NITER) --plioWidth $(PLIO_WIDTH) --strip
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/transpose_partition_pad.pl --file $(REF_SIM_FILE) --outFile ./data/ref_output_stripped.txt --rows $(DIM_COLS) --cols $(DIM_ROWS) --type $(T_DATA) --memWidth $(MEM_WIDTH) --niter $(NITER) --plioWidth $(PLIO_WIDTH) --strip-single
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output_combined.txt ./data/ref_output_stripped.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py functional_check $(STATUS_JSON) ./logs/diff.txt

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py init $(STATUS_JSON) UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF) NITER $(NITER)

get_memory:
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py harvest $(STATUS_JSON) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts Work

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(NUM_INPUT_SAMPLES) $(NITER)
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py -t_in_file T_input_0_0.txt -t_out_file uut_output_0_0.txt -status_file_dir $(STATUS_FILE) -num_of_samples $(NUM_INPUT_SAMPLES) -niter $(NITER) -casc_len $(CASC_LEN) -aiesim_out_dir ./aiesimulator_output -ip transpose_main
	# The "1" is FRAMES_PER_ITER (cf. fft_ifft_dit_1ch's WINDOW_VSIZE/POINT_SIZE, which can be >1
	# for a window packing multiple FFT frames) - transpose always processes exactly one
	# TP_DIM_ROWS x TP_DIM_COLS matrix per graph iteration, so 1 is correct here, not a stub.
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py qor $(STATUS_JSON) Work $(NITER) 1 T_input_ uut_output_ $(PLIO_WIDTH) $(T_DATA) --verbose

create_config:
	echo $(STATUS_FILE)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
