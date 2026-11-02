.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

======================================================================
Bi-directional Data Mover with a User-Defined Programmable Handshake
======================================================================

Overview
========

Feature
-------

Bi-directional data movers are templated designs that provide both read and write access to a multi-dimension array in linear addressing memory and expose two AXI4-Stream ports and two MAXI ports for interfacing. Explicit synchronization of data movement inside the bi-directional data mover is supported with user-programmable hardware that can provide high performance exceeding the capabilities of the software-based control plane. The features of a bi-directional data mover with handshake include:

- Both read and write data-moving operations between DDR and AIE can work simultaneously with one Bi-DM.
- Compositions of four operations are supported: loading data to cache from DDR, sending cache data to AIE, receiving data into cache from AIE, and storing data from cache to DDR.
- Data width can be templated as 32, 64, 128, or 256 bits.
- Templated depth of on-chip cache is 2^N.
- Standalone hardware handshake can be user-programmed among the four components inside one Bi-DM.

Kernel Architecture
-------------------

The bi-directional data mover is composed of four homogeneous units working with a shared on-chip cache. Inside the Bi-DM, four units are each responsible for data movement between DDR/AXI-Stream and on-chip cache. Each unit is composed of a local arithmetic logic unit (ALU) and a local controller. Between the local ALU and controller of a unit, two HLS-Stream ports transfer ACK and pattern-id signals. Among controllers from different units, a ring-style chain transfers handshake signals with the guidance of identifiers ``source-id`` and ``target-id``. The following figure shows the kernel architecture diagram of Bi-DM.

.. image:: /images/bi_dm_4d_kernel_hsk.png
   :alt: various pattern
   :width: 80%
   :align: center

Dedicated control logic is implemented in all units to provide fine-grained control against each BD, which is flexible and user programmable. Through an internal stream channel, each unit within one Bi-DM can communicate with each other to perform the handshake, and the handshake can be terminated by ``0xFF``. Also, the ``target-id`` can be dynamically updated by ``0xFE``. Through the ring chain, each unit can broadcast the updated state appended to its own unique identifier (``source-id``) and broadcast identifier ``0xF`` (``target-id``), and monitor the acknowledge signal from a certain unit.

Enhanced 4D Buffer Descriptor
------------------------------

Each BD contains 24 32-bit integers to fully describe access on four-dimension data. They are the length, offset, tiling, stride, and wrap of each dimension, as well as the dimension order to serialize each 4D cubic. For read/write operations to the on-chip cache, an additional URAM_offset is needed for each BD, with which a 25-word pattern is generated in the input configuration. The following pseudo-code shows the address generation for serializing the read/write:

.. code-block:: cpp

    int uram_offset = pattern_buf++;
    int* buf_dim = pattern_buf;
    int* offset = pattern_buf + 4;
    int* tiling = pattern_buf + 8;
    int* dim_idx = pattern_buf + 12;
    int* stride = pattern_buf + 16;
    int* wrap = pattern_buf + 20;
    for (int w = 0; w < wrap[dim_idx[3]]; w++) {
        for (int z = 0; z < wrap[dim_idx[2]]; z++) {
            for (int y = 0; y < wrap[dim_idx[1]]; y++) {
                for (int x = 0; x < wrap[dim_idx[0]]; x++) {
                    int bias[4];
                    bias[dim_idx[0]] = offset[dim_idx[0]] + stride[dim_idx[0]] * x;
                    bias[dim_idx[1]] = offset[dim_idx[1]] + stride[dim_idx[1]] * y;
                    bias[dim_idx[2]] = offset[dim_idx[2]] + stride[dim_idx[2]] * z;
                    bias[dim_idx[3]] = offset[dim_idx[3]] + stride[dim_idx[3]] * w;
                    for (int d4 = 0; d4 < tiling[3]; d4++) {
                        for (int d3 = 0; d3 < tiling[2]; d3++) {
                            for (int d2 = 0; d2 < tiling[1]; d2++) {
                                for (int d1 = 0; d1 < tiling[0]; d1++) {
                                    elem_address = (bias[3] + d4) * (buf_dim[2] * buf_dim[1] * buf_dim[0]) +
                                                   (bias[2] + d3) * (buf_dim[1] * buf_dim[0]) +
                                                   (bias[1] + d2) * buf_dim[0] +
                                                   (bias[0] + d1);
                                    data_to_be_read = data[uram_offset + elem_address];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

Programmable Handshake
----------------------

Each of the four units inside Bi-DM has one input HLS-Stream port and one output HLS-Stream port to handle synchronization signals from the other units. For each unit, a local controller decides whether to bypass the synchronization signals from other units according to the ``target-id`` or forward them to the local ALU. The local controller has two HLS-Stream ports to handle synchronization signals from other units and two HLS-Stream ports to handle signals from and to the local ALU. The local ALU sends and receives signals from the local controller using one input HLS-Stream port and one output HLS-Stream port.


ALU Specification
=================

Feature
-------

Four independent 8-bit ALUs are implemented in the Bi-DM kernel. Each ALU includes eight 8-bit registers to execute fixed-size 32-bit instructions with two input streams and two output streams to interface with the local controller and local data-moving unit.

- 32-bit instruction
- Up to 1024 32-bit program memory per ALU

Instruction Set
---------------

Five operational instruction types plus an exit instruction are supported:

- MOVE
- POP
- PUSH
- ADD
- JUMP
- EXIT

The following figure shows the Bi-DM instruction set architecture (ISA) format.

.. image:: /images/bi_dm_ISA_format.png
   :alt: various pattern
   :width: 80%
   :align: center
