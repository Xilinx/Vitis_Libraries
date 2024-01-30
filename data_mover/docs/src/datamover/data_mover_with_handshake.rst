.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

========================================================
4D Data-Mover with a User-Defined Programmable Handshake
========================================================

Overview
========

Feature
-------

4D Data Movers are templated designs to read/write access a multi-dimension array in the linear addressing memory and provide an AXI4-Stream to interface with. Explicit synchronization for each data movement is supported with user-programmable hardware which could provide high performance that exceed the capabilities of the software-based control plane. The feature of a 4D Data-Mover with handshake includes:

- Data width could be templated as 32/64/128/256/512 bits.
- Full 4D buffer descriptor is supported.
- Templated depth of on-chip cache should be 2^N.
- Standalone hardware handshake can be user-programmable within or across single data mover boundary.

Kernel Architecture
-------------------

4D Data Mover separates each data movement into two parts by depth-templated on-chip memory. Each element from an AXI4-Stream or AXI-master will first be cached and transferred to the AXI-master or AXI4-Stream with two independent groups of 4D BD. There are two dedicated ALU components to control each stage of data movement. Both the 4D BD and program of ALUs are initialized by the same AXI-master. One DM diagram of an AXI4-Stream to AXI-master is illustrated below.

Dedicated control logic is implemented in ALU to provide fine-grain control against each BD, which is flexible and user programmable. By an internal stream channel, each part within one DM can communicate with each other to do the handshake, and it can be teminated by `0xFF`. By an additional AXI4-Stream chain, each DM can broadcast the updated state appended to its own unique identifer and monitor the acknowledge signal from a certain DM. 

.. image:: /images/4d_kernel_hsk.png
   :alt: various pattern
   :width: 80%
   :align: center

Enhanced 4D Buffer Descriptor
-----------------------------

Each BD contains 24 32-bit integers to fully describe access on four dimension data. They are the length/offset/tiling/stride/wrap of each dimension, as well as the dimension order to serialize each 4D cubic. The address generation for serializing the read/write given as pseudo-code is as follows:

.. code-block:: cpp

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
                                    data_to_be_read = data[elem_address];
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

The ALU1 module has two input synchronization interfaced with AXIS-side component and ALU0. Besides providing the handshake within the DM kernel, ALU0 in each DM also must be responsible for interfacing with external DM kernels by an external AXI4-Stream connection, which receives the states from the input AXI4-Stream interface and forwards it to an internal synchronization module if the identifier is matched, or the input states would be bypassed to its output AXI4-Stream interface.

ALU Specification
=================

Feature
-------

Two independent 8-bit ALUs are implemented in a single DM kernel. Each of the ALU includes eight 8-bit registers to execute the fixed-size 32-bit instructions with three input streams and three output streams to interface within or across the DM boundary.

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

.. image:: /images/ISA_format.png
   :alt: various pattern
   :width: 80%
   :align: center