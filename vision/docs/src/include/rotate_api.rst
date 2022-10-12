Rotate
=======

.. rubric:: API Syntax

Rotate function rotates the input image by 90, 180 or 270 degrees in clockwise direction.

.. code:: c

	template <int INPUT_PTR_WIDTH, int OUTPUT_PTR_WIDTH, int TYPE, int TILE_SZ, int ROWS, int COLS, int NPC>
	void rotate(ap_uint<INPUT_PTR_WIDTH>* src_ptr, 
				ap_uint<OUTPUT_PTR_WIDTH>* dst_ptr, 
				int rows, 
				int cols, 
				int direction) {

The following table describes the template and the function parameters.

.. table:: Table Rotate Parameter Description

    +----------------------+-------------------------------------------------------------+
    | Parameter            | Description                                                 |
    +======================+=============================================================+
    | INPUT_PTR_WIDTH      | Pixel width of input image pointer. Must be power of 2.     |
    +----------------------+-------------------------------------------------------------+
    | OUTPUT_PTR_WIDTH     | Pixel width of output image pointer. Must be power of 2.    |
    +----------------------+-------------------------------------------------------------+
    | TYPE                 | Input and Output Pixel type. Only 8-bit, unsigned and 1,3   | 
    |                      | channels are supported(XF_8UC1, XF_8UC3)                    |
    +----------------------+-------------------------------------------------------------+
    | TILE_SZ              | Tile size                                                   |
    +----------------------+-------------------------------------------------------------+
    | ROWS                 | Maximum height of the image (Must be multiple of NPC)       |
    +----------------------+-------------------------------------------------------------+
    | COLS                 | Maximum width of the image (Must be multiple of NPC)        |
    +----------------------+-------------------------------------------------------------+
    | NPC                  | Number of Pixels to be processed per cycle. NPPC1 and NPPC2 |
    |                      | are supported.                                              |
    +----------------------+-------------------------------------------------------------+
    | src_ptr              | Input Image pointer                                         |
    +----------------------+-------------------------------------------------------------+
    | dst_ptr              | Output Image pointer                                        |
    +----------------------+-------------------------------------------------------------+
    | rows                 | Height of the image                                         |
    +----------------------+-------------------------------------------------------------+
    | cols                 | Width of the image                                          |
    +----------------------+-------------------------------------------------------------+
    | direction            | Direction of rotate, possible values are 90, 180 or 270     |
    +----------------------+-------------------------------------------------------------+
    
.. rubric:: Resource Utilization

The following table summarizes the resource utilization in different configurations, generated using Vitis HLS 2022.1 tool for the xcu200-fsgd2104-2-e, to process a 4k, 3 channel image.  

.. table:: Table Rotate Resource Utilization Summary

    +----------------+----------------+---------------------+----------------------+----------+------+------+------+
    | Operating Mode | Direction of   | Operating Frequency | Utilization Estimate |          |      |      |      |
    |                | Rotate         |                     |                      |          |      |      |      |
    |                |                | (MHz)               |                      |          |      |      |      |
    +                +                +                     +----------------------+----------+------+------+------+
    |                |                |                     | BRAM_18K             | DSP      | FF   | LUT  | URAM |
    +================+================+=====================+======================+==========+======+======+======+
    | 1 Pixel        | 90             | 300                 | 8                    | 14       | 4373 | 4223 | 0    |
    +                +----------------+---------------------+----------------------+----------+------+------+------+
    |                | 180            | 300                 | 8                    | 14       | 4373 | 4223 | 0    |
    +                +----------------+---------------------+----------------------+----------+------+------+------+
    |                | 270            | 300                 | 8                    | 14       | 4373 | 4223 | 0    |
    +----------------+----------------+---------------------+----------------------+----------+------+------+------+
    | 2 Pixel        | 90             | 300                 | 8                    | 18       | 6294 | 6532 | 0    |
    +                +----------------+---------------------+----------------------+----------+------+------+------+
    |                | 180            | 300                 | 8                    | 18       | 6294 | 6532 | 0    |
    +                +----------------+---------------------+----------------------+----------+------+------+------+
    |                | 270            | 300                 | 8                    | 18       | 6294 | 6532 | 0    |
    +----------------+----------------+---------------------+----------------------+----------+------+------+------+

.. rubric:: Performance Estimate

The following table summarizes the performance estimates in different configurations, generated using Vitis HLS 2022.1 tool for the xcu200-fsgd2104-2-e, to process a 4k, 3 channel image.

.. table:: Table Rotate Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               |                  |
    +                +                     +------------------+
    |                |                     | Max (ms)         |
    +================+=====================+==================+
    | 1 pixel        | 300                 | 52.5             |
    +----------------+---------------------+------------------+
    | 2 pixel        | 300                 | 26.5             |
    +----------------+---------------------+------------------+
