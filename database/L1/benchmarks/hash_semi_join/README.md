# Vitis Test on Hash-Semi-Join

## Overview

This project demostrate FPGA acceleration of the following query over TPC-H scale factor 1 data.

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

## Makefile Mostly-Used Targets

  * run\_sw\_emu: software emulation.

  * run\_hw\_emu: hardware emulation.

  * run\_hw: execute on board.

## Dataset

We used the TPC-H dataset generated with ssb-dbgen tool. Due to unknown license, the source code is not directly included. To download the dbgen tool and create data files, follow README.md in db\_benchmark directory.
