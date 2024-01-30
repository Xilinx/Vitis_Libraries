.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Database, Vitis Database Library, Alveo
   :description: Vitis Database Library is an open-sourced Vitis library written in C++ for accelerating database applications in a variety of use cases.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

.. _brief:

================================
Vitis Graph Library Tutorial
================================

Get and Run the Vitis Graph Library
==============================================

Get the Dependencies
------------------------------------

| `Vitis <https://www.xilinx.com/products/design-tools/vitis/vitis-platform.html>`_, Instructions to install AMD Vitis |trade| can be found `here <https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Installation>`_.
| `Alveo U50 packages <https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/alveo/u50.html>`_, Instructions to deploy AMD Alveo |trade| U50 can be found `here <https://www.xilinx.com/support/documentation/boards_and_kits/accelerator-cards/1_8/ug1370-u50-installation.pdf>`_.
| `XRM <https://github.com/Xilinx/XRM>`_ (Optional, only for L3 use cases), Instructions to install XRM can be found `here <https://xilinx.github.io/XRM/Build.html>`_.

Setup Environment
------------------------------------

.. code-block:: bash

   #!/bin/bash
   source <Vitis_install_path>/Vitis/2022.1/settings64.sh
   source /opt/xilinx/xrt/setup.sh
   source /opt/xilinx/xrm/setup.sh
   export PLATFORM_REPO_PATHS=<path to platforms>
   export PLATFORM=xilinx_u50_gen3x16_xdma_5_202210_1
   export TARGET=sw_emu

.. Note:: The TARGET environment variable can be set as sw_emu, hw_emu and hw according to which Vitis target is expected to run.
sw_emu is for C level emulations. hw_emu is for RTL level emulations. hw is for real on-board test. For more information about the Vitis Target, have a look at `here <https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Build-Targets?tocId=8ijg9En3MQ_7CJBZrUFENw>`_.

Download the Vitis Graph Library
------------------------------------

.. code-block:: bash

   #!/bin/bash
   git clone https://github.com/Xilinx/Vitis_Libraries.git
   cd Vitis_Libraries/graph

Run a L3 Example
------------------------------------

.. code-block:: bash

   #!/bin/bash
   cd L3/tests/SSSP  # SSSP is an example case. Please change directory to any other cases in L3/test if interested.
   make help         # show available make command
   make host         # build the binary running on host
   make xclbin       # build the binary running on Alveo
   make run          # run the entire program
   make cleanall
   

For more explanation on L3 cases, refer to :ref:`tutorial::l3`.

Run a L2 Example
------------------------------------

.. code-block:: bash

   #!/bin/bash
   cd L2/tests/shortest_path_float_pred   # shortest_path_float_pred is an example case. Please change directory to any other cases in L2/test if interested.
   make help                              # show available make command
   make host                              # build the binary running on host
   make xclbin                            # build the binary running on Alveo
   make run                               # run the entire program
   make cleanall

For more explanation on L2 cases, refer to :ref:`tutorial::l2`.

Run a L1 Example
------------------------------------

.. code-block:: bash

   #!/bin/bash
   cd L1/tests/hw/dense_similarity_int    # dense_similarity_int is an example case. Please change directory to any other cases in L1/test if interested
   make help                              # show available make command
   make run CSIM=1                        # run C level simulation of the HLS code
   make run CSYNTH=1 COSIM=1              # run RTL level simulation of the HLS code
   make cleanall

For more explanation on L1 cases, refer to :ref:`tutorial::l1`.

How Vitis Graph Library Works
==============================================
AMD Vitis |trade| Graph Library aims to provide reference `Vitis <https://www.xilinx.com/products/design-tools/vitis/vitis-platform.html>`_ 
implementations for a set of graph processing algorithms which fits the `Xilinx Alveo Series <https://www.xilinx.com/products/boards-and-kits/alveo.html>`_
acceleration cards. The API in Vitis Graph Library has been classified into three layers, namely L1/L2/L3. Each targets to serve different audience.

* L3 APIs locate at ``Vitis_Libraries/graph/L3/include``. Pure software APIs are provided to customers who want a fast deployment of graph processing algorithms on Alveo Cards. It provides a series of software designs to efficiently make use of resources in Alveo cards and deliver high performance graph processing.

* L2 APIs locate at ``Vitis_Libraries/graph/L2/include``. They are a number of compute-unit designs running on Alveo cards. It provides a set of compute-unit designs implemented in HLS codes. These L2 APIs need be compiled as OpenCL kernels and are called by OpenCL APIs.

* L1 APIs locate at ``Vitis_Libraries/graph/L1/include``. They are basic components that are used to compose compute-units. The L1 APIs are all well-optimized HLS design and are able to fit into various resource constraints.


.. _tutorial::l3:

L3 API
------------------------------------

Target Audience
^^^^^^^^^^^^^^^^^

If a fast deployment of FPGA accelerated graph processor is required, the Vitis Graph L3 APIs would be the best choice. Pre-designed and well-optimized Vitis compute units are provided in these APIs. And efficient software management of resources is also included in these APIs. To deploy graph accelerators, you need to do is just a simple call of these c++ L3 APIs.

Example Usage
^^^^^^^^^^^^^^^^^

Run the following codes to build the library (Do not forget to install XRT/XRM and setup the environment):

.. code-block:: bash

   #!/bin/bash
   cd Vitis_Libraries/graph/L3/lib
   make libgraphL3
   export LD_LIBRARY_PATH=<PATH TO YOUR Vitis_Libraries/graph/L3/lib>:$LD_LIBRARY_PATH

To make use of the L3/APIs, include ``Vitis_Libraries/graph/L3/include`` path and link ``Vitis_Libraries/graph/L3/lib`` path when compiling the code.

The following steps are usually required to make a call of the L3 APIs:

(1) Setup the handle

.. code-block:: cpp

   xf::graph::L3::Handle::singleOP op0;   // create a configuration of operation (such as shortest path, wcc)
   op0.operationName = "shortestPathFloat";

   xf::graph::L3::Handle handle0;
   handle0.addOp(op0);  // initialize the Alveo board with the required operation, may have more than one kind of operation
   handle0.setUp();     // Download binaries to FPGAs

(2) Setup and Deploy the Graph

.. code-block:: cpp

   xf::graph::Graph<uint32_t, DT> g("CSR", numVertices, numEdges, offsetsCSR, indicesCSR, weightsCSR); // Create the graph
   (handle0.opsp)->loadGraph(g); // Deploy the graph data

(3) Run the required operation

.. code-block:: cpp

   auto ev = xf::graph::L3::shortestPath(handle0, nSource, &sourceID, weighted, g, result, pred); // Run the operation, this is a non-block call, actually start a thread
   int ret = ev.wait(); // wait for the operation to finish

(4) Release resources

.. code-block:: cpp

   (handle0.opsp)->join(); // join the thread
   handle0.free(); // release other memories
   g.freeBuffers(); // release graph memories

.. _tutorial::l2:

L2 API
------------------------------------

Target Audience
^^^^^^^^^^^^^^^^^

If a pure FPGA based graph accelerator is required, the Vitis Graph L2 interface might be interested. The L2 APIs provide HLS function that can be directly built into a Vitis compute-unit (OpenCL kernel). The testcases of the L2 APIs can be good references to compile and run the FPGA binaries (xclbins). Simple OpenCL codes are also provided to make use of the generated FPGA binaries. To efficiently management this FPGA binaries and make use of FPGA resources, refer to :ref:`tutorial::l3`.

Example Usage
^^^^^^^^^^^^^^^^^

The L2 API can be found at ``Vitis_Libraries/graph/L2/include``. A typical code for calling L2 APIs might look like this:

.. code-block:: cpp

   extern "C" void shortestPath_top(ap_uint<32>* config,
                                    ap_uint<512>* offset,
                                    ap_uint<512>* column,
                                    ap_uint<512>* weight,

                                    ap_uint<512>* ddrQue512,
                                    ap_uint<32>* ddrQue,

                                    ap_uint<512>* result512,
                                    ap_uint<32>* result,
                                    ap_uint<512>* pred512,
                                    ap_uint<32>* pred,
                                    ap_uint<8>* info) {
      const int depth_E = E;
      const int depth_V = V;

   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 8 bundle = gmem0 port = config depth = 4
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 8 bundle = gmem0 port = offset depth = depth_V
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem1 port = column depth = depth_E
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem2 port = weight depth = depth_E
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 2 max_read_burst_length = 2 bundle = gmem3 port = ddrQue depth = depth_E*16
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 2 max_read_burst_length = 2 bundle = gmem3 port = ddrQue512 depth = depth_E
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      32 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem4 port = result512 depth = depth_V
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      32 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem4 port = info depth = 8
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      32 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem4 port = result depth = depth_V*16
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem5 port = pred512 depth = depth_V
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem5 port = pred depth = depth_V*16

      xf::graph::singleSourceShortestPath<32, MAXOUTDEGREE>(config, offset, column, weight, ddrQue512, ddrQue, result512,
                                                            result, pred512, pred, info);
   }

It is usually a wrapper function of APIs in ``Vitis_Libraries/graph/L3/lib``. Following might be the code:

.. code-block:: cpp

   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 8 bundle = gmem0 port = config depth = 4
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 8 bundle = gmem0 port = offset depth = depth_V
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem1 port = column depth = depth_E
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
      32 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem2 port = weight depth = depth_E
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 2 max_read_burst_length = 2 bundle = gmem3 port = ddrQue depth = depth_E*16
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 2 max_read_burst_length = 2 bundle = gmem3 port = ddrQue512 depth = depth_E
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      32 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem4 port = result512 depth = depth_V
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      32 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem4 port = info depth = 8
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      32 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem4 port = result depth = depth_V*16
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem5 port = pred512 depth = depth_V
   #pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 32 num_read_outstanding = \
      1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem5 port = pred depth = depth_V*16

These are the HLS pragmas of the interface. They are responsible for configuring the interface of the FPGA binaries and might be vary with Alveo board. For more information about these pragmas, refer to vitis `HLS interface pragma <https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Interfaces>`_.

The steps to compile the C/C++ code into FPGA binaries is in the Makefile of each testcase. It generally has the following two steps:

(1) ``v++ --compile`` to compile the C/C++ code into RTL code. A .xo file is generated in this step.
(2) ``v++ --link`` to link the .xo file into FPGA binaries. A .xclbin file is generated in this step.

For more information about compiling the HLS code, refer to `here <https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Building-the-Device-Binary>`_

The code to make use of the FPGA binaries is usually C/C++ code with OpenCL APIs and typically contains the following steps:

(1) Create the entire platform and OpenCL kernels

.. code-block:: cpp

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    cl::Context context(device, NULL, NULL, NULL, &fail);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &fail);
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins, NULL, &fail);
    cl::Kernel shortestPath;
    shortestPath = cl::Kernel(program, "shortestPath_top", &fail);

(2) Create CL::Buffers and decide which data needs to be transferred to FPGA devices and back to host machine.

.. code-block:: cpp

   std::vector<cl::Memory> ob_in;
   cl::Buffer offset_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(ap_uint<32>) * (numVertices + 1), &mext_o[0]);
   ob_in.push_back(offset_buf);

   std::vector<cl::Memory> ob_out;
   cl::Buffer result_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(float) * ((numVertices + 1023) / 1024) * 1024, &mext_o[6]);
   ob_out.push_back(result_buf);

(3) Set arguments for FPGA OpenCL kernels

.. code-block:: cpp

    shortestPath.setArg(j++, config_buf);
    shortestPath.setArg(j++, offset_buf);
    shortestPath.setArg(j++, column_buf);
    shortestPath.setArg(j++, weight_buf);
    shortestPath.setArg(j++, ddrQue_buf);
    shortestPath.setArg(j++, ddrQue_buf);
    shortestPath.setArg(j++, result_buf);
    shortestPath.setArg(j++, result_buf);
    shortestPath.setArg(j++, pred_buf);
    shortestPath.setArg(j++, pred_buf);
    shortestPath.setArg(j++, info_buf);

(4) Set up event dependencies

.. code-block:: cpp

   std::vector<cl::Event> events_write(1);
   std::vector<cl::Event> events_kernel(1);
   std::vector<cl::Event> events_read(1);

   q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);  // Transfer Host data to Device
   q.enqueueTask(shortestPath, &events_write, &events_kernel[0]); // execution of the OpenCL kernels (FPGA binaries)
   q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]); // Transfer Device data to Host

(5) Run OpenCL tasks and execute FPGA binaries

.. code-block:: cpp

   q.finish()

.. _tutorial::l1:

L1 API
------------------------------------

Target Audience
^^^^^^^^^^^^^^^^^
Target audience of L1 API are users who are familiar with HLS programming and want to test / profile / modify operators or add new operator.
With the HLS test project provided in L1 layer, you could get:

(1) Function correctness test, both in C-simulation and Co-simulation
(2) Performance profiling from HLS synthesis report and Co-simulaiton
(3) Resource and timing evaluation from AMD Vivado |trade| synthesis.


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: