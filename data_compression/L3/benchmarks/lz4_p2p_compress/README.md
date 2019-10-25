This LZ4 P2P Compress application runs with Xilinx compression and standard decompression flow and currently supported with non P2P flow.

* Source codes (data_compression): In this folder all the source files are available.
```
   Host Sources : ./data_compression/L3/src/
   Host Includes : ./data_compression/L3/include/
   Kernel Code  : ./data_compression/L2/src/
   HLS Modules  : ./data_compression/L1/include/
```

* Running Emulation: Steps to build the design and run for sw_emu
```
    $ Setup Xilinx SDx 2019.2 along with XRT 
    $ cd ./data_compression/L3/benchmarks/lz4_p2p_comp/
    $ make run TARGET=sw_emu DEVICE=<path to u.2 xpfm file>
```

* Building Design (xclbin): Steps to build the design and run for hw
```
    $ Setup Xilinx SDx 2019.2 along with XRT 
    $ cd ./data_compression/L3/benchmarks/lz4_p2p_comp/
    $ make all TARGET=hw DEVICE=<path to u.2 xpfm file> 
```

* Input Test Data
  - The input files are placed in data/ folder under L3/benchmarks/lz4_p2p_comp/ which are used for design.

* Following are the step by step instructions to run the design on board.
  - Source the XRT 
```
        $ source /opt/xilinx/xrt/setup.sh
```
  - To Mount the data from SSD
```
        $ mkfs.xfs -f /dev/nvme0n1
        $ mount /dev/nvme0n1 /mnt
        $ cp -rf <./data/> /mnt/ (copy the input files to /mnt path)
```
  - Run the design
```
        To enable P2P flow give 1 else 0
        $ ./build/xil_lz4_8b -cx ./build/compress.xclbin -p2p <0/1> -l <./test.list> 
```

## Results

### Resource Utilization <br />

Table below presents resource utilization of Xilinx LZ4 P2P compress
kernel with 8 engines for single compute unit. It is possible to extend number of engines to achieve higher throughput.


| Design | LUT | LUTMEM | REG | BRAM | URAM| DSP | Fmax (MHz) |
| --------------- | --- | ------ | --- | ---- | --- | -----| -----|
| Compression     | 51389 (14.13%) |14163 (9.81%) | 63547 (7.81%) | 58 (12.83%)| 48 (37.50%)| 1 (0.05%)|215|
| Packer          | 10907 (3.00%) | 1828 (1.27%)| 16637 (2.05%)| 16 (3.54%)| 0 | 2(0.10%)|215|


### Throughput & Compression Ratio

Table below presents the best end to end compress kernel execution with SSD write throughput achieved with two compute units during execution of this application.

| Topic| Results| 
|-------|--------|
|Best Compression Throughput|2.7 GB/s|

Note: Overall throughput can still be increased with multiple compute units.
