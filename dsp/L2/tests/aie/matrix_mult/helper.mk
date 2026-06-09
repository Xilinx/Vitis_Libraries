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

###############################################################################
# Makefile helper used for MM compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../


HELPER:= $(HELPER_CUR_DIR)/.helper

NUM_INPUT_SAMPLES = $(shell echo $$(( $(P_DIM_A) * $(P_DIM_B))))
NUM_MACS_PER_KERNEL = $(shell echo $$(( ( $(P_DIM_A) * $(P_DIM_AB) * $(P_DIM_B) ) / $(P_CASC_LEN) )))
P_INPUT_WINDOW_VSIZE_A = $(shell echo $$(( $(P_DIM_A) * $(P_DIM_AB))))
P_INPUT_WINDOW_VSIZE_B = $(shell echo $$(( $(P_DIM_B) * $(P_DIM_AB))))
PARAM_MAP = T_DATA_A $(T_DATA_A) T_DATA_B $(T_DATA_B) DATA_OUT_TYPE $(DATA_OUT_TYPE) P_DIM_A $(P_DIM_A) P_DIM_AB $(P_DIM_AB) P_DIM_B $(P_DIM_B) P_SHIFT $(P_SHIFT) P_ROUND_MODE $(P_ROUND_MODE) P_SAT_MODE $(P_SAT_MODE) P_DIM_A_LEADING $(P_DIM_A_LEADING) P_DIM_B_LEADING $(P_DIM_B_LEADING) P_DIM_OUT_LEADING $(P_DIM_OUT_LEADING) P_ADD_TILING_A $(P_ADD_TILING_A) P_ADD_TILING_B $(P_ADD_TILING_B) P_ADD_DETILING_OUT $(P_ADD_DETILING_OUT) P_INPUT_WINDOW_VSIZE_A $(P_INPUT_WINDOW_VSIZE_A) P_INPUT_WINDOW_VSIZE_B $(P_INPUT_WINDOW_VSIZE_B) P_CASC_LEN $(P_CASC_LEN) UUT_SSR $(UUT_SSR) AIE_VARIANT $(AIE_VARIANT)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3
# PLIO_WIDTH  = 64
PLIO_WIDTH  = 128

$(HELPER):
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(P_INPUT_WINDOW_VSIZE_A) $(NITER) 0 $(STIM_TYPE_A) 0 0 $(T_DATA_A) 0 1 0 0 0 0 0 ${PLIO_WIDTH}
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(P_INPUT_WINDOW_VSIZE_B) $(NITER) 0 $(STIM_TYPE_B) 0 0 $(T_DATA_B) 0 1 0 0 0 0 0 ${PLIO_WIDTH}
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --aieVariant $(AIE_VARIANT) --inFile $(LOC_INPUT_FILE_A) --inRow $(P_DIM_A)  --inCol $(P_DIM_AB) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --cascLen $(P_CASC_LEN) --ssr $(UUT_SSR) --colMajor $(P_DIM_A_LEADING) --isTiled $(P_ADD_TILING_A) --tileInPlace --plioWidth ${PLIO_WIDTH}
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --aieVariant $(AIE_VARIANT) --inFile $(LOC_INPUT_FILE_B) --inRow $(P_DIM_AB) --inCol $(P_DIM_B)  --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --cascLen $(P_CASC_LEN) --ssr $(UUT_SSR) --colMajor $(P_DIM_B_LEADING) --isTiled $(P_ADD_TILING_B) --tileInPlace  --splitRows --plioWidth ${PLIO_WIDTH}
	TARGET=x86sim UUT_KERNEL=matrix_mult_ref UUT_SIM_FILE=./data/ref_output.txt make run TARGET=x86sim TAG=REF
	make cleanall

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_inputA_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(NUM_INPUT_SAMPLES) $(NITER)

	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py -t_in_file T_inputA_0_0.txt -t_out_file uut_output_0_0.txt -status_file_dir $(STATUS_FILE) -num_of_samples $(NUM_INPUT_SAMPLES) -niter $(NITER) -casc_len $(P_CASC_LEN) -aiesim_out_dir ./aiesimulator_output -ip matMult

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
	echo perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --aieVariant $(AIE_VARIANT) --untile --inFile ./data/uut_output.txt --tileInPlace --inRow $(P_DIM_A) --inCol $(P_DIM_B) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --T_DATA_OUT $(DATA_OUT_TYPE) --ssr $(UUT_SSR) --colMajor $(P_DIM_OUT_LEADING) --isTiled $(P_ADD_DETILING_OUT) --plioWidth ${PLIO_WIDTH}
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --aieVariant $(AIE_VARIANT) --untile --inFile ./data/uut_output.txt --tileInPlace --inRow $(P_DIM_A) --inCol $(P_DIM_B) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --T_DATA_OUT $(DATA_OUT_TYPE) --ssr $(UUT_SSR) --colMajor $(P_DIM_OUT_LEADING) --isTiled $(P_ADD_DETILING_OUT) --plioWidth ${PLIO_WIDTH}
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)

create_config:
	echo $(STATUS_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
