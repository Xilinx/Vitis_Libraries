# Vitis Test on Hash-Group-Aggregate (General Version)

## Overview

This project demostrate FPGA acceleration of the following query over TPC-H scale factor 1 data.

```
select
        max(l_extendedprice), min(l_extendedprice), count_non_zero(l_extendedprice) as revenue
from
        lineitem
group by
        l_orderkey
;

```

## Makefile Mostly-Used Targets

  * run\_sw\_emu: software emulation.

  * run\_hw\_emu: hardware emulation.

  * run\_hw: execute on board.

## Dataset

We used the TPC-H dataset generated with ssb-dbgen tool. Due to unknown license, the source code is not directly included. To download the dbgen tool and create data files, follow README.md in db\_benchmark directory.
