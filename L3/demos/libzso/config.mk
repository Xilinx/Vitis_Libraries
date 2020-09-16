C_COMPUTE_UNITS := 2
H_COMPUTE_UNITS := 2
D_COMPUTE_UNITS := 9 
PARALLEL_BLOCK  := 8
MULTIPLE_BYTES  := 8
VERBOSE := 0
XILINX_DEBUG := yes
ENABLE_SW_CHECKSUM=yes
ENABLE_XRM=no
XILINX_CODE=yes

CXXFLAGS += -DPARALLEL_BLOCK=$(PARALLEL_BLOCK) -DC_COMPUTE_UNIT=$(C_COMPUTE_UNITS) \
            -DH_COMPUTE_UNIT=$(H_COMPUTE_UNITS) \
            -DD_COMPUTE_UNIT=$(D_COMPUTE_UNITS) -DOVERLAP_HOST_DEVICE

CFLAGS += -DPARALLEL_BLOCK=$(PARALLEL_BLOCK) -DC_COMPUTE_UNIT=$(C_COMPUTE_UNITS) \
            -DH_COMPUTE_UNIT=$(H_COMPUTE_UNITS) \
            -DD_COMPUTE_UNIT=$(D_COMPUTE_UNITS) -DOVERLAP_HOST_DEVICE

SFLAGS += -DPARALLEL_BLOCK=$(PARALLEL_BLOCK) -DC_COMPUTE_UNIT=$(C_COMPUTE_UNITS) \
            -DH_COMPUTE_UNIT=$(H_COMPUTE_UNITS) \
            -DD_COMPUTE_UNIT=$(D_COMPUTE_UNITS) -DOVERLAP_HOST_DEVICE

CXXFLAGS += -DVERBOSE_LEVEL=$(VERBOSE)
CFLAGS += -DVERBOSE_LEVEL=$(VERBOSE)
SFLAGS += -DVERBOSE_LEVEL=$(VERBOSE)

# Generate ADLER32 content
ifeq ($(ENABLE_SW_CHECKSUM),yes)
CXXFLAGS += -DENABLE_SW_CHECKSUM
CFLAGS += -DENABLE_SW_CHECKSUM
SFLAGS += -DENABLE_SW_CHECKSUM
endif

# Generate Chunk Mode info
ifeq ($(CHUNK_MODE), yes)
CXXFLAGS += -DCHUNK_MODE
endif

# Enable/Disable XILINX_CODE (Default: Xilinx Code Enabled)
ifeq ($(XILINX_CODE), yes)
CXXFLAGS += -DXILINX_CODE
CFLAGS += -DXILINX_CODE
SFLAGS += -DXILINX_CODE
endif

# Generate Xilinx Debug options
ifeq ($(XILINX_DEBUG), yes)
CXXFLAGS += -DXILINX_DEBUG
endif

# Generate XRM code
ifeq ($(ENABLE_XRM), yes)
CXXFLAGS += -DENABLE_XRM -I$(XILINX_XRM)/include
CFLAGS += -DENABLE_XRM -I$(XILINX_XRM)/include
SFLAGS += -DENABLE_XRM -I$(XILINX_XRM)/include
LDXRM += -L$(XILINX_XRM)/lib/ -lxrm
endif

# Generate ZLIB SO with thirdparty code + Xilinx
# host source code.
lib: $(LIBZLIB_NAME)
	cp $(ZLIBDIR)/ . -rf
	cp $(XFLIB_DIR)/L3/demos/libzso/config.mk $(PWD)/$(ZLIBVER)/
	cp $(XFLIB_DIR)/L3/src/zlib.cpp $(PWD)/$(ZLIBVER)/
	cp $(XFLIB_DIR)/L3/src/xlibz.cpp $(PWD)/$(ZLIBVER)/
	cp $(XFLIB_DIR)/L3/include/zlib.hpp $(PWD)/$(ZLIBVER)/
	cp $(XFLIB_DIR)/L3/include/xlibz.hpp $(PWD)/$(ZLIBVER)/
	cp $(XFLIB_DIR)/common/libs/cmdparser/* $(PWD)/$(ZLIBVER)/ 
	cp $(XFLIB_DIR)/common/libs/logger/* $(PWD)/$(ZLIBVER)/ 
	cp $(XFLIB_DIR)/common/libs/xcl2/* $(PWD)/$(ZLIBVER)/ 
	cd $(ZLIBVER) && make clean && make shared
	rm -rf libz*
	cp $(PWD)/$(ZLIBVER)/libz* . 

# -------------------------------------------------------------------------------
#                 package
PKGDIR := PKG_Release
RM := rm -rf
XCLBIN_NAME := u50_gen3x16_xdma_201920_3
LIB_VERSION := 1.2.7
PROD := 1

DEB:$(PKGDIR)
	cd $(PKGDIR); \
	cmake -DCMAKE_BUILD_TYPE=Release -DPROD=${PROD} -DLIB_VERSION=${LIB_VERSION} -DCMAKE_INSTALL_PREFIX=/opt/xilinx -DXCLBIN_NAME:STRING=${XCLBIN_NAME} -DCPACK_GENERATOR=DEB ..; \
	cmake --build .; \
	cpack --verbose -G DEB ; \
	mv xzlib*.deb .. ; \
	cd ..; \
	rm PKG_Release -r; \
	rm libzxilinx.so.${LIB_VERSION};



RPM:$(PKGDIR)
	cd $(PKGDIR); \
	cmake -DCMAKE_BUILD_TYPE=Release -DPROD=${PROD} -DLIB_VERSION=${LIB_VERSION} -DCMAKE_INSTALL_PREFIX=/opt/xilinx -DXCLBIN_NAME:STRING=${XCLBIN_NAME} -DCPACK_GENERATOR=RPM ..;\
	cmake --build .;\
	cpack --verbose -G RPM;\
	mv xzlib*.rpm ..; \
	cd ..; \
	rm PKG_Release -r; \
	rm libzxilinx.so.${LIB_VERSION};



$(PKGDIR):
	mkdir $(PKGDIR); \
	cp ./pre-compiled/libz.so.${LIB_VERSION} libzxilinx.so.${LIB_VERSION};


clean_package:
	$(RM) $(PKGDIR)
