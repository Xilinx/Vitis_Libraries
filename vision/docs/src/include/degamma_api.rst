
.. _degamma:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Degamma
=========

.. rubric:: API Syntax

Degamma also known as linearization is typically designed to linearize the input from a sensor (or any pre-processing IP) in order to facilitate ISP processing that operates on a linear domain, such as noise reduction.
While non-linearization comes in several forms, this module is served to linearize only simple curves.

This implementation supports Bayer Raw data of 8, 10, 12, and 16 bits per pixel with a maximum of 64 knee points PWL mapping.

.. code:: c

    template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int N>
    void degamma(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
             xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
             ap_ufixed<32, 16> params[3][N][3],
             unsigned short bayerp)

The following table describes the template and the function parameters.

.. table:: Table: HDR Decompand Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value is      |
    |                        | XF_8UC1 and XF_16UC1                                      |
    +----------------------+-------------------------------------------------------------+
    | DST_T                | Output Pixel type. The current supported pixel value is     |
    |                        | XF_8UC1 and XF_16UC1                                      |
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
    | N                    | Configurable number of knee points                          |
    +----------------------+-------------------------------------------------------------+
    | src                  | Input Image                                                 |
    +----------------------+-------------------------------------------------------------+
    | dst                  | Output Image                                                |
    +----------------------+-------------------------------------------------------------+
    | params               | Array containing upper limit, slope, and intercept of linear|
    |                      | equations for Red, Green, and Blue colors.                  |
    +----------------------+-------------------------------------------------------------+
    | bayerp               | Input Bayer pattern. XF_BAYER_BG, XF_BAYER_GB, XF_BAYER_GR  |
    |                      | and XF_BAYER_RG are the supported values.                   |
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2022.2 tool for the xcu200-fsgd2104-2-e, to process a FHD Bayer image.

.. table:: Table: Degamma Resource Utilization Summary

    +----------------+---------------------+------------------+----------+-------+-------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                  |
    |                |                     |                                                    |
    |                | (MHz)               |                                                    |
    +                +                     +------------------+----------+-------+-------+------+
    |                |                     | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +================+=====================+==================+==========+=======+=======+======+
    | 1 Pixel        |  300                | 0                | 4        | 2187  | 2082  | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+
    | 2 Pixel        |  300                | 0                | 8        | 4265   | 4078 | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2022.2 tool for the xcu200-fsgd2104-2-e, to process a FHD Bayer image.

.. table:: Table: Degamma Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 7.2              |
    +----------------+---------------------+------------------+
    | 2 pixel        | 300                 | 3.6              |
    +----------------+---------------------+------------------+
