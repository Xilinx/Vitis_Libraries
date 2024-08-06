.. _defect-detection-pipeline:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Defect Detection Pipeline
===========================

The Defect Detection accelerated application is a machine vision application that automates detection of defects in mangoes and 
sorting in high-speed factory pipelines by using computer vision library functions.
The Defect detection application detects defects in a mango.

The Application has following pipelines:

* Gaussian Otsu pipeline, which includes functions `xf::cv::custom-bgr2y8 <https://docs.xilinx.com/r/en-US/Vitis_Libraries/vision/api-reference.html#custom-bgr2y8>`__, 
`xf::cv::GaussianBlur <https://docs.xilinx.com/r/en-US/Vitis_Libraries/vision/api-reference.html#gaussian-filter>`__, `xf::cv::OtsuThreshold <https://docs.xilinx.com/r/en-US/Vitis_Libraries/vision/api-reference.html#otsu-threshold>`__.

* Preprocess pipeline, which includes functions `xf::cv::Threshold <https://docs.xilinx.com/r/en-US/Vitis_Libraries/vision/api-reference.html#thresholding>`__, ``xf::cv::fw_cca``.

* CCA custom pipeline, which includes functions ``xf::cv::rev_cca``, ``xf::cv::pass_2``.
 
In the Gaussian Otsu pipeline, the BGR input image is converted to custom y8 format and the output is processed through GaussianBlur 
and OtsuThreshold which computes threshold value to mark the defects.

The Preprocess pipeline, which contains the ``xf::cv::Threshold`` function and ``xf::cv::fw_cca`` is moved to the preprocess pipeline to improve performance
where it processes the image in forward direction to get information on edge between mango pixels and background pixels.

The CCA custom pipeline, which includes ``xf::cv::rev_cca``, which processes the image in a backwards direction and ``xf::cv::pass_2`` which adds 
both forward and backward passes and gives the number of defect pixels.

|image1|

|image2|

pass_2()
---------------

.. rubric:: API Syntax

.. code:: c

    template <int SRC_T, int HEIGHT, int WIDTH, int NPC, int XFCVDEPTH_OUT>
    void pass_2(uint8_t* fwd_in_ptr,
                xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& tmp_out_mat,
                uint8_t* out_ptr,
                int& def_pix,
                int height,
                int width)

.. rubric:: Parameter Descriptions

The following table describes the template and the function parameters.

.. table:: Table: Pass_2 Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value is      |
    |                      | XF_8UC1                                                     |                    
    +----------------------+-------------------------------------------------------------+
    | HEIGHT               | Maximum height of the image that hardware kernel must be    |
    |                      | built for                                                   |                
    +----------------------+-------------------------------------------------------------+
    | WIDTH                | Maximum width of the image that hardware kernel must be     |
    |                      | built for                                                   |                                  
    +----------------------+-------------------------------------------------------------+
    | NPC                  | Number of Pixels to be processed per cycle. NPPC1 is        |
    |                      | supported.                                                  |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_OUT        | Depth of Mat Image                                          |
    +----------------------+-------------------------------------------------------------+
    | fwd_in_ptr           | Output Image pointer of fw_cca                              |
    +----------------------+-------------------------------------------------------------+
    | tmp_out_mat          | Output Image of rev_cca                                     |
    +----------------------+-------------------------------------------------------------+
    | out_ptr              | Output Image pointer of pass_2                              |
    +----------------------+-------------------------------------------------------------+
    | def_pix              | number of defect pixels                                     |
    +----------------------+-------------------------------------------------------------+
    | height               | height of Image pointer                                     |
    +----------------------+-------------------------------------------------------------+
    | width                | width of Image pointer                                      |
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2022.2 tool for the xcu200-fsgd2104-2-e, to process a FHD image.  

.. table:: Table: Defect detection Resource Utilization Summary at 300MHz Frequency and 1 Pixel per clock cycle

    +---------------------+------------------+----------+-------+-------+------+
    | Pipelines           |              Utilization Estimate                  |
    |                     |                                                    |
    |                     |                                                    |
    +                     +------------------+----------+-------+-------+------+
    |                     | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +=====================+==================+==========+=======+=======+======+
    |  Gaussian_Otsu      | 12               | 71       | 11703 | 13633 | 0    |
    +---------------------+------------------+----------+-------+-------+------+
    |  Preprocess         | 7                | 9        | 4123  | 5244  | 0    |
    +---------------------+------------------+----------+-------+-------+------+
    |  Custom CCA         | 10               | 10       | 12642 | 13756 | 0    |
    +---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2022.2 tool for the xcu200-fsgd2104-2-e, to process a FHD image.

.. table:: Table: GTM Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Pipelines      | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | Gaussian_Otsu  | 300                 | 7.1              |
    +----------------+---------------------+------------------+
    | Preprocess     | 300                 | 14.4             |
    +----------------+---------------------+------------------+
    | Custom CCA     | 300                 | 7                |
    +----------------+---------------------+------------------+


.. |image1| image:: images/defect_detection_pp.png
   :class: image 

.. |image2| image:: images/cca_kernel.png
   :class: image 
