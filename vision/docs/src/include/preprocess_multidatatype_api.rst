
.. _preprocess-multidatatype:

.. 
   Copyright 2025 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

Preprocess Multidatatype
========================

The ``preprocess_accel`` function is a streaming DNN image-preprocessing
accelerator that resizes an input RGB image and applies per-channel mean
subtraction and scaling. The kernel supports runtime selection among INT8,
FP16, BF16, and FP32 output datatypes when the corresponding compile-time
options are enabled.

The preprocessing operation applied to each channel is:

	output = (input - α) × β

where ``α`` (alpha) is the per-channel mean and ``β`` (beta) is the
per-channel scale. Both values are supplied through the ``params_int`` array.

The reference L1 example is located at
``L1/examples/preprocess_multidatatype``. Runtime datatype tests are under
``L1/tests/preprocess_multidatatype/``.

.. rubric:: Pipeline Stages

The ``preprocess_accel`` dataflow pipeline performs the following steps:

-  **AXI Stream to Mat:** Converts the input AXI4-Stream video interface to
   an internal ``xf::cv::Mat`` (``XF_8UC3``).

-  **Resize:** Scales the input image to the runtime ``resize_width`` x
   ``resize_height`` dimensions using bilinear interpolation (configurable
   via ``INTERPOLATION``).

-  **RGB to RGBA (optional):** When ``RGB2RGBA`` is enabled at compile time,
   the resized image is converted from ``XF_8UC3`` to ``XF_8UC4`` before
   preprocessing.

-  **Preprocess:** Applies ``(input - α) × β`` using one of
   ``xf::cv::preProcess_int8``, ``xf::cv::preProcess_fp16``,
   ``xf::cv::preProcess_bf16``, or ``xf::cv::preProcess_fp32`` based on the
   runtime ``datatype`` argument.

-  **Mat to AXI Stream:** Converts the preprocessed output to an AXI4-Stream
   with optional GBR channel ordering (``XF_AXI_GBR``).


preprocess_accel()
------------------

.. rubric:: API Syntax

.. code:: c

    void preprocess_accel(InStream& s_axis_video,
                          OutStream& m_axis_video,
                          uint32_t params_int[2 * XF_CHANNELS(OUT_TYPE, NPPCX)],
                          int in_img_width,
                          int in_img_height,
                          int resize_width,
                          int resize_height,
                          uint32_t datatype);


.. rubric:: Parameter Descriptions

The following table describes the accelerator function parameters.

.. table:: Table: preprocess_accel Parameter Descriptions

    +-------------------+-----------------------------------------------------------+
    | Parameter         | Description                                               |
    +===================+===========================================================+
    | s_axis_video      | Input AXI4-Stream carrying ``XF_8UC3`` image data.      |
    +-------------------+-----------------------------------------------------------+
    | m_axis_video      | Output AXI4-Stream carrying preprocessed image data in    |
    |                   | the datatype selected at runtime.                         |
    +-------------------+-----------------------------------------------------------+
    | params_int        | Per-channel ``α`` and ``β`` values encoded as 32-bit      |
    |                   | fixed-point integers. Even indices hold ``α``; odd indices  |
    |                   | hold ``β``. Host values are multiplied by 2\ :sup:`23`     |
    |                   | (8388608) before writing to the control register.         |
    +-------------------+-----------------------------------------------------------+
    | in_img_width      | Active width of the input image in pixels.                |
    +-------------------+-----------------------------------------------------------+
    | in_img_height     | Active height of the input image in pixels.               |
    +-------------------+-----------------------------------------------------------+
    | resize_width      | Target output width after resize.                         |
    +-------------------+-----------------------------------------------------------+
    | resize_height     | Target output height after resize.                        |
    +-------------------+-----------------------------------------------------------+
    | datatype          | Runtime output datatype selection. Values are defined in  |
    |                   | ``data_types``: ``INT8`` (0), ``FP16`` (1), ``BF16`` (2),|
    |                   | ``FP32`` (3). The selected datatype must be enabled at    |
    |                   | compile time via the ``SELECT`` macro.                    |
    +-------------------+-----------------------------------------------------------+


preProcess_int8()
-----------------

.. rubric:: API Syntax

.. code:: c

    template <int IN_TYPE,
              int OUT_TYPE,
              int HEIGHT,
              int WIDTH,
              int NPC,
              int WIDTH_A,
              int IBITS_A,
              int WIDTH_B,
              int IBITS_B,
              int WIDTH_OUT,
              int IBITS_OUT,
              int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
              int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
    void preProcess_int8(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                         xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                         float params[2 * XF_CHANNELS(IN_TYPE, NPC)]);


preProcess_fp16()
-----------------

.. rubric:: API Syntax

.. code:: c

    template <int IN_TYPE,
              int OUT_TYPE,
              int HEIGHT,
              int WIDTH,
              int NPC,
              int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
              int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
    void preProcess_fp16(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                         xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                         float params[2 * XF_CHANNELS(IN_TYPE, NPC)]);


preProcess_bf16()
-----------------

.. rubric:: API Syntax

.. code:: c

    template <int IN_TYPE,
              int OUT_TYPE,
              int HEIGHT,
              int WIDTH,
              int NPC,
              int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
              int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
    void preProcess_bf16(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                         xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                         float params[2 * XF_CHANNELS(IN_TYPE, NPC)]);


preProcess_fp32()
-----------------

.. rubric:: API Syntax

.. code:: c

    template <int IN_TYPE,
              int OUT_TYPE,
              int HEIGHT,
              int WIDTH,
              int NPC,
              int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
              int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
    void preProcess_fp32(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                         xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                         float params[2 * XF_CHANNELS(IN_TYPE, NPC)]);


.. rubric:: preProcess Template Parameter Descriptions

The following table describes the common template parameters for all
``preProcess_*`` variants. Implementations are in
``L1/include/dnn/xf_preprocess_generic.hpp``.

.. table:: Table: preProcess_* Template Parameter Descriptions

    +-------------------+-----------------------------------------------------------+
    | Parameter         | Description                                               |
    +===================+===========================================================+
    | IN_TYPE           | Input pixel type (e.g. ``XF_8UC3``, ``XF_8UC4``).         |
    +-------------------+-----------------------------------------------------------+
    | OUT_TYPE          | Output pixel type. Depends on enabled datatype:             |
    |                   | ``XF_8UC3``/``XF_8UC4`` (INT8), ``XF_16UC3``/``XF_16UC4`` |
    |                   | (FP16/BF16), ``XF_32FC3``/``XF_32FC4`` (FP32).            |
    +-------------------+-----------------------------------------------------------+
    | HEIGHT            | Maximum image height the kernel is compiled for.          |
    +-------------------+-----------------------------------------------------------+
    | WIDTH             | Maximum image width the kernel is compiled for.           |
    +-------------------+-----------------------------------------------------------+
    | NPC               | Number of pixels processed per clock cycle (NPPC).        |
    +-------------------+-----------------------------------------------------------+
    | WIDTH_A           | Bit width of the ``α`` (mean) fixed-point value             |
    |                   | (``preProcess_int8`` only).                               |
    +-------------------+-----------------------------------------------------------+
    | IBITS_A           | Number of integer bits in ``α`` (``preProcess_int8``      |
    |                   | only).                                                      |
    +-------------------+-----------------------------------------------------------+
    | WIDTH_B           | Bit width of the ``β`` (scale) fixed-point value          |
    |                   | (``preProcess_int8`` only).                               |
    +-------------------+-----------------------------------------------------------+
    | IBITS_B           | Number of integer bits in ``β`` (``preProcess_int8``      |
    |                   | only).                                                      |
    +-------------------+-----------------------------------------------------------+
    | WIDTH_OUT         | Bit width of the output pixel representation                |
    |                   | (``preProcess_int8`` only).                               |
    +-------------------+-----------------------------------------------------------+
    | IBITS_OUT         | Number of integer bits in the output pixel                  |
    |                   | (``preProcess_int8`` only).                               |
    +-------------------+-----------------------------------------------------------+
    | XFCVDEPTH_IN      | FIFO depth of the input ``xf::cv::Mat`` stream.           |
    +-------------------+-----------------------------------------------------------+
    | XFCVDEPTH_OUT     | FIFO depth of the output ``xf::cv::Mat`` stream.          |
    +-------------------+-----------------------------------------------------------+
    | in_mat            | Input image after resize (and optional RGBA conversion).  |
    +-------------------+-----------------------------------------------------------+
    | out_mat           | Preprocessed output image.                                |
    +-------------------+-----------------------------------------------------------+
    | params            | Float array of per-channel ``α`` and ``β`` values. Even     |
    |                   | indices are ``α``; odd indices are ``β``.                 |
    +-------------------+-----------------------------------------------------------+


.. table:: Table: Compile-Time Configuration Parameters

    +-----------------------------+----------------------------------------------+
    | Parameter                   | Description                                  |
    +=============================+==============================================+
    | HEIGHT / WIDTH              | Maximum input image dimensions.              |
    +-----------------------------+----------------------------------------------+
    | NEWHEIGHT / NEWWIDTH        | Maximum resize output dimensions.            |
    +-----------------------------+----------------------------------------------+
    | NPPCX                       | Pixels per clock cycle (default NPPC4).      |
    +-----------------------------+----------------------------------------------+
    | RGB2RGBA                    | Enable RGB-to-RGBA conversion before         |
    |                             | preprocessing (0 or 1).                      |
    +-----------------------------+----------------------------------------------+
    | XF_AXI_GBR                  | Swap R and G channels on AXI stream output.  |
    +-----------------------------+----------------------------------------------+
    | XF_INT8 / XF_FP16 /         | Enable corresponding output datatype at      |
    | XF_BF16 / XF_FP32           | compile time. Combined into ``SELECT``.      |
    +-----------------------------+----------------------------------------------+
    | INTERPOLATION               | Resize interpolation mode (1 = bilinear).  |
    +-----------------------------+----------------------------------------------+
    | MAXDOWNSCALE                | Maximum downscale factor supported by resize.|
    +-----------------------------+----------------------------------------------+
    | XF_USE_URAM                 | Use URAM for resize line buffers (0 or 1).   |
    +-----------------------------+----------------------------------------------+
    | WIDTH_A / IBITS_A           | Fixed-point width and integer bits for ``α``.|
    +-----------------------------+----------------------------------------------+
    | WIDTH_B / IBITS_B           | Fixed-point width and integer bits for ``β``.|
    +-----------------------------+----------------------------------------------+
    | WIDTH_OUT / IBITS_OUT       | Fixed-point width and integer bits for output|
    +-----------------------------+----------------------------------------------+


.. rubric:: Resource Utilization


The following table summarizes resource utilization for a 1920 x 1080 image,
as generated in the Vitis HLS 2026.1 tool for Versal AI Edge at 300 MHz.

.. table:: Table: preprocess_accel Resource Utilization Summary

    +------------------+---------------------+-----------+-----+------+------+-----+------+------+
    | Function Config  | Image Size          | Frequency | NPC | LUT  | FF   | DSP | BRAM | URAM |
    |                  |                     | (MHz)     |     |      |      |     |      |      |
    +==================+=====================+===========+=====+======+======+=====+======+======+
    | INT8             | 1920 x 1080         | 300       | 4   | 3282 | 2138 | 17  | 0    | 0    |
    +------------------+---------------------+-----------+-----+------+------+-----+------+------+
    | BF16, FP16       | 1920 x 1080         | 300       | 4   | 6866 | 2238 | 1   | 0    | 0    |
    +------------------+---------------------+-----------+-----+------+------+-----+------+------+
    | FP32             | 1920 x 1080         | 300       | 4   | 9672 | 7547 | 77  | 0    | 30   |
    +------------------+---------------------+-----------+-----+------+------+-----+------+------+


.. rubric:: Performance Estimate


The following table summarizes the latency estimates for a Full HD (1920 x 1080)
image at 300 MHz.

.. table:: Table: preprocess_accel Performance Estimate Summary

    +----------------+---------------------+------------------+
    | Operating Mode | Operating Frequency | Latency Estimate |
    |                |                     |                  |
    |                | (MHz)               | Max (ms)         |
    +================+=====================+==================+
    | 4 pixel (NPC4) | 300                 | 1.8              |
    +----------------+---------------------+------------------+
    | 2 pixel (NPC2) | 300                 | 3.6              |
    +----------------+---------------------+------------------+
    | 1 pixel (NPC1) | 300                 | 7.0              |
    +----------------+---------------------+------------------+
