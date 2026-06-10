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
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

HELPER:= $(HELPER_CUR_DIR)/.HELPER
UUT_SSR:= 1
API_IO:= 0

# Verification tolerances, set by AIE_VARIANT.
# AIE-ML (2) and AIE22 (22): hardware vinvsqrt has ~14-bit precision,
# causing accumulated V orthogonality error (~1e-2) and reconstruction
# error (~0.4%) that exceed the strict AIE1 thresholds.
ifeq ($(AIE_VARIANT),2)
RECON_TOLERANCE ?= 1.0
V_ORTH_TOLERANCE ?= 0.05
else ifeq ($(AIE_VARIANT),22)
RECON_TOLERANCE ?= 1.0
V_ORTH_TOLERANCE ?= 0.05
else
RECON_TOLERANCE ?= 0.1
V_ORTH_TOLERANCE ?= 0.01
endif

IN_WINDOW_VSIZE     := $(shell echo $$(( $(DIM_ROWS) * $(DIM_COLS) )))
NO_PAD_WINDOW_VSIZE := $(shell echo $$(( $(DIM_ROWS) * $(DIM_COLS) )))

PARAM_MAP = DATA_TYPE $(DATA_TYPE) DIM_ROWS $(DIM_ROWS) DIM_COLS $(DIM_COLS) PASSES $(PASSES) CASC_LEN $(CASC_LEN) AIE_VARIANT $(AIE_VARIANT)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3

$(HELPER):create_input prep_x86_out
# 	make cleanall

create_config:
	echo validating configuration;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

create_input:
	@echo starting create_input
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE) $(NO_PAD_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) 0 0 $(DATA_TYPE) $(API_IO) 1 0 0 0 0 0 64
	perl $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(LOC_INPUT_FILE) --rows $(DIM_ROWS) --cols $(DIM_COLS) --ssrSplit $(CASC_LEN) --casc 1 --split -t $(DATA_TYPE) --colMajor 1 --plioWidth 64
	echo Input ready

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
	@echo starting get_diff and concatenating output files
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/svd/stitch_outputs.py --rows $(DIM_ROWS) --cols $(DIM_COLS) \
                          --casc_len $(CASC_LEN) --niter $(NITER) \
                          --data_dir ./data --data_type $(DATA_TYPE) --aie_variant $(AIE_VARIANT)
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/svd/svd_verify_all.py . --niter $(NITER) --recon_tolerance $(RECON_TOLERANCE) --v_orth_tolerance $(V_ORTH_TOLERANCE)
# 	cat ./data/uut_output_U.txt ./data/uut_output_S.txt ./data/uut_output_V.txt > ./data/uut_output.txt
# 	cat ./data/numpy_output_U.txt ./data/numpy_output_S.txt ./data/numpy_output_V.txt > ./data/numpy_output.txt
# 	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/numpy_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)
# 	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_svd.py --A ./data/input.txt --U ./data/uut_output_U.txt --S ./data/uut_output_S.txt --V ./data/uut_output_V.txt --dim_rows $(DIM_ROWS) --dim_cols $(DIM_COLS) --data_type $(DATA_TYPE) --niter $(NITER)

get_status:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)  SINGLE_BUF $(SINGLE_BUF)

get_qor:
# 	sh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)

	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py -t_in_file T_input_0_0.txt -t_out_file uut_output_V_0_0.txt -status_file_dir $(STATUS_FILE) -num_of_samples $(NO_PAD_WINDOW_VSIZE) -niter $(NITER) -use_outputs_if_no_inputs False -casc_len $(CASC_LEN) -aiesim_out_dir ./aiesimulator_output -ip svdMain