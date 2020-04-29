# Hash-Join V4

For details on each version of the hash-join, please refer to the HTML doc.

## Overview

This benchmark tests the performance of `hashJoinV4` from `hash_join_v4.h`
with the following query.

```
select
        sum(l_extendedprice * (1 - l_discount)) as revenue
from
        orders1994,
        lineitem
where
        l_orderkey = o_orderkey
;

```

Here `orders1994` is a self-made table, of all `orders` rows with `o_orderdate` between 1994-01-01 (inclusive) and 1995-01-01 (exclusive).

## Dataset

Due to unknown license, the source code of dataset generator is not directly included.
When the project runs, script in `db_data` folder will automatically grep the dataset generator
and compile it from source.

_This project uses 32-bit data for numeric fields._
To benchmark 64-bit performance, edit `host/table_dt.h` and make `TPCH_INT` an `int64_t`.

## Running the Benchmark

Usage can be queried with `make help`. Basic use is:

```
make run TARGET=sw_emu DEVICE=/path/to/xpfm
```

Change `sw_emu` to `hw_emu` or `hw` to run RTL simulation or board test correspondingly.
