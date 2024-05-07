.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

======================================================================
Bi-directional Data-Mover with a User-Defined Programmable Handshake
======================================================================

Overview
========

Feature
-------

Bi-directional data movers are templated designs which provide both read and write access to a multi-dimension array in the linear addressing memory and expose two AXI4-Stream ports and two MAXI ports to interface with. Explicit synchronization of the data movement inside the bi-directional is supported with user-programmable hardware which could provide high performance that exceed the capabilities of the software-based control plane. The feature of a bi-directional Data-Mover with handshake includes:

- Both read and write data moving operations between DDR and AIE can work simultaneously with one Bi-DM
- Allow compostions of four operations: Load data to cache from DDR, Send cache data to AIE, Receive data into cache from AIE, Store data from cache to DDR
- Data width could be templated as 32/64/128/256/512 bits.
- Templated depth of on-chip cache should be 2^N.
- Standalone hardware handshake can be user-programmable among the four components inside one Bi-DM

Kernel Architecture
-------------------
Bi-directional data mover is composed of 4 homogeneous units working with a shared on-chip cache. Inside of the Bi-DM, 4 units are separately responsible for data movement between DDR/AXI-Stream and on-chip cache, which in detail are all composed of a local ALU and a local controller. Between the local ALU and controller of a unit, 2 HLS-Stream ports are used to transfer
ACK and pattern-id signals. While among controllers from different units, a ring-style chain is established through them to transfer handshake signals with the guidance of identifers `source-id` and `target-id`. The kernel architecture diagram of Bi-DM is illustrated below.

.. image:: /images/bi_dm_4d_kernel_hsk.png
   :alt: various pattern
   :width: 80%
   :align: center

Dedicated control logic is implemented in all units to provide fine-grain control against each BD, which is flexible and user programmable. By an internal stream channel, each unit within one Bi-DM can communicate with each other to do the handshake, and it can be teminated by `0xFF`. Additionally, the `source-id` and `target-id` could be dynamically updated by `0xFE`. Through the ring chain, each unit can broadcast the updated state appended to its own unique identifer(aka `source-id`) and broadcast identifer `0xF`(aka `target-id`) and monitor the acknowledge signal from a certain unit.

Enhanced 4D Buffer Descriptor
-----------------------------

Each BD contains 24 32-bit integers to fully describe access on four dimension data. They are the length/offset/tiling/stride/wrap of each dimension, as well as the dimension order to serialize each 4D cubic. As for read/write operations to the on-chip cache, an additional URAM_offset is needed for each BD, with which a 25 bit pattern will be generated in users' input config. The address generation for serializing the read/write given as pseudo-code is as follows:

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

Programmble Handshake
---------------------

Each of the 4 units inside Bi-DM has one input HLS-Stream port and one output HLS-Stream port to deal with synchronizational signals from the other units. While for each unit, a local controller will decide wether to bypass the synchronizational signals from other units according to the `target-id` or forward it into the local ALU. Similar for local controller, it has two HLS-Stream ports to handle synchronizational signals from other units and two HLS-Stream ports to handle signals from/to local ALU. As for local ALU, it just needs to send/receive signals from local controller by an input HLS-Stream port and an output HLS-Stream port.


ALU Specification
=================

Feature
-------

Four independent 8-bit ALUs are implemented in Bi-DM kernel. Each of the ALU includes eight 8-bit registers to execute the fixed-size 32-bit instructions with two input streams and two output streams to interface within local controller and local data-moving unit.

- 32-bit instruction
- Up to 1024 32-bit program memory per ALU

Instruction Set
---------------

Currently, there are five types of instruction sets supported:

- MOVE
- POP
- PUSH
- ADD
- JUMP

.. image:: /images/bi_dm_ISA_format.png
   :alt: various pattern
   :width: 80%
   :align: center