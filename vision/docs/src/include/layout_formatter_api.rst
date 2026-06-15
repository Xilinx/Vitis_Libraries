
.. _layout-formatter:

.. 
   Copyright 2025 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Layout Formatter
================

The ``layout_formatter`` function reformats an input image into a DNN-friendly
tensor layout. It supports runtime selection of output layout format, datatype,
and channel count when the corresponding compile-time options are enabled.

Supported output layouts:

-  **NHWC** — height, width, channel interleaved (single output buffer).
-  **HCWNC4** — height, channel groups of 4, width (single output buffer).
-  **HCWNC8** — height, channel groups of 8, width (single output buffer).
-  **NCHW** — planar channel layout; each channel is written to a separate
   output buffer (up to four M-AXI pointers).

Supported output datatypes: INT8, FP16, BF16, and FP32.

The reference L1 example is located at ``L1/examples/layout_formatter``.
Runtime layout and datatype tests are under ``L1/tests/layout_formatter/``.

.. rubric:: Pipeline Stages

The L1 example pipeline performs the following steps:

-  **Layout Format:** ``xf::cv::layout_formatter`` packs pixels into the
   selected tensor layout and bit width:

   -  **NHWC:** ``format_nhwc`` packs interleaved channel data into a single
      stream, then ``intrStrm2OutMat`` writes to the output buffer.
   -  **HCWNC4 / HCWNC8:** ``format_hcwnc`` groups channels into blocks of 4 or
      8, then writes to the output buffer.
   -  **NCHW:** ``format_nchw`` splits each channel into a separate internal
      stream; ``intrStrm2OutMat_4streams`` writes channel planes to separate
      output buffers.

-  **Output:** For NHWC, HCWNC4, and HCWNC8, a single buffer holds the
   formatted tensor. For NCHW, each active channel plane uses its own buffer.


layout_formatter()
------------------

.. rubric:: API Syntax

.. code:: c

    template <int OUT_PTR_WIDTH,
              int OUT_PTR_WIDTH_NCHW,
              int DATAORDER,
              int DATATYPE,
              int INTYPE,
              int OUTTYPE,
              int OUT_TYPE_NCHW,
              int ROWS,
              int COLS,
              int NPC,
              int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
              int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
    void layout_formatter(xf::cv::Mat<INTYPE, ROWS, COLS, NPC, XFCVDEPTH_IN>& in_mat,
                          ap_uint<OUT_PTR_WIDTH>* out_mat,
                          ap_uint<OUT_PTR_WIDTH_NCHW>* out_mat1,
                          ap_uint<OUT_PTR_WIDTH_NCHW>* out_mat2,
                          ap_uint<OUT_PTR_WIDTH_NCHW>* out_mat3,
                          ap_uint<OUT_PTR_WIDTH_NCHW>* out_mat4,
                          int data_order,
                          int data_type,
                          int out_pixel_channels);


.. rubric:: Parameter Descriptions

The following table describes the template and function parameters.
Implementation is in ``L1/include/imgproc/xf_layout_formatter.hpp``.

.. table:: Table: layout_formatter Template Parameter Descriptions

    +----------------------+-----------------------------------------------------------+
    | Parameter            | Description                                               |
    +======================+===========================================================+
    | OUT_PTR_WIDTH        | AXI pointer width (bits) for NHWC/HCWNC output.           |
    +----------------------+-----------------------------------------------------------+
    | OUT_PTR_WIDTH_NCHW   | AXI pointer width (bits) for NCHW per-channel output.     |
    +----------------------+-----------------------------------------------------------+
    | DATAORDER            | Compile-time bitmask of enabled layouts. Bit 0 = NHWC,    |
    |                      | bit 1 = HCWNC4, bit 2 = HCWNC8, bit 3 = NCHW.             |
    +----------------------+-----------------------------------------------------------+
    | DATATYPE             | Compile-time bitmask of enabled datatypes. Bit 0 = INT8,  |
    |                      | bit 1 = FP16, bit 2 = BF16, bit 3 = FP32.                 |
    +----------------------+-----------------------------------------------------------+
    | INTYPE               | Input pixel type (e.g. ``XF_8UC3``, ``XF_32FC4``).        |
    +----------------------+-----------------------------------------------------------+
    | OUTTYPE              | Output pixel type for NHWC/HCWNC layouts.                 |
    +----------------------+-----------------------------------------------------------+
    | OUT_TYPE_NCHW        | Output pixel type for NCHW per-channel planes.            |
    +----------------------+-----------------------------------------------------------+
    | ROWS                 | Maximum image height the kernel is compiled for.          |
    +----------------------+-----------------------------------------------------------+
    | COLS                 | Maximum image width the kernel is compiled for.           |
    +----------------------+-----------------------------------------------------------+
    | NPC                  | Number of pixels processed per clock cycle (NPPC).        |
    +----------------------+-----------------------------------------------------------+
    | XFCVDEPTH_IN         | FIFO depth of the input ``xf::cv::Mat`` stream.           |
    +----------------------+-----------------------------------------------------------+
    | XFCVDEPTH_OUT        | Output buffer depth (-1 for memory buffer).               |
    +----------------------+-----------------------------------------------------------+
    | in_mat               | Input image.                                              |
    +----------------------+-----------------------------------------------------------+
    | out_mat              | Output buffer for NHWC/HCWNC layouts.                     |
    +----------------------+-----------------------------------------------------------+
    | out_mat1 - out_mat4  | Per-channel output buffers for NCHW layout.               |
    +----------------------+-----------------------------------------------------------+
    | data_order           | Runtime layout selection (``layout_format`` enum).        |
    +----------------------+-----------------------------------------------------------+
    | data_type            | Runtime datatype selection (``data_types`` enum).         |
    +----------------------+-----------------------------------------------------------+
    | out_pixel_channels   | Number of active output channels (1, 3, or 4).            |
    +----------------------+-----------------------------------------------------------+


.. table:: Table: Compile-Time Configuration Parameters

    +-----------------------------+------------------------------------------------+
    | Parameter                   | Description                                    |
    +=============================+================================================+
    | HEIGHT / WIDTH              | Maximum input image dimensions.                |
    +-----------------------------+------------------------------------------------+
    | NPPCX                       | Pixels per clock cycle (default NPPC4).        |
    +-----------------------------+------------------------------------------------+
    | _XF_NHWC_                   | Enable NHWC layout at compile time (0 or 1).   |
    +-----------------------------+------------------------------------------------+
    | _XF_NCHW_                   | Enable NCHW layout at compile time (0 or 1).   |
    +-----------------------------+------------------------------------------------+
    | _XF_HCWNC4_                 | Enable HCWNC4 layout at compile time (0 or 1). |
    +-----------------------------+------------------------------------------------+
    | _XF_HCWNC8_                 | Enable HCWNC8 layout at compile time (0 or 1). |
    +-----------------------------+------------------------------------------------+
    | _XF_RGBA_                   | 1 = 4-channel RGBA input; 0 = 3-channel RGB.   |
    +-----------------------------+------------------------------------------------+
    | XF_INT8 / XF_FP16 /         | Enable corresponding output datatype at        |
    | XF_BF16 / XF_FP32           | compile time. Combined into ``SELECT_TYPE``.   |
    +-----------------------------+------------------------------------------------+
    | SELECT_ORDER                | Combined bitmask of enabled layouts.           |
    +-----------------------------+------------------------------------------------+
    | OUTPUT_PTR_WIDTH            | M-AXI pointer width for NHWC/HCWNC output.     |
    +-----------------------------+------------------------------------------------+
    | OUTPUT_PTR_WIDTH_NCHW       | M-AXI pointer width for NCHW channel output.   |
    +-----------------------------+------------------------------------------------+


.. rubric:: Resource Utilization


The following table summarizes resource utilization for a 1920 x 1080 image,
as generated in the Vitis HLS 2026.1 tool for Versal AI Edge at 300 MHz.

.. table:: Table: layout_formatter Resource Utilization Summary

    +------------------+---------------------+-----------+-----+-------+-------+-----+------+------+
    | Function Config  | Image Size          | Frequency | NPC | LUT   | FF    | DSP | BRAM | URAM |
    |                  |                     | (MHz)     |     |       |       |     |      |      |
    +==================+=====================+===========+=====+=======+=======+=====+======+======+
    | NCHW INT8, FP16  | 1920 x 1080         | 300       | 4   | 5538  | 4319  | 4   | 4    | 0    |
    +------------------+---------------------+-----------+-----+-------+-------+-----+------+------+
    | NCHW FP32        | 1920 x 1080         | 300       | 4   | 9772  | 7032  | 4   | 8    | 0    |
    +------------------+---------------------+-----------+-----+-------+-------+-----+------+------+
    | NHWC INT8, FP16  | 1920 x 1080         | 300       | 4   | 19459 | 4438  | 6   | 7.5  | 0    |
    +------------------+---------------------+-----------+-----+-------+-------+-----+------+------+
    | NHWC FP32        | 1920 x 1080         | 300       | 4   | 32071 | 18184 | 6   | 7.5  | 0    |
    +------------------+---------------------+-----------+-----+-------+-------+-----+------+------+
    | HCWNC INT8, FP16 | 1920 x 1080         | 300       | 4   | 16062 | 3473  | 4   | 7.5  | 0    |
    +------------------+---------------------+-----------+-----+-------+-------+-----+------+------+
    | HCWNC FP32       | 1920 x 1080         | 300       | 4   | 12536 | 7158  | 4   | 7.5  | 0    |
    +------------------+---------------------+-----------+-----+-------+-------+-----+------+------+


.. rubric:: Performance Estimate


The following table summarizes the latency estimates for a Full HD (1920 x 1080)
image at 300 MHz.

.. table:: Table: layout_formatter Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               | Max (ms)         |
    +================+=====================+==================+
    | 4 pixel (NPC4) | 300                 | 1.8              |
    +----------------+---------------------+------------------+
    | 2 pixel (NPC2) | 300                 | 3.6              |
    +----------------+---------------------+------------------+
    | 1 pixel (NPC1) | 300                 | 7.0              |
    +----------------+---------------------+------------------+
