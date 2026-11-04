#
# Copyright (C) 2026, Advanced Micro Devices, Inc.
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
# Makefile helper for eigenvalue compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR  ?= .
HELPER_ROOT_DIR ?= ./../../../../
COMMON_SCRIPTS_DIR = $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

HELPER := $(HELPER_CUR_DIR)/.HELPER
UUT_SSR := 1
API_IO  := 0

# Number of input samples  = DIM_SIZE * DIM_SIZE (square matrix A)
IN_WINDOW_VSIZE  := $(shell echo $$(( $(DIM_SIZE) * $(DIM_SIZE) )))
# Number of output samples = DIM_SIZE (eigenvalues of A)
OUT_WINDOW_VSIZE := $(DIM_SIZE)

PARAM_MAP   = DATA_TYPE $(DATA_TYPE) DIM_SIZE $(DIM_SIZE) \
              PASSES $(PASSES) CASC_LEN $(CASC_LEN) AIE_VARIANT $(AIE_VARIANT)
EXTRA_PARAM_MAP = NITER $(NITER) DIFF_TOLERANCE $(DIFF_TOLERANCE)

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
STATUS_JSON = ./logs/status_$(UUT_KERNEL)_$(PARAMS).json

# Path to the input generator (relative to HELPER_ROOT_DIR so it is found
# regardless of the current build directory).
GEN_INPUT_SCRIPT := $(HELPER_ROOT_DIR)/L2/tests/aie/eigenvalue/gen_eigenvalue_input.py

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
	@echo "Starting create_input..."
	@echo "============================================================"
	mkdir -p $(HELPER_CUR_DIR)/data
	$(VITIS_PYTHON3) $(GEN_INPUT_SCRIPT) \
	    --output    $(LOC_INPUT_FILE) \
	    --dim_size  $(DIM_SIZE) \
	    --niter     $(NITER) \
	    --seed      $(DATA_SEED) \
	    --data_type $(DATA_TYPE)
	@echo "Input ready"

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
	# Stitch the per-port uut_output_0_0.txt (...through N once this core
	# gains more than one output kernel) into the plain uut_output.txt
	# consumed by the diff/functional check, mirroring cholesky/qrd.
	cat $(HELPER_CUR_DIR)/data/uut_output_*.txt > $(HELPER_CUR_DIR)/data/uut_output.txt

prep_aie_out:
	@echo "============================================================"
	@echo "Starting prep_aie_out: collecting UUT aiesim output..."
	@echo "============================================================"
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done
	# Stitch the per-port uut_output_0_0.txt (...through N once this core
	# gains more than one output kernel) into the plain uut_output.txt
	# consumed by the diff/functional check, mirroring cholesky/qrd.
	cat $(HELPER_CUR_DIR)/data/uut_output_*.txt > $(HELPER_CUR_DIR)/data/uut_output.txt

# -----------------------------------------------------------------------
# diff: compare UUT vs REF (informational -- the UUT's shifted/deflated
# Hessenberg-QR and the REF's plain unshifted QR are mathematically
# equivalent but numerically different algorithms, and eigenvalues have no
# guaranteed output order, so this raw numerical diff is not the definitive
# pass/fail determinant), then verify the algorithm-independent eigenvalue
# conditions via test_eigenvalue.py -- run first against the REF output (a
# cross-check on the test harness itself: the REF is an independently-coded,
# differently-shaped algorithm, so if IT ever failed these conditions that
# would point to a bug in the input generator/tolerance/harness rather than
# the UUT -- mirroring the Ref-then-UUT pattern in qrd's helper.mk), then
# against the UUT output, which owns the definitive OVERALL verdict written
# to logs/diff.txt (test_eigenvalue.py overwrites logs/diff.txt on each
# call, so the UUT call -- run last -- is what diff_exit.tcl reads).
# -----------------------------------------------------------------------
diff:
	@echo "============================================================"
	@echo "Starting diff: UUT vs REF numerical comparison (informational)..."
	@echo "============================================================"
	-tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl \
	    $(UUT_SIM_FILE) $(REF_SIM_FILE) ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)
	@echo "============================================================"
	@echo "Starting eigenvalue condition checks (REF, harness cross-check)..."
	@echo "============================================================"
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_eigenvalue.py \
	    --A           $(LOC_INPUT_FILE) \
	    --E           $(REF_SIM_FILE) \
	    --dim_size    $(DIM_SIZE) \
	    --data_type   $(DATA_TYPE) \
	    --niter       $(NITER) \
	    --model_ut    "Ref" \
	    --tolerance   $(DIFF_TOLERANCE)
	@echo "============================================================"
	@echo "Starting eigenvalue condition checks (UUT, authoritative)..."
	@echo "============================================================"
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_eigenvalue.py \
	    --A           $(LOC_INPUT_FILE) \
	    --E           $(UUT_SIM_FILE) \
	    --dim_size    $(DIM_SIZE) \
	    --data_type   $(DATA_TYPE) \
	    --niter       $(NITER) \
	    --model_ut    "UUT" \
	    --tolerance   $(DIFF_TOLERANCE)
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py functional_check $(STATUS_JSON) ./logs/diff.txt

# Backward-compat alias used by the Makefile run target.
get_diff: diff

get_status:
	@echo "============================================================"
	@echo "Starting get_status..."
	@echo "============================================================"
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl \
	    $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) $(EXTRA_PARAM_MAP)
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py init $(STATUS_JSON) UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) $(EXTRA_PARAM_MAP)

get_qor:
	@echo "============================================================"
	@echo "Starting get_qor..."
	@echo "============================================================"
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py \
	    -t_in_file T_input.txt \
	    -t_out_file uut_output_0_0.txt \
	    -status_file_dir $(STATUS_FILE) \
	    -num_of_samples $(IN_WINDOW_VSIZE) \
	    -niter $(NITER) \
	    -use_outputs_if_no_inputs False \
	    -casc_len $(CASC_LEN) \
	    -aiesim_out_dir ./aiesimulator_output \
	    -ip hessenberg_main
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py qor $(STATUS_JSON) Work $(NITER) 1 T_input uut_output_ 64 $(DATA_TYPE) --verbose

get_memory:
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py harvest $(STATUS_JSON) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts Work

# -----------------------------------------------------------------------
# get_stats: per-kernel cycle-count/throughput stats from the aiesim
# instruction profile, mirroring cholesky/qrd's helper.mk. Filtered on the
# "eig_qr_main" kernel (the shifted/deflated QR-iteration stage, bounded by
# TP_PASSES) since it is the compute-bound stage of the two-kernel
# hessenberg+eig_qr pipeline and therefore the more QoR-relevant of the two.
# get_stats.tcl writes the same cycleCountAvg/throughputCycleCount fields as
# get_qor.py above and runs after it (see description.json post_launch), so
# this eig_qr_main figure is the one that ends up authoritative in the
# status file -- get_qor's -ip above is deliberately set to the *other*
# (hessenberg) kernel so both cycle-count sources resolve to a real,
# non-"-1" function rather than one masking the other's failure.
#
# 2026-07-27: repointed from the internal helper names (hessenberg_reduction/
# qr_algorithm) to the REGISTER_FUNCTION'd top-level entry points
# (hessenberg_main/eig_qr_main, see TODO.md item 11) -- these are guaranteed
# NOINLINE_DECL, so they stay real, stable, distinctly-named symbols in
# profile_funct_*.xml even as internal helpers get INLINE_DECL'd (see item 3's
# stage-2a, which inlined hessenberg_reduction/qr_algorithm themselves and
# would otherwise have silently broken this filter).
# -----------------------------------------------------------------------
get_stats:
	@echo "============================================================"
	@echo "Starting get_stats..."
	@echo "============================================================"
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl \
	    $(IN_WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output eig_qr_main $(NITER)

# -----------------------------------------------------------------------
# harvest_mem: AIE tile/memory-bank resource utilization, mirroring
# cholesky/qrd's helper.mk.
# -----------------------------------------------------------------------
harvest_mem:
	@echo "============================================================"
	@echo "Starting harvest_mem..."
	@echo "============================================================"
	$(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh \
	    $(STATUS_FILE) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts
