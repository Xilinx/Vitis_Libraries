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

## Running the Benchmark

Usage can be queried with `make help`. Basic use is:

```
make run TARGET=hw_emu PLATFORM=/path/to/xpfm
```

`hw_emu` is used to to run RTL simulation, change `hw` to `hw_emu` to run board test.


## Dataset

We used the TPC-H dataset generated with ssb-dbgen tool.
The host binary will download the tool and create data files in the same folder as itself.

