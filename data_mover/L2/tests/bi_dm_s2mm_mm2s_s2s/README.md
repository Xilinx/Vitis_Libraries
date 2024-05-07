# AXI-Stream to On-chip Cache to AXI-Stream

This is a PL based simple bi-directional data-mover test, the MM2S kernel and S2MM kernel are short-circuited,
meaning no AIE graph is inserted between. Yet, it is enough to show the PL Bi-DM's functionality.

The `mm2s` and `s2mm` kernels are compiled from pre-configured source in `L2/src/hw` folder.

## Running this test

Cases:
 - TEST_AXIS_TO_DDR
 - TEST_DDR_TO_AXIS
 - TEST_AXIS_CACHE_AXIS

Three cases are included, to target which scenario to test, `CASE` option needs to be specified like:

```
make run TARGET=hw_emu CASE=TEST_AXIS_TO_DDR
```
