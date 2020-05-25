# Hash-Anti-Join

## Overview

This benchmark tests the performance of `anti_join_build_probe` from `hash_anti_join.h`
with the following query.

```
SELECT
        SUM(l_extendedprice * (1 - l_discount))
FROM
        lineitem
WHERE
        l_orderkey NOT IN (SELECT o_orderkey FROM orders1994)
;

```

Here `orders1994` is a self-made table, of all `orders` rows with `o_orderdate` between 1994-01-01 (inclusive) and 1995-01-01 (exclusive).

## Dataset

_This project uses 32-bit data for numeric fields._
To benchmark 64-bit performance, edit `host/table_dt.h` and make `TPCH_INT` an `int64_t`.

## Running the Benchmark

Usage can be queried with `make help`. Basic use is:

```
make run TARGET=sw_emu DEVICE=/path/to/xpfm
```

Change `sw_emu` to `hw_emu` or `hw` to run RTL simulation or board test correspondingly.
