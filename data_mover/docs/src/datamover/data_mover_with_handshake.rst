.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.
.. _L2_DATAMOVER_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

========================================================
4D Data Mover with a User-Defined Programmable Handshake
========================================================

Overview
========

Feature
-------

4D Data Movers are templated designs that provide read/write access to a multi-dimension array in linear addressing memory and expose an AXI4-Stream port for interfacing. Explicit synchronization for each data movement is supported with user-programmable hardware that can provide high performance exceeding the capabilities of the software-based control plane. The features of a 4D Data Mover with handshake include:

- Data width can be templated as 32, 64, 128, or 256 bits.
- Full 4D buffer descriptor is supported.
- Templated depth of on-chip cache is 2^N.
- Standalone hardware handshake can be user-programmed within or across a single data mover boundary.

Kernel Architecture
-------------------

The 4D Data Mover separates each data movement into two parts using depth-templated on-chip memory. Each element from an AXI4-Stream or AXI master is first cached and then transferred to the AXI master or AXI4-Stream using two independent groups of 4D buffer descriptors (BDs). Two dedicated arithmetic logic unit (ALU) components control each stage of data movement. Both the 4D BD and the ALU programs are initialized by the same AXI master. The following figure shows a DM diagram of an AXI4-Stream to AXI master.

Dedicated control logic is implemented in the ALU to provide fine-grained control against each BD, which is flexible and user programmable. Through an internal stream channel, each part within one DM can communicate with each other to perform the handshake, and the handshake can be terminated by ``0xFF``. Through an additional AXI4-Stream chain, each DM can broadcast the updated state appended to its own unique identifier and monitor the acknowledge signal from a certain DM.

.. image:: /images/4d_kernel_hsk.png
   :alt: various pattern
   :width: 80%
   :align: center

Enhanced 4D Buffer Descriptor
------------------------------

Each BD contains 24 32-bit integers to fully describe access on four-dimension data. They are the length, offset, tiling, stride, and wrap of each dimension, as well as the dimension order to serialize each 4D cubic. The following pseudo-code shows the address generation for serializing the read/write:

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

Programmable Handshake
----------------------

The ``ALU1`` module has two input synchronization interfaces: one with the AXI4-Stream-side component and one with ``ALU0``. In addition to providing the handshake within the DM kernel, ``ALU0`` in each DM is also responsible for interfacing with external DM kernels through an external AXI4-Stream connection. This connection receives states from the input AXI4-Stream interface and forwards them to an internal synchronization module if the identifier is matched, or bypasses the input states to the output AXI4-Stream interface if not.

ALU Specification
=================

Feature
-------

Two independent 8-bit ALUs are implemented in a single DM kernel. Each ALU includes eight 8-bit registers to execute fixed-size 32-bit instructions. The AXI master-side ALU has three input streams and three output streams for local and external synchronization, while the AXI4-Stream-side ALU has two input streams and two output streams for local synchronization.

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

The following figure shows the instruction set architecture (ISA) format.

.. image:: /images/ISA_format.png
   :alt: various pattern
   :width: 80%
   :align: center
