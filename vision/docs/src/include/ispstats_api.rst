.. ispstats:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

ISP Stats
================================

The ``ispstats`` function returns a histogram of the input BAYER, BGR, or GRAY
image. 

.. rubric:: API Syntax

.. code:: c

   template <
       int MAX_ZONES, 
       int STATS_SIZE, 
       int FINAL_BINS_NUM, 
       int MERGE_BINS, 
       int SRC_T, 
       int NUM_OUT_CH, 
       int ROWS, 
       int COLS, 
       int NPC = 1, 
       int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT>
   void ispStats(
       xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& _src, 
       uint32_t* stats, 
       ap_uint<13>* max_bins_list, 
       uint16_t roi_tlx, 
       uint16_t roi_tly, 
       uint16_t roi_brx, 
       uint16_t roi_bry, 
       uint16_t zone_col_num, 
       uint16_t zone_row_num, 
       float inputMin, 
       float inputMax, 
       float outputMin, 
       float outputMax
       )
                  
.. rubric:: Parameter Descriptions

The following table describes the template and the function parameters.

.. table:: Table: ispstats Parameter Description

    +-------------------+--------------------------------------------------+
    | Parameter         | Description                                      |
    +===================+==================================================+
    | MAX_ZONES         | Maximum number of possible zones. Possible value |
    |                   | is 64.                                           |
    +-------------------+--------------------------------------------------+
    | STATS_SIZE        | Number of bins per channel for the input image.  |
    |                   | This is equal to the number of output bins if    |
    |                   | merge bins feature is disabled. Possible value   |
    |                   | is 256 and 4096.                                 |
    +-------------------+--------------------------------------------------+
    | FINAL_BINS_NUM    |  Number of output bins per channel if merge      | 
    |                   |  bins feature is enabled. Possible value is 4.   |                                  
    +-------------------+--------------------------------------------------+
    | MERGE_BINS        | To disable or enable merge bins feature. Possible|
    |                   | values are 0 and 1.                              |
    +-------------------+--------------------------------------------------+
    | SRC_T             | Input pixel type. Possible values are XF_8UC3,   |
    |                   | XF_16UC3, XF_8UC1, XF_16UC1.                     |
    +-------------------+--------------------------------------------------+
    | NUM_OUT_CH        | Number of output channels. Possible Values are 1 |
    |                   | and 3.                                           |
    +-------------------+--------------------------------------------------+
    | ROWS              | Maximum height of input image.                   |                                 
    +-------------------+--------------------------------------------------+
    | COLS              | Maximum width of input image.                    |
    +-------------------+--------------------------------------------------+
    | NPC               | Number of pixels to be processed per cycle.      |
    |                   | Possible options are XF_NPPC1 only.              |                                            
    +-------------------+--------------------------------------------------+
    | XFCVDEPTH_IN      | Depth of Input image                             |
    +-------------------+--------------------------------------------------+
    | \_src             | Input image                                      |
    +-------------------+--------------------------------------------------+
    | stats             | Calculated histogram of the image.               |
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
    | inputMin          | Input image range minimum value.                 |
    +-------------------+--------------------------------------------------+
    | inputMax          | Input image range maximum value.                 |
    +-------------------+--------------------------------------------------+
    | outputMin         | Output image range minimum value.                |
    +-------------------+--------------------------------------------------+
    | outputMax         | Output image range maximum value.                |
    +-------------------+--------------------------------------------------+
   
.. rubric:: Resource Utilization


The following table summarizes the resource utilization in different configurations, generated using the Vitis HLS 2023.1 tool for the
zcu102 FPGA.

.. table:: Table: ispstats Function Resource Utilization Summary

    +----------------+---------------------------+------------------+-----------------+---------------------------------+---------------+----------+
    | Operating Mode | Operating Frequency (MHz) | Input pattern    |   MERGE BINS    |               Utilization Estimate                         |     
    +                +                           +                  +                 +----------------+----------------+---------------+----------+
    |                |                           |                  |                 |    BRAM_36K    |      DSPs      | CLB Registers | CLB LUTs | 
    +================+===========================+==================+=================+================+================+===============+==========+
    | 1 Pixel        | 150                       |BGR 8-bit/channel |        0        |      44.5      |        13      |     5692      |   5120   |
    +----------------+---------------------------+------------------+-----------------+----------------+----------------+---------------+----------+
    | 1 Pixel        | 150                       |BGR 16-bit/channel|        0        |      769       |        16      |     6559      |   6218   |
    +----------------+---------------------------+------------------+-----------------+----------------+----------------+---------------+----------+
    | 1 Pixel        | 150                       |BGR 8-bit/channel |        1        |      44.5      |        13      |     5352      |   4974   |
    +----------------+---------------------------+------------------+-----------------+----------------+----------------+---------------+----------+
    | 1 Pixel        | 150                       |BGR 16-bit/channel|        1        |      769       |        16      |     6158      |   6225   |
    +----------------+---------------------------+------------------+-----------------+----------------+----------------+---------------+----------+
    

.. rubric:: Performance Estimate


The following table summarizes the performance of the kernel in 1-pixel
mode as generated using the Vitis HLS 2023.1 tool for the AMD zcu102 FPGA 
to process a BGR FHD (1920x1080) image.

.. table:: Table: ispstats Function Performance Estimate Summary

    +-----------------------------+------------------+------------------+------------------+
    | Operating Mode              | Input pattern    |    MERGE BINS    |Latency Estimate  |
    +                             +                  +                  +------------------+
    |                             |                  |                  | Max Latency (ms) |
    +=============================+==================+==================+==================+
    | 1 pixel operation (150 MHz) |BGR 8-bit/channel |        0         |      14.68       |
    +-----------------------------+------------------+------------------+------------------+
    | 1 pixel operation (150 MHz) |BGR 16-bit/channel|        0         |     21.2514      |
    +-----------------------------+------------------+------------------+------------------+
    | 1 pixel operation (150 MHz) |BGR 8-bit/channel |        1         |     14.7883      |
    +-----------------------------+------------------+------------------+------------------+
    | 1 pixel operation (150 MHz) |BGR 16-bit/channel|        1         |      21.37       |
    +-----------------------------+------------------+------------------+------------------+
        
.. toctree::
    :maxdepth: 1
