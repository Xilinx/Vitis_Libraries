PY_SCRIPT=${XFLIB_DIR}/L1/tests/sw/python/rtm2d/operation.py
RTM_dataType      = float
RTM_numFSMs       = 10
RTM_numBSMs       = 5
RTM_width  = 128
RTM_depth  = 128
RTM_maxDim  = 1280
RTM_MaxB = 40
RTM_NXB = $(RTM_MaxB)
RTM_NZB = $(RTM_MaxB)
RTM_order  = 8
RTM_shots = 1
RTM_time = 10
RTM_parEntries = 8
RTM_verify=1
RTM_nPE=2
RTM_fsx = $(shell expr ${RTM_width} / 2 ) 
RTM_sp = 10
RTM_deviceId = 0

MACROS += -D RTM_dataType=$(RTM_dataType) \
			-D RTM_numFSMs=$(RTM_numFSMs) \
			-D RTM_numBSMs=$(RTM_numBSMs) \
			-D RTM_maxDim=$(RTM_maxDim) \
			-D RTM_order=$(RTM_order) \
			-D RTM_MaxB=$(RTM_MaxB) \
			-D RTM_nPE=$(RTM_nPE) \
			-D RTM_NXB=$(RTM_NXB) \
			-D RTM_NZB=$(RTM_NZB) \
			-D RTM_parEntries=$(RTM_parEntries)
			

ifeq ($(TARGET),$(filter $(TARGET),hw_emu))
	CXXFLAGS += -lxrt_hwemu
else ifeq ($(TARGET),$(filter $(TARGET),sw_emu))
	CXXFLAGS += -lxrt_swemu
else
	CXXFLAGS += -lxrt_core
endif

CXXFLAGS += ${MACROS}
VPP_FLAGS += ${MACROS}

DATA_DIR= ./$(BUILD_DIR)/dataset_h${RTM_depth}_w${RTM_width}_t${RTM_time}_s${RTM_shots}/
HOST_ARGS = $(BINARY_CONTAINERS) $(RTM_depth) $(RTM_width) $(RTM_time) ${RTM_shots} ${RTM_fsx} ${RTM_sp} ${DATA_DIR} ${RTM_verify} ${RTM_deviceId}

data_gen:
	mkdir -p ${DATA_DIR} 
	python3 ${PY_SCRIPT} --func testRTM --path ${DATA_DIR} --depth ${RTM_depth} --width ${RTM_width} --time ${RTM_time} --nxb ${RTM_NXB} --nzb ${RTM_NZB} --order ${RTM_order} --verify ${RTM_verify} --shot ${RTM_shots} --type ${RTM_dataType} --fsx ${RTM_fsx} --sp ${RTM_sp}
