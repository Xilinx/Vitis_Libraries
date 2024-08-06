.. _lens_shading_correction:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Lens Shading Correction
========================

Vignetting/Lens shading refers to the fall-off pixel intensity from the centre towards the edges of the image.
In this algorithm, vignette is corrected by considering the distance between the centre pixel and actual image pixel position. This distance is used to calculate intensity gain per pixel per channel which is used for the correction.

.. rubric:: API Syntax


.. code:: c

  template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT, int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
  void Lscdistancebased(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src, xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst) {
 
.. rubric:: Parameter Descriptions


The following table describes template parameters and arguments to the function.

.. table:: Table: Lens Shading Correction Parameter Descriptions

  +-----------------+-----------------------------------------------------------+
  | Parameter       | Description                                               |
  +=================+===========================================================+
  | SRC_T           | Input pixel type. 8/10/12/16-bit, unsigned, Bayer format  |
  |                 | is supported (XF_8UC1, XF_10UC1, XF_12UC1, XF_16UC1).     |
  +-----------------+-----------------------------------------------------------+
  | DST_T           | Output pixel type. 8/10/12/16-bit, unsigned, Bayer format |
  |                 | is supported (XF_8UC1, XF_10UC1, XF_12UC1, XF_16UC1).     |
  +-----------------+-----------------------------------------------------------+
  | ROWS            | Maximum height of input and output image                  |
  +-----------------+-----------------------------------------------------------+
  | COLS            | Maximum width of input and output image. In case of       |
  |                 | N-pixel parallelism, width should be multiple of N.       |
  +-----------------+-----------------------------------------------------------+
  | NPC             | Number of pixels to be processed per cycle; possible      |
  |                 | options are XF_NPPC1, XF_NPPC2 AND so on                  |
  +-----------------+-----------------------------------------------------------+
  | XFCVDEPTH_IN    | Depth of the input image.                                 |
  +-----------------+-----------------------------------------------------------+
  | XFCVDEPTH_OUT   | Depth of the output image.                                |
  +-----------------+-----------------------------------------------------------+
  | src             | Input image                                               |
  +-----------------+-----------------------------------------------------------+
  | dst             | Output image                                              |
  +-----------------+-----------------------------------------------------------+
  
   
.. rubric:: Resource Utilization

The following table summarizes the resource utilization  of the kernel in different configurations, generated using the Vitis HLS 2020.2 tool, to process a FULL HD image.

.. table:: Table: Lens Shading Correction Resource Utilization Summary

  +--------------------+-------------------------+--------------------------+--------------+--------+---------+---------+
  |   Operating Mode   |   Operating Frequency   |   Utilization Estimate                                               |
  +                    +   (MHz)                 +--------------------------+--------------+--------+---------+---------+
  |                    |                         |   BRAM_18K               |   DSP_48Es   |   FF   |   LUT   |   CLB   |
  +====================+=========================+==========================+==============+========+=========+=========+
  | 1 pixel-8U         | 300                     | 0                        | 20           | 3584   | 2946    | 681     |
  +--------------------+-------------------------+--------------------------+--------------+--------+---------+---------+
  | 1 pixel-16U        | 300                     | 0                        | 20           | 3608   | 2978    | 673     |
  +--------------------+-------------------------+--------------------------+--------------+--------+---------+---------+


.. rubric:: Performance Estimate

The following table summarizes a performance estimate of the kernel in different configurations, as generated using Vitis HLS 2020.2 tool, to process a FULL HD image.

.. table:: Table: Lens Shading Correction Function Performance Estimate Summary

  +--------------------+-------------------------+---------------------------------------------+
  |   Operating Mode   |   Operating Frequency   |  Latency Estimate                           |
  |                    |   (MHz)                 |  **Max (ms)**                               |
  +====================+=========================+=============================================+
  | 1 pixel            | 300                     | 7                                           |
  +--------------------+-------------------------+---------------------------------------------+
  | 2 pixel            | 300                     | 3.6                                         |
  +--------------------+-------------------------+---------------------------------------------+
