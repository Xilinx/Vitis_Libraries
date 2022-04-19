PYONY: all lib

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
DEST_DIR ?= $(CUR_DIR)

all: lib

${DEST_DIR}/swig:
	git clone https://github.com/swig/swig.git ${DEST_DIR}/swig
	cd ${DEST_DIR}/swig && git checkout v4.0.2

${DEST_DIR}/swig/install/bin/swig: ${DEST_DIR}/swig
	cd ${DEST_DIR}/swig && sh ./autogen.sh
	cd ${DEST_DIR}/swig && ./configure --prefix=$(DEST_DIR)/swig/install
	cd ${DEST_DIR}/swig && make
	cd ${DEST_DIR}/swig && make install TARGET=swig

lib: ${DEST_DIR}/swig/install/bin/swig

clean:
	rm -fr $(DEST_DIR)/swig

