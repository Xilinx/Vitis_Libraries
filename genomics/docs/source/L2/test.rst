
.. meta::
   :keywords: Vitis, Library, Smithwaterman, Xilinx, FPGA OpenCL Kernels, Smithwaterman Test, 
   :description: This section provides various application tests
   :xlnxdocumentclass: Document
   :xlnxdocumenttypes: Tutorials

=====
Tests
=====

Tests examples for  **smithwaterman**, **pairhmm** and **smem** kernels are available in the ``L2/tests/`` directory.

.. toctree::
   :maxdepth: 1
   :caption: List of Tests

   smithwaterman.rst
   pairhmm.rst
   smem.rst
   
.. note::
   Execute the following commands before building any of the examples:

.. code-block:: bash
      
   $ source <Vitis_Installed_Path>/installs/lin64/Vitis/2021.1/settings64.sh
   $ source <Vitis_Installed_Path>/xbb/xrt/packages/setup.sh

Build Instructions
------------------

Execute the following commands to compile and test run this example:

.. code-block:: bash
      
   $ make run TARGET=sw_emu

Variable ``TARGET`` can take the following values:

  - **sw_emu**  : software emulation
  
  - **hw_emu**  : hardware emulation
  
  - **hw**  : run on actual hardware

By default, the target device is set as Alveo U200. In order to target a different
device, use the  ``DEVICE`` argument. For example:

.. code-block:: bash

    make run TARGET=sw_emu DEVICE=<new_device.xpfm>

.. note::
   Build instructions explained in this section are common for all the
   applications. The generated executable names may differ.
