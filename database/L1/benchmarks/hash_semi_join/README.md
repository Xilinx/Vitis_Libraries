# Hash-Semi-Join

## Overview

This project shows FPGA performance of the following query over TPC-H scale factor 1 data.
Primitive in use is `hashSemiJoin` from `hash_semi_join.hpp`.

```

select
      sum(l_extendedprice * (1 - l_discount)) as revenue
from
      lineitem
where
      l_orderkey
 in
     (
       select
                 o_orderkey
       from
                 orders
       where     o_orderdate >= date '1994-01-01'
                 and o_orderdate < date '1995-01-01'
     )
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
