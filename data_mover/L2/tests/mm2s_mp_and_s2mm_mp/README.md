# MM2S and S2MM test

This is a PL based simple data-mover test, the MM2S kernel and S2MM kernel are short-circuited,
meaning no AIE graph is inserted between. Yet, it is enough to show the PL data-mover's functionality.

## Dynamic Generation

The `mm2s_mp` and `s2mm_mp` kernels are generated into `pl` folder,
from configurations written in `config_params.json`,
through their library API metadata in `L2/meta`.

The default configuration is dual port. So two buffers are read by `mm2s_mp` into streams and
written into another two buffers by `s2mm_mp`.
The case also ships with a single port configuration, where only one buffer is read and another one is written.

The host software gets aligned throught generated `sw/config.hpp` from `test_gen.py`.

The `scripts/instance_generator.py` is the unified python driver for creating dynamic files,
including both kernel source instances and configuration header for host.


## Running this test

Just like any other L2 Vitis case!

```
make run TARGET=hw_emu
```
This runs with the first(default) parameter set `single_port` in `config_params.json`.

To create single port MM2S and S2MM, i.e. the second parameter set, run the following after `make cleanall`
```
make run TARGET=hw_emu PARAMS=single_port
```
