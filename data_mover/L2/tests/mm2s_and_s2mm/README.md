# MM2S and S2MM test

This is a PL based simple data-mover test, the MM2S kernel and S2MM kernel are short-circuited,
meaning no AIE graph is inserted between. Yet, it is enough to show the PL data-mover's functionality.

The `mm2s` and `s2mm` kernels are compiled from pre-configured source in `L2/src/hw` folder.

## Running this test

Just like any other L2 Vitis case!

```
make run TARGET=hw_emu
```
