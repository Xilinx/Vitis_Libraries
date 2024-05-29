.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _l1_f2daiel3:

Filter2D Pipeline on Multiple AIE Cores
########################################

This example demonstrates how a function/pipeline of functions can run on multiple AI Engine cores to achieve higher throughput. Back-to-back Filter2D pipeline running on three AI Engine cores is demonstrated in this example. The source files can be found in ``L3/tests/aie/Filter2D_multicore/16bit_aie_8bit_pl`` directory.

This example tests the performance of back-to-back Filter2D pipeline with three images being processed in parallel on three AI Engine cores. Each AI Engine core is fed by one instance of Tiler and Stitcher PL kernels. 

The tutorial provides a step-by-step guide that covers commands for building and running the pipeline.

Executable Usage
================

* **Work Directory (Step 1)**

The steps for library download and environment setup can be found in the README of the L3 folder. Refer to :ref:`Getting Started with Vitis Vision AIEngine Library Functions <aie_prerequisites>` for more details. For getting the design,

.. code-block:: bash

   cd L3/tests/aie/Filter2D_multicore/16bit_aie_8bit_pl

* **Build Kernel (Step 2)**

Run the following make command to build your ``XCLBIN`` and host binary targeting a specific device. Be aware that this process can take up to a couple of hours.

.. code-block:: bash

   export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
   make all TARGET=hw

* **Run Kernel (Step 3)**

To get the benchmark results, run the following command.

.. code-block:: bash

   make run TARGET=hw

* **Running on HW**

After the build for hardware target completes, the ``sd_card.img`` file will be generated in the build directory. 

1. Use software such as 'Etcher' to flash the ``sd_card.img`` file onto an SD Card. 
2. After flashing is complete, insert the SD card in the SD card slot on the board, then power on the board.
3. Use 'Teraterm' to connect to the COM port and wait for the system to boot up.
4. After the boot up is done, goto ``/media/sd-mmcblk0p1`` directory and run the executable file.
  
Performance
============

Performance is shown in the following table.

.. table:: Table 1 Performance numbers in terms of FPS (Frames Per Second) for full HD images
    :align: center
	
    +----------------------+--------------+
    |       Dataset        |   FPS        |
    +======================+==============+
    |   Full HD(1920x1080) |   555        |
    +----------------------+--------------+

.. toctree::
    :maxdepth: 1
