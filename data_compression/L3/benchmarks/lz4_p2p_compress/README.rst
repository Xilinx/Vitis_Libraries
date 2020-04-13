This LZ4 P2P Compress application runs with Xilinx compression and
standard decompression flow and currently supported with non P2P flow.

-  Source codes (data_compression): In this folder all the source files
   are available.

::

      Host Sources : ./data_compression/L3/src/
      Host Includes : ./data_compression/L3/include/
      Kernel Code  : ./data_compression/L2/src/
      HLS Modules  : ./data_compression/L1/include/

-  Running Emulation: Steps to build the design and run for sw_emu

::

       $ Setup Xilinx vitis 2019.2 along with XRT 
       $ cd ./data_compression/L3/benchmarks/lz4_p2p_comp/
       $ make run TARGET=sw_emu DEVICE=<path to u.2 xpfm file>

-  Building Design (xclbin): Steps to build the design and run for hw

::

       $ Setup Xilinx vitis 2019.2 along with XRT 
       $ cd ./data_compression/L3/benchmarks/lz4_p2p_comp/
       $ make all TARGET=hw DEVICE=<path to u.2 xpfm file> 

-  Input Test Data

   -  The input files are placed in data/ folder under
      L3/benchmarks/lz4_p2p_comp/ which are used for design.

-  Following are the step by step instructions to run the design on
   board.

   -  Source the XRT

   ::

            $ source /opt/xilinx/xrt/setup.sh

   -  To Mount the data from SSD

   ::

            $ mkfs.xfs -f /dev/nvme0n1
            $ mount /dev/nvme0n1 /mnt
            $ cp -rf <./data/> /mnt/ (copy the input files to /mnt path)

   -  Run the design

   ::

            To enable P2P flow give 1 else 0
            $ ./build/xil_lz4_8b -cx ./build/compress.xclbin -p2p <0/1> -l <./test.list> 

Results
-------

Resource Utilization 
~~~~~~~~~~~~~~~~~~~~~

Table below presents resource utilization of Xilinx LZ4 P2P compress
kernel with 8 engines for single compute unit. It is possible to extend
number of engines to achieve higher throughput.

+-----------------------+---+--------+---+-----+---+-------+-------+
| Design                | L | LUTMEM | R | BRA | U | DSP   | Fmax  |
|                       | U |        | E | M   | R |       | (MHz) |
|                       | T |        | G |     | A |       |       |
|                       |   |        |   |     | M |       |       |
+=======================+===+========+===+=====+===+=======+=======+
| Compression           | 5 | 14163  | 6 | 58  | 4 | 1     | 200   |
|                       | 1 | (9.47% | 4 | (11 | 8 | (0.05 |       |
|                       | 7 | )      | 2 | .58 | ( | %)    |       |
|                       | 7 |        | 0 | %)  | 3 |       |       |
|                       | 2 |        | 9 |     | 7 |       |       |
|                       | ( |        | ( |     | . |       |       |
|                       | 1 |        | 7 |     | 5 |       |       |
|                       | 3 |        | . |     | 0 |       |       |
|                       | . |        | 6 |     | % |       |       |
|                       | 7 |        | 9 |     | ) |       |       |
|                       | 7 |        | % |     |   |       |       |
|                       | % |        | ) |     |   |       |       |
|                       | ) |        |   |     |   |       |       |
+-----------------------+---+--------+---+-----+---+-------+-------+
| Packer                | 1 | 1828   | 1 | 16  | 0 | 2(0.1 | 200   |
|                       | 0 | (1.22% | 6 | (3. |   | 0%)   |       |
|                       | 9 | )      | 7 | 19% |   |       |       |
|                       | 2 |        | 0 | )   |   |       |       |
|                       | 2 |        | 8 |     |   |       |       |
|                       | ( |        | ( |     |   |       |       |
|                       | 2 |        | 2 |     |   |       |       |
|                       | . |        | . |     |   |       |       |
|                       | 9 |        | 0 |     |   |       |       |
|                       | 0 |        | 0 |     |   |       |       |
|                       | % |        | % |     |   |       |       |
|                       | ) |        | ) |     |   |       |       |
+-----------------------+---+--------+---+-----+---+-------+-------+

Throughput & Compression Ratio
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Table below presents the best end to end compress kernel execution with
SSD write throughput achieved with two compute units during execution of
this application.

=========================== ========
Topic                       Results
=========================== ========
Best Compression Throughput 1.6 GB/s
=========================== ========

Note: Overall throughput can still be increased with multiple compute
units.
