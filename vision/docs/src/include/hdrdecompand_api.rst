HDR Decompanding
=================

.. rubric:: API Syntax

For imaging sensors that do not equip with high bit width in the transmission side, they can compress (compand) data in a piece-wise linear (PWL) mapping to a lower bit depth.
The receiving end will do the inverse (or de-compand) to recover the data with satisfiable aliasing.

This implementation supports Bayer Raw data with four knee points. PWL mapping and default equations are provided for 12-bit to 20-bit and 16-bit to 24-bit conversion.

.. code:: c

    template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
    void hdr_decompand(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                   xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                   int params[3][4][3],
                   unsigned short bayerp)

The following table describes the template and the function parameters.

.. table:: Table: HDR Decompand Parameter Description
    
    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value is      |
    |                      | XF_16UC1                                                    |
    +----------------------+-------------------------------------------------------------+
    | DST_T                | Output Pixel type. The current supported pixel value is     |
    |                      | XF_32UC1                                                    |
    +----------------------+-------------------------------------------------------------+
    | ROWS                 | Maximum height of the image that hardware kernel must be    |
    |                      | built for                                                   |                
    +----------------------+-------------------------------------------------------------+
    | COLS                 | Maximum width of the image that hardware kernel must be     |
    |                      | built for                                                   |
    +----------------------+-------------------------------------------------------------+
    | NPC                  | Number of Pixels to be processed per cycle. NPPC1 and NPPC2 |
    |                      | are supported.                                              |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_IN         | Depth of Input Image                                        |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_OUT        | Depth of Output Image                                       |
    +----------------------+-------------------------------------------------------------+
    | src                  | Input Image                                                 |
    +----------------------+-------------------------------------------------------------+
    | dst                  | Output Image                                                |
    +----------------------+-------------------------------------------------------------+
    | params               | array containing upper limit, slope and intercept of linear |
    |                      | equations for Red, Green and Blue colours.                  |
    +----------------------+-------------------------------------------------------------+
    | bayerp               | Input Bayer pattern. XF_BAYER_BG, XF_BAYER_GB, XF_BAYER_GR  |
    |                      | and XF_BAYER_RG are the supported values.                   |
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using the Vitis HLS 2022.2 tool for the xcu200-fsgd2104-2-e, to process an FHD Bayer image.

.. table:: Table: HDR Decompand Resource Utilization Summary

    +----------------+---------------------+------------------+----------+-------+-------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                  |
    |                |                     |                                                    |
    |                | (MHz)               |                                                    |
    +                +                     +------------------+----------+-------+-------+------+
    |                |                     | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +================+=====================+==================+==========+=======+=======+======+
    | 1 Pixel        |  300                | 0                | 9        | 473   | 296   | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+
    | 2 Pixel        |  300                | 0                | 12       | 557   | 518   | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using the Vitis HLS 2022.2 tool for the xcu200-fsgd2104-2-e, to process an FHD Bayer image.

.. table:: Table: HDR Decompand Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 8.8              |
    +----------------+---------------------+------------------+
    | 2 pixel        | 300                 | 4.3              |
    +----------------+---------------------+------------------+
