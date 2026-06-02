
.. _findcontours:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Find Contours
=============

The ``findcontours`` function detects object boundaries in a binary (8-bit) image
using a chain-code contour tracer. Contour vertices are written as packed
``(x, y)`` coordinates; per-contour start indices and the total contour count are
returned in separate buffers. The implementation is in
``L1/include/imgproc/xf_findcontours.hpp``.

.. rubric:: API Syntax


.. code:: c

    template <int SRC_T,
              int INPUT_PTR_WIDTH,
              int OUTPUT_PTR_WIDTH,
              int MAX_H,
              int MAX_W,
              int MAX_TOTAL_POINTS,
              int MAX_CONTOURS,
              int NPPCX = 1,
              int XFCVDEPTH_IN_MAT = 2>
    void findcontours(xf::cv::Mat<SRC_T, MAX_H, MAX_W, NPPCX, XFCVDEPTH_IN_MAT>& _src,
                      int rows,
                      int cols,
                      ap_uint<OUTPUT_PTR_WIDTH>* points_packed,
                      ap_uint<OUTPUT_PTR_WIDTH>* contour_offsets,
                      ap_uint<OUTPUT_PTR_WIDTH>& num_contours);


.. rubric:: Parameter Descriptions


The following table describes the template parameters.

.. table:: Table . findcontours Template Parameter Descriptions

    +---------------------+-------------------------------------------------------+
    | Parameter           | Description                                           |
    +=====================+=======================================================+
    | SRC_T               | Input pixel type. Supported: XF_8UC1 (binary image).  |
    +---------------------+-------------------------------------------------------+
    | INPUT_PTR_WIDTH     | AXI width (bits) for the input image port. Typical: 8.|
    +---------------------+-------------------------------------------------------+
    | OUTPUT_PTR_WIDTH    | AXI width (bits) for output buffers.                  |
    +---------------------+-------------------------------------------------------+
    | MAX_H               | Maximum image height the kernel is compiled for.      |
    +---------------------+-------------------------------------------------------+
    | MAX_W               | Maximum image width the kernel is compiled for.       |
    +---------------------+-------------------------------------------------------+
    | MAX_TOTAL_POINTS    | Maximum number of contour points that can be stored.  |
    +---------------------+-------------------------------------------------------+
    | MAX_CONTOURS        | Maximum number of contours that can be detected.      |
    +---------------------+-------------------------------------------------------+
    | NPPCX               | Pixels per clock.                                     |
    +---------------------+-------------------------------------------------------+
    | XFCVDEPTH_IN_MAT    | Stream depth for the input.                           |
    +---------------------+-------------------------------------------------------+

The following table describes the function parameters.

.. table:: Table . findcontours Function Parameter Descriptions

    +-------------------+-------------------------------------------------------+
    | Parameter         | Description                                           |
    +===================+=======================================================+
    | \_src             | Input binary image .                                  |
    +-------------------+-------------------------------------------------------+
    | rows              | Active image height (``<= MAX_H``).                   |
    +-------------------+-------------------------------------------------------+
    | cols              | Active image width (``<= MAX_W``).                    |
    +-------------------+-------------------------------------------------------+
    | points_packed     | Device buffer for packed contour points.              |
    +-------------------+-------------------------------------------------------+
    | contour_offsets   | Device buffer of start indices into ``points_packed`` |
    |                   | for each contour (length ``MAX_CONTOURS + 1``).       |
    +-------------------+-------------------------------------------------------+
    | num_contours      | Output: number of contours found.                     |
    +-------------------+-------------------------------------------------------+

.. rubric:: Resource Utilization


The following table summarizes resource utilization for a 1200 x 800,
single-channel (XF_8UC1) image, as generated in the Vitis HLS 2025.2 tool for
Versal AI Edge at 300 MHz.

.. table:: Table . findcontours Function Resource Utilization Summary

    +----------------+---------------------+-------+---------+---------+----------+---------+---------+
    | Operating Mode | Operating Frequency | NPC   | LUT     | FF      | DSP      | BRAM    | URAM    |
    |                | (MHz)               |       |         |         |          |         |         |
    +================+=====================+=======+=========+=========+==========+=========+=========+
    | 1 pixel        | 300                 | 1     | 3161    | 2745    | 9        | 126     | 0       |
    +----------------+---------------------+-------+---------+---------+----------+---------+---------+

.. rubric:: Performance Estimate


The following table summarizes performance for the same configuration (1200 x 800,
XF_8UC1, 300 MHz).

.. table:: Table . findcontours Function Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               | Max latency (ms) |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 28               |
    +----------------+---------------------+------------------+
