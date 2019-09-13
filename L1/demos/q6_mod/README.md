# Vitis Project for TPC-H Q6 Modified

## Overview

This project demostrate FPGA acceleration of dynamic-prgrammable filter with a query modified from TPC-H Q6 using scale factor 1 data.

```
select
	sum(l_extendedprice * l_discount) as revenue
from
	lineitem
where
	l_shipdate >= date '1994-01-01' and l_shipdate < date '1995-01-01'
	and l_discount between 0.06 - 0.01 and 0.06 + 0.01
	and l_quantity < 24
	and l_shipdate > l_commitdate;
;
```

## Makefile Targets

  * data: prepare the test data.

  * run\_sw\_emu: software emulation.

  * run\_hw\_emu: hardware emulation.

  * run\_hw: execute on board.

## Dataset

We used the TPC-H dataset generated with ssb-dbgen tool.
The makefile will download the tool and create data files when processing the `data` target.

## Building Notes

Some environment variables have to be set before building the project:

```
bash $ source env.sh
```
