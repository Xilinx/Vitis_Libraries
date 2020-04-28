# Hash-Group-Aggregate (General Version)

## Overview

This benchmark shows FPGA performance of the following query over TPC-H scale factor 1 data.

```
select
        max(l_extendedprice), min(l_extendedprice), count_non_zero(l_extendedprice) as revenue
from
        lineitem
group by
        l_orderkey
;

```

## Dataset

Due to unknown license, the source code of dataset generator is not directly included.
When the project runs, script in `db_data` folder will automatically grep the dataset generator
and compile it from source.

## Running the Benchmark

Usage can be queried with `make help`. Basic use is:

```
make run TARGET=sw_emu DEVICE=/path/to/xpfm
```

Change `sw_emu` to `hw_emu` or `hw` to run RTL simulation or board test correspondingly.
