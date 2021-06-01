###############################################################################
#  Common Makefile used for AIE DSPLIB fir functions compilation, simulation and QoR harvest.
# Function-specific parameters and rules are overriden in tests/<func>/proj/Makefile
###############################################################################

#TODO: Add error handling/messages when we don't find files or environment variables aren't set.

# UUT
UUT_KERNEL         = default
UUT_GRAPH          = $(UUT_KERNEL)_graph
# Test parameters
DATA_TYPE          = cint16
TWIDDLE_TYPE       = cint16
POINT_SIZE         = 1024
FFT_NIFFT          = 1
SHIFT              = 17
DYN_PT_SIZE        = 0
CASC_LEN           = 1
WINDOW_VSIZE       = $(POINT_SIZE)
ifeq ($(DATA_TYPE), cint16)
INPUT_WINDOW_VSIZE = $(WINDOW_VSIZE)+8*$(DYN_PT_SIZE)
else
INPUT_WINDOW_VSIZE = $(WINDOW_VSIZE)+4*$(DYN_PT_SIZE)
endif
DIFF_TOLERANCE     = 0.01
INPUT_FILE         = data/input.txt

REPO_TYPE 		   = SRC
# specify where DSPLIB is coming from (output of tarball or direct source)
ifeq ($(REPO_TYPE), SRC)
	DSPLIB_REPO 	  := $(DSPLIB_ROOT)
else ifeq ($(REPO_TYPE), CUSTOMER)
	DSPLIB_REPO 	  := $(DSPLIB_ROOT)/output/customer
else ifeq ($(REPO_TYPE), INTERNAL)
	DSPLIB_REPO 	  := $(DSPLIB_ROOT)/output/internal
else ifeq ($(REPO_TYPE), PATH)
	DSPLIB_REPO 	  := $(DSPLIB_REPO_PATH)
else
	DSPLIB_REPO 	  := $(DSPLIB_ROOT)
endif

ifeq ($(POINT_SIZE), 16)
	PT_SIZE_PWR       := 4
else ifeq ($(POINT_SIZE), 32)
	PT_SIZE_PWR       := 5
else ifeq ($(POINT_SIZE), 64)
	PT_SIZE_PWR       := 6
else ifeq ($(POINT_SIZE), 128)
	PT_SIZE_PWR       := 7
else ifeq ($(POINT_SIZE), 256)
	PT_SIZE_PWR       := 8
else ifeq ($(POINT_SIZE), 512)
	PT_SIZE_PWR       := 9
else ifeq ($(POINT_SIZE), 1024)
	PT_SIZE_PWR       := 10
else ifeq ($(POINT_SIZE), 2048)
	PT_SIZE_PWR       := 11
else ifeq ($(POINT_SIZE), 4096)
	PT_SIZE_PWR       := 12
endif

# by default we won't create a new tarball and will assume output dir exists
MAKE_ARCHIVE = false
# by default we generate input stimulus. Set to false to use user specific.
GEN_INPUT_DATA = true

SEED                = 0
#  INCONES   = 5;#  ALLONES   = 4;#  IMPULSE   = 3;#  FROM_FILE = 1;  RANDOM    = 0;
STIM_TYPE           = 0

# Test config
KERNEL_HDR         := $(DSPLIB_REPO)/L1/include/hw
KERNEL_SRC         := $(DSPLIB_REPO)/L1/src/hw
TEST_HDR           := $(DSPLIB_REPO)/L1/tests/inc
TEST_SRC           := $(DSPLIB_REPO)/L1/tests/src
GRAPH_SRC          := $(DSPLIB_REPO)/L2/include/hw
GRAPH_TEST_SRC     := $(DSPLIB_REPO)/L2/tests/inc
#AIE_APIE_SRC       := $(XILINX_VITIS_AIETOOLS)/include
NUM_SEC            := 1
USING_UUT_YES      := 1

# Runs for different number of iterations for multi-kernel reload
# in order to fully exercise reloads feature
NITER 			   = 4
NITER_UUT          = $(NITER)
# previously $$(( $(NITER_UUT) / 2)) which is used for the FIR, but not required here since there is no margin to test
NITER_REF          = $(NITER)

LOCK_FILE          = .lock
# Local files, read at simulation. Copy of user passed input file or randomly generated data.
LOC_INPUT_FILE     = data/input.txt

REF_KERNEL         = $(UUT_KERNEL)_ref
REF_GRAPH          = $(REF_KERNEL)_graph
#Unlike the FIR where margin behaviour has to be tested differently in UUT and REF, the window size can be the same for an FFT.
REF_INPUT_WINDOW_VSIZE   = $$(( $(INPUT_WINDOW_VSIZE) ))

UUT_PARAMS_CONCAT   = $(DATA_TYPE)_$(TWIDDLE_TYPE)_$(POINT_SIZE)_$(FFT_NIFFT)_$(SHIFT)_$(CASC_LEN)_$(DYN_PT_SIZE)_$(WINDOW_VSIZE)
TEST_PARAMS_CONCAT  = $(REPO_TYPE)_$(UUT_TARGET)_$(STIM_TYPE)
ALL_PARAMS_CONCAT   = $(UUT_PARAMS_CONCAT)_$(TEST_PARAMS_CONCAT)
UUT_FILE_SUFFIX     = $(UUT_GRAPH)_$(ALL_PARAMS_CONCAT)
REF_FILE_SUFFIX     = $(REF_GRAPH)_$(ALL_PARAMS_CONCAT)
PREPROC_ARGS       = -DINPUT_FILE=$(LOC_INPUT_FILE) -DDATA_TYPE=$(DATA_TYPE) -DTWIDDLE_TYPE=$(TWIDDLE_TYPE) -DPOINT_SIZE=$(POINT_SIZE) -DFFT_NIFFT=$(FFT_NIFFT) -DSHIFT=$(SHIFT) -DCASC_LEN=$(CASC_LEN) -DDYN_PT_SIZE=$(DYN_PT_SIZE) -DWINDOW_VSIZE=$(WINDOW_VSIZE) -DSTIM_TYPE=$(STIM_TYPE)
REF_PREPROC_ARGS   = "-DUUT_GRAPH=$(REF_GRAPH) -DOUTPUT_FILE=$(REF_SIM_FILE) $(PREPROC_ARGS)  -DINPUT_WINDOW_VSIZE=$(REF_INPUT_WINDOW_VSIZE) -DNITER=$(NITER_REF)"
UUT_PREPROC_ARGS   = "-DUUT_GRAPH=$(UUT_GRAPH) -DOUTPUT_FILE=$(UUT_SIM_FILE) $(PREPROC_ARGS)  -DINPUT_WINDOW_VSIZE=$(INPUT_WINDOW_VSIZE) -DNITER=$(NITER_UUT) -DUSING_UUT=$(USING_UUT_YES)"

ifeq ($(TWIDDLE_TYPE), cint32)
	TWIDDLE_SIZE 	  := 8
else ifeq ($(TWIDDLE_TYPE), cfloat)
	TWIDDLE_SIZE 	  := 8
else ifeq ($(TWIDDLE_TYPE), cint16)
	TWIDDLE_SIZE 	  := 4
endif

#Heapsize is temp buffer (s) + twiddle storage, which is 1/2 + 1/4 +...
ifeq ($(DATA_TYPE), cint16)
    # 2 buffers required
#    HEAPSIZE_VAL       = $$(( 2* $(POINT_SIZE) * $(DATA_SIZE) + $(POINT_SIZE) * $(TWIDDLE_SIZE) + 256  ))
#    HEAPSIZE_VAL       = $$(( 2* $(POINT_SIZE) * $(DATA_SIZE)  + 256  ))
    HEAPSIZE_VAL       = 8000
else
    # 1 buffer required
#    HEAPSIZE_VAL       = $$(( $(POINT_SIZE) * $(DATA_SIZE) + $(POINT_SIZE) * $(TWIDDLE_SIZE) + 256  ))
    HEAPSIZE_VAL       = 8000
endif
# $$(( $(POINT_SIZE) * $(TWIDDLE_SIZE) + 1024))
STACKSIZE_VAL      = 600

# Env config
STATUS_LOG_FILE    = ./logs/status_$(UUT_FILE_SUFFIX).txt
# redundant
STATUS_FILE        = $(STATUS_LOG_FILE)
DIFF_OUT_FILE      = diff_output.txt
UUT_DATA_FILE      = ./data/uut_out_$(UUT_FILE_SUFFIX).txt
REF_DATA_FILE      = ./data/ref_out_$(REF_FILE_SUFFIX).txt
TMP_UUT_FILE       = tmp_uut.txt
TMP_REF_FILE       = tmp_ref.txt
UUT_OUT_DIR        = ./aiesimulator_output
REF_OUT_DIR        = ./aiesimulator_ref_output
REF_OUT_DIR_X86    = ./x86simulator_ref_output
UUT_SIM_FILE       = data/uut_output.txt
REF_SIM_FILE       = data/ref_output.txt
UUT_OUT_FILE       = $(UUT_OUT_DIR)/$(UUT_SIM_FILE)
REF_OUT_FILE       = $(REF_OUT_DIR_X86)/$(REF_SIM_FILE)

# TODO: Make this smarter
DEVICE_FILE        = 4x4

TEST_BENCH         = test.cpp
WORK_DIR           = Work
REF_WORK_DIR       = Work_ref
LOG_FILE           = ./logs/log_$(UUT_FILE_SUFFIX).txt
REF_LOG_FILE       = ./logs/ref_log_$(REF_FILE_SUFFIX).txt
PSLINKER_ARGS      = "-L xxx"
OTHER_OPTS         = --pl-freq=1000
SIM_OPTS           = --profile
SIM_OPTS_X86       =
PHRASE_1           = 'COMPILE.*_SUCCESS'
PHRASE_2           = 'SIM.*_SUCCESS'
PHRASE_3           = identical

GET_STATS 	:= false
# remove _FOR_RELEASE_ for the release
#ifdef _DUMMY_PREPROC_DIRECTIVE_FOR_SCRIPT_STRIPPING_DE_FOR_RELEASE_BUG_
GET_STATS 	:= true

DUMP_VCD	   = 0
ifeq ($(DUMP_VCD), 1)
	SIM_OPTS += --dump-vcd $(UUT_KERNEL)_sim
endif

# If debug_coeff_seed is passed, overwrite the default.
ifneq ($(DEBUG_COEFF_SEED),)
	PREPROC_ARGS += -DCOEFF_SEED=$(DEBUG_COEFF_SEED)
endif

# Set to 1 to enable AIE API aka High Level Intrinsics
USE_AIE_API	   = 1
# Use aie api (aka HLI - High Level Intrinsic)
ifeq ($(USE_AIE_API), 1)
	PREPROC_ARGS += -D_DSPLIB_FFT_IFFT_DIT_1CH_HPP_AIE_API_
endif

#endif //_DUMMY_PREPROC_DIRECTIVE_FOR_SCRIPT_STRIPPING_DE_FOR_RELEASE_BUG_

PRGMEM_AWK_POS    := 1
ifeq ($(CASC_LEN), 1)
	PRGMEM_AWK_POS    := 1
else
	PRGMEM_AWK_POS    := 2
endif

UUT_TARGET=hw
# Target specific compile args
ifeq ($(UUT_TARGET), hw)
    UUT_TARGET_COMPILE_ARGS:= -heapsize=$(HEAPSIZE_VAL) -stacksize=$(STACKSIZE_VAL) -Xchess=llvm.xargs="-std=c++17" -Xchess=main:backend.mist2.xargs="+NOdra" --xlopt=1 -Xchess=main:backend.mist2.pnll="off"
	SIM_EXEC=aiesimulator
else ifeq ($(UUT_TARGET), x86sim)
	UUT_TARGET_COMPILE_ARGS:=
	SIM_EXEC=x86simulator
	SIM_OPTS= -o=$(UUT_OUT_DIR)
	GET_STATS 	:= false
endif
# TODO Add options to change target for ref
REF_TARGET=x86sim


PATHTOSCRIPTS     := $(DSPLIB_ROOT)/../../test/
#test is <workspace>/CARDANO_ROOT/test. TODO: figure out where this will be for release (internal release).

#This allows a completely seperate run directory. This location can be overridden from command-line.
# /proj/ipeng_scratch/user
RESULTS_PATH = ./results
# Unique results dir for each test
RESULTS_DIR = $(RESULTS_PATH)/results_$(UUT_FILE_SUFFIX)
RESULTS_BACKUP_DIR = $(RESULTS_DIR)
# File size analysis shows that chesswork/* and sim.out are biggest contributors to results size.
# We also don't want to copy current results, for obvious reasons.
EXCLUDE_COPY = '*chesswork*' '*sim.out' './*results*' '.Xil' '*.pch'
INCLUDE_COPY =
# This gets put in-line with the find command followed by -prune -o <other args> -print
# Essentially, this is the find args to find files that we DON'T want to include in the main find.
# We use it to not include other batch test config results.
PRUNE_COPY = -ipath '*$(UUT_GRAPH)*' -not -ipath '*$(ALL_PARAMS_CONCAT)*'

# Legacy
all_c        : create compile_ref sim_ref create_input compile slp sim check_op_ref get_status_ref summary
# Ref run & UUT run
all 		 : create_r recurse

ref 		 : create_r recurse_ref

all_r        : clean create create_input compile_ref sim_ref compile slp sim check_op_ref get_status_ref clean_result summary

ref_r        : clean create create_input compile_ref sim_ref
# Deb
all_deb      : clean create create_input

# This passes current environment (including args from commands line) to sub-make
recurse :
	@echo starting make in $(RESULTS_DIR) at `date "+%s"`
	$(MAKE) -C $(RESULTS_DIR) all_r
	@echo finished make in $(RESULTS_DIR) at `date "+%s"`

recurse_ref :
	@echo starting make in $(RESULTS_DIR) at `date "+%s"`
	$(MAKE) -C $(RESULTS_DIR) ref_r
	@echo finished make in $(RESULTS_DIR) at `date "+%s"`

## cp_result is now deprecated because testcases are now ran inside results directory
#https://unix.stackexchange.com/a/311983/346866
# TODO: rsync -armR --include="*/" --include="*.csv" --exclude="*" /full/path/to/source/file(s) destination/
# TODO: tee into logs
# TODO: deal with overwrite appropriately (have a force overwrite argument)
# TODO: Add gunzip to results dir (that can be disabled with param)
# TODO: utilise clean step to remove a pre-existing RESULTS_BACKUP_DIR (possibly with force overwrite)
cp_result:
	$(info $(shell mkdir -p $(RESULTS_BACKUP_DIR)))
	$(shell find . \
	$(PRUNE_COPY) -prune -o \
	$(foreach exclusion, $(EXCLUDE_COPY), -not -ipath $(exclusion) ) \
	$(foreach inclusion, $(INCLUDE_COPY), -ipath $(inclusion) ) \
	-print \
	| xargs cp --parents -t ./$(RESULTS_BACKUP_DIR))
	@echo 'done copying'

# remove the sorts of files that we previously excluded from results copy due to disk usage bloat
# we don't need to do any fancy pruning because each testcase shouldn't have any other testcases in there.
#  the word words stuff is just to add an extra -ipath to the end so that we can use the -o notation to apply multiple filters
clean_result:
	$(info $(shell find . \
	\( $(foreach exclusion, $(EXCLUDE_COPY), -ipath $(exclusion) -o ) -ipath $(word $(words $(EXCLUDE_COPY)),$(EXCLUDE_COPY)) \) \
	-print -exec rm -rf {} + ))
	@echo 'done cleaning'

# TODO: Just foreach loop across test param list

create_r:
	@rm -rf $(RESULTS_DIR)
	@mkdir -p $(RESULTS_DIR)/logs $(RESULTS_DIR)/data $(RESULTS_DIR)/data
	@cp -f $(TEST_BENCH) $(RESULTS_DIR)
	@cp -f test.hpp $(RESULTS_DIR)
	@cp -f uut_config.h $(RESULTS_DIR)
	@cp -f Makefile $(RESULTS_DIR)
	@if [ $(GEN_INPUT_DATA) = false ]; then \
		cp -f $(INPUT_FILE) $(RESULTS_DIR)/$(LOC_INPUT_FILE) ;\
	fi

create:
	@echo Start testing|& tee $(LOG_FILE);\
	 echo `pwd`;\
	 echo diff result > $(DIFF_OUT_FILE);\
	 echo "Configuration:"                                   > $(STATUS_FILE);\
	 echo "    UUT_KERNEL:           " $(UUT_KERNEL)         >> $(STATUS_FILE);\
	 echo "    DATA_TYPE:            " $(DATA_TYPE)          >> $(STATUS_FILE);\
	 echo "    TWIDDLE_TYPE:         " $(TWIDDLE_TYPE)       >> $(STATUS_FILE);\
	 echo "    POINT_SIZE:           " $(POINT_SIZE)         >> $(STATUS_FILE);\
	 echo "    FFT_NIFFT:            " $(FFT_NIFFT)          >> $(STATUS_FILE);\
	 echo "    SHIFT:                " $(SHIFT)              >> $(STATUS_FILE);\
	 echo "    CASC_LEN:             " $(CASC_LEN)           >> $(STATUS_FILE);\
	 echo "    DYN_PT_SIZE:          " $(DYN_PT_SIZE)        >> $(STATUS_FILE);\
	 echo "    WINDOW_VSIZE:         " $(WINDOW_VSIZE)       >> $(STATUS_FILE);\
	 echo "Results:"                                         >> $(STATUS_FILE)

create_input:
	@if [ $(GEN_INPUT_DATA) = true ]; then \
		tclsh $(DSPLIB_REPO)/L2/scripts/gen_input.tcl $(LOC_INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER_UUT) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE);\
	fi
	@echo Input ready;\
	rm -f $(LOCK_FILE)
	@if [ $(MAKE_ARCHIVE) = true ]; then \
		echo Creating DSPLIB archive |& tee -a $(LOG_FILE) ;\
		$(DSPLIB_ROOT)/internal/scripts/create_dsplib_zip.pl |& tee -a $(LOG_FILE) ;\
	fi

compile_ref:
	@echo COMPILE_REF_START |& tee $(REF_LOG_FILE);\
	date +%s |& tee -a $(REF_LOG_FILE);\
	date |& tee -a $(REF_LOG_FILE)
	@set -o pipefail; \
	aiecompiler --target x86sim --dataflow -v --use-phy-shim=false -include=$(KERNEL_HDR) -include=$(KERNEL_SRC) -include=$(TEST_HDR) -include=$(TEST_SRC) -include=$(GRAPH_SRC) -include=$(GRAPH_TEST_SRC) $(TEST_BENCH) $(OTHER_OPTS) --device=$(DEVICE_FILE) --test-iterations=$(NITER_REF) -workdir=$(REF_WORK_DIR) -Xpreproc=$(REF_PREPROC_ARGS)|& tee -a $(REF_LOG_FILE);\
	echo $$? > compile_ref_exit; \
	if [ `cat compile_ref_exit` -ne 0 ]; then \
		echo COMPILE_REF_FAILED. ERR_CODE: `cat compile_ref_exit` |& tee -a $(REF_LOG_FILE); \
	else \
		echo COMPILE_REF_SUCCESS |& tee -a $(REF_LOG_FILE); \
	fi ;\
	echo COMPILE_REF_END |& tee -a $(REF_LOG_FILE);\
	date +%s |& tee -a $(REF_LOG_FILE)

sim_ref:
	@echo REF_SIM_START |& tee -a $(REF_LOG_FILE);\
	date +%s |& tee -a $(REF_LOG_FILE);\
	if [ `cat compile_ref_exit` -ne 0 ]; then \
		echo SKIPPING REF_SIMULATION DUE TO COMPILE_REF FAILURE |& tee -a $(LOG_FILE); \
	else \
		set -o pipefail; \
		x86simulator --pkg-dir=$(REF_WORK_DIR) -o=$(REF_OUT_DIR_X86) $(SIM_OPTS_X86) |& tee -a $(REF_LOG_FILE);\
		echo $$? > sim_ref_exit; \
		if [ `cat sim_ref_exit` -ne 0 ]; then \
			echo SIM_REF_FAILED. ERR_CODE: `cat sim_ref_exit` |& tee -a $(REF_LOG_FILE); \
		else \
			echo SIM_REF_SUCCESS |& tee -a $(REF_LOG_FILE); \
		fi ;\
	fi
	@echo REF_SIM_END |& tee -a $(REF_LOG_FILE);\
	date +%s |& tee -a $(REF_LOG_FILE)

#set -o pipefail allows us to catch the error signal from any piped error command
# usually you would use set +o pipefail to reverse this for subsequent commmands,
# but Makefile has every distinct command in a new shell, so it doesn't matter.
compile:
	@echo COMPILE_START |& tee $(LOG_FILE);\
	date +%s |& tee -a $(LOG_FILE);\
	date |& tee -a $(LOG_FILE);\
	set -o pipefail; \
	aiecompiler -Xchess=main:llvm.xargs=-g --target $(UUT_TARGET) $(UUT_TARGET_COMPILE_ARGS) --dataflow -v --use-phy-shim=false -include=$(KERNEL_HDR) -include=$(KERNEL_SRC) -include=$(TEST_HDR) -include=$(TEST_SRC) -include=$(GRAPH_SRC) -include=$(GRAPH_TEST_SRC) $(TEST_BENCH) $(OTHER_OPTS) --device=$(DEVICE_FILE) --test-iterations=$(NITER_UUT) -workdir=$(WORK_DIR) -Xpreproc=$(UUT_PREPROC_ARGS)|& tee -a $(LOG_FILE);\
	echo $$? > compile_exit; \
	if [ `cat compile_exit` -ne 0 ]; then \
		echo COMPILE_FAILED. ERR_CODE: `cat compile_exit`  |& tee -a $(LOG_FILE); \
	else \
		echo COMPILE_SUCCESS |& tee -a $(LOG_FILE); \
	fi
	@echo COMPILE_END |& tee -a $(LOG_FILE);\
	date +%s |& tee -a $(LOG_FILE)


sim:
	@echo SIM_START |& tee -a $(LOG_FILE);\
	date +%s |& tee -a $(LOG_FILE);\
	if [ `cat compile_exit` -ne 0 ]; then \
		echo SKIPPING SIMULATION DUE TO COMPILE FAILURE `cat compile_exit` |& tee -a $(LOG_FILE); \
	else \
		set -o pipefail; \
		$(SIM_EXEC) --pkg-dir=$(WORK_DIR) $(SIM_OPTS) |& tee -a $(LOG_FILE);\
		echo $$? > sim_exit; \
		if [ `cat sim_exit` -ne 0 ]; then \
			echo SIM_FAILED. ERR_CODE: `cat sim_exit` |& tee -a $(LOG_FILE); \
		else \
			echo SIM_SUCCESS |& tee -a $(LOG_FILE); \
		fi ;\
	fi
	@echo SIM_END |& tee -a $(LOG_FILE);\
	date +%s |& tee -a $(LOG_FILE)
slp:
	@echo sleep for $(NUM_SEC) |& tee -a $(LOG_FILE);\
	sleep $(NUM_SEC)

check_op_ref:
	@grep -ve '[XT]' $(UUT_OUT_FILE) > $(UUT_DATA_FILE);\
	grep -ve '[XT]' $(REF_OUT_FILE) > $(REF_DATA_FILE);\
	echo "DIFF_START" >> $(LOG_FILE)
	tclsh $(DSPLIB_REPO)/L2/scripts/diff.tcl $(UUT_DATA_FILE) $(REF_DATA_FILE) $(DIFF_OUT_FILE)  $(DIFF_TOLERANCE) >> $(LOG_FILE)
	echo "DIFF_END" >> $(LOG_FILE)

get_status_ref:
	@grepres1=`grep  $(PHRASE_1) -c $(LOG_FILE)`;\
	 echo "    COMPILE:" $$grepres1 >>$(STATUS_FILE)
	@grepres2=`grep  $(PHRASE_2) -c $(LOG_FILE)`;\
	 echo "    SIM:" $$grepres2 >>$(STATUS_FILE)
	@grepres3=`grep  $(PHRASE_1) -c $(REF_LOG_FILE)`;\
	 echo "    COMPILE_REF:" $$grepres3 >>$(STATUS_FILE)
	@grepres4=`grep  $(PHRASE_2) -c $(REF_LOG_FILE)`;\
	 echo "    SIM_REF:" $$grepres4 >>$(STATUS_FILE)
	@grepres5=`grep  $(PHRASE_3) -c $(DIFF_OUT_FILE)`;\
	 echo "    FUNC:" $$grepres5 >>$(STATUS_FILE);\
	nl=`wc -l '$(REF_DATA_FILE)' | grep -Eo '[0-9]+' |head -1`;\
	echo "    NUM_REF_OUTPUTS:" $$nl >>$(STATUS_FILE);\
	nl=`wc -l '$(UUT_DATA_FILE)' | grep -Eo '[0-9]+' |head -1`;\
	echo "    NUM_UUT_OUTPUTS:" $$nl >>$(STATUS_FILE);\
	t0=`grep COMPILE_REF_START -A 1 $(REF_LOG_FILE)|tail -n 1`;\
	t1=`grep COMPILE_REF_END -A 1 $(REF_LOG_FILE)|tail -n 1`;\
	td=`echo $$t1 - $$t0 |bc`;\
	echo "    COMPILE_REF_TIME:" $$td >>$(STATUS_FILE);\
	t0=`grep REF_SIM_START -A 1 $(REF_LOG_FILE)|tail -n 1`;\
	t1=`grep REF_SIM_END -A 1 $(REF_LOG_FILE)|tail -n 1`;\
	td=`echo $$t1 - $$t0 |bc`;\
	echo "    REF_SIM_TIME:" $$td >>$(STATUS_FILE);\
	t0=`grep COMPILE_START -A 1 $(LOG_FILE)|tail -n 1`;\
	t1=`grep COMPILE_END -A 1 $(LOG_FILE)|tail -n 1`;\
	td=`echo $$t1 - $$t0 |bc`;\
	echo "    COMPILE_TIME:" $$td >>$(STATUS_FILE);\
	t0=`grep SIM_START -A 1 $(LOG_FILE)|tail -n 1`;\
	t1=`grep SIM_END -A 1 $(LOG_FILE)|tail -n 1`;\
	td=`echo $$t1 - $$t0 |bc`;\
	echo "    SIM_TIME:" $$td >>$(STATUS_FILE);\
	archs=`grep ARCHS $(LOG_FILE)| tail -n 1`;\
	echo "   " $$archs >> $(STATUS_FILE);\
	if [ $(GET_STATS) = true -a `cat compile_exit` -eq 0 ]; then \
		tclsh $(DSPLIB_REPO)/L2/scripts/get_stats.tcl $(DATA_TYPE) $(TWIDDLE_TYPE) 1 $(POINT_SIZE) $(CASC_LEN) 1 1 1 0 0 1 $(STATUS_FILE) $(UUT_OUT_DIR) "fftMain" $(NITER_UUT);\
		echo -n "    NUM_BANKS: "                                       >> $(STATUS_FILE);\
		$(PATHTOSCRIPTS)/get_num_banks.sh $(WORK_DIR) dummy             >> $(STATUS_FILE) ;\
		echo -n "    NUM_ME: "                                          >> $(STATUS_FILE);\
		$(PATHTOSCRIPTS)/get_num_me.sh $(WORK_DIR) 1                    >> $(STATUS_FILE) ;\
		echo -n "    DATA_MEMORY: "                                     >> $(STATUS_FILE);\
		$(PATHTOSCRIPTS)/get_data_memory.sh $(WORK_DIR) dummy           >> $(STATUS_FILE);\
	fi
	echo -n "    PROGRAM_MEMORY: "                                  >> $(STATUS_FILE);\
	max_prgmem=`ls $(WORK_DIR)/aie/*_*/Release/*_*.map|xargs grep -A 10 "Section summary for memory 'PM':"| grep "Total" | grep -Po '\d+' `;\
	echo $$max_prgmem                                               >> $(STATUS_FILE);\
	cp $(STATUS_FILE) $(STATUS_LOG_FILE)  ;\
	echo test_complete > $(LOCK_FILE)

# max_prgmem=`ls $(WORK_DIR)/aie/*_*/Release/*_*.map|xargs grep -A 10 "Section summary for memory 'PM':"|grep "Total"| awk '{print $$($(PRGMEM_AWK_POS))}'|sort -n|tail -n 1`;\

get_status_deb:
	if [ $(GET_STATS) = true ]; then \
		echo -n "    PROGRAM_MEMORY: "                                  >> $(STATUS_FILE);\
		max_prgmem=`ls $(WORK_DIR)/aie/*_*/Release/*_*.map|xargs grep -A 10 "Section summary for memory 'PM':"|grep "Total"| awk '{print $$($(PRGMEM_AWK_POS))}'|sort -n`;\
		echo $$max_prgmem                                               ;\
	fi

summary:
	cat $(STATUS_FILE)

.PHONY: clean
clean:
	@rm -rf aiesimulator_output ;\
	rm -rf .Xil;\
	rm -rf xnwOut;\
	rm -rf $(LOG_FILE) $(REF_LOG_FILE);\
	rm -rf  $(TMP_REF_FILE) $(TMP_UUT_FILE) $(STATUS_FILE) $(DIFF_OUT_FILE) aiesimulator.log;\
	rm -f -R $(WORK_DIR) $(UUT_OUT_DIR);\
	rm -f -R $(REF_WORK_DIR) $(REF_OUT_DIR) $(REF_OUT_DIR_X86);\
	rm -f *_exit


