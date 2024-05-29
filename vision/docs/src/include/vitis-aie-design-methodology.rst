.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Vitis AIE Design Methodology
##############################

Following are critical components in making a kernel work on a platform using the AMD Vitis™ software platform:

#. Prepare the Kernels
#. Data Flow Graph construction
#. Setting up platform ports
#. Host code integration
#. Makefile to compile the kernel for x86 simulation / aie simulation / hw-emulation / hw runs

Prepare the Kernels
====================

Kernels are computation functions that form the fundamental building blocks of the data flow graph specifications. Kernels are declared as ordinary C/C++ functions that return void and can use special data types as arguments (discussed in `Window and Streaming Data API`_). Each kernel should be defined in its own source file. This organization is recommended for reusable and faster compilation. Furthermore, the kernel source files should include all relevant header files to allow for independent compilation. It is recommended that a header file (``kernels.h`` in this documentation) should declare the function prototypes for all kernels used in a graph. An example is shown below.

.. code:: c

    #ifndef _KERNELS_16B_H_
    #define _KERNELS_16B_H_

    #include <adf/stream/types.h>
    #include <adf/window/types.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>

    #define PARALLEL_FACTOR_16b 16 // Parallelization factor for 16b operations (16x mults)
    #define SRS_SHIFT 10           // SRS shift used can be increased if input data likewise adjusted)

    void filter2D(adf::input_buffer<int16>& input, const int16_t (&coeff)[16], adf::output_buffer<int16>& output);

    #endif

:ref:`Vitis Vision AIE library functions <aie_library_functions>` packaged with Vitis Vision AIE library are pre optimized vector implementations for various computer vision tasks. These functions can be directly included in user kernel (as shown in example below).

.. code:: c

    #include "imgproc/xf_filter2d_16b_aieml.hpp"
    #include "kernels.h"

    void filter2D(adf::input_buffer<int16>& input, const int16_t (&coeff)[16], adf::output_buffer<int16>& output) {
            xf::cv::aie::filter2D(input, coeff, output);
    };

.. _Buffer and Streaming Data API: https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Buffer-vs.-Stream-in-Data-Communication

Data Flow Graph Construction
=================================

Once AIE kernels have been prepared, next step is to create a Data Flow Graph class which defines the top level ports, `Run time parameters`_, connectivity, constraints etc. This consists of the following steps:

#. Create ``graph.h`` and include Adaptive Data Flow (ADF) header file (``adf.h``). Also include header file with kernel function prototypes (``kernel.h``)

   .. code:: c

      #include <adf.h>
      #include "kernels.h"

#. Define your graph class by using the objects which are defined in the adf name space. All user graphs are derived from the class graph.

   .. code:: c

      include <adf.h>
      #include "kernels.h"
     
      using namespace adf;
     
      class myGraph : public graph {
      private:
          kernel k1;
      };

#. Add top level ports to graph. These ports will be responsible to data transfers to / from the kernels.

   .. code:: c

      #include <adf.h>
      #include "kernels.h"
   
      using namespace adf;
   
      class simpleGraph : public graph {
      private:
          kernel k1;
   
      public:
          port<input> inptr;
          port<output> outptr;
          port<input> kernelCoefficients;
      };


#. Specify connections of top level ports to kernels. Primary connections type are `Window`_, `Stream`_, `Run time parameters`_. Below is example code specifying connectivity.

   .. code:: c

      class myGraph : public adf::graph {
      private:
          kernel k1;
      public:
          port<input> inptr;
          port<output> outptr;
          port<input> kernelCoefficients;
   
          myGraph() {
              k1 = kernel::create(filter2D);
              adf::connect(inptr, k1.in[0]);
              adf::connect<parameter>(kernelCoefficients, async(k1.in[1]));
              adf::connect(k1.out[0], outptr);
          }
      };

#. Specify the source file location and other constraints for each kernel.

   .. code:: c

      class myGraph : public adf::graph {
      private:
          kernel k1;
      public:
          port<input> inptr;
          port<output> outptr;
          port<input> kernelCoefficients;
     
          myGraph() {
              k1 = kernel::create(filter2D);
              adf::connect(inptr, k1.in[0]);
              adf::connect<parameter>(kernelCoefficients, async(k1.in[1]));
              adf::connect(k1.out[0], outptr);
              source(k1) = "xf_filter2d.cc";
              // Initial mapping
              runtime<ratio>(k1) = 0.5;
          }
      };

.. _Run time parameters: https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Run-Time-Graph-Control-API
.. _Stream: https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Buffer-vs.-Stream-in-Data-Communication


Setting up Platform Ports
=============================

The next step is to create a ``graph.cpp`` file with platform ports and virtual platform specification. A virtual platform specification helps to connect the data flow graph written with external I/O mechanisms specific to the chosen target for testing or eventual deployment. 

There are two types of platform ports attributes which describe how data is transferred to / from AIE cores.

.. _plio_aie:

PLIO
------

A PLIO port attribute is used to make external stream connections that cross the AI Engine to programmable logic (PL) boundary. The following example shows how the PLIO attributes shown in the previous table can be used in a program to read input data from a file or write output data to a file. The PLIO width and frequency of the PLIO port are also provided in the PLIO constructor. For more details refer to `PLIO Attributes`_.

.. code:: c

   //Virtual platform ports
   inptr = input_plio::create("DataIn1", adf::plio_128_bits, "data/input_128x16.txt");
   outptr = output_plio::create("DataOut1", adf::plio_128_bits, "data/output.txt");

   //Virtual platform connectivity
   connect<>(inptr.out[0], k1.in[0]);
   connect<parameter>(kernelCoefficients, async(k1.in[1]));
   connect<>(k1.out[0], outptr.in[0]);


.. _PLIO Attributes: https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Configuring-input_plio/output_plio

.. _gmio_aie:

GMIO
--------

A GMIO port attribute is used to make external memory-mapped connections to or from the global memory. These connections are made between an AI Engine graph and the logical global memory ports of a hardware platform design. For more details please refer `GMIO Attributes`_.

.. code:: c

   GMIO gmioIn1("gmioIn1", 64, 1000);
   GMIO gmioOut("gmioOut", 64, 1000);

   connect<>(inptr.out[0], k1.in[0]);
   connect<parameter>(kernelCoefficients, async(k1.in[1]));
   connect<>(k1.out[0], outptr.in[0]);

.. _GMIO Attributes: https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Configuring-input_gmio/output_gmio

Host Code Integration
=========================

Depending on the functional verification model used, the top level application can be written using on of two ways.

x86Simulation / AIE Simulation
--------------------------------

In this mode the top level application can be written inside ``graph.cpp`` file. The application contains an instance of ADF graph and a main function within which API's are called to initialize, run, and end the graph. It may also have additional API's to update `Run time parameters`_. Additionally for hw emulation / hw run modes, the '``main()``' function can be guarded by a ``#ifdef`` to ensure graph is only initialized once, or run only once. The following example code is the simple application defined in `Creating a Data Flow Graph (Including Kernels)`_ with the additional guard macro __AIESIM__ and __X86SIM__.

.. code:: c

   // Virtual platform ports
   inptr = input_plio::create("DataIn1", adf::plio_128_bits, "data/input_128x16.txt");
   outptr = output_plio::create("DataOut1", adf::plio_128_bits, "data/output.txt");

   // Virtual platform connectivity
   connect<>(inptr.out[0], k1.in[0]);
   connect<parameter>(kernelCoefficients, async(k1.in[1]));
   connect<>(k1.out[0], outptr.in[0]);

   #define SRS_SHIFT 10
   float kData[9] = {0.0625, 0.1250, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625};


   #if defined(__AIESIM__) || defined(__X86SIM__)
   int main(int argc, char** argv) {
       filter_graph.init();
       filter_graph.update(filter_graph.kernelCoefficients, float2fixed_coeff<10, 16>(kData).data(), 16);
       filter_graph.run(1);
       filter_graph.end();
       return 0;
   }
   #endif

In case GMIO based ports are used.

.. code:: c

   #if defined(__AIESIM__) || defined(__X86SIM__)
   int main(int argc, char** argv) {
       ...
       ...
       int16_t* inputData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);
       int16_t* outputData = (int16_t*)GMIO::malloc(BLOCK_SIZE_in_Bytes);

       //Prepare input data
       ...
       ...

       filter_graph.init();
       filter_graph.update(filter_graph.kernelCoefficients, float2fixed_coeff<10, 16>(kData).data(), 16);

       filter_graph.run(1);

       //GMIO Data transfer calls
       gmioIn[0].gm2aie_nb(inputData, BLOCK_SIZE_in_Bytes);
       gmioOut[0].aie2gm_nb(outputData, BLOCK_SIZE_in_Bytes);
       gmioOut[0].wait();

       printf("after grph wait\n");
       filter_graph.end();

       ...
   }
   #endif

.. _Creating a Data Flow Graph (Including Kernels): https://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Creating-a-Data-Flow-Graph-Including-Kernels

HW Emulation / HW Run
----------------------------

For x86Simulation, the AIE simulation and top level application had simple ADF API calls to initialize / run / end the graph. However, for actual AI Engine graph applications the host code must do much more than those simple tasks. The top-level PS application running on the Cortex®-A72 controls the graph and PL kernels: manage data inputs to the graph, handle data outputs from the graph, and control any PL kernels working with the graph. Sample code is illustrated below.


.. code:: c

   1.// Open device, load xclbin, and get uuid
       
   xF::deviceInit(xclBinName);

   2. Allocate output buffer objects and map to host memory

   void* srcData = nullptr;
   xrt::bo dst_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
   srcData = src_hndl.map();
   memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));

   3. Get kernel and run handles, set arguments for kernel, and launch kernel.
   xrt::kernel s2mm_khdl = xrt::kernel(xF::gpDhdl, xF::xclbin_uuid, "s2mm"); // Open kernel handle
   xrt::run s2mm_rhdl = s2mm_khdl(out_bohdl, nullptr, OUTPUT_SIZE); // set kernel arg

   // Update graph parameters (RTP) and so on
   auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "filter_graph");
   gHndl.reset();
   gHndl.update("filter_graph.k1.in[1]", float2fixed_coeff<10, 16>(kData));
   gHndl.run(1);
   gHndl.wait();

   4. Wait for kernel completion.
   s2mm_rhdl.wait();

   5. Sync output device buffer objects to host memory.

   dst_hndl.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

   //6. post-processing on host memory - "host_out

:ref:`Vitis Vision AIE library functions <aie_library_functions>` provide optimal vector implementations of various computer vision algorithms. These functions are expected to process high resolution images. However because local memory in AIE core modules is limited, entire images can't be fit into it. Also accessing DDR for reading / writing image data will be highly inefficient both for performance and power. To overcome this limitation, host code is expected to split the high resolution image into smaller tiles which fit in the AIE Engine local memory in ping-pong fashion. Splitting high resolution images into smaller tiles is a complex operation as it need to be aware of overlap regions and borders. Also the tile size is expected to be aligned with vectorization factor of the kernel.

To facilitate this the Vitis Vision Library provides data movers which perform smart tiling / stitching of high resolution images which can meet all the above requirements. There are two versions made available which can provide data movement capabilities both using PLIO and GMIO interfaces. A high-level class abstraction is provided with a simple API interface to facilitate data transfers. The class abstraction allows for seamless transition between the PLIO - GMIO methods of data transfers.

.. _xfcvdatamovers_aie:

xfcvDataMovers
~~~~~~~~~~~~~~

The xfcvDataMovers class provides a high level API abstraction to initiate data transfer from the DDR to the AIE core and vice versa for hw-emulation / hw runs. Because each AIE core has limited local memory which is not sufficient to fit an entire high resolution image (input / output), each image needs to be partitioned into smaller tiles and then send to AIE core for computation. After computation the tiled image at output is stitched back to generate the high resolution image at the output. This process involves complex computation as tiling needs to ensure proper border handling and overlap processing in case of
convolution based kernels.

The xfcvDataMovers class object takes some simple, user provided parameters and provides a simple data transaction API where you do not have to consider the complexity. Moreover it provides a template parameter, using which, the application can switch from PL-based data movement to GMIO-based (and vice versa) seamlessly.

.. csv-table:: Table. xfcvDataMovers Template Parameters
   :file: tables/xfcvDataMoversTemplate.csv
   :widths: 20, 50

.. csv-table:: Table. xfcvDataMovers constructor parameters
   :file: tables/xfcvDataMoversCtor.csv
   :widths: 20, 50

.. note::
   Horizontal overlap and Vertical overlaps should be computed for the complete pipeline. For example if the pipeline has a single 3x3 2D filter then overlap sizes (both horizontal and vertical) will be 1. However in the case of two such filter operations which are back to back, the overlap size will be 2. Currently it is expected that users provide this input correctly.

The data transfer using the xfcvDataMovers class can be done in one of two ways:

#. PLIO data movers

   This is the default mode for xfcvDataMovers class operation. When this method is used, data is transferred using hardware Tiler / Stitcher IPs provided by AMD. The :ref:`Makefile <aie_makefile>` provided with design examples shipped with the library provide the locations of the .xo files for these IPs. It also shows how to incorporate them in the Vitis Build System. You need to create an object of xfcvDataMovers class per input / output image as shown in following code.

   .. Important::
      **The implementations of Tiler and Stitcher for PLIO are provided as .xo files in the 'L1/lib/hw' folder. By using these files, you are agreeing to the terms and conditions specified in the LICENSE.txt file available in the same directory.**

   .. code:: c

      int overlapH = 1;
      int overlapV = 1;
      xF::xfcvDataMovers<xF::TILER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR> tiler(overlapH, overlapV);
      xF::xfcvDataMovers<xF::STITCHER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;

   The choice of MAX_TILE_HEIGHT / MAX_TILE_WIDTH provides constraints on the image tile size which in turn governs local memory usage. The image tile size in bytes can be computed as follows.

   Image tile size = (TILE_HEADER_SIZE_IN_BYTES + MAX_TILE_HEIGHT*MAX_TILE_WIDTH*sizeof(DATA_TYPE))

   Here TILE_HEADER_SIZE_IN_BYTES is 128 bytes for the current version of Tiler / Stitcher. DATA_TYPE in above example is int16_t (2 bytes).o

   .. note::
      The current version of HW data movers have 8_16 configuration (i.e., an 8-bit image element data type on the host side and a 16-bit image element data type on the AIE kernel side). In future more such configurations will be provided (example: 8_8 / 16_16 etc.).

   Tiler / Stitcher IPs use PL resources available on VCK boards. For 8_16 configuration, the following table illustrates resource utilization numbers for these IPs. The numbers correspond to a single instance of each IP.

   .. table:: Table: Tiler / Stitcher Resource Utilization (8_16 config)
      :widths: 10,15,15,15,15,15

      +----------------+--------+-------+-------+--------+---------+
      |                |  LUTs  |  FFs  | BRAMs |  DSPs  |   Fmax  |
      +================+========+=======+=======+========+=========+
      | **Tiler**      |  2761  |  3832 |   5   |   13   | 400 MHz |
      +----------------+--------+-------+-------+--------+---------+
      | **Stitcher**   |  2934  |  3988 |   5   |   7    | 400 MHz |
      +----------------+--------+-------+-------+--------+---------+
      | **Total**      |  5695  |  7820 |   10  |   20   |         |
      +----------------+--------+-------+-------+--------+---------+

#. GMIO data movers

   Transition to GMIO-based data movers can be achieved by using a specialized template implementation of the above class. All above constraints with regard to the image tile size calculation are valid here as well. Sample code is shown below.

   .. code:: c

      xF::xfcvDataMovers<xF::TILER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> tiler(1, 1);
      xF::xfcvDataMovers<xF::STITCHER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR, 1, 0, true> stitcher;

   .. note::
      The last template parameter is set  to true, implying GMIO specialization.

Once the objects are constructed, simple API calls can be made to initiate the data transfers. Sample code is shown below.

.. code:: c

   //For PLIO
   auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size());
   stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

   //For GMIO
   auto tiles_sz = tiler.host2aie_nb(srcData.data(), srcImageR.size(), {"gmioIn[0]"});
   stitcher.aie2host_nb(dstData.data(), dst.size(), tiles_sz, {"gmioOut[0]"});

.. note::
   GMIO data transfers take an additional argument which is the corresponding GMIO port to be used.

.. note::
   For GMIO-based transfers, there is a blocking method as well (host2aie(...) / aie2host(...)). For PLIO-based data transfers only non-blocking API calls are provided.

Using ``tile_sz``, you can run the graph the appropriate number of times.

.. code:: c

   filter_graph_hndl.run(tiles_sz[0] * tiles_sz[1]);

After the runs are started, you need to wait for all transactions to complete.

.. code:: c

   filter_graph_hndl.wait();
   tiler.wait();
   stitcher.wait();
