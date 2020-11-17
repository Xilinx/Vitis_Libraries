DATA_DIR= ./$(BUILD_DIR)/data
GEN_BIN_EXE = ${DATA_DIR}/gen_bin.exe
APP_BIN      = ${DATA_DIR}/app.bin
APP_TXT      = ${DATA_DIR}/app.txt
APP_GOLD_BIN = ${DATA_DIR}/app_gold.bin
APP_GOLD_TXT = ${DATA_DIR}/app_gold.txt

GEN_BIN_PROGRAM=transp 512 512 512 512 rm cm T0 T1 transp 1024 1024 1024 1024 rm cm T2 T3

BLAS_ddrWidth=16 
BLAS_XddrWidth=16 
BLAS_argInstrWidth=1 
BLAS_numKernels=1 

BLAS_transpBlocks=1
BLAS_gemvmGroups=1

BLAS_dataType=float 

BLAS_argPipeline        = 2
BLAS_instructionSizeBytes = 64
BLAS_numKernels         = 1

BLAS_dataEqIntType = float
BLAS_argInstrWidth =   1
BLAS_numInstr      =  64
TEST_MEMCPY        = 0

MACROS += -D TEST_MEMCPY=$(TEST_MEMCPY) \
          -D BLAS_instructionSizeBytes=$(BLAS_instructionSizeBytes) \
          -D BLAS_dataType=$(BLAS_dataType) \
          -D BLAS_dataEqIntType=$(BLAS_dataEqIntType) \
          -D BLAS_ddrWidth=$(BLAS_ddrWidth) \
          -D BLAS_argInstrWidth=$(BLAS_argInstrWidth) \
          -D BLAS_numInstr=$(BLAS_numInstr) \
          -D BLAS_argPipeline=$(BLAS_argPipeline) \
          -D BLAS_transpBlocks=$(BLAS_transpBlocks) \
          -D BLAS_gemvmGroups=$(BLAS_gemvmGroups) \
          -D BLAS_runTransp=1 \
          -D BLAS_runGemv=0 \
          -D BLAS_runGemm=0 \
          -D BLAS_runFcn=0 \
          -D BLAS_numKernels=${BLAS_numKernels}
          
CXXFLAGS += ${MACROS}
VPP_FLAGS += ${MACROS}

CONFIG_INFO = $(shell echo ${MACROS} | sed 's/.*TEST_SDX=1//; s/-D //g; s/ -Wno.*//')

${GEN_BIN_EXE} :$(XFLIB_DIR)/L2/src/sw/gen_bin.cpp
	mkdir -p ${DATA_DIR} 
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

data_gen: ${GEN_BIN_EXE}
	${GEN_BIN_EXE} -write ${APP_BIN} ${GEN_BIN_PROGRAM}
	${GEN_BIN_EXE} -read ${APP_BIN} > ${APP_TXT}
	${GEN_BIN_EXE} -read ${APP_GOLD_BIN} > ${APP_GOLD_TXT}

check: dump_config
	${GEN_BIN_EXE} -read ${DATA_DIR}/app_out0.bin  > ${DATA_DIR}/app_out0.txt
	cmp -i 8192 ${APP_GOLD_BIN} ${DATA_DIR}/app_out0.bin || ${GEN_BIN_EXE} -compare 1e-3 3e-6 ${APP_GOLD_BIN} ${DATA_DIR}/app_out0.bin
	
dump_config: 
	@echo ${CONFIG_INFO}  | tr " " "\n" > ${BUILD_DIR}/config_info.dat