# Compound Sort

## Overview

This benchmark tests the performance of `compoundSort` primitive with an array of integer keys. This primitive is named as compound sort, as it combines `insertSort` and `mergeSort`, to balance storage and compute resource usage. 

## Running the Benchmark

Usage can be queried with `make help`. Basic use is:

```
make run TARGET=sw_emu DEVICE=/path/to/xpfm
```

Change `sw_emu` to `hw_emu` or `hw` to run RTL simulation or board test correspondingly.
