## Data-Mover Tests

This folder contains test for both API-metadata based L2 Kernel APIs and static simple L2 Kernels.

### API-metadata based L2 Kernels

Their source code is mostly generated into projects based on config.
The spec (including generation script) can be found in `L2/meta` directory.

- `s2mm` and `mm2s`: these are the simplest, linear buffer to stream or steam to buffer data movers.
  Unlike the rest 4, these two have statis source file from `L2/src` 
- `s2mm_mp` and `mm2s_mp`: The multi-port version of data movers, so that one Kernel can handle multiple port pairs.
- `s2mm_4d` and `mm2s_4d`: The data mover with SW proragmmability, so that it can read/write DDR tensor in given axes and strides.

### Static L2 Kernels

All their source in `L2/src` folder.

- `send_ram/rom_to_steam`: the ROM version can be called to preload data into chip, and then stream at high thoughput.
  The ROM version do not need preload call, but its data is baked into bitstream and cannot be changed at runtime.
- `load_master_to_stream` and `load_master_to_stream_with_counter`: these are similar to naive `mm2s`,
  yet has two fixed pair of ports, and the later comes with a counter for cycles used to send the data.
  The counter result is stored in an auxilary return buffer.
- `validate_stream_with_master/ram/rom`: receives two streams and check them against DDR buffers/RAMs/ROMs.
  The later two uses on chip memory, and are supposed to introduce no backpressure.
  The RAM version need a special call ahead to preload the on-chip RAM.
