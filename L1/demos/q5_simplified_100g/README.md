# Vitis Project for TPC-H Q5 Simplified

## Overview

This project demostrate FPGA acceleration of the following query, simplified from Query 5, over TPC-H data of scale factor 1, 3, 10, 30 and 100.

```
select
        sum(l_extendedprice * (1 - l_discount)) as revenue
from
        orders,
        lineitem
where
        l_orderkey = o_orderkey
        and o_orderdate >= date '1994-01-01'
        and o_orderdate < date '1995-01-01'
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
