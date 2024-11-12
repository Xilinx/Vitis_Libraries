..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _L1_2DFFT_LABEL:

.. toctree::
   :caption: Table of Contents
   :maxdepth: 3

..    2-Dimensional(Matrix) SSR FFT L1 FPGA Module <L1_2dfft.rst>

.. _L1_2DFFT_OVERVIEW:

============================================
2-Dimensional(Matrix) SSR FFT L1 FPGA Module
============================================

Overview
========

The AMD Vitis |trade| DSP library provides a fully synthesizable 2-Dimensional fast Fourier transform (FFT) as a L1 primitive. This L1 primitive is designed to be easily transformed into an L2 kernel by adding memory adapters. The L1 primitive is designed to have an array of stream interfaces, as wide as the device double-data rate (DDR) memory widths on boards like the AMD Alveo |trade| U200, U250, and U280. Adding memory adapters requires a plugin at the SSR FFT input side which has an AXI interface for connection with the DDR memory on one side, and the other sides need to have a memory wide streaming interface to connect with the 2-D SSR FFT L1 primitive. A second memory plugin is required at the output side of the SSR FFT, which reads in an array of stream data and connects it to the output AXI interface for the DDR memory connection.

Block Level Interface
=====================

The following figure shows the block level interface for the 2-D SSR FFT. Essentially, it is an array of stream interfaces at the input and the output.

.. image:: /images/2-2d_fft_if.jpg
    :alt: doc tool flow
    :width: 30%
    :align: center

2-D SSR FFT Architecture
========================

The 2-Dimensional SSR FFT is built on top of the 1-D SSR FFT. It deploys multiple 1-D SSR FFT processors in parallel to accelerate calculations. Potentially, the 2-D SSR FFT can be accelerated by deploying multiple processors in parallel, to process multiple lines (rows/columns) of input data whose 1-D SSR FFT calculation is independent of each other (Data parallelism). This essentially decreases the latency and also increases the throughput, but the 2-D SSR FFT throughput can also be increased by using a task level pipeline where one set of 1-D SSR FFT processors, also called line processors, work row wise on 2-D input data and another set of line processors work column wise in a task level pipeline on data produced by the row processors as shown in the following figure. The row processors perform 1-D SSR FFT row by row, and the column processors
perform transforms on columns. Row processors connect to column processors through a matrix transposer. The following figure shows a simplified block diagram which gives the 2-D SSR FFT architecture used by the Vitis SSR FFT Library. Essentially, it is a big dataflow pipeline of blocks which perform data permutations and 1-D FFT transforms working either row wise or column wise. Each of the 1-D SSR FFT processors (:ref:`1-D Vitis SSR FFT <L1_FFT_LABEL>`) is a Super Sample Rate streaming processor. The 1-D SSR FFT processors, used along the rows and columns, are supposed to have same configuration which is specified as described in :ref:`Configuration Parameter Structure for Floating Point SSR FFT <FLOAT_FFT_PARAMS_STRUCT_LABEL>`.

.. image:: /images/3-2d_fft_blk_dia.jpg
    :alt: doc tool flow
    :width: 30%
    :align: center

Supported Data Types
====================

2-D SSR FFT currently supports the **complex<float>** and **complex<ap_fixed<>>** type for simulation and synthesis. Additionally, the Vitis SSR FFT library currently does not support standard std::complex<float>. Instead, a complex_wrapper class is shipped with the Vitis SSR FFT library that can be used for simulation and synthesis.

+-----------------------------+-------------------------+--------------------------+
|                             |                         |                          |
| Type                        | Supported for Synthesis | Supported for Simulation |
|                             |                         |                          |
+=============================+=========================+==========================+
| complex_wrapper<float>      | YES                     | YES                      |
+-----------------------------+-------------------------+--------------------------+
| complex_wrapper<ap_fixed<>> | YES                     | YES                      |
+-----------------------------+-------------------------+--------------------------+

.. _2D_FFT_TEMPLATE_PARAMS_LABEL:

L1 API for 2-D SSR FFT
======================

2-D SSR FFT is provided as a template function as follows:

.. code-block:: cpp

   template <unsigned int t_memWidth,
           unsigned int t_numRows,
           unsigned int t_numCols,
           unsigned int t_numKernels,
           typename t_ssrFFTParamsRowProc,
           typename t_ssrFFTParamsColProc,
           unsigned int t_rowInstanceIDOffset,
           unsigned int t_colInstanceIDOffset,
           typename T_elemType // complex element type
           >
   void fft2d(hls::stream<WideInputType> , hls::stream<WideOutputType>);

Template Parameters
-------------------

Function: fft2d is the top-level L1 API which takes a number of template parameters and in/out streams. These template functions describe the architecture and the data path of 1-D SSR FFT processors deployed as line processors. The description of these parameters is as follows:

+-----------------------+----------------------------------------------------------------------+
|                       |                                                                      |
|  Template Parameter   |                          Functionality                               |
+=======================+======================================================================+
|       t_memWidth      | Describes the width of the wide stream in complex<T_elemType> words  |
+-----------------------+----------------------------------------------------------------------+
|       t_numRows       | Number of rows in the 2-D FFT input data (currently limited to 256)  |
+-----------------------+----------------------------------------------------------------------+
|       t_numCols       | Number of column in the 2-D FFT input data (currently limited to 256)|
+-----------------------+----------------------------------------------------------------------+
|      t_numKernels     | Number of parallel kernels used to process the rows/columns          |
+-----------------------+----------------------------------------------------------------------+
| t_ssrFFTParamsRowProc | Configuration parameter structure for the row processors             |
+-----------------------+----------------------------------------------------------------------+
| t_ssrFFTParamsColProc | Configuration parameter structure for the column processors          |
+-----------------------+----------------------------------------------------------------------+
| t_rowInstanceIDOffset | Instance ID offset for the row processors                            |
+-----------------------+----------------------------------------------------------------------+
| t_colInstanceIDOffset | Instance ID offset for the column processors                         |
+-----------------------+----------------------------------------------------------------------+
|       T_elemType      | The inner type for the complex wrapper                               |
+-----------------------+----------------------------------------------------------------------+

* t_memWidth: Gives the width of a wide stream in complex<inner_type> words can be calculated as: t_memWidth=(wide stream size in bits)/(sizeof(complex<T_elemType>)*8).

* t_numRows: 2-D SSR FFT processes a matrix the number of rows in the input matrix given by t_numRows.

* t_numCols: 2-D SSR FFT processes a matrix the number of columns in the input matrix given by t_numCols.

* t_numKernels: 2-D SSR FFT can deploy multiple 1-D SSR FFT processors along the rows and columns to processes numerous row/columns in parallel. This parameter gives the number of processors to be used along the rows/columns. The number of columns used along the rows and columns are the same and equal t_numKernels.

* t_ssrFFTParamsRowProc: Structure of parameters that describes the transform direction, data path, etc. for row processors. The details of the configuration parameter structures can be found in :ref:`Configuration Parameter Structure for Floating Point SSR FFT <FLOAT_FFT_PARAMS_STRUCT_LABEL>`. For the fixed point case (currently not supported), it is :ref:`Configuration Parameter Structure for Fixed Point SSR FFT <FIXED_FFT_PARAMS_STRUCT_LABEL>`.

* t_ssrFFTParamsColProc: Structure of parameters that describes the transform direction, data path, etc. for column processors. The details of the configuration parameter structures can be found in :ref:`Configuration Parameter Structure for Floating Point SSR FFT <FLOAT_FFT_PARAMS_STRUCT_LABEL>`. For the fixed point case (currently not supported), it is :ref:`Configuration Parameter Structure for Fixed Point SSR FFT <FIXED_FFT_PARAMS_STRUCT_LABEL>`.

* t_rowInstanceIDOffset: 2-D SSR FFT deploys multiple 1D SSR FFT processors, and it is required that every 1D SSR FFT processor has a unique ID. If 'M' processors are used along rows, then their IDs will be in the range: (t_rowInstanceIDOffset+1 , t_rowInstanceIDOffset+M). The selection of the offset for rows and columns should be made so these ranges are unique without any overlap. Essentially, it requires that **abs(t_rowInstanceIDOffset - t_colInstanceIDOffset) > M**.

* t_colInstanceIDOffset: 2-D SSR FFT deploys multiple 1D SSR FFT processors, and it is required that every 1D SSR FFT processor has a unique ID. If 'M' processors are used along rows, then their IDs will be in the range: (t_rowInstanceIDOffset+1 , t_rowInstanceIDOffset+M). The selection of the offset for rows and columns should be made so these ranges are unique without any overlap. Essentially, it requires that **abs(t_rowInstanceIDOffset - t_colInstanceIDOffset) > M**.

* T_elemType: 2-D SSR FFT processes complex samples and T_elemType gives the inner type used to store real and imaginary parts. Currently, it supports only float as the inner type for simulation and synthesis.

Constraints on the Choice of Template Parameters
------------------------------------------------

Currently, the template parameters for 2-D SSR FFT should follow the following constraints:

1. The radix ``R`` specified for both the row and column processors in ``t_ssrFFTParamsRowProc`` and ``t_ssrFFTParamsColProc`` should be the same.

2. The possible radix values are 2, 4, 8, and 16.

3. The input data should be a square matrix, i.e. ``t_numRows==t_numCols``.

4. t_rowInstanceIDOffset and t_colInstanceIDOffset should be different and ``abs(t_rowInstanceIDOffset - t_colInstanceIDOffset) > t_numKernels``.

5. The selection of the radix ``R``, ``t_numKernels``, and ``t_memWidth`` should satisfy: ``R*t_numKernels==t_memWidth``. This constraint essentially highlights the fact that number of kernels used should be enough not more nor less to exhaust the input bandwidth.
6. ``t_memWidth`` should be multiple of sizeof(c.omplex<float>).

Library Usage
=============

This section will provide information about how to use 2-D SSR FFT in your project.

Fixed Point 2-D SSR FFT L1 Module Usage
---------------------------------------

To use the fixed point Vitis 2-D SSR FFT L1 module in a C++ HLS design:

1. Clone the Vitis DSP Library git repository, and add the following path to compiler include path:

         ``REPO_PATH/dsp/L1/include/hw/vitis_2dfft/fixed/``

2. Include ``vt_fft.hpp``.

3. Use namespace ``xf::dsp::fft``.

4. Define the parameter structures for 1-D SSR FFT processors used along rows and columns. For example, call them ``params_row`` and ``parms_column`` by extending ``ssr_fft_default_params`` like :ref:`Defining 1-D SSR FFT Parameter Structure <FIXED_FFT_PARAMS_STRUCT_LABEL>`.

5. Call the ``fft2d<8, 16, 16, 2, params_row, params_column, 0,3, std::complex<ap_fixed<...>> >(p_inStream, p_outStream);`` description for the template parameters found in :ref:`2-D SSR FFT Template Parameters<2D_FFT_TEMPLATE_PARAMS_LABEL>`.

The following section gives usage examples and explains some other interface level details for use in a C++ based HLS design. To use the 2-D SSR FFT L1 library:

1. Include the ``vt_fft.hpp`` header:

.. code-block:: cpp

  #include "vt_fft.hpp"

2. Use namespace ``xf::dsp::fft``:

.. code-block:: cpp

   using namespace xf::dsp::fft;

3. Define two C++ structure that extends ssr_fft_default_params, one for the row and one for the column processors:

.. code-block:: cpp

   struct params_row:ssr_fft_default_params
   {
		static const int N = 16;
		static const int R = 4;
		static const scaling_mode_enum scaling_mode = SSR_FFT_NO_SCALING;
		static const fft_output_order_enum output_data_order = SSR_FFT_NATURAL;
		static const int twiddle_table_word_length = 18;
		static const int twiddle_table_integer_part_length = 1;
		static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
		static const butterfly_rnd_mode_enum butterfly_rnd_mode = TRN;
   };

   struct params_column:ssr_fft_default_params
   {
		static const int N = 16;
		static const int R = 4;
		static const scaling_mode_enum scaling_mode = SSR_FFT_NO_SCALING;
		static const fft_output_order_enum output_data_order = SSR_FFT_NATURAL;
		static const int twiddle_table_word_length = 18;
		static const int twiddle_table_integer_part_length = 2;
		static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
		static const butterfly_rnd_mode_enum butterfly_rnd_mode = TRN;
   };

4. Call 2-D FFT L1 module as follows:

.. code-block:: cpp


      fft2d<
            8,
            16,
            16,
            2,
            params_row,
            params_col,
            0,
            3,
            std::complex<ap_fixed<...>>
           >(p_inStream, p_outStream);

Floating Point 2-D SSR FFT L1 Module Usage
------------------------------------------

To use the Vitis 2-D SSR FFT L1 module in a C++ HLS design:

1. Clone the Vitis DSP Library git repository, and add the following path to the compiler include path:

         ``REPO_PATH/dsp/L1/include/hw/vitis_2dfft/float/``

2. Include ``vt_fft.hpp``.

3. Use namespace ``xf::dsp::fft``.

4. Define the parameter structures for 1-D SSR FFT processors used along rows and columns. For example, call them ``params_row`` and ``parms_column`` by extending ``ssr_fft_default_params`` like :ref:`Defining 1-D SSR FFT Parameter Structure <FLOAT_FFT_PARAMS_STRUCT_LABEL>`.

5. Call the ``fft2d<8, 16, 16, 2, params_row, params_column, 0,3, complex_wrapper<float> >(p_inStream, p_outStream);`` description for the template parameters found in :ref:`2-D SSR FFT Template Parameters<2D_FFT_TEMPLATE_PARAMS_LABEL>`.

The following section provides usage examples and explains some other interface level details for use in a C++ based HLS design. To use the 2-D SSR FFT L1 library:

1. Include the ``vt_fft.hpp`` header:

.. code-block:: cpp

  #include "vt_fft.hpp"

2. Use namespace ``xf::dsp::fft``:

.. code-block:: cpp

   using namespace xf::dsp::fft;

3. Define the two C++ structures that extends ssr_fft_default_params, one for the row and one for the column processors:

.. code-block:: cpp

   struct params_row:ssr_fft_default_params
   {
      static const int N = 16;
      static const int R = 4;
      static const fft_output_order_enum output_data_order = SSR_FFT_NATURAL;
      static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
   };

   struct params_column:ssr_fft_default_params
   {
      static const int N = 16;
      static const int R = 4;
      static const fft_output_order_enum output_data_order = SSR_FFT_NATURAL;
      static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
   };

4. Call the 2-D SSR FFT L1 module as follows:

.. code-block:: cpp


      fft2d<
            8,
            16,
            16,
            2,
            params_row,
            params_col,
            0,
            3,
            complex_wrapper<float>
           >(p_inStream, p_outStream);

2-D SSR FFT Examples
====================

The following section gives examples for 2-D floating and fixed point SSR FFTs.

2-D Fixed Point Example
-----------------------

The following section briefly describes how an example project using a Vitis SSR FFT can be built that uses a 2-D fixed point SSR FFT. The source code files are listed which can be used to build HLS project by adding an include path that points to a local copy of the Vitis FFT library.

.. _2D_FIXED_TOP_LEVEL_DEFINITION:

Top Level Definition and main Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following section lists a complete example test that uses the 2-D fixed point SSR FFT L1 module. The header file named ``top_2d_fft_test.hpp`` listed below provides definitions of most of the template parameters and data types used in test and also the declaration of the top level function, ``top_fft2d``, that will be synthesized.

.. code-block:: cpp

	#ifndef _TOP_2D_FFT_TEST_H_
	#define _TOP_2D_FFT_TEST_H_
	#ifndef __SYNTHESIS__
	#include <iostream>
	#endif
	#include "vt_fft.hpp"
	#ifndef __SYNTHESIS__
	#include <iostream>
	#endif
	using namespace xf::dsp::fft;
	typedef ap_fixed<27, 8> T_innerData;
	typedef std::complex<T_innerData> T_elemType;
	const int k_memWidthBits = 512;
	const int k_memWidth = k_memWidthBits / (sizeof(std::complex<T_innerData>) * 8);
	const int k_fftKernelRadix = 4;
	const int k_numOfKernels = k_memWidth / (k_fftKernelRadix);
	const int k_fftKernelSize = 16;
	typedef float T_innerFloat;
	typedef std::complex<T_innerFloat> T_compleFloat;
	const int k_numRows = k_fftKernelSize;
	const int k_numCols = k_fftKernelSize;
	const int k_rowInstanceIDOffset = 40000;
	const int k_colInstanceIDOffset = 80000;
	const int k_totalWideSamples = k_fftKernelSize * k_fftKernelSize / k_memWidth;
	struct FFTParams : ssr_fft_default_params {
	    static const int N = k_fftKernelSize;
	    static const int R = k_fftKernelRadix;
	    static const scaling_mode_enum scaling_mode = SSR_FFT_NO_SCALING;

	    static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
	};
	struct FFTParams2 : ssr_fft_default_params {
	    static const int N = k_fftKernelSize;
	    static const int R = k_fftKernelRadix;
	    static const scaling_mode_enum scaling_mode = SSR_FFT_NO_SCALING;

	    static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
	};

	typedef FFTIOTypes<FFTParams, T_elemType>::T_outType T_outType_row;
	typedef FFTIOTypes<FFTParams2, T_outType_row>::T_outType T_outType;

	typedef WideTypeDefs<k_memWidth, T_elemType>::WideIFType MemWideIFTypeIn;
	typedef WideTypeDefs<k_memWidth, T_elemType>::WideIFStreamType MemWideIFStreamTypeIn;

	typedef WideTypeDefs<k_memWidth, T_outType>::WideIFType MemWideIFTypeOut;
	typedef WideTypeDefs<k_memWidth, T_outType>::WideIFStreamType MemWideIFStreamTypeOut;

	void top_fft2d(MemWideIFStreamTypeIn& p_inStream, MemWideIFStreamTypeOut& p_outStream);
	#endif

The following ``.cpp`` file named ``top_2d_fft_test.cpp`` defines the top level function which essentially calls ``fft2d`` from the Vitis FFT library.

.. code-block:: cpp

	#include "top_2d_fft_test.hpp"
	void top_fft2d(MemWideIFStreamTypeIn& p_inStream, MemWideIFStreamTypeOut& p_outStream) {
	#ifdef _DEBUG_TYPES
	    T_outType_row T_outType_row_temp;
	    T_outType T_outType_temp;
	    MemWideIFTypeIn MemWideIFTypeIn_temp;
	    MemWideIFTypeOut MemWideIFTypeOut_temp;
	#endif

	#pragma HLS INLINE
	#pragma HLS DATA_PACK variable = p_inStream
	#pragma HLS DATA_PACK variable = p_outStream
	#pragma HLS interface ap_ctrl_none port = return

	#ifndef __SYNTHESIS__
	    std::cout << "================================================================================" << std::endl;
	    std::cout << "-------------------Calling 2D SSR FFT Kernel with Parameters--------------------" << std::endl;
	    std::cout << "================================================================================" << std::endl;
	    std::cout << "    The Main Memory Width (no. complex<ap_fixed>)   : " << k_memWidth << std::endl;
	    std::cout << "    The Size of 1D Row Kernel                    : " << FFTParams::N << std::endl;
	    std::cout << "    The SSR for 1D Row Kernel                    : " << FFTParams::R << std::endl;
	    std::cout << "    The Transform Direction for Row Kernel       : "
		      << ((FFTParams::transform_direction == FORWARD_TRANSFORM) ? "Forward" : "Reverse");
	    std::cout << std::endl;

	    std::cout << "    The Size of 1D Column Kernel                 : " << FFTParams2::N << std::endl;
	    std::cout << "    The SSR for 1D Row Kernel                    : " << FFTParams2::R << std::endl;
	    std::cout << "    The Transform Direction for Row Kernel       : "
		      << ((FFTParams2::transform_direction == FORWARD_TRANSFORM) ? "Forward" : "Reverse");
	    std::cout << std::endl;

	    std::cout << "    The Row Instance ID Offset                   : " << k_rowInstanceIDOffset << std::endl;
	    std::cout << "    The Column Instance ID Offset                : " << k_colInstanceIDOffset << std::endl;

	    std::cout << "    Number of 1D Kernels Used Row/Col wise       : " << k_numOfKernels << std::endl;
	    std::cout << "    The Total Number of 1D Kernels Used(row+col) : " << 2 * k_numOfKernels << std::endl;
	    std::cout << "================================================================================" << std::endl;

	#endif
	    fft2d<k_memWidth, k_fftKernelSize, k_fftKernelSize, k_numOfKernels, FFTParams, FFTParams2, k_rowInstanceIDOffset,
		  k_colInstanceIDOffset, T_elemType>(p_inStream, p_outStream);
	}

The ``main`` function is defined as follows which runs an impulse test:

.. code-block:: cpp

	#define TEST_2D_FFT_
	#ifdef TEST_2D_FFT_
	#ifndef __SYNTHESIS__
	#define _DEBUG_TYPES
	#endif
	#include <math.h>
	#include <string>
	#include <assert.h>
	#include <stdio.h>
	#include "top_2d_fft_test.hpp"
	#include "mVerificationUtlityFunctions.hpp"
	#include "vitis_fft/hls_ssr_fft_2d_modeling_utilities.hpp"
	#include "vt_fft.hpp"

	int main(int argc, char** argv) {
	    // 2d input matrix
	    T_elemType l_inMat[k_fftKernelSize][k_fftKernelSize];
	    T_outType l_outMat[k_fftKernelSize][k_fftKernelSize];
	    T_outType l_data2d_golden[k_fftKernelSize][k_fftKernelSize];

	    // init input matrix with real part only impulse
	    for (int r = 0; r < k_fftKernelSize; ++r) {
		for (int c = 0; c < k_fftKernelSize; ++c) {
		    if (r == 0 && c == 0)
		        l_inMat[r][c] = T_compleFloat(1, 0);
		    else
		        l_inMat[r][c] = T_compleFloat(0, 0);
		}
	    }
	    // Wide Stream for reading and streaming a 2-d matrix
	    MemWideIFStreamTypeIn l_matToStream("matrixToStreaming");
	    MemWideIFStreamTypeOut fftOutputStream("fftOutputStream");
	    // Pass same data stream multiple times to measure the II correctly
	    for (int runs = 0; runs < 5; ++runs) {
		stream2DMatrix<k_fftKernelSize, k_fftKernelSize, k_memWidth, T_elemType, MemWideIFTypeIn>(l_inMat,
		                                                                                          l_matToStream);
		top_fft2d(l_matToStream, fftOutputStream);

		printMatStream<k_fftKernelSize, k_fftKernelSize, k_memWidth, MemWideIFTypeOut>(
		    fftOutputStream, "2D SSR FFT Output Natural Order...");
		streamToMatrix<k_fftKernelSize, k_fftKernelSize, k_memWidth, T_outType>(fftOutputStream, l_outMat);
	    } // runs loop

	    T_outType golden_result = T_elemType(1, 0);
	    for (int r = 0; r < k_fftKernelSize; ++r) {
		for (int c = 0; c < k_fftKernelSize; ++c) {
		    if (golden_result != l_outMat[r][c]) return 1;
		}
	    }

	    std::cout << "================================================================" << std::endl;
	    std::cout << "---------------------Impulse test Passed Successfully." << std::endl;
	    std::cout << "================================================================" << std::endl;
	    return 0;
	}
	#endif


Compiling and Building the Example HLS Project
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before compiling and running the example, it is required to set up the path to the HLS compiler which can be done as follows: Change the setting of the environment variable **TA_PATH** to point to the installation path of your Vitis 2021.1 installation, and run the following command to set up the environment.

.. code-block:: bash

   export XILINX_VITIS=${TA_PATH}/Vitis/2021.1
   export XILINX_VIVADO=${TA_PATH}/Vivado/2021.1
   source ${XILINX_VIVADO}/settings64.sh

The example discussed above is also provided as an example test and is available at the following path: ``REPO_PATH/dsp/L1/examples/2Dfix_impulse``. It can be simulated, synthesized, or co-simulated as follows: Simply go to the directory, ``REPO_PATH/dsp/L1/examples/2Dfix_impulse``, and simulate, build, and cosimulate the project using: ``make run XPART='xcu200-fsgd2104-2-e' CSIM=1 CSYNTH=1 COSIM=1``. You can choose the part number as required and by settting CSIM/CSYNTH/COSIM=0, choose what to build and run with the make target.

2-D Floating Point Example
--------------------------

The following section briefly describes how an example project using Vitis SSR FFT can be built that uses 2-D SSR FFT. The source code files are listed which can be used to build HLS project by adding include path that points to a local copy of the Vitis FFT library.

.. _2D_FLOAT_TOP_LEVEL_DEFINITION:

Top Level Definition and main Function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following section lists a complete example test that uses the 2-D SSR FFT L1 module. The header file named ``top_2d_fft_test.hpp`` listed below provides definitions of most of the template parameters and data types used in test and also the declaration of top level function ``top_fft2d`` that will be synthesized.

.. code-block:: cpp

	 #ifndef _TOP_2D_FFT_TEST_H_
	#define _TOP_2D_FFT_TEST_H_
	#ifndef __SYNTHESIS__
	#include <iostream>
	#endif

	#include "vt_fft.hpp"
	#ifndef __SYNTHESIS__
	#include <iostream>
	#endif
	using namespace xf::dsp::fft;
	typedef float T_innerData;
	typedef complex_wrapper<T_innerData> T_elemType;
	const int k_memWidthBits = 512;
	const int k_memWidth = k_memWidthBits / (sizeof(complex_wrapper<T_innerData>) * 8);
	const int k_fftKernelRadix = 4;
	const int k_numOfKernels = k_memWidth / (k_fftKernelRadix);
	const int k_fftKernelSize = 16;
	typedef float T_innerFloat;
	typedef complex_wrapper<T_innerFloat> T_compleFloat;
	const int k_numRows = k_fftKernelSize;
	const int k_numCols = k_fftKernelSize;
	const int k_rowInstanceIDOffset = 40000;
	const int k_colInstanceIDOffset = 80000;
	const int k_totalWideSamples = k_fftKernelSize * k_fftKernelSize / k_memWidth;
	struct FFTParams : ssr_fft_default_params {
	    static const int N = k_fftKernelSize;
	    static const int R = k_fftKernelRadix;

	    static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
	};
	struct FFTParams2 : ssr_fft_default_params {
	    static const int N = k_fftKernelSize;
	    static const int R = k_fftKernelRadix;

	    static const transform_direction_enum transform_direction = FORWARD_TRANSFORM;
	};

	typedef FFTIOTypes<FFTParams, T_elemType>::T_outType T_outType_row;
	typedef FFTIOTypes<FFTParams2, T_outType_row>::T_outType T_outType;

	typedef WideTypeDefs<k_memWidth, T_elemType>::WideIFType MemWideIFTypeIn;
	typedef WideTypeDefs<k_memWidth, T_elemType>::WideIFStreamType MemWideIFStreamTypeIn;

	typedef WideTypeDefs<k_memWidth, T_outType>::WideIFType MemWideIFTypeOut;
	typedef WideTypeDefs<k_memWidth, T_outType>::WideIFStreamType MemWideIFStreamTypeOut;

	void top_fft2d(MemWideIFStreamTypeIn& p_inStream, MemWideIFStreamTypeOut& p_outStream);
	#endif

The following ``.cpp`` file named ``top_2d_fft_test.cpp`` defines the top level function which essentially calls ``fft2d`` from the Vitis FFT library.

.. code-block:: cpp

	#include "top_2d_fft_test.hpp"
	void top_fft2d(MemWideIFStreamTypeIn& p_inStream, MemWideIFStreamTypeOut& p_outStream) {
	#ifdef _DEBUG_TYPES
	    T_outType_row T_outType_row_temp;
	    T_outType T_outType_temp;
	    MemWideIFTypeIn MemWideIFTypeIn_temp;
	    MemWideIFTypeOut MemWideIFTypeOut_temp;
	#endif

	#pragma HLS INLINE
	#pragma HLS DATA_PACK variable = p_inStream
	#pragma HLS DATA_PACK variable = p_outStream
	#pragma HLS interface ap_ctrl_none port = return

	#ifndef __SYNTHESIS__
	    std::cout << "================================================================================" << std::endl;
	    std::cout << "-------------------Calling 2D SSR FFT Kernel with Parameters--------------------" << std::endl;
	    std::cout << "================================================================================" << std::endl;
	    std::cout << "    The Main Memory Width (no. complex<float>)   : " << k_memWidth << std::endl;
	    std::cout << "    The Size of 1D Row Kernel                    : " << FFTParams::N << std::endl;
	    std::cout << "    The SSR for 1D Row Kernel                    : " << FFTParams::R << std::endl;
	    std::cout << "    The Transform Direction for Row Kernel       : "
		      << ((FFTParams::transform_direction == FORWARD_TRANSFORM) ? "Forward" : "Reverse");
	    std::cout << std::endl;

	    std::cout << "    The Size of 1D Column Kernel                 : " << FFTParams2::N << std::endl;
	    std::cout << "    The SSR for 1D Row Kernel                    : " << FFTParams2::R << std::endl;
	    std::cout << "    The Transform Direction for Row Kernel       : "
		      << ((FFTParams2::transform_direction == FORWARD_TRANSFORM) ? "Forward" : "Reverse");
	    std::cout << std::endl;

	    std::cout << "    The Row Instance ID Offset                   : " << k_rowInstanceIDOffset << std::endl;
	    std::cout << "    The Column Instance ID Offset                : " << k_colInstanceIDOffset << std::endl;

	    std::cout << "    Number of 1D Kernels Used Row/Col wise       : " << k_numOfKernels << std::endl;
	    std::cout << "    The Total Number of 1D Kernels Used(row+col) : " << 2 * k_numOfKernels << std::endl;
	    std::cout << "================================================================================" << std::endl;

	#endif
	    fft2d<k_memWidth, k_fftKernelSize, k_fftKernelSize, k_numOfKernels, FFTParams, FFTParams2, k_rowInstanceIDOffset,
		  k_colInstanceIDOffset, T_elemType>(p_inStream, p_outStream);
	}

The ``main`` function is defined as follows which runs an impulse test:

.. code-block:: cpp

	#define TEST_2D_FFT_
	#ifdef TEST_2D_FFT_
	#ifndef __SYNTHESIS__
	#define _DEBUG_TYPES
	#endif
	#include <math.h>
	#include <string>
	#include <assert.h>
	#include <stdio.h>
	#include "top_2d_fft_test.hpp"
	#include "mVerificationUtlityFunctions.hpp"
	#include "vitis_fft/hls_ssr_fft_2d_modeling_utilities.hpp"
	#include "vt_fft.hpp"

	int main(int argc, char** argv) {
	    // 2d input matrix
	    T_elemType l_inMat[k_fftKernelSize][k_fftKernelSize];
	    T_outType l_outMat[k_fftKernelSize][k_fftKernelSize];
	    T_outType l_data2d_golden[k_fftKernelSize][k_fftKernelSize];

	    // init input matrix with real part only impulse
	    for (int r = 0; r < k_fftKernelSize; ++r) {
		for (int c = 0; c < k_fftKernelSize; ++c) {
		    if (r == 0 && c == 0)
		        l_inMat[r][c] = T_compleFloat(1, 0);
		    else
		        l_inMat[r][c] = T_compleFloat(0, 0);
		}
	    }
	    // Wide Stream for reading and streaming a 2-d matrix
	    MemWideIFStreamTypeIn l_matToStream("matrixToStreaming");
	    MemWideIFStreamTypeOut fftOutputStream("fftOutputStream");
	    // Pass same data stream multiple times to measure the II correctly
	    for (int runs = 0; runs < 5; ++runs) {
		stream2DMatrix<k_fftKernelSize, k_fftKernelSize, k_memWidth, T_elemType, MemWideIFTypeIn>(l_inMat,
		                                                                                          l_matToStream);
		top_fft2d(l_matToStream, fftOutputStream);

		printMatStream<k_fftKernelSize, k_fftKernelSize, k_memWidth, MemWideIFTypeOut>(
		    fftOutputStream, "2D SSR FFT Output Natural Order...");
		streamToMatrix<k_fftKernelSize, k_fftKernelSize, k_memWidth, T_outType>(fftOutputStream, l_outMat);
	    } // runs loop

	    T_outType golden_result = T_elemType(1, 0);
	    for (int r = 0; r < k_fftKernelSize; ++r) {
		for (int c = 0; c < k_fftKernelSize; ++c) {
		    if (golden_result != l_outMat[r][c]) return 1;
		}
	    }

	    std::cout << "================================================================" << std::endl;
	    std::cout << "---------------------Impulse test Passed Successfully." << std::endl;
	    std::cout << "================================================================" << std::endl;
	    return 0;
	}
	#endif

Compiling and Building Example HLS Project
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before compiling and running the example, it is required to set up the path to the HLS compiler which can be done as follows: Change the setting of the environment variable, **TA_PATH**, to point to the installation path of your Vitis 2021.1 installation, and run the following command to set up the environment.

.. code-block:: bash

   export XILINX_VITIS=${TA_PATH}/Vitis/2021.1
   export XILINX_VIVADO=${TA_PATH}/Vivado/2021.1
   source ${XILINX_VIVADO}/settings64.sh

The example discussed above is also provided as an example test and available at the following path: ``REPO_PATH/dsp/L1/examples/2Dfloat_impulse``. It can be simulated, synthesized, or cosimulated as follows: Simply go to the directory, ``REPO_PATH/dsp/L1/examples/2Dfloat_impulse``, and simulate, build, and co-simulate project using: ``make run XPART='xcu200-fsgd2104-2-e' CSIM=1 CSYNTH=1 COSIM=1``. You can choose the part number as required, and by settting CSIM/CSYNTH/COSIM=0, choose what to build and run with make target.

2-D SSR FFT Tests
-----------------

Different tests are provided for the fixed point and floating point 2-D SSR FFT. These tests can be ran individually using the makefile, or they can all be lauched at the same time by using a provided script. All the 2-D SSR FFT tests are in the ``REPO_PATH/dsp/L1/tests/hw/2dfft`` folder.

Launching an Individual Test
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To launch an individual test, it is first required to set up the environment for lanching the Vitis HLS Compiler which can be done as follows: Set up the environment variable, **TA_PATH**, to point to the installation path of your Vitis 2021.1 installation, and run the following commands to set up the environment:

.. code-block:: bash

   export XILINX_VITIS=${TA_PATH}/Vitis/2021.1
   export XILINX_VIVADO=${TA_PATH}/Vivado/2021.1
   source ${XILINX_VIVADO}/settings64.sh

Once the environment settings are done, an individual test can be launched by going to the test folder (any folder inside subdirectory at any level of ``REPO_PATH/dsp/L1/test/hw/`` that has Makefile is a test) and running the make command: ``make run XPART='xcu200-fsgd2104-2-e' CSIM=1 CSYNTH=1 COSIM=1``. You can choose the part number as required, and by settting CSIM/CSYNTH/COSIM=0, choose what to build and run with the make target.

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim: