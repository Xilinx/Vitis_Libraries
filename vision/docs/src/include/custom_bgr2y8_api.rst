
.. _custom bgr2y8:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

CUSTOM BGR2Y8
=============

.. rubric:: API Syntax

Custom Y8 image is generated from bgr image using h, s, v values.

.. code:: c

    template <int SRC_T,
            int DST_T,
            int ROWS,
            int COLS,
            int NPC,
            int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
            int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT>
    void custom_bgr2y8(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src_mat,
                    xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& _dst_mat,
                    struct bgr2y8_params params)

The following table describes the template and the function parameters.

.. table:: Table: CUSTOM BGR2Y8 Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value is      |
    |                      | XF_8UC3                                                     |                    
    +----------------------+-------------------------------------------------------------+
    | DST_T                | Output Pixel type. The current supported pixel value is     |
    |                      | XF_8UC1                                                     |
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
    | XFCVDEPTH_IN_1       | Depth of the Input Image                                    |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_OUT_1      | Depth of the Output Image                                   |
    +----------------------+-------------------------------------------------------------+
    | _src_mat             | Input Image                                                 |
    +----------------------+-------------------------------------------------------------+
    | _dst_mat             | Output Image                                                |
    +----------------------+-------------------------------------------------------------+
    | params               | config parameters that includes range of max and min H, S, V|
    |                      | values                                                      |						
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2022.1 tool for the xcu200-fsgd2104-2-e, to process a 4k, three channel image.  

.. table:: Table: Resource Utilization Summary

    +----------------+---------------------+------------------+----------+-------+-------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                  |
    |                |                     |                                                    |
    |                | (MHz)               |                                                    |
    +                +                     +------------------+----------+-------+-------+------+
    |                |                     | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +================+=====================+==================+==========+=======+=======+======+
    | 1 Pixel        |  300                | 2                | 11       | 1120  | 919   | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+
    | 2 Pixel        |  300                | 2                | 17       | 1436  | 1288  | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2022.1 tool for the xcu200-fsgd2104-2-e, to process a 4k, three channel image.

.. table:: Table: Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 27.3             |
    +----------------+---------------------+------------------+
    | 2 pixel        | 300                 | 13.6             |
    +----------------+---------------------+------------------+
