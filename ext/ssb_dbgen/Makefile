.PHONY: all clean

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))

SF ?= 1

all:
	flock $(CUR_DIR)/.lock make -C $(CUR_DIR) sf$(SF)/lineitem.tbl

src:
	git clone https://github.com/electrum/ssb-dbgen.git src

src/dbgen: | src
	sed -i 's/MACHINE =MAC/MACHINE =LINUX/' src/makefile
	sed -i 's/WORKLOAD =SSBM/WORKLOAD =TPCH/' src/makefile
	make -C src

sf%:
	mkdir -p sf$(*)

sf%/lineitem.tbl: | src/dbgen sf%
	(cd src && ./dbgen -s $(*) -f && mv *.tbl ../sf$(*))

clean:
	rm -rf src sf*
