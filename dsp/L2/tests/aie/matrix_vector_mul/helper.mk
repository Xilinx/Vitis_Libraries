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
# Makefile helper used for MM compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

HELPER:= $(HELPER_CUR_DIR)/.helper

WINDOW_VSIZE_A  	= $(shell echo $$(( $(DIM_A) * $(DIM_B) * $(NUM_FRAMES))))
WINDOW_VSIZE_B  	= $(shell echo $$(( $(DIM_B) * $(NUM_FRAMES))))
WINDOW_VSIZE_OUT    = $(shell echo $$(( $(DIM_A) * $(NUM_FRAMES))))
PARAM_MAP = DATA_A $(DATA_A) DATA_B $(DATA_B) DIM_A $(DIM_A) DIM_B $(DIM_B) DIM_A_LEADING $(DIM_A_LEADING) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE) NUM_FRAMES $(NUM_FRAMES) CASC_LEN $(CASC_LEN) UUT_SSR $(UUT_SSR) USE_MATRIX_RELOAD $(USE_MATRIX_RELOAD) API_IO $(API_IO) DUAL_IP $(DUAL_IP) NUM_OUTPUTS $(NUM_OUTPUTS) AIE_VARIANT $(AIE_VARIANT)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

NITER_A_DATA = $(NITER)
ifeq ($(USE_MATRIX_RELOAD), 1)
NITER_A_DATA          = 2
endif
TAG_DUAL_IP = $(shell echo $$(($(API_IO) * $(DUAL_IP))))
TAG_DUAL_OP = $(shell echo $$(( $(API_IO) * ($(NUM_OUTPUTS)-1) )))

DUAL_INPUT_SAMPLES =  $(shell echo $$(($(API_IO) * $(DUAL_IP))))


TS_INPUT = T_inputB_0_0_0.txt
TS_OUTPUT = ./data/uut_output_0_0.txt


$(HELPER):
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(WINDOW_VSIZE_A) $(NITER_A_DATA) 0 $(STIM_TYPE_A) 0 0 $(DATA_A) 0 1 0 0 0 0 0 64;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/matrix_vector_mul/ref_matrix_reload.pl $(USE_MATRIX_RELOAD) $(LOC_INPUT_FILE_A) $(NITER) ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(WINDOW_VSIZE_B) $(NITER) 0 $(STIM_TYPE_B) 0 0 $(DATA_B) 0 1 0 0 0 0 0 64;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(LOC_INPUT_FILE_A) --rows $(DIM_A) --cols $(DIM_B) --ssrSplit $(UUT_SSR) --casc $(CASC_LEN) --split -t $(DATA_A) --colMajor $(DIM_A_LEADING) --plioWidth 64;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/matrix_vector_mul/dual_stream.pl --dual $(TAG_DUAL_IP) -f $(LOC_INPUT_FILE_A) --x_pos $(UUT_SSR) --y_pos $(CASC_LEN) --split -t $(DATA_A) --plioWidth 64;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(LOC_INPUT_FILE_B) --rows 1 --cols $(DIM_B) --ssrClone $(UUT_SSR) --casc $(CASC_LEN) --split -t $(DATA_B) --plioWidth 64 ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/matrix_vector_mul/dual_stream.pl --dual $(TAG_DUAL_IP) -f $(LOC_INPUT_FILE_B) --x_pos $(UUT_SSR) --y_pos $(CASC_LEN) --split -t $(DATA_A) --plioWidth 64;\
	UUT_KERNEL=matrix_vector_mul_ref UUT_SIM_FILE=./data/ref_output.txt make run TARGET=x86sim TAG=REF ;\
	make cleanall

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(TS_INPUT) $(TS_OUTPUT) $(STATUS_FILE) $(WINDOW_VSIZE_A) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE_A) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output matVecMul $(NITER)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

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
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/matrix_vector_mul/dual_stream.pl --dual $(TAG_DUAL_OP) -f ./data/uut_output.txt --x_pos $(UUT_SSR) --zip -t $(DATA_A) -findOutType $(DATA_B) --plioWidth 64;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f ./data/uut_output.txt --rows $(DIM_A) --cols 1 --ssrSplit $(UUT_SSR) --casc 1 --zip -t $(DATA_A) -findOutType $(DATA_B) -n $(NITER) --plioWidth 64
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)

create_config:
	echo $(STATUS_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts