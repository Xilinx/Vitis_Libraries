# Vitis Test on Hash-Join-MPU

## Overview

This bencmark tests the performance of `hash_join_mpu` from `hash_join_mpu.h`
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
When the project runs, script in `db_benchmark` folder will automatically grep the dataset generator and compile it from source.

_This project uses 32-bit data for numeric fields._ To benchmark 64-bit performance, edit `host/table_dt.h` and make `TPCH_INT` an `int64_t`.

## Makefile Mostly-Used Targets

  * run\_sw\_emu: software emulation.

  * run\_hw\_emu: hardware emulation.

  * run\_hw: execute on board.

	* help: show more info.

Before `make` the project, `env.sh` should be setup and sourced.

