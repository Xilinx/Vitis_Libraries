# Hash-Join V2

For details on each version of the hash-join, please refer to the HTML doc.

## Overview

This benchmark tests the performance of `hashJoinMPU` from `hash_join_v2.hpp`
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


## Running the Benchmark

Usage can be queried with `make help`. Basic use is:

```
make run TARGET=sw_emu DEVICE=/path/to/xpfm
```

Change `sw_emu` to `hw_emu` or `hw` to run RTL simulation or board test correspondingly.
