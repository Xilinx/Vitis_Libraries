ISP Stats
================================

The ``ispstats`` function returns histogram of the input BAYER or BGR image. 

.. rubric:: API Syntax

.. code:: c

    template <int MAX_ZONES,
            int STATS_SIZE,
            int FINAL_BINS_NUM,
            int MERGE_BINS,
            int SRC_T,
            int ROWS,
            int COLS,
            int NPC = 1,
            int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT>
    void ispStats(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& _src,
                  uint32_t* stats,
                  uint32_t* max_bins_list,
                  uint16_t roi_tlx,
                  uint16_t roi_tly,
                  uint16_t roi_brx,
                  uint16_t roi_bry,
                  uint16_t zone_col_num,
                  uint16_t zone_row_num)
  
.. rubric:: Parameter Descriptions

The following table describes the template and the function parameters.

.. table:: Table . ispstats Parameter Description

   +-------------------+--------------------------------------------------+
   | Parameter         | Description                                      |
   +===================+==================================================+
   | MAX_ZONES         | Maximum number of possible zones.                |
   +-------------------+--------------------------------------------------+
   | STATS_SIZE        | Number of bins per channel for the input image.  |
   |                   | This is equal to the number of output bins if    |
   |                   | merge bins feature is disabled.                  |
   +-------------------+--------------------------------------------------+
   | FINAL_BINS_NUM    |  Number of output bins per channel if merge      | 
   |                   |  bins feature is enabled.                        |  
   +-------------------+--------------------------------------------------+
   | MERGE_BINS        | To disable or enable merge bins feature.         |
   +-------------------+--------------------------------------------------+
   | SRC_T             | Input pixel type. For BGR, 8UC3 is supported as  |
   |                   | input. For Bayer, 8UC1 is supported              |
   +-------------------+--------------------------------------------------+
   | ROWS              | Maximum height of input image.                   | 
   +-------------------+--------------------------------------------------+
   | COLS              | Maximum width of input image.                    |
   +-------------------+--------------------------------------------------+
   | NPC               | Number of pixels to be processed per cycle,      |
   |                   | possible options are XF_NPPC1 only.              |
   +-------------------+--------------------------------------------------+
   | XFCVDEPTH_IN      | Depth of Input image                             |
   +-------------------+--------------------------------------------------+
   | \_src             | Input image                                      |
   +-------------------+--------------------------------------------------+
   | stats             | Calculted histogram of the image.                |
   +-------------------+--------------------------------------------------+
   | max_bins_list     | List of maximum values per range of bins.        | 
   |                   | This is only applicable if merge bins feature    | 
   |                   | is enabled.                                      |
   +-------------------+--------------------------------------------------+
   | roi_tlx           | Top left x coordinate of ROI                     |
   +-------------------+--------------------------------------------------+
   | roi_tly           | Top left y coordinate of ROI                     |
   +-------------------+--------------------------------------------------+
   | roi_brx           | Bottom right x coordinate of ROI                 |
   +-------------------+--------------------------------------------------+
   | roi_bry           | Bottom right y coordinate of ROI                 |
   +-------------------+--------------------------------------------------+
   | zone_col_num      | Number of zones across column.                   |
   +-------------------+--------------------------------------------------+
   | zone_row_num      | Number of zones across rows.                     |
   +-------------------+--------------------------------------------------+
   
.. rubric:: Resource Utilization


The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2022.1 tool for the
Alveo U50 FPGA.

.. table:: Table . ispstats Function Resource Utilization Summary

    +----------------+---------------------------+-----------------+------------+---------------------------------+------+------+
    | Operating Mode | Operating Frequency (MHz) | Input pattern   | MEGRE BINS |               Utilization Estimate            | 
    +                +                           +                 +            +----------------+----------------+------+------+
    |                |                           |                 |            |    BRAM_18K    |    DSP_48Es    |  FF  |  LUT | 
    +================+===========================+=================+============+================+================+======+======+
    | 1 Pixel        | 150                       |BGR 8-bit/channel|     0      |      144       |        4       | 6011 | 34828|
    +----------------+---------------------------+-----------------+------------+----------------+----------------+------+------+
    | 1 Pixel        | 150                       |BGR 8-bit/channel+     1      |      144       |        4       | 5933 | 36231|
    +----------------+---------------------------+-----------------+------------+----------------+----------------+------+------+

.. rubric:: Performance Estimate


The following table summarizes the performance of the kernel in 1-pixel
mode as generated using Vitis HLS 2022.1 tool for the Xilinx Alveo
U50 FPGA to process a grayscale HD (1920x1080)
image.

.. table:: Table . rgbir2bayer Function Performance Estimate Summary

    +-----------------------------+-----------------+------------------+------------------+
    | Operating Mode              | Input pattern   | MERGE BINS       |Latency Estimate  |
    +                             +                 +                  +------------------+
    |                             |                 |                  | Max Latency (ms) |
    +=============================+=================+==================+==================+
    | 1 pixel operation (150 MHz) |BGR 8-bit/channel|        0         |        16.8      |
    +-----------------------------+-----------------+------------------+------------------+
    | 1 pixel operation (150 MHz) |BGR 8-bit/channel|        1         |        20.3      |
    +-----------------------------+-----------------+------------------+------------------+

.. toctree::
    :maxdepth: 1