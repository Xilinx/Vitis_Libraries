
.. _3D depthmap:

.. 
   Copyright 2026 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

3D depth map
====================

A 3D depth map represents the distance of each point in a scene from the camera. 
It is generated from a stereo disparity map, which measures the pixel shift (disparity) 
between two slightly different images captured by stereo cameras. 
Since closer objects have larger disparities and farther objects have smaller disparities, 
these disparities are mathematically converted into depth values, producing a depth map 
that encodes how far each part of the scene is in 3D space.

					𝑍 = (𝑓⋅𝐵)/𝑑
					
	𝑍 : depth (distance from the camera to the object)
	
	𝑓 : focal length of the camera (in pixels)
	
	𝐵 : baseline, i.e., distance between the two cameras
	
	𝑑 : disparity (pixel shift between left and right images)
	

.. rubric:: API Syntax

.. code:: c

    template <int SRC_T,
            int DST_T,
            int ROWS,
            int COLS,
            int NPC,
            int XFCVDEPTH_disp = _XFCVDEPTH_DEFAULT,
            int XFCVDEPTH_out = _XFCVDEPTH_DEFAULT>
    void depth3D(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_disp>& _disp_mat,
            xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_out>& _depth_mat,
            float focal_len,
            float  base_dis)

The following table describes the template and the function parameters.

.. table:: Table: 3D depthmap Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | SRC_T                | Input Pixel type. The current supported pixel value are     |
    |                      | XF_16UC3, XF_16SC3                                          |
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
    | XFCVDEPTH_IN         | Depth of hls:stream of input xf::cv::Mat                    |
    +----------------------+-------------------------------------------------------------+
    | XFCVDEPTH_OUT        | Depth of hls:stream of output xf::cv::Mat                   |
    +----------------------+-------------------------------------------------------------+
    | _disp_mat            | Input image                                                 |
    +----------------------+-------------------------------------------------------------+
    | _depth_mat           | Output Image                                                |
    +----------------------+-------------------------------------------------------------+
    | focal_len            | focal length of the camera                                  |
    +----------------------+-------------------------------------------------------------+
    | base_dis             | distance between the two cameras                            |
    +----------------------+-------------------------------------------------------------+

.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2024.2 tool for the xcu200-fsgd2104-2-e, to process a 1280x720, three channel image.  

.. table:: Table: 3D depthmap Resource Utilization Summary

    +----------------+---------------------+------------------+----------+-------+-------+------+
    | Operating Mode | Operating Frequency |              Utilization Estimate                  |
    +                +                     +------------------+----------+-------+-------+------+
    |                |       (MHz)         | BRAM_18K         | DSP      | FF    | LUT   | URAM |
    +================+=====================+==================+==========+=======+=======+======+
    |     1 Pixel    |          300        |       0          |    36    | 2128  | 1075  |   0  |
    +----------------+---------------------+------------------+----------+-------+-------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2024.2 tool for the xcu200-fsgd2104-2-e, to process a 1280x720, three channel image.

.. table:: Table: 3D point Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     |       Max (ms)   |
    +================+=====================+==================+
    |      1 Pixel   |         300         |        3.4       |
    +----------------+---------------------+------------------+
