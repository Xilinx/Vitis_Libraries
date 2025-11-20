
.. _3D point cloud:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

3D point cloud
====================

.. rubric:: API Syntax

The function transforms a single-channel disparity map to a 3-channel image representing a 3D surface. 
That is, for each pixel (x,y) and the corresponding disparity d=disparity(x,y) , it computes
.. figure:: ./images/3dpoint.png
   :alt: 
   :figclass: image

.. code:: c

    template <int SRC_T,
            int DST_T,
            int ROWS,
            int COLS,
            int NPC,
            int XFCVDEPTH_disp = _XFCVDEPTH_DEFAULT,
            int XFCVDEPTH_out = _XFCVDEPTH_DEFAULT>
    void reprojectimageto3D(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_disp>& _disp_mat,
            xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_out>& _3D_mat,
            double *Q_mtrx,
            int16_t min_disp,
            bool handle_missval=false)

The following table describes the template and the function parameters.

.. table:: Table: 3D point Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value are     |
    |                      | XF_16SC3                                                    |
    +----------------------+-------------------------------------------------------------+
    | DST_T                | Output co-ordinate type. The current supported value is     |
    |                      | XF_32FC3                                                    |
    +----------------------+-------------------------------------------------------------+
    | ROWS                 | Maximum height of the image that hardware kernel must be    |
    |                      | built for                                                   |
    +----------------------+-------------------------------------------------------------+
    | COLS                 | Maximum width of the image that hardware kernel must be     |
    |                      | built for                                                   |
    +----------------------+-------------------------------------------------------------+
    | NPC                  | Number of Pixels to be processed per cycle. NPPC1           |
    |                      | is supported.                                               |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_IN         | Depth of Input image                                        |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_OUT        | Depth of Output image                                       |
    +----------------------+-------------------------------------------------------------+
    | _disp_mat            | Input Image,  If the disparity is 16-bit signed format,     | 
    |                      | as computed by StereoBM or StereoSGBM and maybe             | 
    |                      | other algorithms, it should be divided by 16                |
    |                      | (and scaled to float) before being used here.               |
    +----------------------+-------------------------------------------------------------+
    | _3D_mat              | Output Image                                                |
    +----------------------+-------------------------------------------------------------+
    | Q_mtrx               | A 4x4 matrix that encodes the transformation from pixel     |
    |                      | coordinates and disparity to 3D coordinates.                | 
    |                      | This matrix is often computed using cv::stereoRectify       |
    +----------------------+-------------------------------------------------------------+
    | min_disp             | minimum value in a input disparity map                      |
    +----------------------+-------------------------------------------------------------+
    | handle_missval       | if it is true then pixels with the minimal disparity that   | 
    |                      | corresponds to the outliers are transformed to 3D  points   |
    |                      | with a very large Z value. currently set to 10000           |
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2024.2 tool for the xcu200-fsgd2104-2-e, to process a 1280x720, three channel image.  

.. table:: Table: 3D point Resource Utilization Summary

    +----------------+---------------------+------------------+----------+-------+-------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                  |
    |                |                     |                                                    |
    |                | (MHz)               |                                                    |
    +                +                     +------------------+----------+-------+-------+------+
    |                |                     | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +================+=====================+==================+==========+=======+=======+======+
    | 1 Pixel        |  300                | 0                | 36       | 40432  | 26476| 0    |
    +----------------+---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2024.2 tool for the xcu200-fsgd2104-2-e, to process a 1280x720, three channel image.

.. table:: Table: 3D point Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 3.4              |
    +----------------+---------------------+------------------+

.. rubric:: Deviations from OpenCV


Listed below are the deviations from the OpenCV:

    #. Output handling

   The output type of the OpenCV can be passed as argument. In the HLS
   implementation, output type will be always float type.

   #. min disparity value
   The min disparity value is computed inside the opencv kernel.In the HLS
   implementation, min disparity value will be computed in testbench and send as argument to kernel.
   
