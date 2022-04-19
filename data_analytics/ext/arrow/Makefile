PYONY: all

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
DEST_DIR ?= $(CUR_DIR)
COMPILER_PATH ?= /usr/bin

all: ${DEST_DIR}/arrow/cpp/release

${DEST_DIR}/arrow:
	git clone https://github.com/apache/arrow.git ${DEST_DIR}/arrow
	cd ${DEST_DIR}/arrow && git checkout release-5.0.0

${DEST_DIR}/arrow/cpp/release: ${DEST_DIR}/arrow
	mkdir -p ${DEST_DIR}/arrow/cpp/release
	cd ${DEST_DIR}/arrow/cpp/release && cmake .. -DCMAKE_CXX_COMPILER=${COMPILER_PATH}/g++ -DCMAKE_C_COMPILER=${COMPILER_PATH}/gcc -DCMAKE_BUILD_TYPE=release -DARROW_CSV:BOOL=ON
	cd ${DEST_DIR}/arrow/cpp/release && make -j 8

clean:
	rm -fr $(DEST_DIR)/arrow

