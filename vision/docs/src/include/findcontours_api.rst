
.. _findcontours:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

FINDCONTOURS
=============

.. rubric:: API Syntax

findcontours is used to detect contours (boundaries of objects) in a binary image. It is a fundamental function for object detection and shape analysis.

.. code:: c

    template <int SRC_T,
              int INPUT_PTR_WIDTH,
              int OUTPUT_PTR_WIDTH,
              int ROWS,
              int COLS,
              int MAX_POINTS,
              int MAX_CONTOURS,
              int NPC,
              int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT>
    void findcontours(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src_mat,
                      int rows, int cols,
                      ap_uint<OUTPUT_PTR_WIDTH>* points_packed,
                      ap_uint<OUTPUT_PTR_WIDTH>* contour_offsets,
                      ap_uint<OUTPUT_PTR_WIDTH>* num_contours
                     )

The following table describes the template and the function parameters.

.. table:: Table: CUSTOM BGR2Y8 Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value is      |
    |                      | XF_8UC1                                                     |                    
    +----------------------+-------------------------------------------------------------+
    | INPUT_PTR_WIDTH      | Input Pointer width. The current supported width value is   |
    |                      | 8                                                           |                    
    +----------------------+-------------------------------------------------------------+
    | OUTPUT_PTR_WIDTH     | Output Pointer width. The current supported width value is  |
    |                      | 32                                                          |                    
    +----------------------+-------------------------------------------------------------+
    | ROWS                 | Maximum height of the image that hardware kernel must be    |
    |                      | built for                                                   |
    +----------------------+-------------------------------------------------------------+
    | COLS                 | Maximum width of the image that hardware kernel must be     |
    |                      | built for                                                   |                             
    +----------------------+-------------------------------------------------------------+
    | MAX_POINTS           | Maximum number of points that hardware kernel must be       |
    |                      | built for                                                   |                             
    +----------------------+-------------------------------------------------------------+
    | MAX_CONTOURS         | Maximum number of contours that hardware kernel must be     |
    |                      | built for                                                   |                             
    +----------------------+-------------------------------------------------------------+
    | NPC                  | Number of Pixels to be processed per cycle. NPPC1 and NPPC2 |
    |                      | are supported.                                              |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_IN_1       | Depth of the Input Image                                    |
    +----------------------+-------------------------------------------------------------+
    | _src_mat             | Input Image                                                 |
    +----------------------+-------------------------------------------------------------+
    | points_packed        | Output of found contour points                              |
    +----------------------+-------------------------------------------------------------+
    | contour_offsets      | Output of offsets for packed contour points                 |
    +----------------------+-------------------------------------------------------------+
    | num_contours         | Output for total contours                                   |
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2025.2 tool for the Versal AI Edge, to process a 1200*800, one channel image.  

.. table:: Table: Resource Utilization Summary

    +----------------+---------------------+------------------+----------+-------+-------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                  |
    |                |                     |                                                    |
    |                | (MHz)               |                                                    |
    +                +                     +------------------+----------+-------+-------+------+
    |                |                     | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +================+=====================+==================+==========+=======+=======+======+
    | 1 Pixel        |  300                | 356              | 14       | 7370  | 7083  | 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2025.2 tool for the Versal AI Edge, to process a 1200x800, one channel image.

.. table:: Table: Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 28               |
    +----------------+---------------------+------------------+
