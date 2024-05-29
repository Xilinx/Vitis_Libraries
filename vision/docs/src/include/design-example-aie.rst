.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Design example Using Vitis Vision AIE Library
###############################################

The following example application performs a 2D filtering operation over a gray scale image. The
convolution kernel is a 3x3 window with floating point representation. The coefficients are
converted to fixed point representation before being passed to the AI Engine core for computation. The
results are cross validated against the OpenCV reference implementation. The example illustrates
both PLIO- and GMIO-based data transfers.

ADF Graph
=========

An AI Engine program consists of a data flow graph specification written in C++. The dataflow graph consists of top-level ports, 
kernel instances, and connectivity. a ``graph.h`` file is created which includes the header ``adf.h``.

For more details on data flow graph creation, refer to `AI Engine Programming`_ .

.. _AI Engine Programming: https://docs.xilinx.com/r/en-US/ug1076-ai-engine-environment/Creating-a-Data-Flow-Graph-Including-Kernels 

.. code:: c

   #include "kernels.h"
   #include <adf.h>

   using namespace adf;

   class myGraph : public adf::graph {
      public:
       kernel k1;
       port<input> inptr;
       port<output> outptr;
       port<input> kernelCoefficients;

     myGraph() {
        k1 = kernel::create(filter2D);
	inptr = input_plio::create("DataIn1", adf::plio_128_bits, "data/input_128x16.txt");
        outptr = output_plio::create("DataOut1", adf::plio_128_bits, "data/output.txt");
	
        adf::connect<>(inptr.out[0], k1.in[0]);
        adf::connect<parameter>(kernelCoefficients, async(k1.in[1]));
        adf::connect<>(k1.out[0], outptr.in[0]);
	
	adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        source(k1) = "xf_filter2d.cc";
        // Initial mapping
        runtime<ratio>(k1) = 0.5;
     };
   }; 

Platform Ports
==============

A top-level application file ``graph.cpp`` is created which contains an instance of the graph class and is connected to a simulation platform. A virtual platform specification helps to connect the data flow graph written with external I/O
mechanisms specific to the chosen target for testing or eventual deployment.

.. code:: c

  #include "graph.h"

  // Graph object
     myGraph filter_graph;

     filter_graph.init();
     filter_graph.update(filter_graph.kernelCoefficients, float2fixed_coeff<10, 16>(kData).data(), 16);
     filter_graph.run(1);
     filter_graph.end();
    
#. PLIO

   A PLIO port attribute is used to make external stream connections that cross the AI Engine to programmable logic (PL) boundary. PLIO attributes are used to specify the port name, port bit width and the input/output file names.
   Note that when simulating PLIO with data files, the data should be organized to accommodate both the width of the PL block as well as the data type of connecting port on the AI Engine block.

   .. code:: c

     //Platform ports
	inptr = input_plio::create("DataIn1", adf::plio_128_bits, "data/input_128x16.txt");
        outptr = output_plio::create("DataOut1", adf::plio_128_bits, "data/output.txt");


#. GMIO

   A GMIO port attribute is used to make external memory-mapped connections to or from the global memory. These connections are made between an AI Engine graph and the logical global memory ports of a hardware platform design.

   .. code:: c

      //Platform ports
      in1 = input_gmio::create("IN", 256,1000);
      out1 = output_gmio::create("OUT", 256, 1000);
   

Host code
=========

Host code ``host.cpp`` will be running on the host processor, which contains the code to initialize and run the datamovers and the ADF graph. XRT APIs are
used to create the required buffers in the device memory. 

First a golden reference image is generated using OpenCV.

.. code:: c

    int run_opencv_ref(cv::Mat& srcImageR, cv::Mat& dstRefImage, float coeff[9]) {
    cv::Mat tmpImage;
    cv::Mat kernel = cv::Mat(3, 3, CV_32F, coeff);
    cv::filter2D(srcImageR, dstRefImage, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_REPLICATE);
    return 0;
    }

Then, ``xclbin`` is loaded on the device and the device handles are created.

.. code:: c

   xF::deviceInit(xclBinName);

Buffers for input and output data are created using the XRT APIs and data from input ``CV::Mat`` is copied to the XRT buffer.

.. code:: c

        void* srcData = nullptr;
        xrt::bo src_hndl = xrt::bo(xF::gpDhdl, (srcImageR.total() * srcImageR.elemSize()), 0, 0);
        srcData = src_hndl.map();
        memcpy(srcData, srcImageR.data, (srcImageR.total() * srcImageR.elemSize()));
	
        // Allocate output buffer
        void* dstData = nullptr;
        xrt::bo ptr_dstHndl = xrt::bo(xF::gpDhdl, (op_height * op_width * srcImageR.elemSize()), 0, 0);
        dstData = ptr_dstHndl.map();
        cv::Mat dst(op_height, op_width, srcImageR.type(), dstData);

``xfcvDataMovers`` objects tiler and stitcher are created. For more details on ``xfcvDataMovers`` refer to :ref:`xfcvDataMovers <xfcvdatamovers_aie>`

.. code:: c

        xF::xfcvDataMovers<xF::TILER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR> tiler(1, 1);
        xF::xfcvDataMovers<xF::STITCHER, int16_t, MAX_TILE_HEIGHT, MAX_TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;

ADF graph is initialized and the filter coefficients are updated.        

.. code:: c

   	auto gHndl = xrt::graph(xF::gpDhdl, xF::xclbin_uuid, "filter_graph");
        gHndl.reset();
        gHndl.update("filter_graph.k1.in[1]", float2fixed_coeff<10, 16>(kData));

Metadata containing the tile information is generated.

.. code:: c

   tiler.compute_metadata(srcImageR.size());

The data transfer to AIE via datamovers is initiated along with graph run. Further execution waits until the data transfer is complete.

.. code:: c

    auto tiles_sz = tiler.host2aie_nb(&src_hndl, srcImageR.size());
    stitcher.aie2host_nb(&dst_hndl, dst.size(), tiles_sz);

    gHndl.run(tiles_sz[0] * tiles_sz[1]);
    gHndl.wait();

    tiler.wait();
    stitcher.wait();

            


.. _aie_makefile:

Makefile
========

Run 'make help' to get the list of supported commands and flows. Running the following commands will initiate a hardware build.

.. code:: c

	source < path-to-Vitis-installation-directory >/settings64.sh
	export SYSROOT=< path-to-platform-sysroot >
	export PLATFORM=< path-to-platform-directory >/< platform >.xpfm
	make all TARGET=hw 
	
	.. include:: include/f2d-l3-pipeline.rst
