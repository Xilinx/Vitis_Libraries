
.. _global tone mapping:

Global Tone Mapping
====================

.. rubric:: API Syntax

In order to display HDR images, tone reproduction operators are applied that reduce the dynamic range to that of display device.
Global Tone Mapping uses same non-linear mapping function to all pixels throughout the image to reduce the dynamic range.

This implementaion is based on the algorithm proposed by Min H. Kim and Jan Kautz.

.. code:: c

    template <int SRC_T,
            int DST_T,
            int SIN_CHANNEL_IN_TYPE,
            int SIN_CHANNEL_OUT_TYPE,
            int ROWS,
            int COLS,
            int NPC,
            int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
            int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
    void gtm(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
            xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
            ap_ufixed<16, 4>& mean1,
            ap_ufixed<16, 4>& mean2,
            ap_ufixed<16, 4>& L_max1,
            ap_ufixed<16, 4>& L_max2,
            ap_ufixed<16, 4>& L_min1,
            ap_ufixed<16, 4>& L_min2,
            float c1,
            float c2)

The following table describes the template and the function parameters.

.. table:: Table GTM Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value is      |
    |                      | XF_16UC3                                                    |
    +----------------------+-------------------------------------------------------------+
    | DST_T                | Output Pixel type. The current supported pixel value is     |
    |                      | XF_8UC3                                                     |
    +----------------------+-------------------------------------------------------------+
    | SIN_CHANNEL_IN_TYPE  | Single channel type. It's pixel value is XF_16UC1           |
    +----------------------+-------------------------------------------------------------+
    | SIN_CHANNEL_OUT_TYPE | Single channel type. It's pixel value is XF_8UC1            |
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
    | XFCVDEPTH_IN         | Depth of Input image                                        |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_OUT        | Depth of Output image                                       |
    +----------------------+-------------------------------------------------------------+
    | src                  | Input Image                                                 |
    +----------------------+-------------------------------------------------------------+
    | dst                  | Output Image                                                |
    +----------------------+-------------------------------------------------------------+
    | mean1                | mean of pixel values computed in current frame              |
    +----------------------+-------------------------------------------------------------+
    | mean2                | mean of pixel values read by next frame                     |
    +----------------------+-------------------------------------------------------------+
    | L_max1               | Maximum pixel value computed in current frame               |
    +----------------------+-------------------------------------------------------------+
    | L_max2               | Maximum pixel value read by next frame                      |
    +----------------------+-------------------------------------------------------------+
    | L_min1               | Minimum pixel value computed in current frame               |
    +----------------------+-------------------------------------------------------------+
    | L_min2               | Minimum pixel value read by next frame                      |
    +----------------------+-------------------------------------------------------------+
    | c1                   | To retain the details in bright area, default value is 3.0, |
    |                      | value ranges from 1 to 7                                    |
    +----------------------+-------------------------------------------------------------+
    | c2                   | Efficiency factor, value ranges from 0.5 to 1 based on      |
    |                      | output device dynamic range                                 |
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2022.1 tool for the xcu200-fsgd2104-2-e, to process a 4k, 3 channel image.  

.. table:: Table GTM Resource Utilization Summary

    +----------------+---------------------+------------------+----------+-------+-------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                  |
    |                |                     |                                                    |
    |                | (MHz)               |                                                    |
    +                +                     +------------------+----------+-------+-------+------+
    |                |                     | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +================+=====================+==================+==========+=======+=======+======+
    | 1 Pixel        |  300                | 0                | 60       | 7949  | 7476  | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+
    | 2 Pixel        |  300                | 0                | 113      | 13579 | 13126 | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2022.1 tool for the xcu200-fsgd2104-2-e, to process a 4k, 3 channel image.

.. table:: Table GTM Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 28.4             |
    +----------------+---------------------+------------------+
    | 2 pixel        | 300                 | 14.7             |
    +----------------+---------------------+------------------+
