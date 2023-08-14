

PARAM_MAP = DATA_TYPE $(DATA_TYPE) MIXER_MODE $(MIXER_MODE) P_API $(P_API) UUT_SSR $(UUT_SSR) DDS_PHASE_INC $(DDS_PHASE_INC) INITIAL_DDS_OFFSET $(INITIAL_DDS_OFFSET) INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE)
UUT_FILE_SUFFIX = $(UUT_KERNEL)_$(DATA_TYPE)_$(MIXER_MODE)_$(INPUT_WINDOW_VSIZE)
STATUS_FILE = ./logs/status_$(UUT_FILE_SUFFIX).txt

gen_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) 0 1

ssr_split:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --split --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE)

ssr_zip:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --zip --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER)


get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) $(1) $(STATUS_FILE) ./aiesimulator_output dds $(NITER)

get_theoretical_min:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_dds_theoretical_min.tcl $(DATA_TYPE) $(MIXER_MODE) $(INPUT_WINDOW_VSIZE) $(UUT_SSR) $(STATUS_FILE) $(UUT_KERNEL)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts