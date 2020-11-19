MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))

XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L1/tests/*}')
XFLIB_DIR := $(abspath $(XF_PROJ_ROOT))

.PHONY: all clean cleanall

LIB_ONIG = $(CUR_DIR)/lib/lib/libonig.so
LIB_XF_RE_C = $(CUR_DIR)/libxfcompile.so

all: $(LIB_XF_RE_C)

oniguruma:
	git clone https://github.com/kkos/oniguruma.git oniguruma

$(LIB_ONIG): | oniguruma
	cd oniguruma && sh ./autogen.sh
	cd oniguruma && ./configure --prefix=$(CUR_DIR)/lib CFLAGS="-O3 -gdwarf-2 -g3"
	cd oniguruma && make
	cd oniguruma && make install

$(LIB_XF_RE_C): $(XFLIB_DIR)/L1/src/sw/xf_re_compile.c $(XFLIB_DIR)/L1/include/sw/xf_data_analytics/text/xf_re_compile.h $(LIB_ONIG)
	gcc -shared -o $@ $(filter %.c,$^) -g -Wall -Werror -fpic -std=c99 -gdwarf-2 \
		-I$(XFLIB_DIR)/L1/include/sw -I$(CUR_DIR)/oniguruma/src -I$(CUR_DIR)/lib/include \
		-L$(CUR_DIR)/lib/lib -lonig

clean:
	rm -rf $(LIB_XF_RE_C)

cleanall:
	rm -rf lib oniguruma $(LIB_XF_RE_C)
