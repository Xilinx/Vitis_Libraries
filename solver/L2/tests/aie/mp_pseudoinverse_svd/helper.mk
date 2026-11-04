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
# Makefile helper for mp_pseudoinverse_svd compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR  ?= .
HELPER_ROOT_DIR ?= ./../../../../
COMMON_SCRIPTS_DIR = $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

HELPER := $(HELPER_CUR_DIR)/.HELPER
UUT_SSR := 1
API_IO  := 0

# Number of input samples  = DIM_ROWS * DIM_COLS (column-major matrix A)
IN_WINDOW_VSIZE  := $(shell echo $$(( $(DIM_ROWS) * $(DIM_COLS) )))
# Number of output samples = DIM_COLS * DIM_ROWS (pseudoinverse A^+, column-major)
OUT_WINDOW_VSIZE := $(shell echo $$(( $(DIM_COLS) * $(DIM_ROWS) )))

PARAM_MAP   = DATA_TYPE $(DATA_TYPE) DIM_ROWS $(DIM_ROWS) DIM_COLS $(DIM_COLS) \
              PASSES $(PASSES) PASSES_SSR $(PASSES_SSR) CASC_LEN $(CASC_LEN) AIE_VARIANT $(AIE_VARIANT)
EXTRA_PARAM_MAP = NITER $(NITER) DIFF_TOLERANCE $(DIFF_TOLERANCE)

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
STATUS_JSON = ./logs/status_$(UUT_KERNEL)_$(PARAMS).json

# Depadded UUT output: strips the kStoreCols alignment-padding rows so that
# both UUT and REF have exactly DIM_COLS x DIM_ROWS samples per iteration.
UUT_DEPADDED_FILE ?= ./data/uut_output_depadded.txt

# depad_output.py lives in the source test directory (HELPER_ROOT_DIR/L2/tests/aie/mp_pseudoinverse_svd),
# NOT in the per-test results directory (HELPER_CUR_DIR).
# Using HELPER_ROOT_DIR ensures the script is always found regardless of where
# this helper.mk is invoked from (source dir vs. per-test results dir).
DEPAD_SCRIPT := $(HELPER_ROOT_DIR)/L2/tests/aie/mp_pseudoinverse_svd/depad_output.py

# -----------------------------------------------------------------------
# Condition-number ceiling for the generated input matrices.
# The Jacobi SVD requires more passes to converge for ill-conditioned
# matrices.  Keeping MAX_COND <= 10 ensures PASSES=3 is sufficient for
# the Gauss-Jordan REF to be numerically stable at float32 precision
# (cond(A^H*A) = MAX_COND^2 = 100; ~5 significant digits remain after
# inversion, well within DIFF_TOLERANCE=0.05).
MAX_COND ?= 10.0
# Maximum singular value / overall scale of generated matrices.
SCALE    ?= 32000.0
# Stimulus type passed to gen_conditioned_input.py:
#   0 = random well-conditioned matrix (default)
#   1 = real-valued matrix wrapped as cfloat (imaginary=0; diagnostic for
#       cfloat complex-arithmetic correctness — A^+ of a real matrix is real)
STIM_TYPE ?= 0

# Path to the conditioned-input generator (relative to HELPER_ROOT_DIR
# so it is found regardless of the current build directory).
GEN_INPUT_SCRIPT := $(HELPER_ROOT_DIR)/L2/tests/aie/mp_pseudoinverse_svd/gen_conditioned_input.py

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3

# -----------------------------------------------------------------------
# Default target (.HELPER): runs during pre_build to prepare all inputs
# and the REF model output before the UUT compilation begins.
# -----------------------------------------------------------------------
$(HELPER): create_input sim_ref prep_ref_out

create_config:
	@echo "============================================================"
	@echo "Starting create_config..."
	@echo "============================================================"
	echo validating configuration;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl \
	    ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

create_input:
	@echo "============================================================"
	@echo "Starting create_input (conditioned matrices, MAX_COND=$(MAX_COND), STIM_TYPE=$(STIM_TYPE))..."
	@echo "============================================================"
	mkdir -p $(HELPER_CUR_DIR)/data
	$(VITIS_PYTHON3) $(GEN_INPUT_SCRIPT) \
	    --output    $(LOC_INPUT_FILE) \
	    --dim_rows  $(DIM_ROWS) \
	    --dim_cols  $(DIM_COLS) \
	    --niter     $(NITER) \
	    --seed      $(DATA_SEED) \
	    --data_type $(DATA_TYPE) \
	    --max_cond  $(MAX_COND) \
	    --scale     $(SCALE) \
	    --stim_type $(STIM_TYPE)
	@echo "Input ready (condition number bounded to $(MAX_COND), STIM_TYPE=$(STIM_TYPE))"

# -----------------------------------------------------------------------
# sim_ref: compile and run the reference graph under x86sim.
# -----------------------------------------------------------------------
sim_ref:
	@echo "============================================================"
	@echo "Starting Reference model simulation (x86sim, TAG=REF)..."
	@echo "============================================================"
	make run TARGET=x86sim TAG=REF
	@echo "============================================================"
	@echo "Completed Reference model simulation"
	@echo "============================================================"

# -----------------------------------------------------------------------
# prep_ref_out: strip x86sim timestamp/marker lines and copy to data/.
# -----------------------------------------------------------------------
prep_ref_out:
	@echo "============================================================"
	@echo "Starting prep_ref_out: collecting REF output..."
	@echo "============================================================"
	mkdir -p $(HELPER_CUR_DIR)/data
	rm -f $(HELPER_CUR_DIR)/data/ref_output.txt
	grep -ve '[XT]' \
	    $(HELPER_CUR_DIR)/x86simulator_output/data/ref_output.txt \
	    > $(HELPER_CUR_DIR)/data/ref_output.txt
	@echo "REF output collected: $(HELPER_CUR_DIR)/data/ref_output.txt"

# -----------------------------------------------------------------------
# prep_x86_out / prep_aie_out: collect UUT simulation output.
# -----------------------------------------------------------------------
prep_x86_out:
	@echo "============================================================"
	@echo "Starting prep_x86_out: collecting UUT x86sim output..."
	@echo "============================================================"
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@echo "============================================================"
	@echo "Starting prep_aie_out: collecting UUT aiesim output..."
	@echo "============================================================"
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

# -----------------------------------------------------------------------
# depad_uut_out: strip the kStoreCols alignment-padding rows from the UUT
# output.
#
# NOTE: depad_output.py is referenced via DEPAD_SCRIPT (= HELPER_ROOT_DIR
# based path), NOT via HELPER_CUR_DIR, because the script only exists in
# the source tree and is not copied to per-test results directories.
# -----------------------------------------------------------------------
depad_uut_out:
	@echo "============================================================"
	@echo "Starting depad_uut_out: stripping alignment padding from UUT output..."
	@echo "============================================================"
	mkdir -p $(HELPER_CUR_DIR)/data
	$(VITIS_PYTHON3) $(DEPAD_SCRIPT) \
	    --input       $(UUT_SIM_FILE) \
	    --output      $(UUT_DEPADDED_FILE) \
	    --dim_cols    $(DIM_COLS) \
	    --dim_rows    $(DIM_ROWS) \
	    --data_type   $(DATA_TYPE) \
	    --niter       $(NITER) \
	    --aie_variant $(AIE_VARIANT)
	@echo "Depadded UUT output written to: $(UUT_DEPADDED_FILE)"

# -----------------------------------------------------------------------
# diff: strip padding, compare depadded UUT vs REF, then verify the four
# Moore-Penrose conditions.
# -----------------------------------------------------------------------
diff: depad_uut_out
	@echo "============================================================"
	@echo "Starting diff: UUT vs REF numerical comparison (depadded)..."
	@echo "============================================================"
	-tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl \
	    $(UUT_DEPADDED_FILE) $(REF_SIM_FILE) ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)
	@echo "============================================================"
	@echo "Starting Moore-Penrose condition checks..."
	@echo "============================================================"
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_mp_pseudoinverse.py \
	    --A           $(LOC_INPUT_FILE) \
	    --Aplus       $(UUT_DEPADDED_FILE) \
	    --dim_rows    $(DIM_ROWS) \
	    --dim_cols    $(DIM_COLS) \
	    --data_type   $(DATA_TYPE) \
	    --niter       $(NITER) \
	    --model_ut    "UUT" \
	    --tol_product    $(DIFF_TOLERANCE) \
	    --tol_hermitian  $(DIFF_TOLERANCE)
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py functional_check $(STATUS_JSON) ./logs/diff.txt

# Backward-compat alias used by the Makefile run target.
get_diff: diff

get_status:
	@echo "============================================================"
	@echo "Starting get_status..."
	@echo "============================================================"
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py init $(STATUS_JSON) UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) NITER $(NITER)
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl \
	    $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) $(EXTRA_PARAM_MAP)

get_qor:
	@echo "============================================================"
	@echo "Starting get_qor..."
	@echo "============================================================"
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py \
	    -t_in_file T_input.txt \
	    -t_out_file uut_output.txt \
	    -status_file_dir $(STATUS_FILE) \
	    -num_of_samples $(IN_WINDOW_VSIZE) \
	    -niter $(NITER) \
	    -use_outputs_if_no_inputs False \
	    -casc_len $(CASC_LEN) \
	    -aiesim_out_dir ./aiesimulator_output \
	    -ip mp_pseudoinverse_svd
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py qor $(STATUS_JSON) Work $(NITER) 1 T_input uut_output 64 $(DATA_TYPE) --verbose

get_memory:
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py harvest $(STATUS_JSON) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts Work
