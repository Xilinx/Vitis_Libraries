
UPSHIFT_CT ?= 0
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) COEFF_TYPE $(COEFF_TYPE) FIR_LEN $(FIR_LEN) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE) CASC_LEN $(CASC_LEN) DUAL_IP $(DUAL_IP) USE_COEFF_RELOAD $(USE_COEFF_RELOAD) NUM_OUTPUTS $(NUM_OUTPUTS) UUT_SSR $(UUT_SSR) PORT_API $(PORT_API) INTERPOLATE_FACTOR $(INTERPOLATE_FACTOR) DECIMATE_FACTOR $(DECIMATE_FACTOR) UUT_PARA_DECI_POLY $(UUT_PARA_DECI_POLY) UUT_PARA_INTERP_POLY $(UUT_PARA_INTERP_POLY) UPSHIFT_CT $(UPSHIFT_CT)
DUAL_INPUT_SAMPLES =  $(shell echo $$(($(PORT_API) * $(DUAL_IP))))
DUAL_OUTPUT_SAMPLES = $(shell echo $$(( $(PORT_API) * ($(NUM_OUTPUTS)-1) )))
UUT_COEFF_RELOAD_HEADER_MODE = $(shell echo $$(( $(USE_COEFF_RELOAD) & 2)))
REF_COEFF_RELOAD_HEADER_MODE = $(shell echo $$(( ($(USE_COEFF_RELOAD) & 2)  * 100 + 99 )))
OUTPUT_WINDOW_VSIZE = $(shell echo $$(( $(INPUT_WINDOW_VSIZE) * ($(INTERPOLATE_FACTOR)/$(UUT_PARA_INTERP_POLY))  / ($(DECIMATE_FACTOR)/$(UUT_PARA_DECI_POLY))  )))

UUT_FILE_SUFFIX = $(UUT_KERNEL)_$(AIE_VARIANT)_$(DATA_TYPE)_$(COEFF_TYPE)_$(FIR_LEN)_$(SHIFT)_$(ROUND_MODE)_$(INTERPOLATE_FACTOR)_$(DECIMATE_FACTOR)_$(INPUT_WINDOW_VSIZE)_$(CASC_LEN)_$(DUAL_IP)_$(USE_COEFF_RELOAD)_$(NUM_OUTPUTS)_$(PORT_API)_$(UUT_SSR)_$(UUT_PARA_INTERP_POLY)_$(UUT_PARA_DECI_POLY)
LOG_FILE = ./logs/log_$(UUT_FILE_SUFFIX).txt
STATUS_LOG_FILE = ./logs/status_$(UUT_FILE_SUFFIX).txt
STATUS_FILE = $(STATUS_LOG_FILE)

ifeq ($(TAG), REF)
TAG_SSR_IN = 1
TAG_SSR_OUT = 1
TAG_DUAL_INP = 0
TAG_DUAL_OP = 0
TAG_COEFF_RELOAD_HEADER_MODE = $(REF_COEFF_RELOAD_HEADER_MODE)
else
TAG_SSR_IN = $(shell echo $$(( $(UUT_SSR) * $(UUT_PARA_DECI_POLY) )))
TAG_SSR_OUT = $(shell echo $$(( $(UUT_SSR) * $(UUT_PARA_INTERP_POLY) )))
TAG_DUAL_INP = $(DUAL_INPUT_SAMPLES)
TAG_COEFF_RELOAD_HEADER_MODE = $(UUT_COEFF_RELOAD_HEADER_MODE)
TAG_DUAL_OP = $(DUAL_OUTPUT_SAMPLES)
endif

gen_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) $(PORT_API) 1

ssr_split:
#perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(TAG_SSR) --split --dual $(TAG_DUAL_INP) -k $(TAG_COEFF_RELOAD_HEADER_MODE) -w ${INPUT_WINDOW_VSIZE} -c $(COEFF_TYPE) -fl $(FIR_LEN)
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(TAG_SSR_IN)    --split --dual $(TAG_DUAL_INP) -k 0 -w ${INPUT_WINDOW_VSIZE} -c $(COEFF_TYPE) -fl $(FIR_LEN)

ssr_zip:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(TAG_SSR_OUT) --zip --dual $(TAG_DUAL_OP) -k 0 -w $(OUTPUT_WINDOW_VSIZE)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output filter $(NITER)

get_theoretical_min:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_fir_theoretical_min.tcl $(DATA_TYPE) $(COEFF_TYPE) $(FIR_LEN) $(INPUT_WINDOW_VSIZE) $(CASC_LEN) $(INTERPOLATE_FACTOR) $(DECIMATE_FACTOR) $(SYMMETRY_FACTOR) $(UUT_SSR) $(UUT_PARA_INTERP_POLY) $(UUT_PARA_DECI_POLY) $(STATUS_FILE) $(UUT_KERNEL)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
