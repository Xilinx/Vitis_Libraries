
.. _findcontours:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Find Contours
=============

The ``findcontours`` function detects object boundaries in a binary (8-bit) image
using a chain-code contour tracer. Contour vertices are written to
``points_packed`` as 32-bit words; ``contour_offsets`` records where each
contour begins and ends in that array, and ``num_contours`` returns the total
number of contours found. The implementation is in
``L1/include/imgproc/xf_findcontours.hpp``.

.. rubric:: Output Point Packing

Each contour vertex is stored as one 32-bit word in ``points_packed``:

- Bits ``[15:0]`` hold the **x** coordinate (16-bit unsigned).
- Bits ``[31:16]`` hold the **y** coordinate (16-bit unsigned).

All contours are stored back-to-back in ``points_packed``. The
``contour_offsets`` array (length ``MAX_CONTOURS + 1``) indexes into that flat
array:

- ``contour_offsets[0]`` is always ``0``.
- For ``c >= 1``, ``contour_offsets[c]`` is the exclusive end index for
  contour ``c - 1`` (the number of 32-bit words consumed by contours ``0``
  through ``c - 1``).

Contour ``k`` therefore spans ``points_packed[contour_offsets[k]]`` through
``points_packed[contour_offsets[k + 1] - 1]``. The number of points in
contour ``k`` is ``contour_offsets[k + 1] - contour_offsets[k]``.

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
    | OUTPUT_PTR_WIDTH    | AXI width (bits) for output pointers. Typical: 32.    |
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
    | points_packed     | Flat array of packed contour vertices. Each entry is  |
    |                   | one 32-bit word: ``y[31:16] | x[15:0]``. Contours are |
    |                   | stored sequentially (see Output Point Packing).       |
    +-------------------+-------------------------------------------------------+
    | contour_offsets   | Index table into ``points_packed`` (length            |
    |                   | ``MAX_CONTOURS + 1``). ``contour_offsets[0] = 0``;    |
    |                   | ``contour_offsets[c]`` is the exclusive end index for   |
    |                   | contour ``c - 1``.                                      |
    +-------------------+-------------------------------------------------------+
    | num_contours      | Output: number of contours found.                     |
    +-------------------+-------------------------------------------------------+

.. rubric:: Deviation from OpenCV


**API and parameters**

- There are no ``mode`` (for example ``RETR_EXTERNAL``, ``RETR_TREE``) or
  ``method`` (for example ``CHAIN_APPROX_SIMPLE``, ``CHAIN_APPROX_NONE``)
  arguments. Retrieval and chain encoding behavior are fixed in hardware.
- There is no ``hierarchy`` output and no optional ``offset`` argument.
- Outputs are written to ``points_packed``, ``contour_offsets``, and
  ``num_contours`` instead of an array of point lists.

**Contour representation**

- OpenCV returns each contour as a sequence of ``cv::Point`` values (full
  chain or approximated, depending on ``method``).
- This kernel stores vertices only when the 8-connected chain direction
  changes, which compresses the boundary similarly in spirit to
  ``CHAIN_APPROX_SIMPLE`` but is not guaranteed to match OpenCV point
  for point.
- Each vertex is one 32-bit word ``y[31:16] | x[15:0]`` (see Output Point
  Packing above), not a ``cv::Point`` object.

**Capacity and input**

- ``MAX_CONTOURS`` and ``MAX_TOTAL_POINTS`` are compile-time limits. When
  either limit is reached, additional contours or points are not written.
- Input must be a single-channel 8-bit binary image (``XF_8UC1``).

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

.. note::

   Latency is proportional to the number of contours detected in the input
   image. Images with more or longer contours take longer to process than
   sparse binary images with few boundaries.

.. table:: Table . findcontours Function Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               | Max latency (ms) |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 28               |
    +----------------+---------------------+------------------+
